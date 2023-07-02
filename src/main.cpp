
#include "tklib.h"

#pragma warning(push, 0)
#include <gl/GL.h>
#include "external/glcorearb.h"
#include "external/wglext.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winhttp.h>
#include <xaudio2.h>
#include <tlhelp32.h>

#define DR_MP3_IMPLEMENTATION
#define DRMP3_ASSERT assert
#include "external/mp3lib.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT assert
#include "external/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert assert
#include "external/stb_truetype.h"
#pragma warning(pop)

#include "config.h"
#define m_equals(...) = __VA_ARGS__
#include "shader_shared.h"
#include "secret.h"
#include "math.h"
#include "http.h"
#include "sound.h"
#include "json.h"
#include "hashtable.h"
#include "ui.h"
#include "draw.h"
#include "main.h"

#include "user_config.h"

#define m_gl_funcs \
X(PFNGLBUFFERDATAPROC, glBufferData) \
X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
X(PFNGLGENBUFFERSPROC, glGenBuffers) \
X(PFNGLBINDBUFFERPROC, glBindBuffer) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
X(PFNGLCREATESHADERPROC, glCreateShader) \
X(PFNGLSHADERSOURCEPROC, glShaderSource) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
X(PFNGLATTACHSHADERPROC, glAttachShader) \
X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
X(PFNGLCOMPILESHADERPROC, glCompileShader) \
X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback) \
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \
X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
X(PFNGLUSEPROGRAMPROC, glUseProgram) \
X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
X(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT) \
X(PFNWGLGETSWAPINTERVALEXTPROC, wglGetSwapIntervalEXT)

#define X(type, name) global type name = null;
m_gl_funcs
#undef X

global s_xaudio_state xaudio_state;

global int g_width = 0;
global int g_height = 0;
global s_v2 g_window_size = c_base_res;
global s_v2 g_window_center = g_window_size * 0.5f;
global s_sarray<s_transform, 8192> transforms;
global s_input g_input = zero;
global float total_time = 0;
global float delta = 0;
global s_v2 mouse = zero;
global s_carray<s_font, e_font_count> g_font_arr;
global s_carray<s_sarray<s_transform, 8192>, e_font_count> text_arr;

global s_sarray<s_str<600>, 4> messages;
global s_lin_arena sound_arena = zero;

global s_texture checkmark_texture;
global s_tts_queue tts_queue;

global s_ui ui;
global s_sarray<s_char_event, 64> char_event_arr;
global s_user_data g_user_data = zero;

global s_mutex g_twitch_chat_mutex;
global s_mutex g_http_mutex;
global volatile e_twitch_chat_event g_twitch_chat_event;
global b8 g_connected_to_twitch_chat;
global s_thread g_twitch_chat_thread;
global s_app* app;

// @Note(tkap, 04/05/2023): This is only used by the twitch chat thread. We could use "g_user_data.target_channel", but that may cause a race condition,
// so the twitch chat thread makes a copy
global s_str<64> g_target_channel;

#include "http.cpp"
#include "sound.cpp"
#include "json.cpp"
#include "log.cpp"
#include "ui.cpp"
#include "draw.cpp"
#include "hashtable.cpp"



int main(int argc, char** argv)
{
  argc -= 1;
  argv += 1;
  
  g_twitch_chat_mutex = make_mutex();
  g_http_mutex = make_mutex();
  
  // @TODO(tkap, 04/05/2023): Replace with arena after we sort out all of our memory problems
  app = (s_app*)calloc(1, sizeof(s_app));
  
  HINSTANCE instance = GetModuleHandle(null);
  char* class_name = "twitch_bot";
  sound_arena.init(100 * c_mb);
  
  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = null;
  PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = null;
  
  // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		dummy start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
  {
    WNDCLASSEX window_class = zero;
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_OWNDC;
    window_class.lpfnWndProc = DefWindowProc;
    window_class.lpszClassName = class_name;
    window_class.hInstance = instance;
    assert(RegisterClassEx(&window_class));
    
    HWND window = CreateWindowEx(
                                 0,
                                 class_name,
                                 "TKCHAT",
                                 WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 null,
                                 null,
                                 instance,
                                 null
                                 );
    assert(window != INVALID_HANDLE_VALUE);
    
    HDC dc = GetDC(window);
    
    PIXELFORMATDESCRIPTOR pfd = zero;
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int format = ChoosePixelFormat(dc, &pfd);
    assert(DescribePixelFormat(dc, format, sizeof(pfd), &pfd));
    assert(SetPixelFormat(dc, format, &pfd));
    
    HGLRC glrc = wglCreateContext(dc);
    assert(wglMakeCurrent(dc, glrc));
    
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)load_gl_func("wglCreateContextAttribsARB");
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)load_gl_func("wglChoosePixelFormatARB");
    
    assert(wglMakeCurrent(null, null));
    assert(wglDeleteContext(glrc));
    assert(ReleaseDC(window, dc));
    assert(DestroyWindow(window));
    assert(UnregisterClass(class_name, instance));
    
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		dummy end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  
  HDC dc = null;
  HWND window = null;
  // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		window start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
  {
    WNDCLASSEX window_class = zero;
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.lpszClassName = class_name;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursor(null, IDC_ARROW);
    assert(RegisterClassEx(&window_class));
    
    window = CreateWindowEx(
                            // WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
                            0,
                            class_name,
                            "Twitch Bot",
                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            // WS_POPUP | WS_VISIBLE,
                            // WS_POPUP,
                            CW_USEDEFAULT, CW_USEDEFAULT, (int)c_base_res.x, (int)c_base_res.y,
                            null,
                            null,
                            instance,
                            null
                            );
    assert(window != INVALID_HANDLE_VALUE);
    
    SetLayeredWindowAttributes(window, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    dc = GetDC(window);
    
    int pixel_attribs[] = {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
      WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
      WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
      WGL_SWAP_METHOD_ARB, WGL_SWAP_COPY_ARB,
      WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
      WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
      WGL_COLOR_BITS_ARB, 32,
      WGL_DEPTH_BITS_ARB, 24,
      WGL_STENCIL_BITS_ARB, 8,
      0
    };
    
    PIXELFORMATDESCRIPTOR pfd = zero;
    pfd.nSize = sizeof(pfd);
    int format;
    u32 num_formats;
    assert(wglChoosePixelFormatARB(dc, pixel_attribs, null, 1, &format, &num_formats));
    assert(DescribePixelFormat(dc, format, sizeof(pfd), &pfd));
    SetPixelFormat(dc, format, &pfd);
    
    int gl_attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 3,
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
      WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
      0
    };
    HGLRC glrc = wglCreateContextAttribsARB(dc, null, gl_attribs);
    assert(wglMakeCurrent(dc, glrc));
    
#define X(type, name) name = (type)load_gl_func(#name);
    m_gl_funcs
#undef X
    
    glDebugMessageCallback(gl_debug_callback, null);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		window end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  
  // @Note(tkap, 04/05/2023): Read user data if it exists
  {
    char* data = read_file_quick("user_data.txt", &sound_arena);
    if(data)
    {
      struct s_query_data
      {
        char* query;
        s_str<64>* target;
      };
      constexpr s_query_data query_arr[] = {
        {.query = "target_channel=", .target = &g_user_data.target_channel},
        {.query = "access_token=", .target = &g_user_data.access_token},
      };
      for(int query_i = 0; query_i < array_count(query_arr); query_i++)
      {
        auto q = query_arr[query_i];
        int q_len = (int)strlen(q.query);
        int q_index = str_find_from_left(data, q.query);
        if(q_index != -1)
        {
          char* val_start = data + q_index + q_len;
          int val_len = str_find_from_left(val_start, "\n");
          if(val_len == -1)
          {
            val_len = (int)strlen(val_start);
          }
          q.target->from_data(val_start, val_len);
          q.target->len -= str_trim_from_right(q.target->data);
        }
      }
    }
  }
  
  maybe_connect_to_twitch_chat(&sound_arena);
  
  // stbi_set_flip_vertically_on_load(true);
  checkmark_texture = load_texture_from_file("assets/checkmark.png", GL_LINEAR);
  
  g_font_arr[e_font_small] = load_font("assets/consola.ttf", 24, &sound_arena);
  g_font_arr[e_font_medium] = load_font("assets/consola.ttf", 36, &sound_arena);
  g_font_arr[e_font_big] = load_font("assets/consola.ttf", 72, &sound_arena);
  
  u32 vao;
  u32 ssbo;
  u32 program;
  {
    u32 vertex = glCreateShader(GL_VERTEX_SHADER);
    u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
    char* header = "#version 430 core\n";
    char* vertex_src = read_file_quick("shaders/vertex.vertex", &sound_arena);
    char* fragment_src = read_file_quick("shaders/fragment.fragment", &sound_arena);
    char* vertex_src_arr[] = {header, read_file_quick("src/shader_shared.h", &sound_arena), vertex_src};
    char* fragment_src_arr[] = {header, read_file_quick("src/shader_shared.h", &sound_arena), fragment_src};
    glShaderSource(vertex, array_count(vertex_src_arr), vertex_src_arr, null);
    glShaderSource(fragment, array_count(fragment_src_arr), fragment_src_arr, null);
    glCompileShader(vertex);
    char buffer[1024] = zero;
    check_for_shader_errors(vertex, buffer);
    glCompileShader(fragment);
    check_for_shader_errors(fragment, buffer);
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glUseProgram(program);
  }
  
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  
  glGenBuffers(1, &ssbo);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(transforms.elements), null, GL_DYNAMIC_DRAW);
  
  platform_init_sound();
  for(int channel_idx = 0; channel_idx < c_number_channels; channel_idx++)
  {
    xaudio_state.submixes[channel_idx].pSubmixVoice->SetVolume(c_global_volume);
  }
  
  s_thread tts_queue_thread = zero;
  s_thread redemptions_thread = zero;
  tts_queue_thread.init(handle_tts_queue);
  redemptions_thread.init(handle_redemptions);
  
  MSG win_msg = zero;
  b8 running = true;
  while(running)
  {
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		window messages start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    while(PeekMessageA(&win_msg, null, 0, 0, PM_REMOVE) != 0)
    {
      if(win_msg.message == WM_QUIT)
      {
        running = false;
        break;
      }
      TranslateMessage(&win_msg);
      DispatchMessage(&win_msg);
    }
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		window messages end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    
    
    {
      POINT point = zero;
      GetCursorPos(&point);
      ScreenToClient(window, &point);
      mouse.x = clamp((float)point.x, 0.0f, g_window_size.x);
      mouse.y = clamp((float)point.y, 0.0f, g_window_size.y);
    }
    
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		app update start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    {
      {
        s_v2 label_pos = v2(100, 100);
        ui_label("Channel", label_pos);
        s_v2 button_pos = label_pos;
        button_pos.x += 200;
        button_pos.y -= g_font_arr[e_font_medium].size;
        auto channel_input_data = ui_input_box(
                                               "channel_input", button_pos, v2(256, 64),
                                               g_user_data.target_channel.len > 0 ? g_user_data.target_channel.data : null
                                               );
        if(channel_input_data.submitted)
        {
          g_user_data.target_channel = channel_input_data.input;
          save_user_data();
          maybe_disconnect_from_twitch_chat();
          maybe_connect_to_twitch_chat(&sound_arena);
          // connect_to_twitch_chat(channel_input_data.input.data, connect_to_twitch_chat_callback);
          // printf("connecting to %s\n", channel_input_data.input.data);
        }
      }
      
      if(ui_button("Exit", v2(100, 400), v2(256, 64)).clicked)
      {
        running = false;
      }
    }
    
    {
      s_v2 pos = v2(100.0f, g_window_size.y - 100);
      for(int i = 0; i < messages.count; i++)
      {
        auto foo = messages[i];
        if(foo.data[0])
        {
          draw_text(foo.data, pos, e_layer_base, v4(0, 1, 0), e_font_medium, false);
          pos.y -= 100;
        }
      }
    }
    
    if(g_twitch_chat_event != e_twitch_chat_event_none)
    {
      switch(g_twitch_chat_event)
      {
        case e_twitch_chat_event_connected:
        {
          g_connected_to_twitch_chat = true;
          make_timed_message(format_text("Connected to %s", g_user_data.target_channel.data));
          log_info(format_text("Connected to %s", g_user_data.target_channel.data));
        } break;
        invalid_default_case;
      }
      
      g_twitch_chat_mutex.lock();
      g_twitch_chat_event = e_twitch_chat_event_none;
      g_twitch_chat_mutex.unlock();
    }
    
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		handle timed messages start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    foreach(msg_i, msg, app->timed_message_arr)
    {
      s_v2 pos = v2(g_window_size.x / 2, 64 * (msg_i + 1));
      s_v4 color = msg->color;
      float percent_passed = msg->time_passed / msg->duration;
      color.w = 1.0f - powf(percent_passed, 4);
      draw_text(msg->message.data, pos, 5, color, e_font_medium, true);
      msg->time_passed += delta;
      if(msg->time_passed >= msg->duration)
      {
        app->timed_message_arr.remove_and_shift(msg_i--);
      }
    }
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		handle timed messages end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    
    soft_reset_ui();
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		app update end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    
    // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		render start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    {
      // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glClearDepth(0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glViewport(0, 0, g_width, g_height);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_GREATER);
      
      int location = glGetUniformLocation(program, "window_size");
      glUniform2fv(location, 1, &g_window_size.x);
      
      glBindTexture(GL_TEXTURE_2D, checkmark_texture.id);
      
      if(transforms.count > 0)
      {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*transforms.elements) * transforms.count, transforms.elements);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, transforms.count);
        transforms.count = 0;
      }
      
      for(int font_i = 0; font_i < e_font_count; font_i++)
      {
        if(text_arr[font_i].count > 0)
        {
          s_font* font = &g_font_arr[font_i];
          glBindTexture(GL_TEXTURE_2D, font->texture.id);
          glEnable(GL_BLEND);
          glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
          glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*text_arr[font_i].elements) * text_arr[font_i].count, text_arr[font_i].elements);
          glDrawArraysInstanced(GL_TRIANGLES, 0, 6, text_arr[font_i].count);
          text_arr[font_i].count = 0;
        }
      }
    }
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		render end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    
    for(int k_i = 0; k_i < c_max_keys; k_i++)
    {
      g_input.keys[k_i].count = 0;
    }
    
    char_event_arr.count = 0;
    
    SwapBuffers(dc);
    
    // @TODO(tkap, 24/04/2023): Real timing
    Sleep(10);
    delta = 0.01f;
    total_time += delta;
  }
  
  return 0;
}

LRESULT window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  
  switch(msg)
  {
    
    case WM_CLOSE:
    case WM_DESTROY:
    {
      PostQuitMessage(0);
    } break;
    
    case WM_CHAR:
    {
      char_event_arr.add({.is_symbol = false, .c = (int)wparam});
    } break;
    
    case WM_SIZE:
    {
      g_width = LOWORD(lparam);
      g_height = HIWORD(lparam);
      g_window_size = v2(g_width, g_height);
      g_window_center = g_window_size * 0.5f;
    } break;
    
    case WM_LBUTTONDOWN:
    {
      g_input.keys[left_button].is_down = true;
      g_input.keys[left_button].count += 1;
    } break;
    
    case WM_LBUTTONUP:
    {
      g_input.keys[left_button].is_down = false;
      g_input.keys[left_button].count += 1;
    } break;
    
    case WM_RBUTTONDOWN:
    {
      g_input.keys[right_button].is_down = true;
      g_input.keys[right_button].count += 1;
    } break;
    
    case WM_RBUTTONUP:
    {
      g_input.keys[right_button].is_down = false;
      g_input.keys[right_button].count += 1;
    } break;
    
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      int key = (int)wparam;
      b8 is_down = !(bool)((HIWORD(lparam) >> 15) & 1);
      if(key < c_max_keys)
      {
        
        s_stored_input si = zero;
        si.key = key;
        si.is_down = is_down;
        apply_event_to_input(&g_input, si);
      }
      
      if(is_down)
      {
        char_event_arr.add({.is_symbol = true, .c = (int)wparam});
      }
    } break;
    
    default:
    {
      result = DefWindowProc(window, msg, wparam, lparam);
    }
  }
  
  return result;
}

s_mp3_sound get_tiktok_tts(char* encoded_text, s_tts_voice tiktok_voice, s_lin_arena* arena)
{
  char* tiktok_header =
    format_text("User-Agent: com.zhiliaoapp.musically/2022600030"
                " (Linux; U; Android 7.1.2; es_ES; SM-G988N; Build/NRD90M;tt-ok/3.12.13.1)\r\n"
                "Cookie: sessionid=%s", c_tiktok_session_id);
  s_http_get tiktok_data =
    http_post(format_text("https://api22-normal-c-useast1a.tiktokv.com/media/api/text/speech/invoke/?"
                          "text_speaker=%s&req_text=%s&speaker_map_type=0&aid=1233",
                          tiktok_voice.api_voice_name, encoded_text), tiktok_header, arena);
  
  char* tiktok_mp3 = &tiktok_data.text[str_find_from_left(tiktok_data.text, "v_str") + 8];
  int tiktok_mp3_size = str_find_from_left(tiktok_mp3, "\"");
  
  s_mp3_sound mp3_sound = zero;
  
  if(tiktok_mp3_size)
  {
    std::string tiktok_mp3_decoded = base64_decode(tiktok_mp3, tiktok_mp3_size);
    mp3_sound = load_mp3_from_data((char*)tiktok_mp3_decoded.c_str(), (int)tiktok_mp3_decoded.size(), arena);
    
    // Resample to 44100
    int num_source_samples = mp3_sound.sample_count;
    int num_target_samples = (int)((u64)num_source_samples * 44100 / 48000);
    for(int sample_idx = 0; sample_idx < num_target_samples; sample_idx++)
    {
      s16 * source_buffer = (s16*)mp3_sound.data;
      s16 * target_buffer = (s16*)mp3_sound.data;
      
      double d_idx = sample_idx * (double) num_source_samples / num_target_samples;
      int source_idx = (int)d_idx;
      double t = d_idx - source_idx;
      target_buffer[sample_idx] =
      (s16)(source_buffer[source_idx] * (1.0 - t) +
            source_buffer[min(source_idx + 1, num_source_samples - 1)] * t);
    }
    
    // mp3_sound.size = num_target_samples * sizeof(s16);
    mp3_sound.sample_count = num_target_samples;
  }
  
  return mp3_sound;
}

func s_mp3_sound get_streamelements_tts(char* encoded_text, s_tts_voice streamelements_voice, s_lin_arena* arena)
{
  s_http_get data =
    http_get(format_text("https://api.streamelements.com/kappa/v2/speech?voice=%s&text=%s",
                         streamelements_voice.api_voice_name, encoded_text), 0, arena);
  
  s_mp3_sound sound = zero;
  if(data.size_in_bytes)
  {
    sound = load_mp3_from_data(data.text, data.size_in_bytes, arena);
  }
  
  return sound;
}

func s_mp3_sound get_tts(char* encoded_text, s_tts_voice tts_voice, s_lin_arena* arena)
{
  switch(tts_voice.tts_api)
  {
    case tts_api_tiktok:
    {
      return get_tiktok_tts(encoded_text, tts_voice, arena);
      break;
    }
    
    case tts_api_streamelements:
    {
      return get_streamelements_tts(encoded_text, tts_voice, arena);
      break;
    }
    
    default:
    {
      assert(false && "Unrecognized tts api");
      return zero;
    }
  }
}

func void send_chat_message(s_socket* connect_socket, char* message)
{
  message = format_text("PRIVMSG #%s : %s", g_target_channel.data, message);
  send_string(connect_socket, message);
}

func void send_chat_command(s_socket* connect_socket, char* user, char* message)
{
  // Check if @%s is present in the message
  if(strcmp(message, "@%s") >= 0)
  {
    message = format_text(message, user);
  }
  
  send_chat_message(connect_socket, message);
}

DWORD handle_tts_queue(void* param)
{
  unreferenced(param);
  
  s_lin_arena arena = zero;
  arena.init(c_mb * 100);
  
  while(true) // Loop
  {
    Sleep(100);
    
    // Get new TTs message when nothing is playing
    if(!tts_queue.tts_message_count)
    {
      continue;
    }
    
    // Check if TTS is already playing
    bool playing_tts = false;
    s_xaudio_submix submix = xaudio_state.submixes[e_sound_type_tts];
    for(int voice_idx = 0; voice_idx < c_max_concurrent_sounds; voice_idx++)
    {
      if(submix.voices[voice_idx].playing)
      {
        playing_tts = true;
        break;
      }
    }
    
    if(playing_tts)
    {
      continue;
    }
    
    // Get new TTS Message and check if there is a message
    s_tts_message tts_message = tts_queue.tts_queue[tts_queue.current_queue_idx];
    char* msg = tts_message.message;
    tts_queue.tts_message_count--;
    tts_queue.current_queue_idx =
    (tts_queue.current_queue_idx + 1) % c_max_tts_queue_entries;
    if(strlen(msg) <= 0)
    {
      continue;
    }
    
    s_mp3_sound final_sound = zero;
    
    // Example TTS: !tts Cakez -word -stitch i like going to school
    // Split msg into Words, delimiter ' '
    int word_count = 0;
    int char_count = 0;
    char words[256][c_max_tts_message_chars] = zero;
    for(int char_idx = 0; char_idx < strlen(msg); char_idx++)
    {
      char c = msg[char_idx];
      
      if(character_predicate(c, char_pred_whitespace) && char_count > 0)
      {
        word_count++;
        char_count = 0;
        
        continue;
      }
      
      // Add char to word
      words[word_count][char_count++] = c;
    }
    
    // Final word
    if(char_count > 0)
    {
      word_count++;
    }
    
    int merged_tts_length = 0;
    int tts_text_length = 0;
    int encoded_char_count = 0;
    char* merged_tts = (char*)arena.get_clear(c_merged_tts_buffer_size);
    char* tts_text = (char*)arena.get_clear(c_tts_text_buffer_size);
    char* tts_text_encoded = (char*)arena.get_clear(c_tts_text_encoded_buffer_size);
    s_tts_voice current_voice = tts_voices[0]; // Brian
    
    for(int word_idx = 0; word_idx < word_count; word_idx++)
    {
      char* word = words[word_idx];
      
      if(word[strlen(word) - 1] == ':') // Load new Voice
      {
        word[strlen(word) - 1] = 0; // Remove ':'
        
        bool found_voice = false;
        for(int tts_voice_idx = 0; tts_voice_idx < array_count(tts_voices); tts_voice_idx++)
        {
          if(strncmp(tts_voices[tts_voice_idx].voice_name, word, strlen(word)) == 0)
          {
            // Copy message before the Change
            if(tts_text_length > 0)
            {
              // Encode the message
              for(int char_idx = 0; char_idx < tts_text_length; char_idx++)
              {
                u8 c = tts_text[char_idx];
                s32 code = c_uri_encode_tbl_codes[c];
                
                if (code)
                {
                  *((s32 *)&tts_text_encoded[encoded_char_count]) = code;
                  encoded_char_count += 3;
                }
                else
                {
                  tts_text_encoded[encoded_char_count++] = c;
                }
              }
              
              s_mp3_sound mp3_sound = get_tts(tts_text_encoded, current_voice, &arena);
              if(mp3_sound.get_size())
              {
                memcpy(&merged_tts[merged_tts_length], mp3_sound.data, mp3_sound.get_size());
                merged_tts_length += mp3_sound.get_size();
                final_sound.sample_count += mp3_sound.sample_count;
              }
              
              memset(tts_text, 0, c_tts_text_buffer_size);
              memset(tts_text_encoded, 0, c_tts_text_encoded_buffer_size);
              encoded_char_count = 0;
              tts_text_length = 0;
            }
            
            current_voice = tts_voices[tts_voice_idx];
            found_voice = true;
            break;
          }
        }
        
        if(found_voice)
        {
          continue;
        }
      }
      
      if(word[0] == '-') // Load a Sound
      {
        char* sound_path = format_text("assets/sounds/%s.mp3", word + 1);
        s_mp3_sound sound = load_mp3_from_file(sound_path, &arena);
        
        if(sound.data)
        {
          // Copy message before the sound
          if(tts_text_length > 0)
          {
            // Encode the message
            for(int char_idx = 0; char_idx < tts_text_length; char_idx++)
            {
              u8 c = tts_text[char_idx];
              s32 code = c_uri_encode_tbl_codes[c];
              
              if (code)
              {
                *((s32 *)&tts_text_encoded[encoded_char_count]) = code;
                encoded_char_count += 3;
              }
              else
              {
                tts_text_encoded[encoded_char_count++] = c;
              }
            }
            
            s_mp3_sound mp3_sound = get_tts(tts_text_encoded, current_voice, &arena);
            if(mp3_sound.get_size())
            {
              memcpy(&merged_tts[merged_tts_length], mp3_sound.data, mp3_sound.get_size());
              merged_tts_length += mp3_sound.get_size();
              final_sound.sample_count += mp3_sound.sample_count;
            }
            
            memset(tts_text, 0, c_tts_text_buffer_size);
            memset(tts_text_encoded, 0, c_tts_text_encoded_buffer_size);
            encoded_char_count = 0;
            tts_text_length = 0;
          }
          
          // This copies the Sound data from above (before if) into the merged tts
          memcpy(&merged_tts[merged_tts_length], sound.data, sound.get_size());
          merged_tts_length += sound.get_size();
          final_sound.sample_count += sound.sample_count;
        }
        else
        {
          sprintf(&tts_text[tts_text_length], "%s ", word);
          tts_text_length += (int)strlen(word) + 1;
        }
      }
      else
      {
        sprintf(&tts_text[tts_text_length], "%s ", word);
        tts_text_length += (int)strlen(word) + 1;
      }
    }
    
    if(tts_text_length > 0)
    {
      // Encode the message
      for(int char_idx = 0; char_idx < tts_text_length; char_idx++)
      {
        u8 c = tts_text[char_idx];
        s32 code = c_uri_encode_tbl_codes[c];
        
        if (code)
        {
          *((s32 *)&tts_text_encoded[encoded_char_count]) = code;
          encoded_char_count += 3;
        }
        else
        {
          tts_text_encoded[encoded_char_count++] = c;
        }
      }
      
      s_mp3_sound mp3_sound = get_tts(tts_text_encoded, current_voice, &arena);
      if(mp3_sound.get_size())
      {
        memcpy(&merged_tts[merged_tts_length], mp3_sound.data, mp3_sound.get_size());
        merged_tts_length += mp3_sound.get_size();
        final_sound.sample_count += mp3_sound.sample_count;
      }
      
      memset(tts_text, 0, c_tts_text_buffer_size);
      memset(tts_text_encoded, 0, c_tts_text_encoded_buffer_size);
      encoded_char_count = 0;
      tts_text_length = 0;
    }
    
    if(merged_tts_length > 0)
    {
      final_sound.channels = 2;
      final_sound.data = (s16*)merged_tts;
      platform_play_sound(final_sound);
    }
    
    arena.used = 0;
  }
  
  return 0;
}

void add_tts_to_queue(char* msg)
{
  if (msg && strlen(msg) >= 1 &&
      tts_queue.tts_message_count < c_max_tts_queue_entries)
  {
    int queue_idx =
    (tts_queue.current_queue_idx + tts_queue.tts_message_count) %
      c_max_tts_queue_entries;
    
    s_tts_message tts_message = zero;
    memcpy(tts_message.message, msg, strlen(msg));
    tts_queue.tts_queue[queue_idx] = tts_message;
    tts_queue.tts_message_count++;
  }
}

func void complete_redemption(char* header, char* reward_id,
                              char* request_id, s_lin_arena* arena)
{
  char url[1024] = {};
  sprintf(url,
          "https://api.twitch.tv/helix/channel_points/custom_rewards/redemptions?"
          "broadcaster_id=%d&reward_id=%s&id=%s",
          c_twitch_broadcaster_id, reward_id, request_id);
  
  char* data = "{\"status\": \"FULFILLED\"}";
  s_http_get result = http_request(url, header, arena, L"PATCH", data);
  assert(result.status_code == 0);
}

DWORD handle_redemptions(void* param)
{
  unreferenced(param);
  
  s_lin_arena arena = zero;
  arena.init(c_mb * 5);
  
  char header[128] = {};
  sprintf(header, "Authorization: Bearer %s\r\nClient-Id: %s\r\nContent-Type: application/json",
          c_twitch_refresh_token, c_twitch_client_id);
  
  // Get All redemptions first
  char* request = format_text("https://api.twitch.tv/helix/channel_points/custom_rewards?"
                              "broadcaster_id=%d&only_manageable_rewards=true",
                              c_twitch_broadcaster_id);
  
  
  s_http_get result = http_get(request, header, &arena);
  char* result_text = result.text;
  while(true)
  {
    if(str_find_from_left(result_text, "id") == -1)
    {
      break;
    }
    
    s_redemption* redemption = &redemptions[redemption_count++];
    
    int id_pos = str_find_from_left(result_text, "\"id\"");
    if(id_pos != -1)
    {
      memcpy(redemption->reward_id, &result_text[id_pos + 6], 36);
    }
    
    int cost_pos = str_find_from_left(result_text, "\"cost\"");
    if(cost_pos != -1)
    {
      char* cost = &result_text[cost_pos + 7];
      redemption->cost = atoi(cost);
    }
    
    int title_pos = str_find_from_left(result_text, "\"title\"");
    if(title_pos != -1)
    {
      char* title = &result_text[title_pos + 9];
      int title_end_pos = str_find_from_left(title, "\"");
      memcpy(redemption->title, title, title_end_pos);
    }
    
    result_text += title_pos + 50;
  }
  
  while(true)
  {
    Sleep(300);
    
    for(int redemption_index = 0; redemption_index < redemption_count; redemption_index++)
    {
      arena.used = 0;
      s_redemption redemption = redemptions[redemption_index];
      
      request = format_text("https://api.twitch.tv/helix/"
                            "channel_points/custom_rewards/redemptions?"
                            "broadcaster_id=%d&status=UNFULFILLED&reward_id=%s",
                            c_twitch_broadcaster_id, redemption.reward_id);
      
      result = http_get(request, header, &arena);
      
      // log_info(result.text);
      
      // Find ID of the request
      char request_id[37] = {};
      int id_pos = str_find_from_left(result.text, "\"id\"");
      if(id_pos != -1)
      {
        memcpy(request_id, &result.text[id_pos + 6], 36);
      }
      
      if(str_find_from_left(result.text, "UNFULFILLED") != -1)
      {
        if(strcmp(redemption.title, "Random Clip") == 0)
        {
          // TODO Turn into a file to load, or create a "bot state"
          char* disabled_videos[] =
          {
            "bit_clip.mp4"
          };
          
          // Grab all videos that are not Disabled
          
          int file_name_count = 0;
          char file_names[128][256] = {};
          
          while(get_next_filename(file_names[file_name_count], "assets/videos/*"))
          {
            char* file_name = file_names[file_name_count];
            
            bool video_disabled = false;
            for(int video_idx = 0; video_idx < array_count(disabled_videos);
                video_idx++)
            {
              char* disabled_video = disabled_videos[video_idx];
              if(strcmp(file_name, disabled_video) == 0)
              {
                video_disabled = true;
              }
            }
            
            if(!video_disabled)
            {
              file_name_count++;
            }
            else
            {
              memset(file_name, 0, 256);
            }
          }
          
          local_persist int already_played_count = 0;
          local_persist char already_played[128][256] = {};
          local_persist s_random random = {};
          
          if(already_played_count >= file_name_count)
          {
            already_played_count = 0;
            memset(already_played, 0, 128 * 256);
          }
          
          char* chosen_video = 0;
          while(true)
          {
            int possible_video_idx = random.rand_range_ii(0, file_name_count - 1);
            char* possible_video = file_names[possible_video_idx];
            
            bool video_already_played = false;
            for(int video_idx = 0; video_idx < already_played_count;
                video_idx++)
            {
              char* played_video = already_played[video_idx];
              if(strcmp(played_video, possible_video) == 0)
              {
                video_already_played = true;
                break;
              }
            }
            
            if(!video_already_played)
            {
              chosen_video = possible_video;
              break;
            }
          }
          
          assert(chosen_video && "Could not find a Video to play");
          
          // Add to already_played
          memcpy(already_played[already_played_count++],
                 chosen_video, strlen(chosen_video));
          
          // Full Path
          chosen_video = format_text("assets/videos/%s", chosen_video);
          log_info(chosen_video);
          
          char* video_template =
            read_file_quick("video_template.html", &arena);
          char* empty_video_template =
            read_file_quick("empty_video_template.html", &arena);
          
          char* video_html = format_text(video_template, chosen_video);
          write_file_quick("video.html",
                           video_html,
                           (int)strlen(video_html));
          
          unsigned long duration = videoinfo_duration(chosen_video);
          
          Sleep(duration * 1000);
          write_file_quick("video.html",
                           empty_video_template,
                           (int)strlen(empty_video_template));
          
          complete_redemption(header, redemption.reward_id, request_id, &arena);
          
          continue;
        }
        
        if(strcmp(redemption.title, "GIGACHAD") == 0)
        {
          int keys[] = {VK_SHIFT,  VK_LCONTROL, VK_LMENU, VK_F2};
          press_key(keys, array_count(keys));
          add_tts_to_queue("-chad");
          complete_redemption(header, redemption.reward_id, request_id, &arena);
          Sleep(10000);
          press_key(keys, array_count(keys));
          
          continue;
        }
        
        if(strcmp(redemption.title, "Sadge") == 0)
        {
          // Globally Press "CRTL + SHIFT + ALT + F3"
          int keys[] = {VK_SHIFT,  VK_LCONTROL, VK_LMENU, VK_F3};
          press_key(keys, array_count(keys));
          add_tts_to_queue("-sad");
          complete_redemption(header, redemption.reward_id, request_id, &arena);
          Sleep(10000);
          press_key(keys, array_count(keys));
          
          continue;
        }
        
        if(strcmp(redemption.title, "5Header") == 0)
        {
          // Globally Press "CRTL + SHIFT + ALT + F1"
          int keys[] = {VK_SHIFT,  VK_LCONTROL, VK_LMENU, VK_F1};
          press_key(keys, array_count(keys));
          add_tts_to_queue("c3: Ahm, master! -typing01 The calculations are in, and Ahm, -typing02 The powers of your Brain are overwhelming");
          complete_redemption(header, redemption.reward_id, request_id, &arena);
          Sleep(10000);
          press_key(keys, array_count(keys));
          
          continue;
        }
        
        if(strcmp(redemption.title, "TTS") == 0)
        {
          // TODO: redemption.text
          
          continue;
        }
        
        if(strcmp(redemption.title, "Skip TTS") == 0)
        {
          // Remove the first entry in the TTS queue
          
          continue;
        }
        
        if(strcmp(redemption.title, "Clown") == 0)
        {
          // Globally Press "CRTL + SHIFT + ALT + F4"
          int keys[] = {VK_SHIFT,  VK_LCONTROL, VK_LMENU, VK_F4};
          press_key(keys, array_count(keys));
          add_tts_to_queue("-clown");
          complete_redemption(header, redemption.reward_id, request_id, &arena);
          Sleep(10000);
          press_key(keys, array_count(keys));
          
          continue;
        }
        
        if(strcmp(redemption.title, "Shock!") == 0)
        {
          // Globally Press "CRTL + SHIFT + ALT + F5"
          int keys[] = {VK_SHIFT,  VK_LCONTROL, VK_LMENU, VK_F5};
          press_key(keys, array_count(keys));
          add_tts_to_queue("-wow");
          complete_redemption(header, redemption.reward_id, request_id, &arena);
          Sleep(10000);
          press_key(keys, array_count(keys));
          
          continue;
        }
      }
    }
  }
}

DWORD handle_twitch_chat(void* param)
{
  unreferenced(param);
  
  g_target_channel = g_user_data.target_channel;
  
  s_lin_arena arena = zero;
  arena.init(100 * c_mb);
  
  constexpr char* default_port = "6667";
  
  WSADATA wsaData;
  struct addrinfo* result = null;
  struct addrinfo* ptr = null;
  struct addrinfo hints = zero;
  
  int iResult;
  
  // Initialize Winsock
  int wsa_startup_result = WSAStartup(MAKEWORD(2,2), &wsaData);
  if(wsa_startup_result != 0)
  {
    printf("WSAStartup failed with error: %d\n", wsa_startup_result);
    return 1;
  }
  
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  
  // Resolve the server address and port
  int get_addr_result = getaddrinfo("irc.chat.twitch.tv", default_port, &hints, &result);
  if(get_addr_result != 0)
  {
    printf("getaddrinfo failed with error: %d\n", get_addr_result);
    WSACleanup();
    return 1;
  }
  
  char buffer[8192] = zero;
  s_socket connect_socket = zero;
  
  
  float periodicMessagesTimer = 0;
  LARGE_INTEGER ticksPerSecond;
  LARGE_INTEGER lastTickCount, currentTickCount;
  QueryPerformanceFrequency(&ticksPerSecond);
  QueryPerformanceCounter(&lastTickCount);
  float dt = 0;
  
  while(true)
  {
    connect_socket = zero;
    for(ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
      // Create a SOCKET for connecting to server
      connect_socket.win_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
      if(connect_socket.win_socket == INVALID_SOCKET)
      {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        assert(false);
        return 1;
      }
      
      // Connect to server.
      iResult = connect(connect_socket.win_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
      if(iResult == SOCKET_ERROR)
      {
        closesocket(connect_socket.win_socket);
        connect_socket.win_socket = INVALID_SOCKET;
        continue;
      }
      break;
    }
    
    if(connect_socket.win_socket == INVALID_SOCKET)
    {
      printf("Unable to connect to server!\n");
      WSACleanup();
      assert(false);
      return 1;
    }
    
    send_string(&connect_socket, format_text("PASS oauth:%s", c_access_token));
    send_string(&connect_socket, format_text("NICK %s", g_target_channel.data));
    send_string(&connect_socket, "CAP REQ :twitch.tv/commands twitch.tv/tags");
    receive(&connect_socket, buffer, sizeof(buffer));
    send_string(&connect_socket, format_text("JOIN #%s", g_target_channel.data));
    receive(&connect_socket, buffer, sizeof(buffer));
    if(connect_socket.error)
    {
      closesocket(connect_socket.win_socket);
      continue;
    }
    break;
  }
  
  freeaddrinfo(result);
  
  g_twitch_chat_mutex.lock();
  g_twitch_chat_event = e_twitch_chat_event_connected;
  g_twitch_chat_mutex.unlock();
  
  while(true)
  {
    QueryPerformanceCounter(&currentTickCount);
    
    u64 elapsedTicks = currentTickCount.QuadPart - lastTickCount.QuadPart;
    
    // Convert to Microseconds to not loose precision, by deviding a small numbner by a large one
    u64 elapsedTimeInMicroseconds = (elapsedTicks * 1000000) / ticksPerSecond.QuadPart;
    
    lastTickCount = currentTickCount;
    
    // Time in milliseconds
    dt = (float)elapsedTimeInMicroseconds / 1000.0f;
    
    // Lock dt to 50ms
    if (dt > 50.0f)
    {
      dt = 50.0f;
    }
    
    
    receive(&connect_socket, buffer, sizeof(buffer));
    assert(!connect_socket.error);
    // printf("%s\n", buffer);
    
    char* temp = buffer;
    char* query = "PRIVMSG #";
    int privmsg_index = str_find_from_left(temp, query);
    int ping_index = str_find_from_left(temp, "PING");
    int bits_amount = 0;
    char var_name[64][600] = zero;
    char var_value[64][600] = zero;
    char* username = null;
    int var_index = 0;
    if(privmsg_index != -1)
    {
      char* cursor = temp;
      b8 found_end = false;
      while(*cursor && !found_end)
      {
        int var_char_index = 0;
        while(*cursor != '=')
        {
          var_name[var_index][var_char_index++] = *cursor;
          cursor++;
        }
        var_name[var_index][var_char_index] = 0;
        cursor++;
        
        var_char_index = 0;
        while(*cursor && *cursor != ';' && *cursor != '\r')
        {
          var_value[var_index][var_char_index++] = *cursor;
          cursor++;
        }
        
        if(!*cursor || *cursor == '\r') { found_end = true; }
        
        var_value[var_index][var_char_index] = 0;
        
        if(strcmp(var_name[var_index], "display-name") == 0)
        {
          username = var_value[var_index];
        }
        
        if(strcmp(var_name[var_index], "user-type") == 0)
        {
          break;
        }
        
        if(strcmp(var_name[var_index], "bits") == 0)
        {
          char* bits_amount_string = var_value[var_index];
          
          bits_amount = atoi(bits_amount_string);
        }
        
        cursor++;
        var_index++;
      }
      
      char* msg = &temp[privmsg_index];
      msg = &msg[str_find_from_left(msg, ":") + 1];
      str_trim_from_right(msg);
      
      if(bits_amount >= c_min_bits_for_notification)
      {
        char* bits_message = format_text("-blblbl storm: @ %s, Thank you for %d Bits: %s",
                                         username, bits_amount, msg);
        add_tts_to_queue(bits_message);
      }
      
      for(int command_index = 0; command_index < array_count(commands); command_index++)
      {
        s_command command = commands[command_index];
        if(strncmp(msg, command.command, strlen(command.command)) == 0)
        {
          // TODO: Sprintf to reply to the user
          send_chat_command(&connect_socket, username, command.response);
          // send_string(&connect_socket, command.response);
        }
      }
      
      if(strncmp(msg, "!tts ", strlen("!tts ")) == 0)
      {
        // Skip !tts
        msg += 5;
        add_tts_to_queue(msg);
      }
      
      // s_str<600> foo;
      // foo.from_cstr(format_text("%s: %s", username, msg));
      // if(messages.count >= messages.max_elements())
      // {
      // messages.count -= 1;
      // }
      // messages.insert(0, foo);
    }
    else if(ping_index != -1)
    {
      log_info("Received PING");
      send_string(&connect_socket, "PONG tmi.twitch.tv");
      assert(!connect_socket.error);
      log_info("Sent PONG");
    }
    
    memset(buffer, 0, 8192);
    arena.used = 0;
  }
  
  return 0;
}

func b8 send_string(s_socket* in_socket, char* data)
{
  assert(in_socket->win_socket != INVALID_SOCKET);
  if(in_socket->error) { return false; }
  
  data = format_text("%s\r\n", data);
  int len = (int)strlen(data);
  
  int send_result = send(in_socket->win_socket, data, len, 0);
  if(send_result == SOCKET_ERROR)
  {
    printf("send failed with error: %d\n", WSAGetLastError());
    in_socket->error = true;
    return false;
  }
  assert(send_result == len);
  return true;
}

func b8 receive(s_socket* in_socket, char* buffer, int buffer_size)
{
  assert(in_socket->win_socket != INVALID_SOCKET);
  if(in_socket->error) { return false; }
  
  int recv_result = recv(in_socket->win_socket, buffer, buffer_size - 1, 0);
  if(recv_result > 0)
  {
    buffer[recv_result] = 0;
    return true;
  }
  else if(recv_result == 0)
  {
    in_socket->error = true;
    return false;
  }
  else
  {
    in_socket->error = true;
    return false;
  }
}

func PROC load_gl_func(char* name)
{
  PROC result = wglGetProcAddress(name);
  if(!result)
  {
    printf("Failed to load %s\n", name);
    assert(false);
  }
  return result;
}

void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
  unreferenced(userParam);
  unreferenced(length);
  unreferenced(id);
  unreferenced(type);
  unreferenced(source);
  if(severity >= GL_DEBUG_SEVERITY_HIGH)
  {
    printf("GL ERROR: %s\n", message);
    assert(false);
  }
}

func void apply_event_to_input(s_input* in_input, s_stored_input event)
{
  in_input->keys[event.key].is_down = event.is_down;
  in_input->keys[event.key].count += 1;
}

func b8 check_for_shader_errors(u32 id, char* out_error)
{
  int compile_success;
  char info_log[1024];
  glGetShaderiv(id, GL_COMPILE_STATUS, &compile_success);
  
  if(!compile_success)
  {
    glGetShaderInfoLog(id, 1024, null, info_log);
    printf("Failed to compile shader:\n%s", info_log);
    
    if(out_error)
    {
      strcpy(out_error, info_log);
    }
    
    return false;
  }
  return true;
}


func s_v2 get_text_size_with_count(char* text, e_font font_id, int count)
{
  assert(count >= 0);
  if(count <= 0) { return zero; }
  s_font* font = &g_font_arr[font_id];
  
  s_v2 size = zero;
  size.y = font->size;
  
  for(int char_i = 0; char_i < count; char_i++)
  {
    char c = text[char_i];
    s_glyph glyph = font->glyph_arr[c];
    if(char_i == count - 1 && c != ' ')
    {
      size.x += glyph.width;
    }
    else
    {
      size.x += glyph.advance_width * font->scale;
    }
  }
  
  return size;
}

func s_v2 get_text_size(char* text, e_font font_id)
{
  return get_text_size_with_count(text, font_id, (int)strlen(text));
}

func s_font load_font(char* path, float font_size, s_lin_arena* arena)
{
  s_font font = zero;
  font.size = font_size;
  
  u8* file_data = (u8*)read_file_quick(path, arena);
  assert(file_data);
  
  stbtt_fontinfo info = zero;
  stbtt_InitFont(&info, file_data, 0);
  
  stbtt_GetFontVMetrics(&info, &font.ascent, &font.descent, &font.line_gap);
  
  font.scale = stbtt_ScaleForPixelHeight(&info, font_size);
  constexpr int max_chars = 128;
  s_sarray<u8*, max_chars> bitmap_arr;
  constexpr int padding = 10;
  int total_width = padding;
  int total_height = 0;
  for(int char_i = 0; char_i < max_chars; char_i++)
  {
    s_glyph glyph = zero;
    u8* bitmap = stbtt_GetCodepointBitmap(&info, 0, font.scale, char_i, &glyph.width, &glyph.height, 0, 0);
    stbtt_GetCodepointBox(&info, char_i, &glyph.x0, &glyph.y0, &glyph.x1, &glyph.y1);
    stbtt_GetGlyphHMetrics(&info, char_i, &glyph.advance_width, null);
    
    total_width += glyph.width + padding;
    total_height = max(glyph.height + padding * 2, total_height);
    
    font.glyph_arr[char_i] = glyph;
    bitmap_arr.add(bitmap);
  }
  
  // @Fixme(tkap, 21/04/2023): Use arena
  u8* gl_bitmap = (u8*)calloc(1, sizeof(u8) * 4 * total_width * total_height);
  
  int current_x = padding;
  for(int char_i = 0; char_i < max_chars; char_i++)
  {
    s_glyph* glyph = &font.glyph_arr[char_i];
    u8* bitmap = bitmap_arr[char_i];
    for(int y = 0; y < glyph->height; y++)
    {
      for(int x = 0; x < glyph->width; x++)
      {
        u8 src_pixel = bitmap[x + y * glyph->width];
        u8* dst_pixel = &gl_bitmap[((current_x + x) + (padding + y) * total_width) * 4];
        dst_pixel[0] = src_pixel;
        dst_pixel[1] = src_pixel;
        dst_pixel[2] = src_pixel;
        dst_pixel[3] = src_pixel;
      }
    }
    
    glyph->uv_min.x = current_x / (float)total_width;
    glyph->uv_max.x = (current_x + glyph->width) / (float)total_width;
    
    glyph->uv_min.y = padding / (float)total_height;
    glyph->uv_max.y = (padding + glyph->height) / (float)total_height;
    
    current_x += glyph->width + padding;
  }
  
  font.texture = load_texture_from_data(gl_bitmap, total_width, total_height, GL_LINEAR);
  
  foreach_raw(bitmap_i, bitmap, bitmap_arr)
  {
    stbtt_FreeBitmap(bitmap, null);
  }
  
  free(gl_bitmap);
  
  return font;
}

func s_texture load_texture_from_data(void* data, int width, int height, u32 filtering)
{
  assert(data);
  u32 id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
  
  s_texture texture = zero;
  texture.id = id;
  texture.size = v2(width, height);
  return texture;
}

func s_texture load_texture_from_file(char* path, u32 filtering)
{
  int width, height, num_channels;
  void* data = stbi_load(path, &width, &height, &num_channels, 4);
  s_texture texture = load_texture_from_data(data, width, height, filtering);
  stbi_image_free(data);
  return texture;
}

func b8 is_key_down(int key)
{
  assert(key < c_max_keys);
  return g_input.keys[key].is_down;
}

func b8 is_key_up(int key)
{
  assert(key < c_max_keys);
  return !g_input.keys[key].is_down;
}

func b8 is_key_pressed(int key)
{
  assert(key < c_max_keys);
  return (g_input.keys[key].is_down && g_input.keys[key].count == 1) || g_input.keys[key].count > 1;
}

func b8 is_key_released(int key)
{
  assert(key < c_max_keys);
  return (!g_input.keys[key].is_down && g_input.keys[key].count == 1) || g_input.keys[key].count > 1;
}

func void press_key(int* keys, int key_count)
{
  INPUT inputs[32] = {0};
  for(int key_index = 0; key_index < key_count; key_index++)
  {
    INPUT* input = &inputs[key_index];
    input->type = INPUT_KEYBOARD;
    input->ki.wVk = (WORD)keys[key_index];
    
  }
  int send_keys = SendInput(key_count, inputs, sizeof(INPUT));
  assert(keys != 0 && send_keys != 0);
  
  for(int key_index = 0; key_index < key_count; key_index++)
  {
    INPUT* input = &inputs[key_index];
    input->ki.dwFlags = KEYEVENTF_KEYUP;
  }
  send_keys = SendInput(key_count, inputs, sizeof(INPUT));
  assert(send_keys != 0);
}

func s_char_event get_char_event()
{
  if(!char_event_arr.is_empty())
  {
    return char_event_arr.remove_and_shift(0);
  }
  return zero;
}

func void maybe_connect_to_twitch_chat(s_lin_arena* arena)
{
  assert(!g_connected_to_twitch_chat);
  if(g_user_data.target_channel.len <= 0) { return; }
  
  char* header = format_text(
                             "Authorization: Bearer %s\r\n"
                             "Client-Id: %s\r\n"
                             "Content-Type: application/json",
                             c_twitch_refresh_token, c_twitch_client_id
                             );
  char* request = format_text("https://api.twitch.tv/helix/users?login=%s", g_user_data.target_channel.data);
  s_http_get result = http_get(request, header, arena);
  
  s_json* json = parse_json(result.text);
  if(!json) { return; } // @Note(tkap, 04/05/2023): Shouldn't happen
  s_json* data = json->get("data");
  if(!data) { return; } // @Note(tkap, 04/05/2023): Shouldn't happen
  
  // @Note(tkap, 04/05/2023): Will happen if "target_channel" is not a valid twitch channel
  if(data->value_count <= 0)
  {
    make_timed_message("Invalid twitch channel", rgb(0xFF0000));
    return;
  }
  
  g_twitch_chat_thread.init(handle_twitch_chat);
}

func void maybe_disconnect_from_twitch_chat()
{
  if(g_connected_to_twitch_chat)
  {
    make_timed_message("Disconnected", rgb(0xFF0000));
    g_connected_to_twitch_chat = false;
    g_twitch_chat_thread.terminate();
  }
}

func void save_user_data()
{
  s_str_sbuilder<4096> builder;
  struct s_temp
  {
    char* name;
    s_str<64>* target;
  };
  constexpr s_temp temp_arr[] = {
    {.name = "target_channel", .target = &g_user_data.target_channel},
    {.name = "access_token", .target = &g_user_data.access_token},
  };
  for(int temp_i = 0; temp_i < array_count(temp_arr); temp_i++)
  {
    auto temp = temp_arr[temp_i];
    builder.add_line("%s=%s", temp.name, temp.target->data);
  }
  write_file_quick("user_data.txt", builder.cstr(), builder.len);
}

func void make_timed_message(char* text, s_v4 color)
{
  s_timed_message msg = zero;
  msg.message.from_cstr(text);
  msg.duration = 2;
  msg.color = color;
  app->timed_message_arr.add_checked(msg);
}