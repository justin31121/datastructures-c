#ifndef PLAYER_H_H
#define PLAYER_H_H

#ifdef PLAYER_IMPLEMENTATION

#ifdef _WIN32
#define XAUDIO_IMPLEMENTATION
#endif //_WIN32

#define THREAD_IMPLEMENTATION
#define DECODER_IMPLEMENTATION
#endif //PLAYER_IMPLEMENTATION

#ifndef PLAYER_DEF
#define PLAYER_DEF static inline
#endif //PLAYER_DEF

#define PLAYER_BUFFER_SIZE 8192
#define PLAYER_N 5
#define PLAYER_VOLUME 0.1f

#ifdef linux
#include <alsa/asoundlib.h>
#endif //linux

#include "decoder.h"
#include "xaudio.h"
#include "thread.h"

typedef struct{
    FILE *file;
    Decoder_Fmt fmt;
    int channels;

    float volume;
    float duration; //fancy
    float duration_abs;
    int den;

    bool decoder_used;
    bool playing;

    Decoder_Buffer buffer;
    Decoder decoder;
    Thread audio_thread;
    Thread decoding_thread;
    int samples;

    int sample_rate;

#ifdef _WIN32
    XAudio2Device device;
#elif linux
    snd_pcm_t *device;
#endif //linux
}Player;

PLAYER_DEF bool player_init(Player *player, Decoder_Fmt fmt, int channels, int sample_rate);
PLAYER_DEF void player_free(Player *player);

PLAYER_DEF bool player_open_file(Player *player, const char *filepath);
PLAYER_DEF bool player_open_file_from_memory(Player *player,
					     const char *memory, unsigned long long memory_size);
PLAYER_DEF bool player_close_file(Player *player);

PLAYER_DEF bool player_device_init(Player *player, int sample_rate);
PLAYER_DEF bool player_device_free(Player *player);

PLAYER_DEF bool player_play(Player *player);
PLAYER_DEF bool player_stop(Player *player);
PLAYER_DEF bool player_toggle(Player *player);

PLAYER_DEF void *player_play_thread(void *arg);

////////////////////////////////////////////////////
//TODO: right now i am stopping the play_thread for changing the volume etc.
//Maybe do that differently

PLAYER_DEF bool player_get_volume(Player *player, float *volume);
PLAYER_DEF bool player_set_volume(Player *player, float volume);
PLAYER_DEF bool player_get_duration_abs(Player *player, float *duration);
PLAYER_DEF bool player_get_timestamp_abs(Player *player, float *timestamp);
PLAYER_DEF bool player_get_duration(Player *player, float *duration);
PLAYER_DEF bool player_get_timestamp(Player *player, float *timestamp);
PLAYER_DEF bool player_seek(Player *player, float secs);

#ifdef PLAYER_IMPLEMENTATION

PLAYER_DEF bool player_init(Player *player, Decoder_Fmt fmt, int channels, int sample_rate) {

    if(!xaudio_init(channels, sample_rate)) {
	return false;
    }

    if(!decoder_buffer_init(&player->buffer, PLAYER_N, PLAYER_BUFFER_SIZE)) {
	return false;
    }

    player->fmt = fmt;
    player->channels = channels;
    player->sample_rate = 0;
    player->volume = PLAYER_VOLUME;
    player->file = NULL;

    player->decoder_used = false;
    player->playing = false;

    return true;
}

PLAYER_DEF bool player_device_init(Player *player, int sample_rate) {

    if(player->sample_rate == sample_rate) {
	return true;
    }

    player_device_free(player);
  
#ifdef _WIN32
    WAVEFORMATEX waveFormat;
    decoder_get_waveformat(&waveFormat,
			   player->fmt,
			   player->channels,
			   sample_rate);  
    if(!xaudio_device_init(&player->device, &waveFormat)) {
	return false;
    }
  
    player->audio_thread = NULL;
    player->decoding_thread = NULL;
    player->samples = DECODER_XAUDIO2_SAMPLES;
#endif //_WIN32
  
    /*
      #elif linux

      player->pcm = NULL;
      if(snd_pcm_open(&player->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
      return false;
      }
      if(snd_pcm_set_params(player->pcm, SND_PCM_FORMAT_S16_LE, //TODO unhardcode everything
      SND_PCM_ACCESS_RW_INTERLEAVED, channels,
      sample_rate, 0, sample_rate / 4) < 0) {
      return false;
      }
      snd_pcm_uframes_t buffer_size = 0;
      snd_pcm_uframes_t period_size = 0;
      if(snd_pcm_get_params(player->pcm, &buffer_size, &period_size) < 0) {
      return false;
      }
  
      player->audio_thread = 0;  
      player->decoding_thread = 0;
      player->samples = period_size;
      #endif //_WIN32
    */

    player->sample_rate = sample_rate;
  
    return true;
}

//TODO
PLAYER_DEF bool player_device_free(Player *player) {
    if(player->sample_rate == 0) {
	return false;
    }

#ifdef _WIN32
    xaudio_device_free(&player->device);
#endif //_WIN32

    player->sample_rate = 0;
  
    return true;
}

PLAYER_DEF bool player_open_file(Player *player, const char *filepath) {

    int sample_rate = 44100;
    if(!player_device_init(player, sample_rate)) { //this sets player->samples
	return false;
    }

    player->file = fopen(filepath, "rb");

    if(!decoder_init(&player->decoder,
		     //decoder_init_from_file_read, decoder_init_from_file_seek,
		     avio_read, avio_seek,
		     player->file,
		     player->fmt,
		     player->channels,
		     player->volume,
		     player->samples)) {
	return false;
    }
    player->decoder_used = true;

    double volume;
    av_opt_get_double(player->decoder.swr_context, "rmvol", 0, &volume);
    player->volume = (float) volume;

    float total = (float) player->decoder.av_format_context->duration / AV_TIME_BASE / 60.0f;
    float secs = floorf(total);
    float comma = (total - secs) * 60.0f / 100.0f;

    player->duration = secs + comma;
    player->duration_abs = total * 60.f;

    AVRational rational = player
	->decoder
	.av_format_context
	->streams[player->decoder.stream_index]
	->time_base;
    player->den = rational.den;
  
    return true;
}

PLAYER_DEF bool player_open_file_from_memory(Player *player, const char *memory,
					     unsigned long long memory_size) {

    int sample_rate = 44100;
    if(!player_device_init(player, sample_rate)) { //this sets player->samples
	return false;
    }
        
    if(!decoder_init_from_memory(&player->decoder, memory, memory_size,
				 player->fmt,
				 player->channels,
				 player->volume,
				 player->samples)) {
	return false;
    }
    player->decoder_used = true;

    double volume;
    av_opt_get_double(player->decoder.swr_context, "rmvol", 0, &volume);
    player->volume = (float) volume;

    float total = (float) player->decoder.av_format_context->duration / AV_TIME_BASE / 60.0f;
    float secs = floorf(total);
    float comma = (total - secs) * 60.0f / 100.0f;

    player->duration = secs + comma;
    player->duration_abs = total * 60.f;

    AVRational rational = player
	->decoder
	.av_format_context
	->streams[player->decoder.stream_index]
	->time_base;
    player->den = rational.den;
  
    return true;
}

PLAYER_DEF bool player_close_file(Player *player) {
    if(!player->decoder_used) {
	return false;
    }

    player_stop(player);
    
    decoder_free(&player->decoder);
    if(player->file) {
	fclose(player->file);
	player->file = NULL;
    }
    player->decoder_used = false;

    return true;
}

PLAYER_DEF void *player_play_thread(void *arg) {
    Player *player = (Player *) arg;

    //reset buffer
    player->buffer.play_step = 0;
    player->buffer.fill_step = 0;
    player->buffer.last_size = 0;
    player->buffer.extra_size = 0;

    //asynchronisly fill buffer
    if(!decoder_start_decoding(&player->decoder, &player->buffer, &player->decoding_thread)) {
	fprintf(stderr, "ERROR: player_play_thread could not start decoding_thread!\n");
	return NULL;
    }

    //play buffer
    char *data;
    int data_size;
    while(player->playing &&
	  decoder_buffer_next(&player->buffer, &data, &data_size)) {
    
	if(player->volume > 0.0f) {
#ifdef _WIN32
	    xaudio_device_play(&player->device, data, data_size);
#elif linux
	    int out_samples = data_size / player->decoder.sample_size;
	    int ret = snd_pcm_writei(player->device, data, out_samples);
	  
	    if(ret < 0) {
		if((ret == snd_pcm_recover(player->device, ret, 1)) == 0) {
		    //log something ??
		}
	    }
      
#endif //_WIN32
	} else {
	    thread_sleep(player->buffer.buffer_size * 1000 / player->sample_rate / player->decoder.sample_size);
	}
    
    }

    if(player->buffer.last_size < 0) {
	player_stop(player);
	player->buffer.last_size = -3;
    }
  
    return NULL;
}

PLAYER_DEF bool player_play(Player *player) {
    if(!player->decoder_used || player->playing) {
	return false;
    }

    //start playing
    if(!thread_create(&player->audio_thread, player_play_thread, player)) {
	return false;
    }
  
    player->playing = true;
    return true;
}

PLAYER_DEF bool player_stop(Player *player) {
    if(!player->playing) { //actually !player->decoder_used || !player->playing
	return false;
    }

    //stop player
    player->playing = false; //this would be better seperated
    thread_join(player->audio_thread);
    player->buffer.last_size = -2;
    thread_join(player->decoding_thread);

#ifdef _WIN32
    player->audio_thread = NULL;
    player->decoding_thread = NULL;
#elif __GNUC__
    player->audio_thread = 0;
    player->decoding_thread = 0;
#endif //_WIN32
  
    return true;
}

PLAYER_DEF bool player_toggle(Player *player) {
    if(!player->decoder_used) {
	return false;
    }

    return player->playing
	? player_stop(player)
	: player_play(player);
}

PLAYER_DEF bool player_get_volume(Player *player, float *volume) {
    if(!player->decoder_used) {
	return false;
    }

    *volume = player->volume;

    return true;
}

PLAYER_DEF bool player_set_volume(Player *player, float volume) {

    if(volume < 0.0f || volume > 1.0f) {
	return false;
    }
  
    if(!player->decoder_used) {
	return false;
    }

    bool stopped = player_stop(player);

    //set volume
    av_opt_set_double(player->decoder.swr_context, "rmvol", volume, 0);  
    swr_init(player->decoder.swr_context);

    //cache volume
    player->volume = volume;

    if(stopped && !player_play(player)) {
	return false;
    }

    return true;
}

PLAYER_DEF bool player_get_duration_abs(Player *player, float *duration) {
    if(!player->decoder_used) {
	return false;
    }

    *duration = player->duration_abs;

    return true;
}

PLAYER_DEF bool player_get_timestamp_abs(Player *player, float *timestamp) {
    if(!player->decoder_used) {
	return false;
    }

    *timestamp = (float) player->decoder.pts / (float) player->den;

    return true;
}

PLAYER_DEF bool player_get_duration(Player *player, float *duration) {
    if(!player->decoder_used) {
	return false;
    }

    *duration = player->duration;

    return true;
}

PLAYER_DEF bool player_get_timestamp(Player *player, float *timestamp) {
    if(!player->decoder_used) {
	return false;
    }

    float secs = (float) player->decoder.pts / (float) player->den;
    *timestamp = floorf(secs / 60.0f);
    *timestamp += (float) ((int) secs % 60) / 100;

    return true;
}

PLAYER_DEF bool player_seek(Player *player, float secs) {
    if(!player->decoder_used) {
	return false;
    }
  
    bool stopped = player_stop(player);

    int64_t seek = player->den * (int) secs; //player->decoder.pts + player->den*secs
    av_seek_frame(player->decoder.av_format_context, player->decoder.stream_index, seek, 0);
    player->decoder.pts = seek;

    if(stopped && !player_play(player)) {
	return false;
    }

    return true;
}

PLAYER_DEF void player_free(Player *player) {
    player_stop(player);
    player_close_file(player);
    player_device_free(player);
    decoder_buffer_free(&player->buffer);
}

#endif //PLAYER_IMPLEMENTATION

#endif //PLAYER_H_H
