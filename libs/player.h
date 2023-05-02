#ifndef PLAYER_H_H
#define PLAYER_H_H

#ifdef PLAYER_IMPLEMENTATION

#ifdef _WIN32
#define XAUDIO_IMPLEMENTATION
#endif //_WIN32

#define THREAD_IMPLEMENTATION
#define DECODER_IMPLEMENTATION
#define HTTP_IMPLEMENTATION
#endif //PLAYER_IMPLEMENTATION

#ifndef PLAYER_DEF
#define PLAYER_DEF static inline
#endif //PLAYER_DEF

#define PLAYER_BUFFER_SIZE 4096
#define PLAYER_N 8
#define PLAYER_VOLUME 0.1f

#define PLAYER_BUFFER_CAP HTTP_BUFFER_CAP

#ifdef linux
#include <alsa/asoundlib.h>
#endif //linux

#include "http.h"
#include "decoder.h"
#include "xaudio.h"
#include "thread.h"

typedef struct{
  const char *memory;
  long long unsigned int size;
  long long unsigned int pos;
}Player_Memory;

typedef struct{
  Http http;
  const char *route;
  bool ssl;
  
  char buffer[PLAYER_BUFFER_CAP];
  ssize_t nbytes_total;
  int offset;

  int len;
  int pos;
}Player_Socket;

PLAYER_DEF bool player_socket_init(Player_Socket *s, const char *url, int start, int end);
PLAYER_DEF void player_socket_free(Player_Socket *s);

typedef struct{
  Decoder_Fmt fmt;
  int channels;

  float current_volume;
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

  FILE *file;
  Player_Memory decoder_memory;
  Player_Socket decoder_socket;

  int sample_rate;
#ifdef _WIN32
  XAudio2Device device;
#elif linux
  snd_pcm_t *device;
#endif //linux
}Player;

PLAYER_DEF bool player_init(Player *player, Decoder_Fmt fmt, int channels, int sample_rate);
PLAYER_DEF void player_free(Player *player);

//FROM FILE
PLAYER_DEF bool player_open_file(Player *player, const char *filepath);
PLAYER_DEF int player_decoder_file_read(void *opaque, uint8_t *buf, int buf_size);
PLAYER_DEF int64_t player_decoder_file_seek(void *opaque, int64_t offset, int whence);

//FROM MEMORY
PLAYER_DEF bool player_open_memory(Player *player, const char *memory, unsigned long long memory_size);
PLAYER_DEF int player_decoder_memory_read(void *opaque, uint8_t *buf, int buf_size);
PLAYER_DEF int64_t player_decoder_memory_seek(void *opaque, int64_t offset, int whence);

//FROM URL
PLAYER_DEF bool player_open_url(Player *player, const char *url);
PLAYER_DEF int player_decoder_url_read(void *opaque, uint8_t *buf, int buf_size);
PLAYER_DEF int64_t player_decoder_url_seek(void *opaque, int64_t offset, int whence);


PLAYER_DEF bool player_close(Player *player);

PLAYER_DEF bool player_device_init(Player *player, int sample_rate);
PLAYER_DEF bool player_device_free(Player *player);

PLAYER_DEF bool player_play(Player *player);
PLAYER_DEF bool player_stop(Player *player);
PLAYER_DEF bool player_toggle(Player *player);

PLAYER_DEF void *player_play_thread(void *arg);

////////////////////////////////////////////////////

PLAYER_DEF void player_init_stats(Player *player);

PLAYER_DEF bool player_set_volume(Player *player, float volume);
PLAYER_DEF bool player_get_duration_abs(Player *player, float *duration);
PLAYER_DEF bool player_get_timestamp_abs(Player *player, float *timestamp);
PLAYER_DEF bool player_get_duration(Player *player, float *duration);
PLAYER_DEF bool player_get_timestamp(Player *player, float *timestamp);
PLAYER_DEF bool player_seek(Player *player, float secs);

#ifdef PLAYER_IMPLEMENTATION

//TODO: right now i assume that the url, accepts ranges in bytes
PLAYER_DEF bool player_socket_init(Player_Socket *socket, const char *url, int start, int end) {
  if(url) {
    size_t url_len = strlen(url);    
    bool ssl;
    size_t hostname_len;

    int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);
    if(hostname < 0) {
      return false;
    }

    int directory_len = url_len - hostname - hostname_len;
    const char *route = "/";
    if(directory_len>0) {
      route = url + hostname + hostname_len;
    }
    
    if(!http_init2(&socket->http, url + hostname, hostname_len, ssl)) {
      return false;
    }
    socket->route = route;
    socket->ssl = ssl;
  } else {
    if(!http_init2(&socket->http, socket->http.host, socket->http.host_len, socket->ssl)) {
      return false;
    }
  }

  char request_buffer[PLAYER_BUFFER_CAP];
  if(end == 0) {
    HttpHeader header;
    if(!sendf(http_send_len2, &socket->http, request_buffer, PLAYER_BUFFER_CAP,
	      "HEAD %s HTTP/1.1\r\n"
	      "Host: %.*s\r\n"
	      "\r\n",
	      socket->route, socket->http.host_len,
	      socket->http.host)) {
      return false;
    }

    if(!http_read_body(&socket->http, NULL, NULL, &header)) {
      return false;
    }

    string key = STRING("Content-Length");
    string key2 = STRING("content-length");
    string value;
    if(!http_header_has(&header, key, &value) && !http_header_has(&header, key2, &value)) {
      return false;
    }

    if(!string_chop_int(&value, &end)) {
      return false;
    }
  }

  http_free(&socket->http);
  if(!http_init2(&socket->http, socket->http.host, socket->http.host_len, socket->ssl)) {
    return false;
  }

  if(!sendf(http_send_len2, &socket->http, request_buffer, PLAYER_BUFFER_CAP,
	    "GET %s HTTP/1.1\r\n"
	    "Host: %.*s\r\n"
	    "Range: bytes=%d-%d\r\n"
	    "\r\n", socket->route, socket->http.host_len, socket->http.host, start, end)) {
    return false;
  }

  socket->pos = start;
  socket->len = end;
  socket->offset = 0;
  if(!http_skip_headers(&socket->http,
			socket->buffer, PLAYER_BUFFER_CAP,
			&socket->nbytes_total, &socket->offset)) {
    printf("here it fails\n");
    return false;
  }
  
  return true;
}

PLAYER_DEF void player_socket_free(Player_Socket *s) {
  http_free(&s->http);
}

PLAYER_DEF bool player_init(Player *player, Decoder_Fmt fmt, int channels, int sample_rate) {
  (void) sample_rate;

#ifdef _WIN32
  if(!xaudio_init(channels, sample_rate)) {
    return false;
  }
#endif //_WIN32

  if(!decoder_buffer_init(&player->buffer, PLAYER_N, PLAYER_BUFFER_SIZE)) {
    return false;
  }

  player->fmt = fmt;
  player->channels = channels;
  player->sample_rate = 0;
  player->file = NULL;
  player->current_volume = -1.f;
  player->decoder_memory = (Player_Memory) {0};
  player->decoder_socket.len = -1;

  player->decoder_used = false;
  player->playing = false;

#ifdef _WIN32    
  player->samples = DECODER_XAUDIO2_SAMPLES;
#else
  player->samples = 32;
#endif //_WIN32

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
#elif linux

  (void) sample_rate;
  player->device = NULL;
  if(snd_pcm_open(&player->device, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
    return false;
  }
  if(snd_pcm_set_params(player->device, SND_PCM_FORMAT_S16_LE, //TODO unhardcode everything
			SND_PCM_ACCESS_RW_INTERLEAVED, player->channels,
			sample_rate, 0, sample_rate / 4) < 0) {
    return false;
  }
  snd_pcm_uframes_t buffer_size = 0;
  snd_pcm_uframes_t period_size = 0;
  if(snd_pcm_get_params(player->device, &buffer_size, &period_size) < 0) {
    return false;
  }
  
  player->audio_thread = 0;  
  player->decoding_thread = 0;
  //player->samples = period_size;
#endif //_WIN32

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
#else
  snd_pcm_drain(player->device);
  snd_pcm_close(player->device);
  player->device = NULL;  
#endif //_WIN32

  player->sample_rate = 0;
  
  return true;
}

PLAYER_DEF void player_init_stats(Player *player) {
  
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
}

PLAYER_DEF int player_decoder_file_read(void *opaque, uint8_t *buf, int buf_size) {
  FILE *f = (FILE *)opaque;

  int bytes_read = fread(buf, 1, buf_size, f);

  if (bytes_read == 0) {
    if(feof(f)) return AVERROR_EOF;
    else return AVERROR(errno);
  }
  
  return bytes_read;
}

PLAYER_DEF int64_t player_decoder_file_seek(void *opaque, int64_t offset, int whence) {
  
  FILE *file = (FILE *)opaque;
  
  if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
    return AVERROR_INVALIDDATA;
  }

  if(fseek(file, offset, whence)) {
    return AVERROR(errno);
  }

  return ftell(file);
}

PLAYER_DEF int player_decoder_memory_read(void *opaque, uint8_t *buf, int _buf_size) {
  Player_Memory *memory = (Player_Memory *) opaque;

  size_t buf_size = (size_t) _buf_size;

  if (buf_size > memory->size - memory->pos) {
    buf_size = memory->size - memory->pos;
  }

  if (buf_size <= 0) {
    return AVERROR_EOF;
  }

  memcpy(buf, memory->memory + memory->pos, buf_size);
  memory->pos += buf_size;

  return (int )buf_size;
}

PLAYER_DEF int64_t player_decoder_memory_seek(void *opaque, int64_t offset, int whence) {

  Player_Memory *memory = (Player_Memory *) opaque;

  switch (whence) {
  case SEEK_SET:
    memory->pos = offset;
    break;
  case SEEK_CUR:
    memory->pos += offset;
    break;
  case SEEK_END:
    memory->pos = memory->size + offset;
    break;
  case AVSEEK_SIZE:
      return (int64_t) memory->size;
  default:
    return AVERROR_INVALIDDATA;
  }

  if (memory->pos > memory->size) {
    return AVERROR(EIO);
  }
    
  return memory->pos;
}

PLAYER_DEF int player_decoder_url_read(void *opaque, uint8_t *buf, int _buf_size) {
  Player_Socket *socket = (Player_Socket *) opaque;

  int buf_size = _buf_size;
  int buf_off = 0;

  do{

    if(socket->nbytes_total > 0) {

      int len = (int) (socket->nbytes_total - (ssize_t) socket->offset);
      if(socket->pos + len > socket->len) {
	len = socket->len - socket->pos;
      }
      
      if(len == 0) {
	socket->nbytes_total -= (ssize_t) socket->offset;
	socket->offset = 0;
	continue;
      }

      if(len > buf_size) {
	len = buf_size;
	
	memcpy(buf + buf_off, socket->buffer + socket->offset, len);

	socket->offset += len;
      } else {

	memcpy(buf + buf_off, socket->buffer + socket->offset, len);
	
	socket->nbytes_total = 0;
	socket->offset = 0;
      }

      socket->pos += len;
      buf_size -= len;
      buf_off += len;
      
    } else {
      socket->nbytes_total = http_read(&socket->http, socket->buffer, PLAYER_BUFFER_CAP);
      
      if(socket->nbytes_total == -1) {
	return -1; //network ERRROR 
      }

    }

    if(socket->pos > socket->len) {
      return AVERROR_EOF;
    }

  }while(buf_size > 0 && socket->pos < socket->len);

  
  return _buf_size;
}

PLAYER_DEF int64_t player_decoder_url_seek(void *opaque, int64_t offset, int whence) {
    Player_Socket *socket = (Player_Socket *) opaque;

    int pos = socket->pos;

    switch (whence) {
    case SEEK_SET:
	pos = offset;
	break;
    case SEEK_CUR:
	pos += offset;
	break;
    case SEEK_END:
	pos = socket->len + offset;
	break;
    case AVSEEK_SIZE:
	return (int64_t) socket->len;
    default:
	printf("something different: %d\n", whence); fflush(stdout);
	return AVERROR_INVALIDDATA;
    }

    if(pos < 0 || pos > socket->len) return AVERROR_EOF;
    if(pos - socket->pos > (10 * PLAYER_BUFFER_CAP) || pos < socket->pos) {
      player_socket_free(socket);
      player_socket_init(socket, NULL, pos, socket->len);
    }

    while(socket->pos < pos){
	if(socket->nbytes_total > 0) {

	    int len = (int) (socket->nbytes_total - (ssize_t) socket->offset);
	    if(socket->pos + len > socket->len) {
		len = socket->len - socket->pos;
	    }
      
	    if(len == 0) {
		socket->nbytes_total -= (ssize_t) socket->offset;
		socket->offset = 0;
		continue;
	    }

	    if(socket->pos + len > pos) {
		len = pos - socket->pos;
		socket->offset += len;
	    } else {
		socket->nbytes_total = 0;
		socket->offset = 0;			
	    }

	    socket->pos += len;
	} else if(socket->pos < pos) {
	    socket->nbytes_total = http_read(&socket->http, socket->buffer, PLAYER_BUFFER_CAP);      
	    if(socket->nbytes_total == -1) {
		return -1; //network ERRROR 
	    }
	}	
	if(socket->pos > socket->len) {
	    return AVERROR_EOF;
	}
    }

    return (int64_t) socket->pos;
}

PLAYER_DEF bool player_open_file(Player *player, const char *filepath) {

  player->file = fopen(filepath, "rb");
  if(!player->file) {
    return false;
  }
    
  if(!decoder_init(&player->decoder,
		   player_decoder_file_read, player_decoder_file_seek,
		   player->file,
		   player->fmt,
		   player->channels,
		   PLAYER_VOLUME,
		   player->samples)) {
    fclose(player->file);
    player->file = NULL;
    return false;
  }

  if(!player_device_init(player, player->decoder.sample_rate)) {
    decoder_free(&player->decoder);
    fclose(player->file);
    player->file = NULL;
    return false;
  }
  player->decoder_used = true;

  player_init_stats(player);
  return true;
}

PLAYER_DEF bool player_open_memory(Player *player, const char *memory,
					unsigned long long memory_size) {

  player->decoder_memory.memory = memory;
  player->decoder_memory.size = memory_size;
  player->decoder_memory.pos = 0;
  if(!decoder_init(&player->decoder,
		   player_decoder_memory_read,
		   player_decoder_memory_seek, &player->decoder_memory,
		   player->fmt, player->channels, player->current_volume, player->samples)) {
    player->decoder_memory = (Player_Memory) {0};
    return false;
  }

  if(!player_device_init(player, player->decoder.sample_rate)) {
    return false;
  }
  player->decoder_used = true;

  player_init_stats(player);
  return true;
}

PLAYER_DEF bool player_open_url(Player *player, const char *url) {

  if(!player_socket_init(&player->decoder_socket, url, 0, 0)) {
    return false;
  }

  if(!decoder_init(&player->decoder,
		   player_decoder_url_read,
		   player_decoder_url_seek, &player->decoder_socket,
		   player->fmt, player->channels, player->current_volume, player->samples)) {
    player_socket_free(&player->decoder_socket);
    player->decoder_memory = (Player_Memory) {0};
    return false;
  }

  if(!player_device_init(player, player->decoder.sample_rate)) {
    return false;
  }
  player->decoder_used = true;

  player_init_stats(player);
  
  return true;
}

PLAYER_DEF bool player_close(Player *player) {
  if(!player->decoder_used) {
    return false;
  }

  player_stop(player);
    
  decoder_free(&player->decoder);

  //FREE any kind of track ? 
  if(player->file) {
    fclose(player->file);
    player->file = NULL;
  }
  player->decoder_memory = (Player_Memory) {0};

  if(player->decoder_socket.len != -1) {
    player_socket_free(&player->decoder_socket);
    player->decoder_socket.len = -1;
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
    
    if(player->current_volume > 0.0f) {
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

PLAYER_DEF bool player_set_volume(Player *player, float volume) {

  if(volume < 0.0f || volume > 1.0f) {
    return false;
  }
  
  if(!player->decoder_used) {
    return false;
  }
  player->decoder.target_volume = volume;

  //cache volume
  player->current_volume = volume;

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
  player_close(player);
  player_device_free(player);
  decoder_buffer_free(&player->buffer);
}

#endif //PLAYER_IMPLEMENTATION

#endif //PLAYER_H_H
