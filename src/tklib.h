
// @Note(tkap, 08/03/2023): Needed for atan2f
#ifndef _MATH_H
#include <math.h>
#endif

// @Note(tkap, 08/03/2023): Needed for memset
#ifndef INC_STRING
#include <string.h>
#endif

// @Note(tkap, 08/03/2023): va_start
#ifndef _INC_STDARG
#include <stdarg.h>
#endif

// @Note(tkap, 08/03/2023): atoi
#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

// @Note(tkap, 08/03/2023): vsnprintf
#ifndef _INC_STDIO
#include <stdio.h>
#endif

#ifndef _STDINT
#include <stdint.h>
#endif

#ifndef _WINDOWS_
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// @Note(tkap, 21/03/2023): PathFileExistsA
#ifndef _INC_SHLWAPI
#include <shlwapi.h>
#endif

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef s8 b8;
typedef s32 b32;

#define func static
#define global static
#define local_persist static
#define zero {}
#define null NULL
#define array_count(arr) (sizeof((arr)) / sizeof((arr)[0]))
#define invalid_default_case default: { assert(!"Invalid default case"); }
#define invalid_else else { assert(false); }

#define breakable_block__(a, b) for(int a##b = 1; a##b--;)
#define breakable_block_(a) breakable_block__(tkinternal_condblock, a)
#define breakable_block breakable_block_(__LINE__)

#define unreferenced(thing) (void)thing

#ifdef m_debug
#define assert(cond) do { if(!(cond)) { on_failed_assert(#cond, __FILE__, __LINE__); }} while(0)
#else
#define assert(cond)
#endif

global constexpr s64 c_max_s8 = INT8_MAX;
global constexpr s64 c_max_s16 = INT16_MAX;
global constexpr s64 c_max_s32 = INT32_MAX;
global constexpr s64 c_max_s64 = INT64_MAX;
global constexpr s64 c_max_u8 = UINT8_MAX;
global constexpr s64 c_max_u16 = UINT16_MAX;
global constexpr s64 c_max_u32 = UINT32_MAX;
global constexpr u64 c_max_u64 = UINT64_MAX;

global constexpr float tau = 6.2831853071f;

global constexpr s64 c_kb = 1024;
global constexpr s64 c_mb = 1024 * c_kb;
global constexpr s64 c_gb = 1024 * c_mb;

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		function headers start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

template <typename T>
[[nodiscard]]
func T at_most(T high, T current);

func char* format_text(char* text, ...);
func void on_failed_assert(char* cond, char* file, int line);
func b8 is_whitespace(char c);
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		function headers end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


typedef DWORD (*thread_proc)(void*);

struct s_mutex
{
  HANDLE mutex;

  void lock()
  {
    assert(mutex);
    DWORD wait_result = WaitForSingleObject(mutex, INFINITE);

    if(wait_result == WAIT_OBJECT_0)
    {
      return;
    }
    invalid_else;
  }

  void unlock()
  {
    assert(mutex);
    if(!ReleaseMutex(mutex))
    {
      assert(false);
    }
  }
};

func s_mutex make_mutex()
{
  s_mutex mutex = zero;
  mutex.mutex = CreateMutex(null, false, null);
  assert(mutex.mutex);
  return mutex;
}

struct s_thread
{
  HANDLE thread;
  thread_proc target_func;

  DWORD init(thread_proc in_func, void* param = null)
  {
    target_func = in_func;
    assert(target_func);

    DWORD id = 0;
    thread = CreateThread(null, 0, in_func, param, 0, &id);
    return id;
  }

  void terminate()
  {
    assert(thread);
    BOOL result = TerminateThread(thread, 0);
    assert(result);
  }
};


struct s_v2
{
  float x;
  float y;
};

template <typename T>
constexpr func s_v2 v2(T x, T y)
{
  return {(float)x, (float)y};
}

template <typename t1, typename t2>
constexpr func s_v2 v2(t1 a, t2 b)
{
  return {(float)a, (float)b};
}

func float v2_angle(s_v2 v)
{
  return atan2f(v.y, v.x);
}

struct s_v4
{
  float x;
  float y;
  float z;
  float w;
};

template <typename T>
func s_v4 v4(T v)
{
  return {(float)v, (float)v, (float)v, (float)v};
}

template <typename T>
func s_v4 v4(T x, T y, T z)
{
  return {(float)x, (float)y, (float)z, 1.0f};
}

template <typename T>
func s_v4 v4(T x, T y, T z, T w)
{
  return {(float)x, (float)y, (float)z, (float)w};
}

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		random start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

struct s_random
{
  u32 seed;

  u32 randu()
  {
    seed = seed * 2147001325 + 715136305;
    return 0x31415926 ^ ((seed >> 16) + (seed << 16));
  }

  f64 randf()
  {
    return (f64)randu() / (f64)4294967295;
  }

  float randf32()
  {
    return (float)randu() / (float)4294967295;
  }

  f64 randf2()
  {
    return randf() * 2 - 1;
  }

  u64 randu64()
  {
    return (u64)(randf() * (f64)c_max_u64);
  }

  // min inclusive, max inclusive
  int rand_range_ii(int min, int max)
  {
    if(min > max)
    {
      int temp = min;
      min = max;
      max = temp;
    }

    return min + (randu() % (max - min + 1));
  }

  // min inclusive, max exclusive
  int rand_range_ie(int min, int max)
  {
    if(min > max)
    {
      int temp = min;
      min = max;
      max = temp;
    }

    return min + (randu() % (max - min));
  }

  float randf_range(float min_val, float max_val)
  {
    if(min_val > max_val)
    {
      float temp = min_val;
      min_val = max_val;
      max_val = temp;
    }

    float r = (float)randf();
    return min_val + (max_val - min_val) * r;
  }

  b8 chance100(float chance)
  {
    assert(chance >= 0);
    assert(chance <= 100);
    return chance / 100 >= randf();
  }

  b8 chance1(float chance)
  {
    assert(chance >= 0);
    assert(chance <= 1);
    return chance >= randf();
  }

  int while_chance1(float chance)
  {
    int result = 0;
    while(chance1(chance))
    {
      result += 1;
    }
    return result;
  }
};

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		random end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		arrays start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
#define foreach__(a, index_name, element_name, array) if(0) finished##a: ; else for(auto element_name = &(array).elements[0];;) if(1) goto body##a; else while(1) if(1) goto finished##a; else body##a: for(int index_name = 0; index_name < (array).count && (bool)(element_name = &(array)[index_name]); index_name++)
#define foreach_(a, index_name, element_name, array) foreach__(a, index_name, element_name, array)
#define foreach(index_name, element_name, array) foreach_(__LINE__, index_name, element_name, array)

#define foreach_raw__(a, index_name, element_name, array) if(0) finished##a: ; else for(auto element_name = (array).elements[0];;) if(1) goto body##a; else while(1) if(1) goto finished##a; else body##a: for(int index_name = 0; index_name < (array).count && (void*)&(element_name = (array)[index_name]); index_name++)
#define foreach_raw_(a, index_name, element_name, array) foreach_raw__(a, index_name, element_name, array)
#define foreach_raw(index_name, element_name, array) foreach_raw_(__LINE__, index_name, element_name, array)

template <typename T, int N>
struct s_stack
{
  int count = 0;
  T elements[N];

  void push(T e)
  {
    assert(count < N);
    elements[count++] = e;
  }

  T pop()
  {
    assert(count > 0);
    return elements[--count];
  }

  T* get_last()
  {
    assert(count > 0);
    return &elements[count - 1];
  }

  b8 is_empty()
  {
    return count <= 0;
  }

};

template <typename T, int N>
struct s_sarray
{
  static_assert(N > 0);
  int count = 0;
  T elements[N];

  constexpr T& operator[](int index)
  {
    assert(index >= 0);
    assert(index < count);
    return elements[index];
  }

  constexpr T get(int index)
  {
    return (*this)[index];
  }

  constexpr void remove_and_swap(int index)
  {
    assert(index >= 0);
    assert(index < count);
    count -= 1;
    elements[index] = elements[count];
  }

  constexpr T remove_and_shift(int index)
  {
    assert(index >= 0);
    assert(index < count);
    T result = elements[index];
    count -= 1;

    int to_move = count - index;
    if(to_move > 0)
    {
      memcpy(elements + index, elements + index + 1, to_move * sizeof(T));
    }
    return result;
  }

  constexpr T* get_ptr(int index)
  {
    return &(*this)[index];
  }

  constexpr void swap(int index0, int index1)
  {
    assert(index0 >= 0);
    assert(index1 >= 0);
    assert(index0 < count);
    assert(index1 < count);
    assert(index0 != index1);
    T temp = elements[index0];
    elements[index0] = elements[index1];
    elements[index1] = temp;
  }

  constexpr T get_random(s_random* rng)
  {
    assert(count > 0);
    int index = rng->randu() % count;
    return elements[index];
  }

  constexpr T get_last()
  {
    assert(count > 0);
    return elements[count - 1];
  }

  constexpr T* get_last_ptr()
  {
    assert(count > 0);
    return &elements[count - 1];
  }

  constexpr int add(T element)
  {
    assert(count < N);
    elements[count] = element;
    count += 1;
    return count - 1;
  }

  constexpr b8 add_checked(T element)
  {
    if(count < N)
    {
      add(element);
      return true;
    }
    return false;
  }

  constexpr b8 contains(T what)
  {
    for(int element_i = 0; element_i < count; element_i++)
    {
      if(what == elements[element_i])
      {
        return true;
      }
    }
    return false;
  }

  constexpr void insert(int index, T element)
  {
    assert(index >= 0);
    assert(index < N);
    assert(index <= count);

    int to_move = count - index;
    count += 1;
    if(to_move > 0)
    {
      memmove(&elements[index + 1], &elements[index], to_move * sizeof(T));
    }
    elements[index] = element;
  }

  constexpr int max_elements()
  {
    return N;
  }

  constexpr b8 is_last(int index)
  {
    assert(index >= 0);
    assert(index < count);
    return index == count - 1;
  }

  constexpr b8 is_full()
  {
    return count >= N;
  }

  constexpr T pop()
  {
    assert(count > 0);
    T element = elements[count - 1];
    count -= 1;
    return element;
  }

  constexpr b8 is_empty()
  {
    return count <= 0;
  }

};

// @Note(tkap, 24/11/2022): If we for some reason add more member variables to this struct it may be break serialization.
// But why would we?
template <typename T, int N>
struct s_carray
{
  static_assert(N > 0);
  T elements[N];

  T& operator[](int index)
  {
    assert(index >= 0);
    assert(index < N);
    return elements[index];
  }

  void set(int index, T val)
  {
    (*this)[index] = val;
  }

  T get(int index)
  {
    return (*this)[index];
  }

  T* get_ptr(int index)
  {
    return &(*this)[index];
  }

  int max_elements()
  {
    return N;
  }

  void clear()
  {
    memset(elements, 0, sizeof(elements));
  }

  void set_to_one()
  {
    memset(elements, true, sizeof(elements));
  }

  int count_true()
  {
    static_assert(sizeof(T) == sizeof(b8));
    int result = 0;
    for(int i = 0; i < N; i++)
    {
      if(elements[i]) { result += 1; }
    }
    return result;
  }

  void copy_from(s_carray<T, N>* from)
  {
    assert(max_elements() == from->max_elements());
    memcpy(elements, from->elements, sizeof(elements));
  }

  void copy_memory(void* source, size_t size)
  {
    assert(size > 0);
    assert(size <= sizeof(elements));
    memcpy(elements, source, size);
  }

  void copy_into_and_advance(u8** mem)
  {
    assert(mem);
    assert(*mem);
    u8* temp = *mem;
    memcpy(temp, elements, sizeof(elements));
    temp += sizeof(elements);
    *mem = temp;
  }
};

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		arrays end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

template <typename T>
struct s_maybe
{
  b8 valid;
  T value;
};

template <typename T>
func s_maybe<T> maybe(T value)
{
  s_maybe<T> maybe;
  maybe.valid = true;
  maybe.value = value;
  return maybe;
}

template <typename T>
func s_maybe<T> ignore(T value)
{
  s_maybe<T> maybe = zero;
  return maybe;
}

func void on_failed_assert(char* cond, char* file, int line)
{
  char* text = format_text("FAILED ASSERT IN %s (%i)\n%s\n", file, line, cond);
  printf("%s\n", text);
  int result = MessageBox(null, text, "Assertion failed", MB_RETRYCANCEL | MB_TOPMOST);
  if(result != IDRETRY)
  {
    if(IsDebuggerPresent())
    {
      __debugbreak();
    }
    else
    {
      ExitProcess(1);
    }
  }
}

func float range_lerp(float v, float amin, float amax, float bmin, float bmax)
{
  float p = ((v - amin) / (amax - amin));
  return bmin + (bmax - bmin) * p;
}

func float lerp(float a, float b, float t)
{
  return a + (b - a) * t;
}

func s_v4 lerp_color(s_v4 a, s_v4 b, float t)
{
  s_v4 result;
  result.x = lerp(a.x, b.x, t);
  result.y = lerp(a.y, b.y, t);
  result.z = lerp(a.z, b.z, t);
  result.w = a.w;
  return result;
}

func s_v2 lerp(s_v2 a, s_v2 b, float t)
{
  s_v2 result;
  result.x = lerp(a.x, b.x, t);
  result.y = lerp(a.y, b.y, t);
  return result;
}

func s_v4 lerp(s_v4 a, s_v4 b, float t)
{
  s_v4 result;
  result.x = lerp(a.x, b.x, t);
  result.y = lerp(a.y, b.y, t);
  result.z = lerp(a.z, b.z, t);
  result.w = lerp(a.w, b.w, t);
  return result;
}

func s_v4 rand_color(s_random* rng)
{
  s_v4 result;
  result.x = rng->randf32();
  result.y = rng->randf32();
  result.z = rng->randf32();
  result.w = 1;
  return result;
}

func s_v2 v2_from_angle(float angle)
{
  return v2(
            cosf(angle),
            sinf(angle)
            );
}

func s_v4 multiply_rgb(s_v4 v4, float v)
{
  s_v4 result;
  result.x = v4.x * v;
  result.y = v4.y * v;
  result.z = v4.z * v;
  result.w = v4.w;
  return result;
}

func constexpr s_v4 rgb(int hex)
{
  s_v4 result;
  result.x = ((hex & 0xFF0000) >> 16) / 255.0f;
  result.y = ((hex & 0x00FF00) >> 8) / 255.0f;
  result.z = ((hex & 0x0000FF)) / 255.0f;
  result.w = 1;
  return result;
}

func s_v4 brighter(s_v4 color, float mul)
{
  assert(mul > 1);
  s_v4 result;
  result.x = at_most(1.0f, color.x * mul);
  result.y = at_most(1.0f, color.y * mul);
  result.z = at_most(1.0f, color.z * mul);
  result.w = 1;
  return result;
}

func s_v4 darker(s_v4 color, float div)
{
  assert(div > 1);
  s_v4 result;
  result.x = color.x / div;
  result.y = color.y / div;
  result.z = color.z / div;
  result.w = 1;
  return result;
}

template <typename T>
[[nodiscard]]
func T at_most(T high, T current)
{
  return current > high ? high : current;
}

template <typename T>
[[nodiscard]]
func T at_least(T low, T current)
{
  return current < low ? low : current;
}

template <typename T>
[[nodiscard]]
func T clamp(T current, T low, T high)
{
  return at_least(low, at_most(high, current));
}

[[nodiscard]]
func b8 rect_collides_rect(s_v2 pos1, s_v2 size1, s_v2 pos2, s_v2 size2)
{
  return pos1.x + size1.x >= pos2.x && pos1.x <= pos2.x + size2.x &&
    pos1.y + size1.y >= pos2.y && pos1.y <= pos2.y + size2.y;
}

[[nodiscard]]
// @Note(tkap, 07/03/2023): Top left
func s_v2 random_point_in_rect(s_v2 size, s_random* rng)
{
  return v2(
            rng->randf32() * size.x,
            rng->randf32() * size.y
            );
}

[[nodiscard]]
func int circular_index(int index, int size)
{
  assert(size > 0);
  if(index >= 0)
  {
    return index % size;
  }
  return (size - 1) - ((-index - 1) % size);
}

func s_v4 make_color(float v)
{
  return v4(v, v, v, 1.0f);
}

func void make_process_close_when_app_closes(HANDLE process)
{
  HANDLE job = CreateJobObjectA(null, null);
  assert(job);

  JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info = zero;
  job_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  BOOL set_info_result = SetInformationJobObject(job, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info));
  UNREFERENCED_PARAMETER(set_info_result);
  assert(set_info_result);

  BOOL assign_result = AssignProcessToJobObject(job, process);
  UNREFERENCED_PARAMETER(assign_result);
  assert(assign_result);
}

func s_v4 hsv_to_rgb(float hue, float saturation, float value)
{
  s_v4 color;
  color.w = 1;

  // Red channel
  float k = fmodf((5.0f + hue/60.0f), 6);
  float t = 4.0f - k;
  k = (t < k)? t : k;
  k = (k < 1)? k : 1;
  k = (k > 0)? k : 0;
  color.x = (value - value*saturation*k);

  // Green channel
  k = fmodf((3.0f + hue/60.0f), 6);
  t = 4.0f - k;
  k = (t < k)? t : k;
  k = (k < 1)? k : 1;
  k = (k > 0)? k : 0;
  color.y = (value - value*saturation*k);

  // Blue channel
  k = fmodf((1.0f + hue/60.0f), 6);
  t = 4.0f - k;
  k = (t < k)? t : k;
  k = (k < 1)? k : 1;
  k = (k > 0)? k : 0;
  color.z = (value - value*saturation*k);

  return color;
}

func int roundfi(float x)
{
  return (int)roundf(x);
}

func int floorfi(float x)
{
  return (int)floorf(x);
}

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		memory start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
struct s_lin_arena
{
  size_t used = 0;
  size_t capacity;
  u8* memory = null;
  s_sarray<size_t, 16> push_stack;

  void init(size_t in_capacity)
  {
    assert(in_capacity > 0);
    capacity = in_capacity;
    memory = (u8*)malloc(in_capacity);
  }

  constexpr void* get(size_t in_wanted)
  {
    assert(in_wanted > 0);
    assert(in_wanted <= capacity);
    size_t wanted = (in_wanted + 7) & ~7;
    assert(used + wanted <= capacity);

    void* result = memory + used;
    used += wanted;
    return result;
  }

  void* get_clear(size_t in_wanted)
  {
    void* result = get(in_wanted);
    memset(result, 0, in_wanted);

    return result;
  }

  constexpr void push()
  {
    push_stack.add(used);
  }

  constexpr void pop()
  {
    used = push_stack.pop();
  }

};


// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		memory end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		string stuff start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

func char* handle_plural(int amount)
{
  return (amount == 1) ? "" : format_text("s");
}

func void my_strcpy(char* dest, int dest_capacity, const char* source)
{
#ifdef m_debug

  int result = strcpy_s(dest, dest_capacity, source);
  assert(result == 0);

#else

  strcpy_s(dest, dest_capacity, source);

#endif
}


template <int max_chars>
struct s_str
{
  int len = 0;
  char data[max_chars + 1];

  void from_data(char* src, int size_in_bytes)
  {
    assert(size_in_bytes <= max_chars);
    memcpy(data, src, size_in_bytes);
    data[size_in_bytes] = 0;
    len = size_in_bytes;
  }

  template <int other_max_chars>
	b8 equals(s_str<other_max_chars>* other)
  {
    if(len != other->len) { return false; }
    return memcmp(data, other->data, len) == 0;
  }

  b8 equals(char* other)
  {
    int other_len = (int)strlen(other);
    assert(other_len > 0);
    if(len != other_len) { return false; }
    return memcmp(data, other, len) == 0;
  }

  void from_cstr(char* src)
  {
    from_data(src, (int)strlen(src));
  }

  int find(char* needle)
  {
    int haystack_len = len;
    int needle_len = (int)strlen(needle);
    assert(haystack_len > 0);
    assert(needle_len > 0);
    if(needle_len > haystack_len) { return -1; }

    for(int i = 0; i < haystack_len - (needle_len - 1); i++)
    {
      b8 all_match = true;
      for(int j = 0; j < needle_len; j++)
      {
        char haystack_c = data[i + j];
        char needle_c = needle[j];

        if(haystack_c != needle_c)
        {
          all_match = false;
          break;
        }

      }
      if(all_match)
      {
        return i;
      }
    }
    return -1;
  }

  constexpr int get_max_chars()
  {
    return max_chars;
  }

};

template <int max_chars>
func s_str<max_chars> make_str(char* str)
{
  s_str<max_chars> result;
  my_strcpy(result.data, sizeof(result.data), str);
  return result;
}

template <int max_chars>
struct s_str_sbuilder
{
  int len = 0;
  int tab_count = 0;
  char data[max_chars + 1];

  constexpr void add_(char* what, b8 use_tabs, va_list args)
  {
    if(use_tabs)
    {
      for(int tab_i = 0; tab_i < tab_count; tab_i++)
      {
        data[len++] = '\t';
      }
    }
    char* where_to_write = &data[len];
    int written = vsnprintf(where_to_write, max_chars + 1 - len, what, args);
    assert(written > 0 && written < max_chars);
    len += written;
    assert(len < max_chars);
    data[len] = 0;
  }

  constexpr void add(char* what, ...)
  {
    va_list args;
    va_start(args, what);
    add_(what, false, args);
    va_end(args);
  }

  constexpr void add_char(char c)
  {
    assert(len < max_chars);
    data[len++] = c;
    data[len] = 0;
  }

  constexpr void add_with_tabs(char* what, ...)
  {
    va_list args;
    va_start(args, what);
    add_(what, true, args);
    va_end(args);
  }

  constexpr void add_line(char* what, ...)
  {
    va_list args;
    va_start(args, what);
    add_(what, false, args);
    va_end(args);
    add("\n");
  }

  constexpr void add_line_with_tabs(char* what, ...)
  {
    va_list args;
    va_start(args, what);
    add_(what, true, args);
    va_end(args);
    add("\n");
  }

  constexpr void add_tabs()
  {
    for(int tab_i = 0; tab_i < tab_count; tab_i++)
    {
      data[len++] = '\t';
    }
  }

  constexpr void line()
  {
    add("\n");
  }

  constexpr char* cstr()
  {
    return data;
  }

  constexpr void push_tab()
  {
    assert(tab_count <= 64);
    tab_count++;
  }

  constexpr void pop_tab()
  {
    assert(tab_count > 0);
    tab_count--;
  }

};


func char* format_text(char* text, ...)
{
  local_persist HANDLE mutex = NULL;

  if(!mutex)
  {
    mutex = CreateMutex(
                        NULL,              // default security attributes
                        FALSE,             // initially not owned
                        NULL);             // unnamed mutex

    if (mutex == NULL)
    {
      printf("CreateMutex error: %d\n", GetLastError());
      return 0;
    }
  }

  // Wait indefinately
  // Other results can be WAIT_ABANDONED
  DWORD wait_result = WaitForSingleObject(mutex, INFINITE);
  assert(wait_result == WAIT_OBJECT_0);

  constexpr int max_format_text_buffers = 64;
  constexpr int max_text_buffer_length = 4096 * 2;

  local_persist char buffers[max_format_text_buffers][max_text_buffer_length] = zero;
  local_persist int index = 0;

  char* current_buffer = buffers[index];
  memset(current_buffer, 0, max_text_buffer_length);

  va_list args;
  va_start(args, text);
#ifdef m_debug
  int written = vsnprintf(current_buffer, max_text_buffer_length, text, args);
  assert(written > 0 && written < max_text_buffer_length);
#else
  vsnprintf(current_buffer, max_text_buffer_length, text, args);
#endif
  va_end(args);

  index += 1;
  if(index >= max_format_text_buffers) { index = 0; }
  assert(ReleaseMutex(mutex));

  return current_buffer;
}

func int str_find_from_right(char* haystack, char needle)
{
  int haystack_len = (int)strlen(haystack);

  for(int i = 0; i < haystack_len; i--)
  {
    if(haystack[i] == needle)
    {
      return i;
    }
  }

  return -1;
}

func int str_find_from_right(char* haystack, char* needle)
{
  int haystack_len = (int)strlen(haystack);
  int needle_len = (int)strlen(needle);
  if(needle_len > haystack_len) { return -1; }

  for(int i = haystack_len - 1; i >= needle_len - 1; i--)
  {
    b8 match = true;
    for(int j = 0; j < needle_len; j++)
    {
      char haystack_c = haystack[i - j];
      char needle_c = needle[needle_len - j - 1];

      if(haystack_c != needle_c)
      {
        match = false;
        break;
      }
    }
    if(match)
    {
      return i - (needle_len - 1);
    }
  }
  return -1;
}

func int str_find_from_left(char* haystack, char* needle)
{
  int haystack_len = (int)strlen(haystack);
  int needle_len = (int)strlen(needle);
  if(needle_len > haystack_len) { return -1; }

  for(int haystack_i = 0; haystack_i < haystack_len - (needle_len - 1); haystack_i++)
  {
    b8 found = true;
    for(int needle_i = 0; needle_i < needle_len; needle_i++)
    {
      char haystack_c = haystack[haystack_i + needle_i];
      char needle_c = needle[needle_i];
      if(haystack_c != needle_c)
      {
        found = false;
        break;
      }
    }
    if(found)
    {
      return haystack_i;
    }
  }
  return -1;
}

// @Note(tkap, 11/03/2023): Don't pass in a read-only string here!
func b8 str_remove_from_left(char* haystack, char* needle)
{
  int index = str_find_from_left(haystack, needle);
  if(index == -1) { return false; }

  int haystack_len = (int)strlen(haystack);
  int needle_len = (int)strlen(needle);
  int to_copy = haystack_len - (index + needle_len);
  memmove(&haystack[index], &haystack[index + needle_len], to_copy);
  haystack[haystack_len - needle_len] = 0;
  return true;
}

// @Note(tkap, 11/03/2023): Don't pass in a read-only string here!
func void str_remove_from_right_until(char* haystack, char* needle, b8 remove_needle)
{
  int index = str_find_from_right(haystack, needle);
  assert(index != -1);

  int needle_len = (int)strlen(needle);
  haystack[index + (remove_needle ? 0: needle_len)] = 0;
}

func wchar_t* str_to_wide(char* str, s_lin_arena* arena)
{
  int len = (int)strlen(str);
  assert(len > 0);
  int buffer_size = (len + 1) * sizeof(wchar_t);
  wchar_t* result = (wchar_t*)arena->get(buffer_size);
  int characters_written = MultiByteToWideChar(
                                               CP_UTF8,
                                               0,
                                               str,
                                               -1,
                                               result,
                                               buffer_size
                                               );
  assert(characters_written > 0);
  return result;
}

// @Note(tkap, 04/05/2023): Returns how many whitespace characters were removed
func int str_trim_from_right(char* str)
{
  int result = 0;
  int len = (int)strlen(str);
  assert(len > 0);
  for(int i = len - 1; i >= 0; i--)
  {
    if(is_whitespace(str[i]))
    {
      str[i] = 0;
      result += 1;
    }
    else { break; }
  }
  return result;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		string stuff end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		file stuff start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

struct s_read_file_result
{
  b8 success;
  int file_size;
  DWORD bytes_read;
  HANDLE file;
  char* data;
};

func s_read_file_result read_file(char* path, s_lin_arena* arena)
{
  s_read_file_result result = zero;
  result.file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
  if(result.file == INVALID_HANDLE_VALUE) { return result; }

  result.file_size = GetFileSize(result.file, null);
  result.data = (char*)arena->get(result.file_size + 1);

  BOOL read_result = ReadFile(result.file, result.data, result.file_size, &result.bytes_read, null);
  if(read_result == 0)
  {
    result.data = null;
  }
  else
  {
    result.success = true;
    result.data[result.bytes_read] = 0;
  }
  return result;
}

func b8 file_exists(char* path)
{
  return PathFileExistsA(path) == TRUE;
}

func char* read_file_quick(char* path, s_lin_arena* arena)
{
  s_read_file_result result = read_file(path, arena);
  if(result.file != INVALID_HANDLE_VALUE) { CloseHandle(result.file); }
  return result.data;
}

func void write_file_quick(char* path, void* data, int data_size)
{
  HANDLE file = CreateFile(path, GENERIC_WRITE, 0, null, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, null);
  if(file == INVALID_HANDLE_VALUE) { return; }

  DWORD bytes_written;
  WriteFile(file, data, data_size, &bytes_written, null);
  CloseHandle(file);
}

bool get_next_filename(char* fileName, char* folderPath)
{
  bool result = false;

  local_persist WIN32_FIND_DATAA fileData;
  local_persist HANDLE dirHandle = 0;

  if (!dirHandle)
  {
    fileData = {};

    dirHandle = FindFirstFileA(folderPath, &fileData);
  }

  if (dirHandle != INVALID_HANDLE_VALUE)
  {
    result = FindNextFileA(dirHandle, &fileData);

    // Skip '.' and '..'
    {
      if (result &&
          fileData.cFileName[0] == '.' &&
          fileData.cFileName[1] == 0)
      {
        result = FindNextFileA(dirHandle, &fileData);
      }

      if (result &&
          fileData.cFileName[0] == '.' &&
          fileData.cFileName[1] == '.' &&
          fileData.cFileName[2] == 0)
      {
        result = FindNextFileA(dirHandle, &fileData);
      }
    }

    if (result)
    {
      memset(fileName, 0, MAX_PATH);
      memcpy(fileName, fileData.cFileName, strlen(fileData.cFileName));
    }
    else
    {
      dirHandle = 0;
    }
  }
  else
  {
    // TODO: Assert worthy?
    assert(0 && "No File present");
  }

  return result;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		file stuff end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// @Note(tkap, 11/03/2023): If not passing an arena, this function will use format_text, which should not be stored,
// as the buffer will get overwritten as some point
func char* get_executable_path(s_lin_arena* arena)
{
  char* path;
  if(arena)
  {
    path = (char*)arena->get(MAX_PATH);
    GetModuleFileName(null, path, MAX_PATH);
  }
  else
  {
    char temp_path[MAX_PATH];
    GetModuleFileName(null, temp_path, MAX_PATH);
    path = format_text("%s", temp_path);
  }
  str_remove_from_right_until(path, "\\", true);
  return path;
}


// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		parsing start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

enum e_token
{
  e_token_invalid,
  e_token_eof,
  e_token_hex_number,
  e_token_number,
  e_token_real_number,
  e_token_operator,
  e_token_identifier,
  e_token_string,
  e_token_format_string,
  e_token_whitespace,
};

struct s_token
{
  e_token type;
  int length;
  int line_num;
  char* at;
};

struct s_tokenizer
{
  b8 allow_failure;
  b8 tokenize_whitespace;
  int line_num;
  char comment_str[3];
  char* at;
};

func b8 is_letter(char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

func b8 is_number(char c)
{
  return (c >= '0' && c <= '9');
}

func b8 is_hex(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

func b8 is_whitespace(char c)
{
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

func void eat_whitespace(s_tokenizer* tokenizer)
{
  while(true)
  {
    if(tokenizer->at[0] == '\n')
    {
      tokenizer->at += 1;
      tokenizer->line_num += 1;
    }
    else if(is_whitespace(tokenizer->at[0]))
    {
      tokenizer->at += 1;
    }
    else { break; }
  }
}

func s_token next_token(s_tokenizer* tokenizer)
{
  if(tokenizer->line_num == 0) { tokenizer->line_num = 1; }
  s_token token = zero;

  while(true)
  {
    char* start = tokenizer->at;
    if(!tokenizer->tokenize_whitespace)
    {
      eat_whitespace(tokenizer);
    }

    // @Note(tkap, 13/05/2022): Skip comments
    if(tokenizer->comment_str[0])
    {
      if(memcmp(tokenizer->at, tokenizer->comment_str, strlen(tokenizer->comment_str)) == 0)
      {
        tokenizer->at += strlen(tokenizer->comment_str);

        while(tokenizer->at[0] != '\0' && tokenizer->at[0] != '\n')
        {
          tokenizer->at += 1;
        }
      }
    }

    if(start == tokenizer->at) { break; }
  }

  if(tokenizer->at[0] == '\0')
  {
    token.type = e_token_eof;
    token.length = 1;
    token.at = tokenizer->at;
    tokenizer->at += 1;
  }

  // @Note(tkap, 13/05/2022): Whitespace
  else if(is_whitespace(tokenizer->at[0]))
  {
    assert(tokenizer->tokenize_whitespace);
    token.type = e_token_whitespace;
    token.at = tokenizer->at;

    eat_whitespace(tokenizer);

    token.length = (int)(tokenizer->at - token.at);
  }

  // Strings
  else if(tokenizer->at[0] == '\"' || (tokenizer->at[0] == 'f' && tokenizer->at[1] == '\"'))
  {
    if(tokenizer->at[0] == '\"')
    {
      token.type = e_token_string;
    }
    else
    {
      token.type = e_token_format_string;
      tokenizer->at += 1;
    }
    token.at = tokenizer->at;

    while(true)
    {
      tokenizer->at += 1;
      if(tokenizer->at[0] == '\0')
      {
        printf("Found end of file before end of string\n");
        assert(false);
      }
      else if(tokenizer->at[0] == '\"' && tokenizer->at[-1] != '\\')
      {
        break;
      }
    }
    tokenizer->at += 1;
    token.length = (int)(tokenizer->at - token.at);
  }

  // @Note(tkap, 05/04/2023): Hex number
  else if(tokenizer->at[0] == '0' && tokenizer->at[1] == 'x')
  {
    tokenizer->at += 2;
    token.at = tokenizer->at;

    int digits = 0;
    while(is_hex(tokenizer->at[0]))
    {
      tokenizer->at += 1;
      digits += 1;
    }

    // @Fixme(tkap, 05/04/2023): proper error
    if(digits <= 0 || digits > 8)
    {
      assert(false);
    }

    token.type = e_token_hex_number;
    token.length = (int)(tokenizer->at - token.at);
  }

  else if(is_number(tokenizer->at[0]))
  {
    token.at = tokenizer->at;
    tokenizer->at += 1;

    b8 found_dot = false;
    b8 success = true;
    while(true)
    {
      if(is_number(tokenizer->at[0]))
      {
        tokenizer->at += 1;
      }
      else if(tokenizer->at[0] == '.')
      {
        if(found_dot)
        {
          success = false;
          break;
        }
        found_dot = true;
        tokenizer->at += 1;
      }
      else if(tokenizer->at[0] == 'f')
      {
        if(found_dot)
        {
          tokenizer->at += 1;
        }
        else
        {
          success = false;
        }
        break;
      }
      else if(is_letter(tokenizer->at[0]))
      {
        success = false;
        break;
      }
      else
      {
        break;
      }
    }

    if(success)
    {
      token.type = found_dot ? e_token_real_number : e_token_number;
      token.length = (int)(tokenizer->at - token.at);
    }
  }

  else if(is_letter(tokenizer->at[0]) || tokenizer->at[0] == '_')
  {
    token.type = e_token_identifier;
    token.at = tokenizer->at;

    while(
          is_letter(tokenizer->at[0]) ||
          is_number(tokenizer->at[0]) ||
          tokenizer->at[0] == '_'
          )
    {
      tokenizer->at += 1;
    }

    token.length = (int)(tokenizer->at - token.at);
  }

  // else if(
  // 	tokenizer->at[0] == '(' ||
  // 	tokenizer->at[0] == ')' ||
  // 	tokenizer->at[0] == ',' ||
  // 	tokenizer->at[0] == ';' ||
  // 	tokenizer->at[0] == '{' ||
  // 	tokenizer->at[0] == '}'
  // )
  // {
  // 	token.type = e_token_separator;
  // 	token.at = tokenizer->at;
  // 	token.length = 1;
  // 	tokenizer->at += 1;
  // }

  // Length 3 operators
  else if(tokenizer->at[0] == '.' && tokenizer->at[1] == '.' && tokenizer->at[2] == '.')
  {
    token.type = e_token_operator;
    token.at = tokenizer->at;
    tokenizer->at += 3;
    token.length = 3;
  }

  // Length 2 operators
  else if(
          (tokenizer->at[0] == '=' && tokenizer->at[1] == '=') ||
          (tokenizer->at[0] == '+' && tokenizer->at[1] == '=') ||
          (tokenizer->at[0] == '-' && tokenizer->at[1] == '=') ||
          (tokenizer->at[0] == '*' && tokenizer->at[1] == '=') ||
          (tokenizer->at[0] == '/' && tokenizer->at[1] == '=') ||
          (tokenizer->at[0] == '&' && tokenizer->at[1] == '&') ||
          (tokenizer->at[0] == '|' && tokenizer->at[1] == '|') ||
          (tokenizer->at[0] == '!' && tokenizer->at[1] == '=') ||
          (tokenizer->at[0] == '>' && tokenizer->at[1] == '=') ||
          (tokenizer->at[0] == '<' && tokenizer->at[1] == '=') ||
          (tokenizer->at[0] == ':' && tokenizer->at[1] == '=')
          )
  {
    token.type = e_token_operator;
    token.at = tokenizer->at;
    tokenizer->at += 2;
    token.length = 2;
  }

  // Length 1 operators
  else if(
          tokenizer->at[0] == '=' ||
          tokenizer->at[0] == '+' ||
          tokenizer->at[0] == '-' ||
          tokenizer->at[0] == '*' ||
          tokenizer->at[0] == '/' ||
          tokenizer->at[0] == '<' ||
          tokenizer->at[0] == '>' ||
          tokenizer->at[0] == '.' ||
          tokenizer->at[0] == '@' ||
          tokenizer->at[0] == '[' ||
          tokenizer->at[0] == ']' ||
          tokenizer->at[0] == '(' ||
          tokenizer->at[0] == '#' ||
          tokenizer->at[0] == ')' ||
          tokenizer->at[0] == ';' ||
          tokenizer->at[0] == '{' ||
          tokenizer->at[0] == '}' ||
          tokenizer->at[0] == ',' ||
          tokenizer->at[0] == '%' ||
          tokenizer->at[0] == '?' ||
          tokenizer->at[0] == ':' ||
          tokenizer->at[0] == '&' ||
          tokenizer->at[0] == '|' ||
          tokenizer->at[0] == '!' ||
          tokenizer->at[0] == '\''
          )
  {
    token.type = e_token_operator;
    token.at = tokenizer->at;
    token.length = 1;
    tokenizer->at += 1;
  }

  if(token.length <= 0)
  {
    if(!tokenizer->allow_failure)
    {
      printf(
             "Failed to parse\n"
             "Starting at: '%.32s'...\n",
             tokenizer->at
             );
      assert(false);
    }
    else
    {
      token.at = tokenizer->at;
    }
  }

  token.line_num = tokenizer->line_num;

  return token;
}

func b8 consume_token(char* str, s_tokenizer* tokenizer)
{
  s_tokenizer temp = *tokenizer;
  int str_len = (int)strlen(str);
  s_token token = next_token(&temp);
  if(token.length != str_len) { return false; }

  if(memcmp(token.at, str, token.length) == 0)
  {
    *tokenizer = temp;
    return true;
  }
  return false;
}

func b8 consume_token(e_token type, s_tokenizer* tokenizer, s_token* out_token)
{
  s_tokenizer temp = *tokenizer;
  *out_token = zero;
  s_token token = next_token(&temp);
  if(token.type == type)
  {
    *tokenizer = temp;
    *out_token = token;
    return true;
  }
  return false;
}

func b8 require_token(char* str, s_tokenizer tokenizer)
{
  return consume_token(str, &tokenizer);
}

func b8 require_token(e_token type, s_tokenizer tokenizer, s_token* out_token)
{
  return consume_token(type, &tokenizer, out_token);
}

func s64 token_to_int(s_token token)
{
  assert(token.type == e_token_number || token.type == e_token_hex_number);

  char buffer[64] = zero;
  memcpy(buffer, token.at, token.length);
  buffer[token.length] = 0;
  if(token.type == e_token_hex_number)
  {
    char* end;
    return (s64)_strtoui64(buffer, &end, 16);
  }
  else
  {
    char* end;
    return (s64)_strtoui64(buffer, &end, 10);
  }
}

func float token_to_float(s_token token)
{
  assert(token.type == e_token_real_number);
  char buffer[64];
  memcpy(buffer, token.at, token.length);
  buffer[token.length] = 0;
  return (float)atof(buffer);
}

func s_token peek_token(s_tokenizer tokenizer)
{
  return next_token(&tokenizer);
}

func b8 match_token(s_token token, e_token type, char* text)
{
  if(token.type != type) { return false; }
  if(!text) { return true; }
  int text_len = (int)strlen(text);
  if(token.length != text_len) { return false; }
  return memcmp(token.at, text, token.length) == 0;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		parsing end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

func void set_window_size_to_monitor_size(HWND window)
{
  RECT rect = zero;
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &rect, 0);
  SetWindowPos(window, 0, rect.left, rect.top, rect.right, rect.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
}

func void print_win32_error()
{
  int error_ = GetLastError();
  LPTSTR error_text = null;

  FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                null,
                error_,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&error_text,
                0,
                null
                );

  if(error_text)
  {
    printf("%s", error_text);
    LocalFree(error_text);
  }
}

char* c_base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool is_base64(char c)
{
  return (isalnum(c) || (c == '+') || (c == '/'));
}

char* base64_decode(char* encoded_string, int length, s_lin_arena* arena)
{
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];

  int ret_len = 0;
  int ret_buffer_len = length * 5;
  char* ret = (char*)arena->get_clear(ret_buffer_len);

  while (length-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = (unsigned char)str_find_from_right(c_base64_chars, char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret[ret_len++] = char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = (unsigned char)str_find_from_right(c_base64_chars, char_array_4[i]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret[ret_len++] = char_array_3[j];
  }

  assert(ret_len < ret_buffer_len);

  return ret;
}

#include <string>
#include <vector>
static std::string base64_decode(char* encoded_string, int str_len)
{
  std::string out;

  std::vector<int> T(256,-1);
  for (int i=0; i<64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;

  int val=0, valb=-8;
  for (int char_idx = 0; char_idx < str_len; char_idx++)
  {
    unsigned char c = encoded_string[char_idx];

    if (T[c] == -1)
    {
      break;
    }

    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0)
    {
      out.push_back(char((val>>valb)&0xFF));
      valb -= 8;
    }
  }
  return out;
}

func int ceilfi(float f)
{
  return (int)ceilf(f);
}


char* videoinfo_mem(char* haystack, unsigned int sizehaystack,
                    char* needle, unsigned int sizeneedle)
{
  int i = 0;
  int end = sizehaystack - sizeneedle;
  for(i = 0; i < end; ++i) {
    if(memcmp(haystack+i,needle,sizeneedle) == 0) {
      return haystack+i;
    }
  }
  return null;
}

void* videoinfo_find(char* filename, char* find, int size, int resultSize)
{
  FILE* fp = null;
  char* buffer = null;
  char* result = null;
  char* pos = null;
	int bufferSize = 2048;
	double filesize = 0;
	double split = 16;
	double splitsize = 0;
	double start = 0;
	double end = 0;
	double i = 0;
	int read = 0;

  if(resultSize > bufferSize)
  {
    resultSize = bufferSize;
  }

  buffer = (char*)malloc(bufferSize);
  if(buffer == null)
  {
    return null;
  }

  fp = fopen(filename,"rb");
  if(fp == null)
  {
    free(buffer);
    return null;
  }

  fseek(fp,0,SEEK_END);
  filesize = ftell(fp);
  rewind(fp);

  split = ceil(filesize / 100000);
  splitsize = ceil(filesize/((split > 0 )? split : 1));

  for(i = split-1; i >= 0; --i)
  {
    start = (i * splitsize);
    end = start + splitsize;
    fseek(fp, (long)start, SEEK_SET);

    while((read = (unsigned int)fread(buffer,1,bufferSize,fp)) != 0)
    {
      if((pos = videoinfo_mem(buffer,bufferSize,find,size)) != NULL)
      {
        result = (char*)malloc(resultSize);
        memcpy(result,pos,resultSize);
        i = -1;
        break;
      }

      if(read != bufferSize || ftell(fp) >= end)
      {
        break; // go onto next split
      }
    }

  }

  fclose(fp);
  free(buffer);
  return result;
}

unsigned long videoinfo_flip(unsigned long val)
{
  unsigned long out = 0;
  out += (val & 0x000000FF) << 24;
  out += (val & 0xFF000000) >> 24;
  out += (val & 0x0000FF00) << 8;
  out += (val & 0x00FF0000) >> 8;

  return out;
}

unsigned long videoinfo_duration(char* filename)
{
  unsigned long duration = 0;
  char version = 0;
  void* data = null;
  char* pos = null;
  unsigned long timescale = 0;
  unsigned long timeunits = 0;
  int bytesize = 4;

  data = videoinfo_find(filename,"mvhd",4,64);
  if(data == null)
  {
    goto clean;
  }

  pos = (char*)data;
  pos += 4; // skip mvhd

  version = *pos++;
  pos+=3; //skip flags

  if(version == 1)
  {
    bytesize = 8;
  }
  else
  {
    bytesize = 4;
  }

  pos += bytesize; // skip created date
  pos += bytesize; // skip modified date

  memcpy(&timescale, pos, 4);
  memcpy(&timeunits, pos + 4, bytesize);

  timescale = videoinfo_flip(timescale);
  timeunits = videoinfo_flip(timeunits);

  if(timescale > 0 && timeunits > 0)
  {
    duration = timeunits/timescale;
  }

  clean:
  free(data);
  return duration;
}