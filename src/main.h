
enum e_twitch_chat_event
{
  e_twitch_chat_event_none,
  e_twitch_chat_event_connected,
  e_twitch_chat_event_disconnected,
};

struct s_socket
{
  b8 error;
  SOCKET win_socket;
};

struct s_key
{
  b8 is_down;
  int count;
};

struct s_stored_input
{
  b8 is_down;
  int key;
};

struct s_input
{
  s_key keys[c_max_keys];
};


struct s_glyph
{
  int advance_width;
  int width;
  int height;
  int x0;
  int y0;
  int x1;
  int y1;
  s_v2 uv_min;
  s_v2 uv_max;
};

struct s_texture
{
  u32 id;
  s_v2 size;
  s_v2 sub_size;
};

struct s_font
{
  float size;
  float scale;
  int ascent;
  int descent;
  int line_gap;
  s_texture texture;
  s_carray<s_glyph, 1024> glyph_arr;
};

struct s_char_event
{
  b8 is_symbol;
  int c;
};


enum e_tts_api
{
  tts_api_streamelements,
  tts_api_tiktok
};

struct s_tts_voice
{
  char* voice_name;
  char* api_voice_name;
  e_tts_api tts_api;
};

global s_tts_voice tts_voices[] =
{
  {"brian", "Brian", tts_api_streamelements},
  {"storm", "en_us_stormtrooper", tts_api_tiktok},
  {"stitch", "en_us_stitch", tts_api_tiktok},
  {"ghost", "en_us_ghostface", tts_api_tiktok},
  {"chewbacca", "en_us_chewbacca", tts_api_tiktok},
  {"c3", "en_us_c3po", tts_api_tiktok},
  {"rocket", "en_us_rocket", tts_api_tiktok},
  {"sing", "en_male_m03_lobby", tts_api_tiktok},
  {"sing2", "en_female_f08_salut_damour", tts_api_tiktok},
  {"old", "en_male_narration", tts_api_tiktok},
  {"jp5", "jp_005", tts_api_tiktok},
  {"pirate", "en_male_pirate", tts_api_tiktok},
  {"zhiyu", "Zhiyu", tts_api_streamelements},
  {"trickster", "en_male_grinch", tts_api_tiktok},
  {"wizard", "en_male_wizard", tts_api_tiktok},
  {"opera", "en_female_ht_f08_halloween", tts_api_tiktok},
  {"leota", "en_female_madam_leota", tts_api_tiktok},
  {"ghosthost", "en_male_ghosthost", tts_api_tiktok},
  {"empathetic", "en_female_samc", tts_api_tiktok},
  {"serious", "en_male_cody", tts_api_tiktok},
  {"grandma", "en_female_grandma", tts_api_tiktok},
};

struct s_tts_message
{
  char message[c_max_tts_message_chars];
};

struct s_tts_queue
{
  int tts_message_count;
  int current_queue_idx;
  s_tts_message tts_queue[c_max_tts_queue_entries];
};

struct s_user_data
{
  s_str<64> access_token;
  s_str<64> target_channel;
};

struct s_timed_message
{
  float time_passed;
  float duration;
  s_v4 color;
  s_str<64> message;
};

struct s_app
{
  s_sarray<s_timed_message, 16> timed_message_arr;
};

struct s_redemption
{
  char title[128];
  char reward_id[42];

  bool paused;
  bool enabled;
  bool user_input;
  int cost;
  int cooldown_seconds;
  int max_uses_per_user_per_stream;
  int max_uses_per_stream;
};

global int redemption_count;
global s_redemption redemptions[c_max_redemptions];

global s_redemption redemptions2[] =
{
  {"tts_reward", "8df23dae-194b-4b93-9fa0-67f029b1fbdd"},
  {"video", "1610268b-ff0e-48e6-90f5-11677cdf1893"},
  {"5head", "89c58dd6-f359-469c-8b5a-1b0691fbb8fb"},
  {"GIGACHAD", "3ac7ade1-3f6f-4115-9c3a-d935ee5c4a86"},
  {"Sad", "51e5c659-2140-4ba9-bb2b-3d0cf851f286"},
  {"Clown", "a07a634d-3a45-42ba-a5d2-38fffeada1e3"},
  {"Shock", "c4f46580-6bfd-4c7d-bc7c-15cf86174bfe"},
  {"Skip TTS", "c4f46580-6bfd-4c7d-bc7c-15cf8617"},
};

struct s_command
{
  char* command;
  char* response;
};

LRESULT window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);

DWORD handle_twitch_chat(void* param);
DWORD handle_tts_queue(void* param);
DWORD handle_redemptions(void* param);

func b8 send_string(s_socket* in_socket, char* data);
func b8 receive(s_socket* in_socket, char* buffer, int buffer_size);
func PROC load_gl_func(char* name);
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
func void apply_event_to_input(s_input* in_input, s_stored_input event);
func b8 check_for_shader_errors(u32 id, char* out_error);
func s_v2 get_text_size(char* text, e_font font_id);
func s_font load_font(char* path, float font_size, s_lin_arena* arena);
func s_texture load_texture_from_data(void* data, int width, int height, u32 filtering);
func b8 is_key_down(int key);
func b8 is_key_up(int key);
func b8 is_key_pressed(int key);
func b8 is_key_released(int key);
// func void press_key(int keys);
func void press_key(int* keys, int key_count);
func s_texture load_texture_from_file(char* path, u32 filtering);
func s_char_event get_char_event();
func s_v2 get_text_size_with_count(char* text, e_font font_id, int count);
func s_mp3_sound get_tts(char* encoded_text, s_tts_voice tts_voice, s_lin_arena* arena);
func void maybe_connect_to_twitch_chat(s_lin_arena* arena);
func void maybe_disconnect_from_twitch_chat();
func void save_user_data();
func void make_timed_message(char* text, s_v4 color = rgb(0x00FF00));