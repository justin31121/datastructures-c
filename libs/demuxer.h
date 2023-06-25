#ifndef DEMUXER_H
#define DEMUXER_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#ifdef DEMUXER_IMPLEMENTATION
#  define IO_IMPLEMENTATION
#  define PLAYER_IMPLEMENTATION
#endif //DEMUXER_IMPLEMENTATION

#include "io.h"
#include "player.h"
#include <assert.h>

#ifndef DEMUXER_DEF
#define DEMUXER_DEF static inline
#endif //DEMUXER_DEF

#define SAMPLES 1024

typedef struct{
  Io_File file;
  Player_Socket socket;
    
  AVIOContext *av_io_context;
  AVFormatContext *av_format_context;
  AVPacket *packet;
  AVFrame *frame;

		   //VIDEO
		   int video_index;
  AVCodecContext *video_av_codec_context;
  AVRational video_time_base;
  int video_stride;
  int video_height;
  int video_width;

  struct SwsContext* video_sws_context;

  int64_t *video_ptss;
  unsigned char *video_buffer;
    
  int video_buffer_size;
  int video_buffer_cap;
  int64_t video_buffer_count;

  //AUDIO
  int audio_index;
  AVCodecContext *audio_av_codec_context;
  AVRational audio_time_base;
  int audio_sample_rate;    

  SwrContext *audio_swr_context;

  int64_t *audio_ptss;
  unsigned char *audio_buffer;

  int audio_buffer_size;
  int audio_buffer_cap;
  int64_t audio_buffer_count;
    
  //DECODING
  int receiving;
    
}Demuxer;

DEMUXER_DEF bool demuxer_init(Demuxer *demuxer, const char *filepath);
DEMUXER_DEF bool demuxer_decode(Demuxer *demuxer, bool *video);
DEMUXER_DEF void demuxer_free(Demuxer *demuxer);
DEMUXER_DEF int demuxer_file_read(void *opaque, uint8_t *buf, int buf_size);
DEMUXER_DEF int64_t demuxer_file_seek(void *opaque, int64_t offset, int whence);

#ifdef DEMUXER_IMPLEMENTATION

DEMUXER_DEF bool demuxer_av_codec_init(AVCodecContext **av_codec_context, AVCodecParameters *av_codec_parameters) {
  const AVCodec *av_codec = avcodec_find_decoder(av_codec_parameters->codec_id);
  if(!av_codec) {
    return false;
  }
    
  *av_codec_context = avcodec_alloc_context3(av_codec);
  if(!(*av_codec_context)) {
    return false;
  }

  if(avcodec_parameters_to_context(*av_codec_context, av_codec_parameters) < 0) {
    return false;
  }

  if(avcodec_open2(*av_codec_context, av_codec, NULL) < 0) {
    return false;
  }
    
  return true;
}

DEMUXER_DEF bool demuxer_init(Demuxer *demuxer, const char *filepath) {

  if(io_file_open(&demuxer->file, filepath)) {
    demuxer->av_io_context = avio_alloc_context(NULL, 0, 0, &demuxer->file, demuxer_file_read, NULL, demuxer_file_seek);        
  } else if(player_socket_init(&demuxer->socket, filepath, 0, 0)) {
    demuxer->av_io_context = avio_alloc_context(NULL, 0, 0, &demuxer->socket, player_decoder_url_read, NULL, player_decoder_url_seek);
  } else {
    return false;
  }
  if(!demuxer->av_io_context) {
    return false;
  }

  demuxer->av_format_context = avformat_alloc_context();
  if(!demuxer->av_format_context) {
    return false;
  }

  demuxer->av_format_context->pb = demuxer->av_io_context;
  demuxer->av_format_context->flags = AVFMT_FLAG_CUSTOM_IO;
  if (avformat_open_input(&demuxer->av_format_context, "", NULL, NULL) != 0) {
    return false;
  }

  if(avformat_find_stream_info(demuxer->av_format_context, NULL) < 0) {
    return false;
  }
  //av_dump_format(demuxer->av_format_context, 0, "", 0);

  demuxer->video_index = -1;
  demuxer->audio_index = -1;

  AVStream* stream;
  AVCodecParameters *av_codec_parameters = NULL;
  for(size_t i=0;i<demuxer->av_format_context->nb_streams;i++) {
    av_codec_parameters = demuxer->av_format_context->streams[i]->codecpar;
    if(av_codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
      demuxer->audio_index = (int) i;
    } else if(av_codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      demuxer->video_index = (int) i;
    }
  }

  if(demuxer->video_index == -1 || demuxer->audio_index == -1) {
    return false;
  }

  demuxer->packet = av_packet_alloc();
  if(!demuxer->packet) {
    return false;
  }
  
  demuxer->frame = av_frame_alloc();
  if(!demuxer->frame) {
    return false;
  }

  //VIDEO
  stream = demuxer->av_format_context->streams[demuxer->video_index];
  demuxer->video_time_base = stream->time_base;
    
  av_codec_parameters = demuxer->av_format_context->streams[demuxer->video_index]->codecpar;
  if(!demuxer_av_codec_init(&demuxer->video_av_codec_context, av_codec_parameters)) {
    return false;
  }

  {
    int video_width = av_codec_parameters->width;
    int video_height = av_codec_parameters->height;
    printf("Video size: (%d, %d)\n", video_width, video_height);

    int input_pixel_format = av_codec_parameters->format;
    int rgb_pixel_format = AV_PIX_FMT_RGB24;
    demuxer->video_sws_context =
      sws_getContext(video_width, video_height, input_pixel_format,
		     video_width, video_height, rgb_pixel_format,
		     SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if(!demuxer->video_sws_context) {
      return false;
    }
	
    demuxer->video_width = video_width;
    demuxer->video_height = video_height;  
    demuxer->video_stride = video_width * 3;       

    demuxer->video_buffer_cap = 100;
    demuxer->video_buffer_size = video_width * video_height * 3;
    demuxer->video_buffer = malloc(demuxer->video_buffer_size * demuxer->video_buffer_cap);
    if(!demuxer->video_buffer) {
      return false;
    }
    demuxer->video_buffer_count = 0;	

    demuxer->video_ptss = (int64_t *) malloc(	    
					     sizeof(int64_t) * demuxer->video_buffer_cap);
    if(!demuxer->video_ptss) {
      return false;
    }
    demuxer->video_ptss[0] = 0;
  }


  //AUDIO
  stream = demuxer->av_format_context->streams[demuxer->audio_index];
  demuxer->audio_time_base = stream->time_base;
    
  av_codec_parameters = demuxer->av_format_context->streams[demuxer->audio_index]->codecpar;
  if(!demuxer_av_codec_init(&demuxer->audio_av_codec_context, av_codec_parameters)) {
    return false;
  }

  {
    enum AVSampleFormat av_sample_format = AV_SAMPLE_FMT_S16;
    int s16_bits_per_sample = 16;
    int channels = 2;
    int samples = SAMPLES;
    int sample_size = channels * s16_bits_per_sample / 8;
    demuxer->audio_sample_rate = demuxer->audio_av_codec_context->sample_rate;

    demuxer->audio_swr_context = swr_alloc();
    if(!demuxer->audio_swr_context) {
      return false;
    }

    const char *ch_layout = "mono";
    if(channels == 1) {
      ch_layout = "mono";
    } else if(channels == 2) {
      ch_layout = "stereo";
    } else {
      return false;
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
    av_opt_set(demuxer->audio_swr_context, "in_channel_layout", chLayoutDescription, 0); //in_channel_layout
    av_opt_set_int(demuxer->audio_swr_context, "in_sample_fmt", demuxer->audio_av_codec_context->sample_fmt, 0);
    av_opt_set_int(demuxer->audio_swr_context, "in_sample_rate", demuxer->audio_av_codec_context->sample_rate, 0);
    //av_opt_set(demuxer->audio_swr_context, "out_chlayout", ch_layout, 0); //out_channel_layout
    av_opt_set(demuxer->audio_swr_context, "out_channel_layout", ch_layout, 0); //out_channel_layout
    av_opt_set_int(demuxer->audio_swr_context, "out_sample_fmt", av_sample_format, 0);
    av_opt_set_int(demuxer->audio_swr_context, "out_sample_rate", demuxer->audio_av_codec_context->sample_rate, 0);

    if(swr_init(demuxer->audio_swr_context) < 0) {
      return false;
    }

    demuxer->audio_buffer_cap = 200;
    demuxer->audio_buffer_size = samples * sample_size;
    demuxer->audio_buffer = (unsigned char*) malloc(
						    demuxer->audio_buffer_size * demuxer->audio_buffer_cap);
    if(!demuxer->audio_buffer) {
      return false;
    }
    demuxer->audio_buffer_count = 0;

    demuxer->audio_ptss = (int64_t *) malloc(	    
					     sizeof(int64_t) * demuxer->audio_buffer_cap);
    if(!demuxer->audio_ptss) {
      return false;
    }
    demuxer->audio_ptss[0] = 0;
  }

  demuxer->receiving = -1;

  return true;
}

DEMUXER_DEF bool demuxer_decode(Demuxer *demuxer, bool *video) {

  if(demuxer->receiving == -1) {
    if(av_read_frame(demuxer->av_format_context, demuxer->packet) < 0) {
      return false;
    }

    if(demuxer->packet->stream_index == demuxer->video_index) {
      if(avcodec_send_packet(demuxer->video_av_codec_context, demuxer->packet) < 0) {
	fprintf(stderr, "ERROR: fatal error in video_avcodec_send_packet\n");
	exit(1);
      }
		
    } else if(demuxer->packet->stream_index == demuxer->audio_index) {
      if(avcodec_send_packet(demuxer->audio_av_codec_context, demuxer->packet) < 0) {
	fprintf(stderr, "ERROR: fatal error in avcodec_send_packet\n");
	exit(1);
      }
    } else {
      fprintf(stderr, "ERROR: Unexpected stream\n");
      exit(1);	    
    }

    demuxer->receiving = demuxer->packet->stream_index;
  }
  if(demuxer->receiving == demuxer->video_index) {
    if(avcodec_receive_frame(demuxer->video_av_codec_context, demuxer->frame) >= 0) {

      if(*video) {
	demuxer->video_ptss[demuxer->video_buffer_count % demuxer->video_buffer_cap] = demuxer->frame->pts;
	unsigned char *buffer = demuxer->video_buffer + (demuxer->video_buffer_count % demuxer->video_buffer_cap) * demuxer->video_buffer_size;
	sws_scale(demuxer->video_sws_context, (const uint8_t * const*) demuxer->frame->data, demuxer->frame->linesize, 0, demuxer->video_height, &buffer, &demuxer->video_stride);

	demuxer->video_buffer_count = demuxer->video_buffer_count + 1;
      }

      *video = true;
	    
      av_frame_unref(demuxer->frame);
    } else {
      demuxer->receiving = -1;
      av_packet_unref(demuxer->packet);
    }
  } else {
    if(avcodec_receive_frame(demuxer->audio_av_codec_context, demuxer->frame) >= 0) {

      if(!(*video)) {
	demuxer->audio_ptss[demuxer->audio_buffer_count % demuxer->audio_buffer_cap] = demuxer->frame->pts;
	unsigned char *buffer = demuxer->audio_buffer + (demuxer->audio_buffer_count % demuxer->audio_buffer_cap) * demuxer->audio_buffer_size;
	int out_samples = swr_convert(demuxer->audio_swr_context, &buffer, SAMPLES, (const unsigned char **) (demuxer->frame->data), demuxer->frame->nb_samples);
	assert(SAMPLES == out_samples);

	demuxer->audio_buffer_count = demuxer->audio_buffer_count + 1;
      }
      *video = false;
	    
      av_frame_unref(demuxer->frame);
    } else {
      demuxer->receiving = -1;
      av_packet_unref(demuxer->packet);
    }
  }

  return true;
}

//TODO
DEMUXER_DEF void demuxer_free(Demuxer *demuxer) {
  (void) demuxer;
}

DEMUXER_DEF int demuxer_file_read(void *opaque, uint8_t *buf, int buf_size) {
  Io_File *f = (Io_File *)opaque;

  size_t bytes_read = io_file_fread(buf, 1, buf_size, f);

  if (bytes_read == 0) {
    if(io_file_feof(f)) return AVERROR_EOF;
    else return AVERROR(errno);
  }
  
  return (int) bytes_read;
}

DEMUXER_DEF int64_t demuxer_file_seek(void *opaque, int64_t offset, int whence) {
  
  Io_File *file = (Io_File *)opaque;
  
  if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
    return AVERROR_INVALIDDATA;
  }

  if(io_file_fseek(file, (long) offset, whence)) {
    return AVERROR(errno);
  }

  return io_file_ftell(file);
}

#endif //DEMUXER_IMPLEMENTATION

#endif //DEMUXER_H
