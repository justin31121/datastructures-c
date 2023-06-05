#ifndef LIBAV_H_H
#define LIBAV_H_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>

#include <stdbool.h>

#ifndef LIBAV_DEF
#define LIBAV_DEF static inline
#endif //LIBAV_DEF

typedef struct {
    char copy_video;
    char copy_audio;
    char *output_extension;
    char *muxer_opt_key;
    char *muxer_opt_value;
    char *video_codec;
    char *audio_codec;
    char *codec_priv_key;
    char *codec_priv_value;
}Libav_StreamingParams;

typedef struct {
    AVFormatContext *avfc;
    const AVCodec *video_avc;
    const AVCodec *audio_avc;
    AVStream *video_avs;
    AVStream *audio_avs;
    AVCodecContext *video_avcc;
    AVCodecContext *audio_avcc;
    int video_index;
    int audio_index;
    const char *filename;
}Libav_StreamingContext;

LIBAV_DEF int libav_open_media(const char *filename, AVFormatContext **avfc);
LIBAV_DEF int libav_prepare_decoder(Libav_StreamingContext *sc);
LIBAV_DEF int libav_fill_stream_info(AVStream *avs, const AVCodec **avc, AVCodecContext **avcc);
LIBAV_DEF int libav_prepare_copy(AVFormatContext *avfc, AVStream **avs, AVCodecParameters *decoder_par);
LIBAV_DEF int libav_remux(AVPacket **pkt, AVFormatContext **avfc, AVRational decoder_tb, AVRational encoder_tb);

LIBAV_DEF bool libav_merge(const char *video_filename, const char *audio_filename, const char *output_filename);

#ifdef LIBAV_IMPLEMENTATION

#ifdef _MSC_VER
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")
#endif //_WIN32

LIBAV_DEF int libav_remux(AVPacket **pkt, AVFormatContext **avfc, AVRational decoder_tb, AVRational encoder_tb) {
    av_packet_rescale_ts(*pkt, decoder_tb, encoder_tb);
    /*
      (*pkt)->pts = av_rescale_q_rnd((*pkt)->pts, decoder_tb, encoder_tb, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
      (*pkt)->dts = av_rescale_q_rnd((*pkt)->dts, decoder_tb, encoder_tb, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
      (*pkt)->duration = av_rescale_q((*pkt)->duration, decoder_tb, encoder_tb);
    */
    if(av_interleaved_write_frame(*avfc, *pkt) < 0) {
	printf("Error while copying stream packet");
	return -1;
    }
    //av_packet_unref(*pkt);
    return 0;
}


LIBAV_DEF int libav_prepare_copy(AVFormatContext *avfc, AVStream **avs, AVCodecParameters *decoder_par) {
    *avs = avformat_new_stream(avfc, NULL);
    avcodec_parameters_copy((*avs)->codecpar, decoder_par);
    return 0;  
}


LIBAV_DEF int libav_fill_stream_info(AVStream *avs, const AVCodec **avc, AVCodecContext **avcc) {
    *avc = avcodec_find_decoder(avs->codecpar->codec_id);
    if(!*avc) {
	printf("Failed to find the codec\n");
	return -1;
    }

    *avcc = avcodec_alloc_context3(*avc);
    if(!*avcc) {
	printf("Failed to find the codec context\n");
	return -1;		
    }

    if(avcodec_parameters_to_context(*avcc, avs->codecpar) < 0) {
	printf("Failed to fill the codec context\n");
	return -1;
    }

    if(avcodec_open2(*avcc, *avc, NULL) < 0) {
	printf("Failed to open codec\n");
	return -1;
    }

    return 0;
}

LIBAV_DEF int libav_prepare_decoder(Libav_StreamingContext *sc) {
    for(size_t i=0;i<sc->avfc->nb_streams;i++) {
	//AVCodecParameters *pLocalCodecParameters = sc->avfc->streams[i]->codecpar;
	//const AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
        
	//VIDEO
	if(sc->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
	    sc->video_avs = sc->avfc->streams[i];
	    sc->video_index = (int) i;

	    /*
	      printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
	      printf("AVStream->time_base before open coded %d/%d\n", sc->avfc->streams[i]->time_base.num,
	      sc->avfc->streams[i]->time_base.den);
	      printf("AVStream->r_frame_rate before open coded %d/%d\n", sc->avfc->streams[i]->r_frame_rate.num,
	      sc->avfc->streams[i]->r_frame_rate.num);
	      printf("AVStream->start_time %" PRId64 "\n", sc->avfc->streams[i]->start_time);
	      printf("AVStream->duration %" PRId64 "\n", sc->avfc->streams[i]->duration);
	    */

	    if(libav_fill_stream_info(sc->video_avs, &sc->video_avc, &sc->video_avcc)) {
		return -1;
	    }
	}
	//AUDIO
	else if(sc->avfc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
	    sc->audio_avs = sc->avfc->streams[i];
	    sc->audio_index = (int) i;

	    /*
	      printf("Audio Codec: %d channels, sample rate %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
	      printf("AVStream->time_base before open coded %d/%d\n", sc->avfc->streams[i]->time_base.num,
	      sc->avfc->streams[i]->time_base.den);
	      printf("AVStream->r_frame_rate before open coded %d/%d\n", sc->avfc->streams[i]->r_frame_rate.num,
	      sc->avfc->streams[i]->r_frame_rate.num);
	      printf("AVStream->start_time %" PRId64 "\n", sc->avfc->streams[i]->start_time);
	      printf("AVStream->duration %" PRId64 "\n", sc->avfc->streams[i]->duration);
	    */

	    if(libav_fill_stream_info(sc->audio_avs, &sc->audio_avc, &sc->audio_avcc)) {
		return -1;
	    }
	}
	//OTHER
	else {
	    printf("Skipping streams other than audio and video\n");
	}
	//printf("\tCodec %s ID %d bit_rate %lld\n", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
    }

    return 0;
}


LIBAV_DEF int libav_open_media(const char *filename, AVFormatContext **avfc) {
    *avfc = avformat_alloc_context();
    if(!*avfc) {
	printf("Failed to allocate memory for format\n");
	return -1;
    }

    if(avformat_open_input(avfc, filename, NULL, NULL) != 0) {
	printf("Failed to open input file %s\n", filename);
	return -1;
    }

    if(avformat_find_stream_info(*avfc, NULL) < 0) {
	printf("Failed to get stream info\n");
	return -1;
    }

    return 0;
}

LIBAV_DEF bool libav_merge(const char *video_filename, const char *audio_filename, const char *output_filename) {
  
    Libav_StreamingContext *video_decoder = (Libav_StreamingContext *) calloc(1, sizeof(Libav_StreamingContext));
    video_decoder->filename = video_filename;
    Libav_StreamingContext *audio_decoder = (Libav_StreamingContext *) calloc(1, sizeof(Libav_StreamingContext));
    audio_decoder->filename = audio_filename;
  
    Libav_StreamingContext *encoder = (Libav_StreamingContext *) calloc(1, sizeof(Libav_StreamingContext));
    encoder->filename = output_filename;

    if(libav_open_media(video_decoder->filename, &video_decoder->avfc)) return false;
    if(libav_prepare_decoder(video_decoder)) return false;
    if(libav_open_media(audio_decoder->filename, &audio_decoder->avfc)) return false;
    if(libav_prepare_decoder(audio_decoder)) return false;

    //printf("Videoforamt %s, duration %lld us, bit_rate %lld\n", video_decoder->avfc->iformat->name, video_decoder->avfc->duration, video_decoder->avfc->bit_rate);
  
    //printf("Audioforamt %s, duration %lld us, bit_rate %lld\n", audio_decoder->avfc->iformat->name, audio_decoder->avfc->duration, audio_decoder->avfc->bit_rate);

    avformat_alloc_output_context2(&encoder->avfc, NULL, NULL, encoder->filename);
    if(!encoder->avfc) {
	printf("Could not allocate memory for output format\n");
	return false;
    }

    if(libav_prepare_copy(encoder->avfc, &encoder->video_avs, video_decoder->video_avs->codecpar)) {
	return false;
    }

    if(libav_prepare_copy(encoder->avfc, &encoder->audio_avs, audio_decoder->audio_avs->codecpar)) {
	return false;
    }

    if(encoder->avfc->oformat->flags & AVFMT_GLOBALHEADER) {
	encoder->avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if(!(encoder->avfc->oformat->flags & AVFMT_NOFILE)) {
	if(avio_open(&encoder->avfc->pb, encoder->filename, AVIO_FLAG_WRITE) < 0) {
	    printf("Could not open the output file\n");
	    return false;
	}
    }

    AVDictionary *muxer_opts = NULL;

    if(avformat_write_header(encoder->avfc, &muxer_opts) < 0) {
	printf("An error occurred when opening output file\n");
	return false;
    }

    //START WHILE LOOP: TODO

    AVFrame *video_input_frame = av_frame_alloc();
    if(!video_input_frame) {
	printf("Failed to allocate memory for AVFrame\n");
	return false;
    }

    AVPacket *video_input_packet = av_packet_alloc();
    if(!video_input_packet) {
	printf("Failed to allocate memory for AVPacket\n");
	return false;
    }

    AVFrame *audio_input_frame = av_frame_alloc();
    if(!audio_input_frame) {
	printf("Failed to allocate memory for AVFrame\n");
	return false;
    }

    AVPacket *audio_input_packet = av_packet_alloc();
    if(!audio_input_packet) {
	printf("Failed to allocate memory for AVPacket\n");
	return false;
    }

    int continueReadVideo = 1;
    int continueReadAudio = 1;  

    while(1) {

	if(continueReadVideo) {
	    int readVideo = av_read_frame(video_decoder->avfc, video_input_packet);
	    if(readVideo>=0) {
		if(video_decoder->avfc->streams[video_input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		    video_input_packet->stream_index = 0;
		    if(libav_remux(&video_input_packet, &encoder->avfc, video_decoder->video_avs->time_base, encoder->video_avs->time_base)) {
			return false;
		    }
		} else {
		    printf("Ignoring all non other packets in video decoder\n");
		}
	    }
	    if(readVideo < 0) {
		continueReadVideo = 0;
	    }
	}

	if(continueReadAudio) {
	    int readAudio = av_read_frame(audio_decoder->avfc, audio_input_packet);
	    if(readAudio>=0) {
		if(audio_decoder->avfc->streams[audio_input_packet->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
		    audio_input_packet->stream_index = 1;
		    if(libav_remux(&audio_input_packet, &encoder->avfc, audio_decoder->audio_avs->time_base, encoder->audio_avs->time_base)) {
			return false;
		    }
		} else {
		    printf("Ignoring all non other packets in audio decoder\n");
		}
	    }
	    if(readAudio < 0) {
		continueReadAudio = 0;
	    }
	}
    
	if((!continueReadVideo) && (!continueReadAudio)) break;
    }

    //FREE

    av_write_trailer(encoder->avfc);

    if(muxer_opts != NULL) {
	av_dict_free(&muxer_opts);
	muxer_opts = NULL;
    }

    if(video_input_frame != NULL) {
	av_frame_free(&video_input_frame);
	video_input_frame = NULL;
    }

    if(video_input_packet != NULL) {
	av_packet_free(&video_input_packet);
	video_input_packet = NULL;
    }

    if(audio_input_frame != NULL) {
	av_frame_free(&audio_input_frame);
	audio_input_frame = NULL;
    }

    if(audio_input_packet != NULL) {
	av_packet_free(&audio_input_packet);
	audio_input_packet = NULL;
    }

    avformat_close_input(&video_decoder->avfc);
    avformat_close_input(&audio_decoder->avfc);

    avformat_free_context(video_decoder->avfc);
    video_decoder->avfc = NULL;
    avformat_free_context(audio_decoder->avfc);
    audio_decoder->avfc = NULL;
    avformat_free_context(encoder->avfc);
    encoder->avfc = NULL;

    avcodec_free_context(&video_decoder->video_avcc);
    video_decoder->video_avcc = NULL;
    avcodec_free_context(&audio_decoder->video_avcc);
    audio_decoder->video_avcc = NULL;
    avcodec_free_context(&encoder->video_avcc);
    encoder->video_avcc = NULL;

    free(video_decoder);
    video_decoder = NULL;
    free(audio_decoder);
    audio_decoder = NULL;
    free(encoder);
    encoder = NULL;
  
    return true;
}

#endif //LIBAV_IMPLEMENTATION

#endif //LIBAV_H_H
