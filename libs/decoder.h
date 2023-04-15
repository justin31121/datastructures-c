#ifndef DECODER_H_H_
#define DECODER_H_H_

#include <stdbool.h>
#include <assert.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

#ifndef DECODER_DEF
#define DECODER_DEF static inline
#endif //DECODER_DEF

#define DECODER_XAUDIO2_SAMPLES 64 //TODO: fix this

#ifdef _WIN32
#include <windows.h>
#include <mmreg.h>
#endif

#ifdef DECODER_IMPLEMENTATION
#define THREAD_IMPLEMENTATION
#endif //DECODER_IMPLEMENTATION

#include "thread.h"

typedef enum {
  DECODER_FMT_NONE = 0,
  DECODER_FMT_U8,
  DECODER_FMT_S16,
  DECODER_FMT_S32,
  DECODER_FMT_FLT,
  DECODER_FMT_DBL,
  DECODER_FMT_U8P,
  DECODER_FMT_S16P,
  DECODER_FMT_S32P,
  DECODER_FMT_FLTP,
  DECODER_FMT_DBLP,
}Decoder_Fmt;

DECODER_DEF enum AVSampleFormat decoder_fmt_to_libav_fmt(Decoder_Fmt fmt);
DECODER_DEF const char *decoder_fmt_to_cstr(Decoder_Fmt fmt);
DECODER_DEF int decoder_fmt_to_bits_per_sample(Decoder_Fmt fmt);

typedef struct Decoder Decoder;

typedef struct{
  int fill_step;
  int play_step;
  int last_size;

  int extra_size;
  
  int n;
  int buffer_size;
  char *buffers;

  int *buffers_size;

  Decoder *decoder;
}Decoder_Buffer;

DECODER_DEF bool decoder_buffer_init(Decoder_Buffer *buffer, int n, int buffer_size);
DECODER_DEF bool decoder_buffer_fill(Decoder_Buffer *buffer, Decoder *decoder, int index);
DECODER_DEF void decoder_buffer_reset(Decoder_Buffer *buffer);
DECODER_DEF bool decoder_buffer_next(Decoder_Buffer *buffer, char **data, int *data_size);
DECODER_DEF void decoder_buffer_free(Decoder_Buffer *buffer);

struct Decoder{
  AVFormatContext *av_format_context;
  AVCodecContext *av_codec_context;
  SwrContext *swr_context;
  int stream_index;
  unsigned char *buffer;

  AVPacket *packet;
  AVFrame *frame;
  int pts;

  int samples;
  int sample_size;

  bool continue_receive;
  bool continue_convert;
};

DECODER_DEF bool decoder_init(Decoder *decoder, const char *file_path,
			      Decoder_Fmt fmt,
			      int channels,
			      int sample_rate,
			      float volume,
			      int samples);
DECODER_DEF bool decoder_decode(Decoder *decoder, int *out_samples);
DECODER_DEF void *decoder_start_decoding_function(void *arg);
DECODER_DEF bool decoder_start_decoding(Decoder *decoder, Decoder_Buffer *buffer, Thread *id);
DECODER_DEF void decoder_free(Decoder *decoder);

#ifdef _WIN32
DECODER_DEF void decoder_get_waveformat(WAVEFORMATEX *waveFormat,
					Decoder_Fmt fmt,
					int channels,
					int sample_rate);
#endif //_WIN32

#ifdef DECODER_IMPLEMENTATION

#ifdef _WIN32
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")
#endif //_WIN32

DECODER_DEF const char *decoder_fmt_to_cstr(Decoder_Fmt fmt) {
  switch(fmt) {
  case DECODER_FMT_NONE:
    return "DECODER_FMT_NONE";
  case DECODER_FMT_S16:
    return "DECODER_FMT_S16";
  case DECODER_FMT_S32:
    return "DECODER_FMT_S32";
  case DECODER_FMT_FLT:
    return "DECODER_FMT_FLT";
  default:
    return "Unknown Decoder_Fmt!";
  }
}

DECODER_DEF int decoder_fmt_to_bits_per_sample(Decoder_Fmt fmt) {
  switch(fmt) {
  case DECODER_FMT_S16:
    return 16;
  case DECODER_FMT_S32:
    return 32;
  case DECODER_FMT_FLT:
    return 32;
  default: {
    fprintf(stderr, "ERROR: decoder_fmt_to_bits_per_channel: nimplemented Fmt: '%s'\n",
	    decoder_fmt_to_cstr(fmt));
    exit(1);
  } 
  }  
}

DECODER_DEF enum AVSampleFormat decoder_fmt_to_libav_fmt(Decoder_Fmt fmt) {
  switch(fmt) {
  case DECODER_FMT_S16:
    return AV_SAMPLE_FMT_S16;
  case DECODER_FMT_S32:
    return AV_SAMPLE_FMT_S32;
  case DECODER_FMT_FLT:
    return AV_SAMPLE_FMT_FLT;
  default: {
    fprintf(stderr, "ERROR: decoder_fmt_to_libav_fmt: nimplemented Fmt: '%s'\n",
	    decoder_fmt_to_cstr(fmt));
    exit(1);
  } 
  }
}

DECODER_DEF bool decoder_buffer_init(Decoder_Buffer *buffer, int n, int buffer_size) {
  
  buffer->buffers = (char *) malloc(sizeof(char) * (n+1) * buffer_size);
  if(!buffer->buffers) {
    fprintf(stderr, "ERROR: decoder_buffer_init: Can not allocate enough memory\n");
    return false;
  }

  buffer->n = n;
  buffer->buffer_size = buffer_size;
  buffer->fill_step = 0;
  buffer->play_step = 0;
  buffer->last_size = 0;
  buffer->extra_size = 0;

  return true;
}

DECODER_DEF void decoder_buffer_reset(Decoder_Buffer *buffer) {
  buffer->fill_step = 0;
  buffer->play_step = 0;
  buffer->last_size = 0;
  buffer->extra_size = 0;
}

DECODER_DEF bool decoder_buffer_fill(Decoder_Buffer *buffer, Decoder *decoder, int index) {
  int data_size = 0;
  int out_samples;

  if(buffer->extra_size > 0) {
    memcpy(buffer->buffers + index * buffer->buffer_size,
	   buffer->buffers + buffer->n * buffer->buffer_size,
	   buffer->extra_size);
    buffer->extra_size = 0;
    data_size += buffer->extra_size;
  }

  while(data_size < buffer->buffer_size && decoder_decode(decoder, &out_samples)) {
    int out_samples_size = out_samples * decoder->sample_size;
    
    if(data_size + out_samples_size > buffer->buffer_size) {

      //copy portion that fits
      int expected_sample_size = buffer->buffer_size - data_size;
      assert(expected_sample_size % decoder->sample_size == 0);
      memcpy(buffer->buffers + index * buffer->buffer_size +
	     data_size,
	     decoder->buffer,
	     expected_sample_size);
      data_size += expected_sample_size;

      //move portion that overflows to last/special buffer
      int unexpected_sample_size = out_samples_size - expected_sample_size;
      assert(buffer->extra_size == 0);
      assert(unexpected_sample_size <= buffer->buffer_size);
      memcpy(buffer->buffers + buffer->n * buffer->buffer_size,
	     decoder->buffer + expected_sample_size,
	     unexpected_sample_size);
      buffer->extra_size = unexpected_sample_size;
      
    } else {          
      memcpy(buffer->buffers +
	     index * buffer->buffer_size +
	     data_size,
	     decoder->buffer, out_samples_size);
      data_size += out_samples_size;
    }

  }

  if(data_size != buffer->buffer_size) {
    //buffer->last_size = data_size;
  }

  return true;
}

DECODER_DEF bool decoder_buffer_next(Decoder_Buffer *buffer, char **data, int *data_size) {

  *data = buffer->buffers + (buffer->play_step++ % buffer->n) * buffer->buffer_size;
  *data_size = buffer->last_size + buffer->buffer_size * (1 - buffer->last_size);

  int last_size = buffer->last_size;
    
  if(buffer->last_size > 0) {
    buffer->last_size = -1;
  }

  return last_size >= 0;
}

DECODER_DEF void decoder_buffer_free(Decoder_Buffer *buffer) {
  free(buffer->buffers);
  
}

DECODER_DEF bool decoder_init_fail(const char *function) {
  //fprintf(stderr, "ERROR: Can not initialize decoder: '%s' failed\n", function);
  return false;
}

//TODO: relase allocated resources, if failure midway
//Right now failing to init, may leak memory
DECODER_DEF bool decoder_init(Decoder *decoder, const char *file_path,
			      Decoder_Fmt fmt,
			      int channels,
			      int sample_rate,
			      float volume,
			      int samples) {
  *decoder = (Decoder) {0};

  decoder->continue_receive = false;
  decoder->continue_convert = false;

  decoder->samples = samples;
  decoder->sample_size = channels * decoder_fmt_to_bits_per_sample(fmt) / 8;

  enum AVSampleFormat av_sample_format = decoder_fmt_to_libav_fmt(fmt);
  
  if(avformat_open_input(&decoder->av_format_context, file_path, NULL, NULL) < 0) {
    return decoder_init_fail("acformat_open_input");
  }

  if(avformat_find_stream_info(decoder->av_format_context, NULL) < 0) {
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("avformat_find_stream_info");
  }

  decoder->stream_index = -1;

  const AVCodec *av_codec = NULL;
  AVCodecParameters *av_codec_parameters = NULL;
  for(size_t i=0;i<decoder->av_format_context->nb_streams;i++) {
    av_codec_parameters = decoder->av_format_context->streams[i]->codecpar;
    //const char *av_codec_cstr = av_get_media_type_string(av_codec_parameters->codec_type);
    //fprintf(stderr, "INFO: av_codec_parameters->codec_type: %s\n", av_codec_cstr ? av_codec_cstr : "not found");
    
    if(av_codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) { //av_codec_parameters->codec_type // AVMEDIA_TYPE_AUDIO
      decoder->stream_index = (int) i;
      if(!(av_codec = avcodec_find_decoder(av_codec_parameters->codec_id))) {
	avformat_close_input(&decoder->av_format_context);
	decoder->av_format_context = NULL;
	return decoder_init_fail("cannot find codec");
      }
      break;
    }
  }
  if(av_codec == NULL) {
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("can not find av_codec");
  }

  if(!(decoder->av_codec_context = avcodec_alloc_context3(av_codec))) {
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("avcodec_alloc_context3");
  }

  if(avcodec_parameters_to_context(decoder->av_codec_context, av_codec_parameters) < 0) {
    avcodec_close(decoder->av_codec_context);
    avcodec_free_context(&decoder->av_codec_context);
    decoder->av_codec_context = NULL;
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("avcodec_parameters_to_context");
  }

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif //__GNUC__
  if(decoder->av_codec_context->channel_layout) {
    decoder->av_codec_context->channel_layout = AV_CH_LAYOUT_STEREO;
  }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__  

  if(avcodec_open2(decoder->av_codec_context, av_codec, NULL) < 0) {
    avcodec_close(decoder->av_codec_context);
    avcodec_free_context(&decoder->av_codec_context);
    decoder->av_codec_context = NULL;
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("avcodec_open2");
  }

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif //__GNUC__

  int channel_layout;
  if(channels == 1) {
    channel_layout = AV_CH_LAYOUT_MONO;
  } else if(channels == 2) {
    channel_layout = AV_CH_LAYOUT_STEREO;
  } else {
    fprintf(stderr, "ERROR: Only support MONO and STEREO. Channels given: %d\n", channels);
    return false;
  }

  if(!(decoder->swr_context = swr_alloc_set_opts(NULL, channel_layout,
						 av_sample_format,
						 sample_rate,
						 decoder->av_codec_context->channel_layout,
						 decoder->av_codec_context->sample_fmt,
						 decoder->av_codec_context->sample_rate,
						 0, NULL))) {
    avcodec_close(decoder->av_codec_context);
    avcodec_free_context(&decoder->av_codec_context);
    decoder->av_codec_context = NULL;
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("swr_alloc_set_opts");
  }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__  
  
  av_opt_set_double(decoder->swr_context, "rmvol", volume, 0);
  
  if(swr_init(decoder->swr_context) < 0) {
    swr_free(&decoder->swr_context);
    decoder->swr_context = NULL;
    avcodec_close(decoder->av_codec_context);
    avcodec_free_context(&decoder->av_codec_context);
    decoder->av_codec_context = NULL;
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("swr_init");
  }
  //swr_set_quality(decoder->swr_context, 7);
  //swr_set_resample_mode(decoder->swr_context, SWR_FILTER_TYPE_CUBIC);
  
  //Should need a buffer size of 4096. Although max_buffer_size returns 8192.
  //Wouldn't it be better to allocate just 4096, since I am only requesting 1024
  //at a time.
  //
  //  samples = 1024
  //  sample_size = 4
  /*
  int max_buffer_size = av_samples_get_buffer_size(NULL, decoder->av_codec_context->channels,
						   samples,
						   decoder->av_codec_context->sample_fmt, 1);
  */
  
  int max_buffer_size = decoder->samples * decoder->sample_size;
  
  decoder->buffer = (unsigned char*) malloc(max_buffer_size);
  if(!(decoder->buffer)) {
    swr_free(&decoder->swr_context);
    decoder->swr_context = NULL;
    avcodec_close(decoder->av_codec_context);
    avcodec_free_context(&decoder->av_codec_context);
    decoder->av_codec_context = NULL;
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;    
    return decoder_init_fail("Can not allocate enough memory");
  }

  if(!(decoder->packet = av_packet_alloc())) {
    swr_free(&decoder->swr_context);
    decoder->swr_context = NULL;
    avcodec_close(decoder->av_codec_context);
    avcodec_free_context(&decoder->av_codec_context);
    decoder->av_codec_context = NULL;
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("av_packet_alloc");
  }
  if(!(decoder->frame = av_frame_alloc())) {
    av_packet_free(&decoder->packet);
    decoder->packet = NULL;
    swr_free(&decoder->swr_context);
    decoder->swr_context = NULL;
    avcodec_close(decoder->av_codec_context);
    avcodec_free_context(&decoder->av_codec_context);
    decoder->av_codec_context = NULL;
    avformat_close_input(&decoder->av_format_context);
    decoder->av_format_context = NULL;
    return decoder_init_fail("av_frame_alloc");
  }
  
  return true;
}

//TODO: check stream_index
DECODER_DEF bool decoder_decode(Decoder *decoder, int *out_samples) {
  if(!decoder->continue_convert) {
    
    if(!decoder->continue_receive) {
      if(av_read_frame(decoder->av_format_context, decoder->packet) < 0) {
	decoder->continue_receive = false;
	decoder->continue_convert = false;
	return false;
      }
      if(decoder->packet->stream_index != decoder->stream_index) {
	decoder->continue_receive = false;
	decoder->continue_convert = false;

	av_packet_unref(decoder->packet);
	return true;
      }
    
      decoder->continue_receive = true;

      if(avcodec_send_packet(decoder->av_codec_context, decoder->packet) < 0) {
	fprintf(stderr, "ERROR: fatal error in avcodec_send_packet\n");
	exit(1);
      }
    }  

    if(avcodec_receive_frame(decoder->av_codec_context, decoder->frame) >= 0) {

      *out_samples = swr_convert(decoder->swr_context, &decoder->buffer, decoder->samples,
				 (const unsigned char **) (decoder->frame->data),
				 decoder->frame->nb_samples);

      decoder->pts = decoder->frame->pts;
      
      if(*out_samples > 0) {
	decoder->continue_convert = true;
      } else {
	decoder->continue_convert = false;

	av_frame_unref(decoder->frame);
      }
    } else {
      *out_samples = 0;
      
      decoder->continue_convert = false;
      decoder->continue_receive = false;

      av_packet_unref(decoder->packet);
    }

    return true;
  }
      
  *out_samples = swr_convert(decoder->swr_context, &decoder->buffer, decoder->samples, NULL, 0);

  if(*out_samples > 0) {
    decoder->continue_convert = true;
  } else {
    decoder->continue_convert = false;    
    av_packet_unref(decoder->packet);
  }

  return true;
}


//TODO: reduce this "distance > buffer->n" condition
DECODER_DEF void *decoder_start_decoding_function(void *arg) {
  
  Decoder_Buffer *buffer = (Decoder_Buffer *) arg;
  Decoder *decoder = buffer->decoder;
  
  while(buffer->last_size == 0) {

    while(buffer->play_step + 2 >= buffer->fill_step &&
	  (buffer->fill_step % buffer->n) != (buffer->play_step & buffer->n)) {
      decoder_buffer_fill(buffer, decoder, (buffer->fill_step++) % buffer->n);	
    }

    thread_sleep(10);
  }

  return NULL;
}

DECODER_DEF bool decoder_start_decoding(Decoder *decoder, Decoder_Buffer *buffer, Thread *id) {
  for(int i=0;i<buffer->n;i++) {
    decoder_buffer_fill(buffer, decoder, buffer->fill_step++);
  }

  buffer->decoder = decoder;
  if(!thread_create(id, decoder_start_decoding_function, buffer)) {
    return false;
  }
  
  return true;
}

DECODER_DEF void decoder_free(Decoder *decoder) {
  
  decoder->continue_receive = false;
  decoder->continue_convert = false;
  
  av_frame_free(&decoder->frame);
  decoder->frame = NULL;

  av_packet_free(&decoder->packet);
  decoder->packet = NULL;

  free(decoder->buffer);
  decoder->buffer = NULL;
  
  swr_free(&decoder->swr_context);
  decoder->swr_context = NULL;

  avcodec_close(decoder->av_codec_context);
  avcodec_free_context(&decoder->av_codec_context);
  decoder->av_codec_context = NULL;

  avformat_close_input(&decoder->av_format_context);
  decoder->av_format_context = NULL;
}

#ifdef _WIN32
DECODER_DEF void decoder_get_waveformat(WAVEFORMATEX *waveFormat,
					Decoder_Fmt fmt,
					int channels,
					int sample_rate) {
  switch(fmt) {
  case DECODER_FMT_S32:
  case DECODER_FMT_S16: {
    waveFormat->wFormatTag = WAVE_FORMAT_PCM;
  } break;
  case DECODER_FMT_FLT: {
    waveFormat->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
  } break;
  default: {
    fprintf(stderr, "ERROR: decoder_get_waveformat: unimplemented Fmt: '%s'\n",
	    decoder_fmt_to_cstr(fmt));
    exit(1);
  } break;
  }

  waveFormat->nChannels = channels;
  waveFormat->nSamplesPerSec = sample_rate;
  waveFormat->wBitsPerSample = decoder_fmt_to_bits_per_sample(fmt);
  waveFormat->nBlockAlign = (waveFormat->nChannels * waveFormat->wBitsPerSample) / 8;
  waveFormat->nAvgBytesPerSec = waveFormat->nSamplesPerSec * waveFormat->nBlockAlign;
  waveFormat->cbSize = 0; 
}
#endif //_WIN32

#endif //DECODER_IMPLEMENTATION

#endif //DECODER_H_H_
