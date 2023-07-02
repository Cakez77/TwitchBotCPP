
struct s_mp3_sound
{
	s16* data;
  int sample_count;
  int sample_rate;
  int channels;

  int get_size()
  {
    return sample_count * sizeof(s16);
  }
};

struct s_xaudio_voice : IXAudio2VoiceCallback
{
  bool playing;
  int size;
  char* buffer;
  IXAudio2SourceVoice* voice;

  // Unused methods are stubs
  void OnStreamEnd() {playing = false; if(buffer) {free(buffer); size = 0;}}
	#pragma warning(push)
	#pragma warning(disable : 4100)
  void OnBufferStart(void* pBufferContext) {playing = true;}

  // Unused methods are stubs
  void OnVoiceProcessingPassEnd() {}
  void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
  void OnBufferEnd(void* pBufferContext) {}
  void OnLoopEnd(void* pBufferContext) {}
  void OnVoiceError(void* pBufferContext, HRESULT Error) {}
	#pragma warning(pop)
};

struct s_xaudio_submix
{
  IXAudio2SubmixVoice* pSubmixVoice;
  s_xaudio_voice voices[c_max_concurrent_sounds];
};

enum e_sound_types
{
  e_sound_type_tts,
  e_sound_type_game_background,
  e_sound_type_game_sfx,

  e_sound_type_count
};

struct s_xaudio_state
{
  IXAudio2* device;
  IXAudio2MasteringVoice* mastering_voice;

  int tts_queue_entry_count;
	XAUDIO2_BUFFER tts_queue[c_max_tts_queue_entries];

  s_xaudio_submix submixes[c_max_sound_channels];
};


func bool platform_init_sound();