#ifndef DEMUXER_H
#define DEMUXER_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define SAMPLES 1024

#ifdef DEMUXER_IMPLEMENTATION

typedef struct{
  bool skip_extract_frame;
  
  AVIOContext *av_io_context;
  AVFormatContext *av_format_context;
  AVPacket *packet;
  AVFrame *frame;
  Io_File file;
  int64_t duration;

  //AUDIO
  int audio_stream;
  unsigned char *audio_buffer;
  AVCodecContext *audio_av_codec_context;
  SwrContext *audio_swr_context;
  int audio_samples;
  int audio_sample_rate;
  AVRational audio_time_base;

  //VIDEO
  int video_stream;
  AVCodecContext *video_av_codec_context;
  struct SwsContext* video_sws_context;
  AVRational video_time_base;

  unsigned char *video_rgb_buffer;
  int video_rgb_buffer_stride;
  int video_rgb_buffer_height;
}Demuxer;

typedef void (*Demuxer_On_Decode)(Demuxer *demuxer, int64_t pts, int out_samples, void *arg);

void demuxer_init(Demuxer *demuxer, const char *filepath);
void demuxer_seek(Demuxer *demuxer, float p);
bool demuxer_decode(Demuxer *demuxer, Demuxer_On_Decode on_decode, void *arg);

typedef struct{
  const char *memory;
  long long unsigned int size;
  long long unsigned int pos;
}Player_Memory;

int file_read(void *opaque, uint8_t *buf, int buf_size);
int64_t file_seek(void *opaque, int64_t offset, int whence);
int memory_read(void *opaque, uint8_t *buf, int _buf_size);
int64_t memory_seek(void *opaque, int64_t offset, int whence);

#endif //DEMUXER_IMPLEMENTATION

//////////////////////////////////////////////////////////////////////////////////

void panic(const char *mess) {
  fprintf(stderr, "ERROR: %s\n", mess); fflush(stderr);
  exit(1);
}

void demuxer_init(Demuxer *demuxer, const char *filepath) {
  demuxer->skip_extract_frame = false;

  if(!io_file_open(&demuxer->file, filepath)) {
    panic("file_open");
  }
  
  demuxer->av_io_context =
  avio_alloc_context(NULL, 0, 0, &demuxer->file, file_read, NULL, file_seek);
  if(!demuxer->av_io_context) {
    panic("avio_alloc_context");
  }

  demuxer->av_format_context = avformat_alloc_context();
  if(!demuxer->av_format_context) {
    panic("avformat_alloc_context");
  }

  demuxer->av_format_context->pb = demuxer->av_io_context;
  demuxer->av_format_context->flags = AVFMT_FLAG_CUSTOM_IO;
  if (avformat_open_input(&demuxer->av_format_context, "", NULL, NULL) != 0) {
    panic("avformat_open_input");
  }

  if(avformat_find_stream_info(demuxer->av_format_context, NULL) < 0) {
    panic("avformat_find_stream_info");
  }
  av_dump_format(demuxer->av_format_context, 0, "", 0);

  demuxer->audio_stream = -1; // for now unused
  demuxer->video_stream = -1;

  AVCodecParameters *av_codec_parameters = NULL;
  for(size_t i=0;i<demuxer->av_format_context->nb_streams;i++) {
    av_codec_parameters = demuxer->av_format_context->streams[i]->codecpar;
    if(av_codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
      demuxer->audio_stream = (int) i;
    } else if(av_codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      demuxer->video_stream = (int) i;
    }
  }

  if(demuxer->video_stream == -1) {
    fprintf(stderr, "ERROR: Can not find video stream in file: %s\n", filepath);
    panic("");
  }

  ///////////////////////VIDEO

  av_codec_parameters = demuxer->av_format_context->streams[demuxer->video_stream]->codecpar;
  const AVCodec *video_av_codec = avcodec_find_decoder(av_codec_parameters->codec_id);
  if(!video_av_codec) {
    panic("video_av_codec");
  }
  int video_width = av_codec_parameters->width;
  int video_height = av_codec_parameters->height;
  printf("Video size: (%d, %d)\n", video_width, video_height);
  AVStream* videoStream = demuxer->av_format_context->streams[demuxer->video_stream];
  demuxer->video_time_base = videoStream->time_base;
  
  demuxer->video_av_codec_context = avcodec_alloc_context3(video_av_codec);
  if(!demuxer->video_av_codec_context) {
    panic("avcodec_alloc_context3");
  }

  if(avcodec_parameters_to_context(demuxer->video_av_codec_context, av_codec_parameters) < 0) {
    panic("avcodec_parameters_to_context");    
  }

  if(avcodec_open2(demuxer->video_av_codec_context, video_av_codec, NULL) < 0) {
    panic("avcodec_open2");
  }

  ///////////////////////AUDIO

  av_codec_parameters = demuxer->av_format_context->streams[demuxer->audio_stream]->codecpar;
  const AVCodec *audio_av_codec = avcodec_find_decoder(av_codec_parameters->codec_id);
  if(!video_av_codec) {
    panic("video_av_codec");
  }
  AVStream* audioStream = demuxer->av_format_context->streams[demuxer->audio_stream];
  demuxer->audio_time_base = audioStream->time_base;

  demuxer->audio_av_codec_context = avcodec_alloc_context3(audio_av_codec);
  if(!demuxer->audio_av_codec_context) {
    panic("avcodec_alloc_context3");
  }

  if(avcodec_parameters_to_context(demuxer->audio_av_codec_context, av_codec_parameters) < 0) {
    panic("avcodec_parameters_to_context");    
  }

  if(avcodec_open2(demuxer->audio_av_codec_context, audio_av_codec, NULL) < 0) {
    panic("avcodec_open2");
  }

  ///////////// PREPARE
    
  demuxer->packet = av_packet_alloc();
  if(!demuxer->packet) {
    panic("av_packet_alloc");
  }
  
  demuxer->frame = av_frame_alloc();
  if(!demuxer->frame) {
    panic("av_frame_alloc");
  }

  /////////////// VIDEO PREPARE

  av_codec_parameters = demuxer->av_format_context->streams[demuxer->video_stream]->codecpar;
  int input_pixel_format = av_codec_parameters->format;
  int rgb_pixel_format = AV_PIX_FMT_RGB24;
  demuxer->video_sws_context =
    sws_getContext(video_width, video_height, input_pixel_format,
		   video_width, video_height, rgb_pixel_format,
		   SWS_FAST_BILINEAR, NULL, NULL, NULL);
  if(!demuxer->video_sws_context) {
    panic("sws_getContext");
  }

  demuxer->video_rgb_buffer = malloc(video_width * video_height * 3);
  if(!demuxer->video_rgb_buffer) {
    panic("malloc");
  }
  demuxer->video_rgb_buffer_height = video_height;
  demuxer->video_rgb_buffer_stride = video_width * 3;

  demuxer->duration = demuxer->av_format_context->duration / AV_TIME_BASE;

  /////////////// AUDIO PREPARE
  
  av_codec_parameters = demuxer->av_format_context->streams[demuxer->audio_stream]->codecpar;
  enum AVSampleFormat av_sample_format = AV_SAMPLE_FMT_S16;
  int s16_bits_per_sample = 16;
  int channels = 2;
  int samples = SAMPLES;
  int sample_size = channels * s16_bits_per_sample / 8;
  demuxer->audio_sample_rate = demuxer->audio_av_codec_context->sample_rate;

  demuxer->audio_samples = samples;
  
  demuxer->audio_swr_context = swr_alloc();
  if(!demuxer->audio_swr_context) {
    panic("swr_alloc");
  }

  const char *ch_layout = "mono";
  if(channels == 1) {
      ch_layout = "mono";
  } else if(channels == 2) {
      ch_layout = "stereo";
  } else {
    panic("unsupported channel layout");
  }

  /*
  char chLayoutDescription[128];
  int sts = av_channel_layout_describe(&av_codec_parameters->ch_layout, chLayoutDescription, sizeof(chLayoutDescription));
  if(sts < 0) {
    panic("av_channel_layout_describe");
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
    panic("swr_init");
  }

  int max_buffer_size = samples * sample_size;
  demuxer->audio_buffer = (unsigned char*) malloc(max_buffer_size);
  if(!demuxer->audio_buffer) {
    panic("malloc");
  }
  
}

void demuxer_seek(Demuxer *demuxer, float p) {
  int64_t seek_to = (int64_t) (p * (float) demuxer->duration);
  if(seek_to > 0) seek_to--;

  av_frame_unref(demuxer->frame);
  av_packet_unref(demuxer->packet);
  
  av_seek_frame(demuxer->av_format_context, demuxer->video_stream, (int64_t) demuxer->video_time_base.den * seek_to , 0);
  av_seek_frame(demuxer->av_format_context, demuxer->audio_stream, (int64_t) demuxer->audio_time_base.den * seek_to, 0);

  for(int i=0;i<10;i++) {
    while(av_read_frame(demuxer->av_format_context, demuxer->packet) < 0) ;
    if(demuxer->packet->stream_index == demuxer->video_stream) {
      demuxer->skip_extract_frame = true;
      break;
    }
  }
  
}

bool demuxer_decode(Demuxer *demuxer, Demuxer_On_Decode on_decode, void *arg) {

  if(demuxer->skip_extract_frame) {
    demuxer->skip_extract_frame = false;
  } else {
    if(av_read_frame(demuxer->av_format_context, demuxer->packet) < 0) {
      return false;
    }    
  }
  
  if(demuxer->packet->stream_index == demuxer->video_stream) {
    //printf("read video frame\n");

    if(avcodec_send_packet(demuxer->video_av_codec_context, demuxer->packet)) {
      panic("avcodec_send_packet");
    }

    int response;
    while( (response = avcodec_receive_frame(demuxer->video_av_codec_context, demuxer->frame)) >= 0) {
      if(response < 0) {
	panic("avcodec_receive_frame");
      } else if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
	break;
      }

      //LOG
      /*
	printf("Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame %d [DTS %d], frame_number %d\n",
	demuxer->video_av_codec_context->frame_number,
	av_get_picture_type_char(demuxer->frame->pict_type),
	demuxer->frame->pkt_size,
	demuxer->frame->format,
	(int) demuxer->frame->pts,
	demuxer->frame->key_frame,
	demuxer->frame->coded_picture_number,
	demuxer->video_av_codec_context->frame_number);
      */
      //EXTRACT 
      sws_scale(demuxer->video_sws_context, (const uint8_t * const*) demuxer->frame->data, demuxer->frame->linesize, 0, demuxer->video_rgb_buffer_height,
		    &demuxer->video_rgb_buffer, &demuxer->video_rgb_buffer_stride);

      //DO something..
      on_decode(demuxer, demuxer->frame->pts, -1, arg);
	
      av_frame_unref(demuxer->frame);
    }
  } else if(demuxer->packet->stream_index == demuxer->audio_stream) { //for now continue
    //printf("read audio frame\n");

    if(avcodec_send_packet(demuxer->audio_av_codec_context, demuxer->packet)) {
      panic("avcodec_send_packet");
    }

    int response;
    while( (response = avcodec_receive_frame(demuxer->audio_av_codec_context, demuxer->frame)) >= 0) {
      if(response < 0) {
	panic("avcodec_receive_frame");
      } else if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
	break;
      }

      //TODO: maybe in the future you have to supply out_samples?
      int out_samples = swr_convert(demuxer->audio_swr_context, &demuxer->audio_buffer, demuxer->audio_samples,
				    (const unsigned char **) (demuxer->frame->data),
				    demuxer->frame->nb_samples);
      while(out_samples > 0) {
	on_decode(demuxer, demuxer->frame->pts, out_samples, arg);
	out_samples = swr_convert(demuxer->audio_swr_context, &demuxer->audio_buffer, demuxer->audio_samples, NULL, 0);
      }

      av_frame_unref(demuxer->frame);
    }
  }

  av_packet_unref(demuxer->packet);
  return true;
}

void demuxer_decode2(Demuxer *demuxer, Demuxer_On_Decode on_decode, void *arg) {

  while(av_read_frame(demuxer->av_format_context, demuxer->packet) >= 0) {
    if(demuxer->packet->stream_index == demuxer->video_stream) {
      //printf("read video frame\n");

      if(avcodec_send_packet(demuxer->video_av_codec_context, demuxer->packet)) {
	panic("avcodec_send_packet");
      }

      int response;
      while( (response = avcodec_receive_frame(demuxer->video_av_codec_context, demuxer->frame)) >= 0) {
	if(response < 0) {
	  panic("avcodec_receive_frame");
	} else if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
	  break;
	}

	//LOG
	/*
	printf("Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame %d [DTS %d], frame_number %d\n",
	       demuxer->video_av_codec_context->frame_number,
	       av_get_picture_type_char(demuxer->frame->pict_type),
	       demuxer->frame->pkt_size,
	       demuxer->frame->format,
	       (int) demuxer->frame->pts,
	       demuxer->frame->key_frame,
	       demuxer->frame->coded_picture_number,
	       demuxer->video_av_codec_context->frame_number);
	*/
	//EXTRACT 
	sws_scale(demuxer->video_sws_context, (const uint8_t * const*) demuxer->frame->data, demuxer->frame->linesize, 0, demuxer->video_rgb_buffer_height,
		  &demuxer->video_rgb_buffer, &demuxer->video_rgb_buffer_stride);

	//DO something..
	on_decode(demuxer, demuxer->frame->pts, -1, arg);
	
	av_frame_unref(demuxer->frame);
      }
    } else if(demuxer->packet->stream_index == demuxer->audio_stream) { //for now continue
      //printf("read audio frame\n");

      if(avcodec_send_packet(demuxer->audio_av_codec_context, demuxer->packet)) {
	panic("avcodec_send_packet");
      }

      int response;
      while( (response = avcodec_receive_frame(demuxer->audio_av_codec_context, demuxer->frame)) >= 0) {
	if(response < 0) {
	  panic("avcodec_receive_frame");
	} else if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
	  break;
	}

	//TODO: maybe in the future you have to supply out_samples?
	int out_samples = swr_convert(demuxer->audio_swr_context, &demuxer->audio_buffer, demuxer->audio_samples,
				      (const unsigned char **) (demuxer->frame->data),
				      demuxer->frame->nb_samples);
	while(out_samples > 0) {
	  on_decode(demuxer, demuxer->frame->pts, out_samples, arg);
	  out_samples = swr_convert(demuxer->audio_swr_context, &demuxer->audio_buffer, demuxer->audio_samples, NULL, 0);
	}

	av_frame_unref(demuxer->frame);
      }
    }
    
    av_packet_unref(demuxer->packet);

  }   
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

#endif //DEMUXER_H
