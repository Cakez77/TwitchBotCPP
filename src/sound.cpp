
func s_mp3_sound load_mp3_from_data(char* data, int data_size, s_lin_arena* arena)
{
	s_mp3_sound sound = zero;

	drmp3 mp3;
	if(!drmp3_init_memory(&mp3, data, data_size, null))
	{
		assert(false && "Failed to initialize MP3 data");
	}

	u64 frames_to_read = drmp3_get_pcm_frame_count(&mp3);
	constexpr int padding = 1 * c_kb;
	sound.data = (s16*)arena->get(frames_to_read * sizeof(drmp3_int16) + padding);

	u32 sample_count = (u32)drmp3_read_pcm_frames_s16(&mp3, frames_to_read, (drmp3_int16*)sound.data);
	sound.channels = mp3.channels;
	sound.sample_count = sample_count;
	sound.sample_rate = mp3.sampleRate;

	if(sound.channels == 1)
	{
		/*
		s16* temp = (s16*)arena->get(sound.get_size() * 2);
		for(int sample_i = 0; sample_i < sound.sample_count; sample_i++)
		{
			s16 sample = sound.data[sample_i];
			temp[sample_i * 2] = sample;
			temp[sample_i * 2 + 1] = sample;
		}
		sound.channels = 2;
		sound.data = temp;
		*/
	}
	else if(sound.channels == 2)
	{

	}
	else
	{
		assert(false);
		return zero;
	}

	// assert(sound.channels == 2);
	if(sound.sample_rate == 22050)
	{
		/*
		s16* temp = (s16*)arena->get(sound.get_size() * 2);
		for(int sample_i = 0; sample_i < sound.sample_count; sample_i++)
		{
			// @TODO(tkap, 25/04/2023): Interpolate?
			s16 left = sound.data[sample_i * 2];
			s16 right = sound.data[sample_i * 2 + 1];
			temp[sample_i * 4] = left;
			temp[sample_i * 4 + 1] = right;
			temp[sample_i * 4 + 2] = left;
			temp[sample_i * 4 + 3] = right;
		}
		sound.sample_count *= 2;
		sound.sample_rate = 44100;
		sound.data = temp;
		*/
	}

	// @Note(tkap, 25/04/2023): Tiktok sends this
	else if(sound.sample_rate == 24000)
	{
		/*
		// @Note(tkap, 25/04/2023): This code is kinda retarded and not how you are supposed to resample, but I don't know what I'm doing :)
		f64 ratio = 44100.0 / sound.sample_rate;
		s16* temp = (s16*)arena->get((int)ceil(sound.get_size() * ratio));
		f64 pool = 0;
		int target_sample_index = 0;
		for(int sample_i = 0; sample_i < sound.sample_count; sample_i++)
		{
			s16 left = sound.data[sample_i * 2];
			s16 right = sound.data[sample_i * 2 + 1];
			pool += ratio;
			int pool_int = (int)pool;
			pool -= pool_int;
			for(int pool_i = 0; pool_i < pool_int; pool_i++)
			{
				temp[target_sample_index * 2] = left;
				temp[target_sample_index * 2 + 1] = right;
				target_sample_index += 1;
			}
		}
		sound.sample_count = (int)ceil(sound.sample_count * ratio);
		sound.sample_rate = 44100;
		sound.data = temp;
		*/
	}
	else if(sound.sample_rate == 44100)
	{

	}
	else if(sound.sample_rate == 48000)
	{
		// @TODO(tkap, 25/04/2023):
		assert(false);
	}
	else
	{
		assert(false);
		return zero;
	}

	return sound;
}

func s_mp3_sound load_mp3_from_file(char* file_path, s_lin_arena* arena)
{
	if(file_exists(file_path))
	{
		s_read_file_result mp3_file = read_file(file_path, arena);
		s_mp3_sound sound = load_mp3_from_data(mp3_file.data, mp3_file.file_size, arena);
		return sound;
	}
	else
	{
		return zero;
	}
}

func bool platform_play_sound(s_mp3_sound sound, bool loop = false, int submix_index = 0)
{
	assert(submix_index < c_max_sound_channels && "Submix Index out of bounds");

	s_xaudio_voice* voice = 0;
	s_xaudio_submix* submix = &xaudio_state.submixes[submix_index];
	for(int voice_index = 0; voice_index < c_max_concurrent_sounds; voice_index++)
	{
		s_xaudio_voice* xaudio_voice = &submix->voices[voice_index];

		if(!xaudio_voice->playing)
		{
			voice = xaudio_voice;
			break;
		}
	}

	if(voice)
	{
		voice->buffer = (char*)malloc(sound.get_size());
		memcpy(voice->buffer, sound.data, sound.get_size());
		voice->size = sound.get_size();
		XAUDIO2_BUFFER audio_buffer = {0};
		audio_buffer.AudioBytes = voice->size;
		audio_buffer.pAudioData = (BYTE*)voice->buffer;
		audio_buffer.Flags = XAUDIO2_END_OF_STREAM;
		audio_buffer.LoopCount = loop ? XAUDIO2_MAX_LOOP_COUNT : 0;

		int result = voice->voice->SubmitSourceBuffer(&audio_buffer);
		if(result)
		{
			assert(false && "Failed to play Sound");
			return false;
		}

		voice->voice->Start(0, 0);

	}

	return true;
}

func bool platform_play_sound(char* sound_name, s_lin_arena* arena)
{
	char* sound_path = format_text("assets/sounds/%s.mp3", sound_name);
	s_mp3_sound sound = load_mp3_from_file(sound_path, arena);

	return platform_play_sound(sound);
}

func bool platform_init_sound()
{
	// See: https://learn.microsoft.com/en-us/windows/win32/xaudio2/how-to--initialize-xaudio2
	if(CoInitializeEx(0, COINIT_MULTITHREADED) != 0)
	{
		assert(false && "Failed to initialize xAudio2");
		return false;
	}

	if(XAudio2Create(&xaudio_state.device, 0, XAUDIO2_USE_DEFAULT_PROCESSOR) != 0)
	{
		assert(false && "Failed to Create xAudio2 Device");
		return false;
	}

	// This is like the primary buffer in DirectSound
	if(xaudio_state.device->CreateMasteringVoice(&xaudio_state.mastering_voice) != 0)
	{
		assert(false && "Failed to Create xAudio2 Mastering Voice");
		return false;
	}

	// Adjust these to your specific WAV format used in your game
	// You can refactor this to be a different format or auto-detect it if you want to support
	// different exported formats. But I usually stick to just one format for my games.
	WAVEFORMATEX waveFormat = zero;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = c_number_channels;
	waveFormat.wBitsPerSample = c_bits_per_sample;
	waveFormat.nSamplesPerSec = c_samples_per_sec;
	waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

	XAUDIO2_SEND_DESCRIPTOR sendDesc = {0};
	XAUDIO2_VOICE_SENDS voiceSends = {0};

	for(int submit_index = 0; submit_index < c_max_sound_channels; submit_index++)
	{
		s_xaudio_submix* submix  = &xaudio_state.submixes[submit_index];

		// Submix
		{
			if(xaudio_state.device->CreateSubmixVoice(
				&submix->pSubmixVoice,
				c_number_channels, c_samples_per_sec,
				XAUDIO2_VOICE_USEFILTER, 0, 0, 0)
			)
			{
				assert(false && "Failed to Create xAudio2 Background Submix");
				return false;
			}
		}

		//Voices
		{
			sendDesc.Flags = 0;                           // Either 0 or XAUDIO2_SEND_USEFILTER.
			sendDesc.pOutputVoice = submix->pSubmixVoice; // This send's destination voice.

			voiceSends.SendCount = 1;                     // Number of sends from this voice.
			voiceSends.pSends = &sendDesc;                // Array of SendCount send descriptors.

			for(int voiceIdx = 0; voiceIdx < c_max_concurrent_sounds; voiceIdx++)
			{
				// These are like the secondary buffers in DirectSound,
				int result = xaudio_state.device->CreateSourceVoice(
					&submix->voices[voiceIdx].voice,
					&waveFormat, XAUDIO2_VOICE_NOPITCH,
					XAUDIO2_DEFAULT_FREQ_RATIO,
					(IXAudio2VoiceCallback*)&submix->voices[voiceIdx],
					&voiceSends
				);
				if(result)
				{
					assert(false && "Failed to Create xAudio2 Voice");
					return false;
				}
			}
		}
	}

	return true;
}