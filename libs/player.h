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

#define PLAYER_BUFFER_SIZE 4096
#define PLAYER_N 2
#define PLAYER_VOLUME 0.2f

#include "decoder.h"
#include "xaudio.h"
#include "thread.h"

typedef struct{
  Decoder_Fmt fmt;
  int channels;
  int sample_rate;

  float volume;
  float duration;
  float duration_secs;
  int den;

  bool decoder_used;
  bool playing;

  Decoder_Buffer buffer;
  Decoder decoder;
  Thread audio_thread;
  Thread decoding_thread;
}Player;

//Player also initializes xaudio. Thats maybe not ideal
PLAYER_DEF bool player_init(Player *player, Decoder_Fmt fmt, int channels, int sample_rate);
PLAYER_DEF void player_free(Player *player);

PLAYER_DEF bool player_open_file(Player *player, const char *filepath);
PLAYER_DEF bool player_close_file(Player *player);

PLAYER_DEF bool player_play(Player *player);
PLAYER_DEF bool player_stop(Player *player);
PLAYER_DEF bool player_toggle(Player *player);

PLAYER_DEF void player_play_thread(void *arg);

////////////////////////////////////////////////////
//TODO: right now i am stopping the play_thread for changing the volume etc.
//Maybe do that differently

PLAYER_DEF bool player_get_volume(Player *player, float *volume);
PLAYER_DEF bool player_set_volume(Player *player, float volume);
PLAYER_DEF bool player_get_duration_secs(Player *player, float *duration);
PLAYER_DEF bool player_get_timestamp_secs(Player *player, float *timestamp);
PLAYER_DEF bool player_get_duration(Player *player, float *duration);
PLAYER_DEF bool player_get_timestamp(Player *player, float *timestamp);
PLAYER_DEF bool player_seek(Player *player, int secs);

#ifdef PLAYER_IMPLEMENTATION

PLAYER_DEF bool player_init(Player *player, Decoder_Fmt fmt, int channels, int sample_rate) {

  WAVEFORMATEX waveFormat;
  decoder_get_waveformat(&waveFormat,
			 fmt,
			 channels,
			 sample_rate);  
  if(!xaudio_init(&waveFormat)) {
    return false;
  }
  
  if(!decoder_buffer_init(&player->buffer, PLAYER_N, PLAYER_BUFFER_SIZE)) {
    return false;
  }

  player->fmt = fmt;
  player->channels = channels;
  player->sample_rate = sample_rate;

  player->audio_thread = NULL;
  player->decoding_thread = NULL;
  
  player->decoder_used = false;
  player->playing = false;

  return true;
}

PLAYER_DEF bool player_open_file(Player *player, const char *filepath) {

  if(!decoder_init(&player->decoder, filepath,
		   player->fmt,
		   player->channels,
		   player->sample_rate,
		   PLAYER_VOLUME,
		   DECODER_XAUDIO2_SAMPLES)) { //TODO: fix this
    return false;
  }
  player->decoder_used = true;

  double volume;
  av_opt_get_double(player->decoder.swr_context, "rmvol", 0, &volume);
  player->volume = (float) volume;

  float total = (float) player->decoder.av_format_context->duration / AV_TIME_BASE / 60.0f;
  float secs = floorf(total);
  float comma = (total - secs) * 60.0f / 100.0f;

  player->duration = total * 60.f;
  player->duration_secs = secs + comma;

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
  player->decoder_used = false;

  return true;
}

PLAYER_DEF void player_play_thread(void *arg) {
  Player *player = (Player *) arg;

  //reset buffer
  player->buffer.play_step = 0;
  player->buffer.fill_step = 0;
  player->buffer.last_size = 0;

  //asynchronisly fill buffer
  if(!decoder_start_decoding(&player->decoder, &player->buffer, &player->decoding_thread)) {
    fprintf(stderr, "ERROR: player_play_thread could not start decoding_thread!\n");
    return;
  }

  //play buffer
  char *data;
  int data_size;
  while(player->playing &&
	decoder_buffer_next(&player->buffer, &data, &data_size)) {
    
    if(player->volume > 0.0f) {
      xaudio_play(data, data_size); 
    } else {
      Sleep(player->buffer.buffer_size * 1000 / player->sample_rate / player->decoder.sample_size);
    }
    
  }
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

  player->audio_thread = NULL;
  player->decoding_thread = NULL;
  
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

PLAYER_DEF bool player_get_duration_secs(Player *player, float *duration) {
  if(!player->decoder_used) {
    return false;
  }

  *duration = player->duration_secs;

  return true;
}

PLAYER_DEF bool player_get_timestamp_secs(Player *player, float *timestamp) {
  if(!player->decoder_used) {
    return false;
  }

  float secs = (float) player->decoder.pts / (float) player->den;
  *timestamp = floorf(secs / 60.0f);
  *timestamp += (float) ((int) secs % 60) / 100;

  return true;
}

PLAYER_DEF bool player_get_duration(Player *player, float *duration) {
  if(!player->decoder_used) {
    return false;
  }

  *duration = player->duration_secs;

  return true;
}

PLAYER_DEF bool player_get_timestamp(Player *player, float *timestamp) {
  if(!player->decoder_used) {
    return false;
  }

  *timestamp = (float) player->decoder.pts / (float) player->den;

  return true;
}

PLAYER_DEF bool player_seek(Player *player, int secs) {
  if(!player->decoder_used) {
    return false;
  }
  
  bool stopped = player_stop(player);

  int64_t seek = player->decoder.pts + player->den*secs;
  av_seek_frame(player->decoder.av_format_context, player->decoder.stream_index, seek, 0);

  if(stopped && !player_play(player)) {
    return false;
  }

  return true;
}

PLAYER_DEF void player_free(Player *player) {
  player_stop(player);
  player_close_file(player);
  decoder_buffer_free(&player->buffer);
}

#endif //PLAYER_IMPLEMENTATION

#endif //PLAYER_H_H
