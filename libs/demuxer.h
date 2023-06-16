#ifndef DEMUXER_H
#define DEMUXER_H

#ifdef DEMUXER_IMPLEMENTATION
#  define HTTP_IMPLEMENTATION
#endif //DEMUXER_IMPLEMENTATION

#include "../libs/http.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define SAMPLES 1024

#ifdef DEMUXER_IMPLEMENTATION
#define DEMUXER_BUFFER_CAP (8912)

typedef struct{
  Http http;
  const char *route;
  bool ssl;
  bool close_connection;
  
  int64_t len;
  int64_t pos;
}Player_Socket;

typedef struct{
  bool skip_extract_frame;
  int64_t duration;
  
  AVIOContext *av_io_context;
  AVFormatContext *av_format_context;
  AVPacket *packet;
  AVFrame *frame;
  
  Player_Socket socket;
  Io_File file;

  // INFO
  int stream;
  int pts;
  AVCodecContext *av_codec_context;
  AVRational time_base;
  union{
    SwrContext *swr_context;
    struct SwsContext* sws_context;  
  }as;
  int samples;
  int sample_rate;
  int buffer_stride;
  int buffer_height;
  int buffer_width;

  // DATA
  bool is_audio;
  unsigned char *buffer;
  int64_t *ptss;
  int64_t cap;
  int64_t count;
}Demuxer;

typedef void (*Demuxer_On_Decode)(Demuxer *demuxer, int64_t pts, int out_samples, void *arg);

void demuxer_init(Demuxer *demuxer, const char *filepath);
int64_t demuxer_seek(Demuxer *demuxer, float p);
bool demuxer_decode(Demuxer *demuxer, Demuxer_On_Decode on_decode, void *arg);

bool player_socket_init(Player_Socket *s, const char *url, int start, int end);
void player_socket_free(Player_Socket *s);

typedef struct{
  const char *memory;
  long long unsigned int size;
  long long unsigned int pos;
}Player_Memory;

int file_read(void *opaque, uint8_t *buf, int buf_size);
int64_t file_seek(void *opaque, int64_t offset, int whence);
int memory_read(void *opaque, uint8_t *buf, int _buf_size);
int64_t memory_seek(void *opaque, int64_t offset, int whence);
int url_read(void *opaque, uint8_t *buf, int buf_size);
int64_t url_seek(void *opaque, int64_t offset, int whence);


#endif //DEMUXER_IMPLEMENTATION

//////////////////////////////////////////////////////////////////////////////////

void panic2(const char *mess) {
  fprintf(stderr, "ERROR: %s\n", mess); fflush(stderr);
  exit(1);
}

void demuxer_init(Demuxer *demuxer, const char *filepath) {
  demuxer->skip_extract_frame = false;

  if(io_file_open(&demuxer->file, filepath)) {
    demuxer->av_io_context =
      avio_alloc_context(NULL, 0, 0, &demuxer->file, file_read, NULL, file_seek);
    if(!demuxer->av_io_context) {
      panic2("avio_alloc_context");
    }

  } else if(player_socket_init(&demuxer->socket, filepath, 0, 0)) {
    demuxer->av_io_context =
      avio_alloc_context(av_malloc(DEMUXER_BUFFER_CAP), DEMUXER_BUFFER_CAP, 0, &demuxer->socket, url_read, NULL, url_seek);
    if(!demuxer->av_io_context) {
      panic2("avio_alloc_context");
    }
	  
  } else {
    panic2("file_open");	  
  }
  

  demuxer->av_format_context = avformat_alloc_context();
  if(!demuxer->av_format_context) {
    panic2("avformat_alloc_context");
  }

  demuxer->av_format_context->pb = demuxer->av_io_context;
  demuxer->av_format_context->flags = AVFMT_FLAG_CUSTOM_IO;
  if (avformat_open_input(&demuxer->av_format_context, "", NULL, NULL) != 0) {
    panic2("avformat_open_input");
  }

  if(avformat_find_stream_info(demuxer->av_format_context, NULL) < 0) {
    panic2("avformat_find_stream_info");
  }
  av_dump_format(demuxer->av_format_context, 0, "", 0);

  demuxer->stream = -1; // for now unused

  AVCodecParameters *av_codec_parameters = NULL;
  for(size_t i=0;i<demuxer->av_format_context->nb_streams;i++) {
    av_codec_parameters = demuxer->av_format_context->streams[i]->codecpar;
    if(demuxer->is_audio) {
      if(av_codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
	demuxer->stream = (int) i;	    
      }
    } else {
      if(av_codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
	demuxer->stream = (int) i;
      }	
    }    
  }

  if(demuxer->stream == -1) {
    fprintf(stderr, "ERROR: Can not find stream in file: %s\n", filepath);
    panic2("asfasdf");
  }

  demuxer->packet = av_packet_alloc();
  if(!demuxer->packet) {
    panic2("av_packet_alloc");
  }
  
  demuxer->frame = av_frame_alloc();
  if(!demuxer->frame) {
    panic2("av_frame_alloc");
  }

  av_codec_parameters = demuxer->av_format_context->streams[demuxer->stream]->codecpar;
  const AVCodec *av_codec = avcodec_find_decoder(av_codec_parameters->codec_id);
  if(!av_codec) {
    panic2("av_codec");
  }
  demuxer->av_codec_context = avcodec_alloc_context3(av_codec);
  if(!demuxer->av_codec_context) {
    panic2("avcodec_alloc_context3");
  }

  if(avcodec_parameters_to_context(demuxer->av_codec_context, av_codec_parameters) < 0) {
    panic2("avcodec_parameters_to_context");    
  }

  if(avcodec_open2(demuxer->av_codec_context, av_codec, NULL) < 0) {
    panic2("avcodec_open2");
  }
  AVStream* stream = demuxer->av_format_context->streams[demuxer->stream];
  demuxer->time_base = stream->time_base;

  if(demuxer->is_audio) {   // AUDIO      

    enum AVSampleFormat av_sample_format = AV_SAMPLE_FMT_S16;
    int s16_bits_per_sample = 16;
    int channels = 2;
    int samples = SAMPLES;
    int sample_size = channels * s16_bits_per_sample / 8;
    demuxer->sample_rate = demuxer->av_codec_context->sample_rate;

    demuxer->samples = samples;
  
    demuxer->as.swr_context = swr_alloc();
    if(!demuxer->as.swr_context) {
      panic2("swr_alloc");
    }

    const char *ch_layout = "mono";
    if(channels == 1) {
      ch_layout = "mono";
    } else if(channels == 2) {
      ch_layout = "stereo";
    } else {
      panic2("unsupported channel layout");
    }

    /*
      char chLayoutDescription[128];
      int sts = av_channel_layout_describe(&av_codec_parameters->ch_layout, chLayoutDescription, sizeof(chLayoutDescription));
      if(sts < 0) {
      panic2("av_channel_layout_describe");
      }
    */
    char *chLayoutDescription = "stereo";

    //av_opt_set(demuxer->audio_swr_context, "in_chlayout", chLayoutDescription, 0); //in_channel_layout
    av_opt_set(demuxer->as.swr_context, "in_channel_layout", chLayoutDescription, 0); //in_channel_layout
    av_opt_set_int(demuxer->as.swr_context, "in_sample_fmt", demuxer->av_codec_context->sample_fmt, 0);
    av_opt_set_int(demuxer->as.swr_context, "in_sample_rate", demuxer->av_codec_context->sample_rate, 0);
    //av_opt_set(demuxer->audio_swr_context, "out_chlayout", ch_layout, 0); //out_channel_layout
    av_opt_set(demuxer->as.swr_context, "out_channel_layout", ch_layout, 0); //out_channel_layout
    av_opt_set_int(demuxer->as.swr_context, "out_sample_fmt", av_sample_format, 0);
    av_opt_set_int(demuxer->as.swr_context, "out_sample_rate", demuxer->av_codec_context->sample_rate, 0);

    if(swr_init(demuxer->as.swr_context) < 0) {
      panic2("swr_init");
    }

    int max_buffer_size = samples * sample_size;
    demuxer->buffer = (unsigned char*) malloc(max_buffer_size);
    if(!demuxer->buffer) {
      panic2("malloc");
    }

  }  else { // VIDEO
    int video_width = av_codec_parameters->width;
    int video_height = av_codec_parameters->height;
    printf("Video size: (%d, %d)\n", video_width, video_height);
  
    //////////////////////////////////////////////

    int input_pixel_format = av_codec_parameters->format;
    int rgb_pixel_format = AV_PIX_FMT_RGB24;
    demuxer->as.sws_context =
      sws_getContext(video_width, video_height, input_pixel_format,
		     video_width, video_height, rgb_pixel_format,
		     SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if(!demuxer->as.sws_context) {
      panic2("sws_getContext");
    }

    demuxer->buffer = malloc(video_width * video_height * 3);
    if(!demuxer->buffer) {
      panic2("malloc");
    }
    demuxer->buffer_width = video_width;
    demuxer->buffer_height = video_height;  
    demuxer->buffer_stride = video_width * 3;

    demuxer->duration = demuxer->av_format_context->duration / AV_TIME_BASE;
    //////////////////////////////
	
    int64_t bytes_per_pixel = 3;
    demuxer->count = 0;
    demuxer->cap = 40;
	
    int64_t size = video_width * video_height
      * bytes_per_pixel * demuxer->cap;

    demuxer->buffer = (unsigned char *) malloc(size);
    if(!demuxer->buffer) {
      panic("Can not allocate enough memory\n");
    }

    demuxer->ptss = (int64_t *) malloc(sizeof(int64_t) * demuxer->cap);
    if(!demuxer->ptss) {
      panic("Can not allocate enough memory\n");
    }

  }
}

int64_t demuxer_seek(Demuxer *demuxer, float p) {
  int64_t seek_to_sec = (int64_t) (p * (float) demuxer->duration);
  int64_t seek_to_ms = (int64_t) demuxer->time_base.den * seek_to_sec;

  av_seek_frame(demuxer->av_format_context, demuxer->stream, seek_to_ms , AVSEEK_FLAG_BACKWARD);
  while(1) {
    while(av_read_frame(demuxer->av_format_context, demuxer->packet) < 0) ;
    if(demuxer->packet->stream_index == demuxer->stream) {
      demuxer->skip_extract_frame = true;
      break;
    }
  }
  //av_seek_frame(demuxer->av_format_context, demuxer->audio_stream, seek_to_audio, AVSEEK_FLAG_ANY);

  return -1;
}

bool demuxer_decode(Demuxer *demuxer, Demuxer_On_Decode on_decode, void *arg) {

  if(demuxer->skip_extract_frame) {
    demuxer->skip_extract_frame = false;
  } else {
    if(av_read_frame(demuxer->av_format_context, demuxer->packet) < 0) {
      return false;
    }    
  }

  if(demuxer->packet->stream_index == demuxer->stream) {	
    if(avcodec_send_packet(demuxer->av_codec_context, demuxer->packet)) {
      panic2("avcodec_send_packet");
    }

    int response;
    while( (response = avcodec_receive_frame(demuxer->av_codec_context, demuxer->frame)) >= 0) {
      if(response < 0) {
	panic2("avcodec_receive_frame");
      } else if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
	break;
      }
      demuxer->pts = demuxer->frame->pts;

      if(demuxer->is_audio) {
	//TODO: maybe in the future you have to supply out_samples?
	int out_samples = swr_convert(demuxer->as.swr_context, &demuxer->buffer, demuxer->samples, (const unsigned char **) (demuxer->frame->data), demuxer->frame->nb_samples);
      
	while(out_samples > 0) {
	  on_decode(demuxer, demuxer->pts, out_samples, arg);
	  out_samples = swr_convert(demuxer->as.swr_context, &demuxer->buffer, demuxer->samples, NULL, 0);
	}
      } else {
	int64_t index = demuxer->count % demuxer->cap;
	unsigned char *buffer = demuxer->buffer + (index * demuxer->buffer_width * demuxer->buffer_height * (int64_t) 3);
	sws_scale(demuxer->as.sws_context, (const uint8_t * const*) demuxer->frame->data, demuxer->frame->linesize, 0, demuxer->buffer_height, &buffer, &demuxer->buffer_stride);
	demuxer->ptss[index] = demuxer->pts;
	demuxer->count++;
      }

      av_frame_unref(demuxer->frame);
    }
  }

  av_packet_unref(demuxer->packet);
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int file_read(void *opaque, uint8_t *buf, int buf_size) {
  Io_File *f = (Io_File *)opaque;

  size_t bytes_read = io_file_fread(buf, 1, buf_size, f);

  if (bytes_read == 0) {
    if(io_file_feof(f)) return AVERROR_EOF;
    else return AVERROR(errno);
  }
  
  return (int) bytes_read;
}

int64_t file_seek(void *opaque, int64_t offset, int whence) {
  
  Io_File *file = (Io_File *)opaque;
  
  if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
    return AVERROR_INVALIDDATA;
  }

  if(io_file_fseek(file, (long) offset, whence)) {
    return AVERROR(errno);
  }

  return io_file_ftell(file);
}

int memory_read(void *opaque, uint8_t *buf, int _buf_size) {
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

int64_t memory_seek(void *opaque, int64_t offset, int whence) {

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

bool player_socket_init(Player_Socket *socket, const char *url, int start, int end) {
  if(url) {
    size_t url_len = strlen(url);    
    bool ssl;
    size_t hostname_len;

    int hostname = http_find_hostname(url, url_len, &hostname_len, &ssl);
    if(hostname < 0) {
      return false;
    }

    size_t directory_len = url_len - hostname - hostname_len;
    const char *route = "/";
    if(directory_len>0) {
      route = url + hostname + hostname_len;
    }
    
    if(!http_init2(&socket->http, url + hostname, hostname_len, ssl)) {
      return false;
    }
    socket->route = route;
    socket->ssl = ssl;
    socket->close_connection = true;
  } else {
    if(socket->close_connection) {
      if(!http_init2(&socket->http, socket->http.host, socket->http.host_len, socket->ssl)) {
	return false;
      }	  
    }
  }

  bool close_connection = socket->close_connection;

#define PLAYER_BUFFER_CAP 8192
  char request_buffer[PLAYER_BUFFER_CAP];
  if(end == 0) {
    HttpHeader header;
    if(!sendf(http_send_len2, &socket->http, request_buffer, PLAYER_BUFFER_CAP,
	      "HEAD %s HTTP/1.1\r\n"
	      "Host: %.*s\r\n"
	      "Connection: Keep-Alive\r\n"
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
      printf("string_chop_int ? \n"); fflush(stdout);
      return false;
    }

    if(http_header_has(&header, STRING("Connection"), &value) &&
       (string_eq(value, STRING("close")) || string_eq(value, STRING("Close"))) ) {
      close_connection = true;
    }

    if(close_connection) {
      http_free(&socket->http);
      if(!http_init2(&socket->http, socket->http.host, socket->http.host_len, socket->ssl)) {
	return false;
      }
    }
  }

  if(!sendf(http_send_len2, &socket->http, request_buffer, PLAYER_BUFFER_CAP,
	    "GET %s HTTP/1.1\r\n"
	    "Host: %.*s\r\n"
	    "Connection: Keep-Alive\r\n"
	    "Range: bytes=%d-%d\r\n"
	    "\r\n", socket->route, socket->http.host_len, socket->http.host, start, end)) {
    return false;
  }

  socket->pos = start;
  socket->len = end;

  ssize_t nbytes_total;
  ssize_t offset;
  if(!http_skip_headers(&socket->http,
			request_buffer, PLAYER_BUFFER_CAP,
			&nbytes_total, &offset)) {
    return false;
  }
  
  return true;
}

void player_socket_free(Player_Socket *s) {
  http_free(&s->http);
}

int url_read(void *opaque, uint8_t *buf, int _buf_size) {
  Player_Socket *socket = (Player_Socket *) opaque;

  int buf_size = _buf_size;
  int buf_off = 0;

  while(buf_size > 0) {
    ssize_t nbytes_total = http_read(&socket->http, buf + buf_off, buf_size);
    if(nbytes_total == -1) {
      return -1; //network ERRROR
    }
    socket->pos += nbytes_total;
    buf_size -= (int) nbytes_total;
    buf_off += (int) nbytes_total;

    if(socket->pos > socket->len) {
      return AVERROR_EOF;
    }
  }

  return _buf_size;
}

int64_t url_seek(void *opaque, int64_t offset, int whence) {

  Player_Socket *socket = (Player_Socket *) opaque;

  int64_t pos = socket->pos;

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
    //TODO: handle this error, log or something
    return AVERROR_INVALIDDATA;
  }

  if(pos < 0 || pos > socket->len) return AVERROR_EOF;
  if(pos - socket->pos > (10 * PLAYER_BUFFER_CAP) || pos < socket->pos) {
    if(socket->close_connection) {
      player_socket_free(socket);
    }
    if(!player_socket_init(socket, NULL, (int) pos, (int) socket->len)) {
      //TODO: handle this error, log or something
      return -1;
    }
  }

  while(socket->pos < pos){
    int count = pos - socket->pos;
    ssize_t nbytes_total = http_read(&socket->http, NULL, count);
    if(nbytes_total != -1) {
      socket->pos += (int64_t) nbytes_total;
    } else {	    
      if(socket->close_connection) {
	player_socket_free(socket);
      }
      if(!player_socket_init(socket, NULL, (int) pos, (int) socket->len)) {
	//TODO: handle this error, log or something
	return -1;
      }
    }       
  }

  return socket->pos;
}


#endif //DEMUXER_H
