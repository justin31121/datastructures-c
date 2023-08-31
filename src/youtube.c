#define YOUTUBE_IMPLEMENTATION
#include "../libs/youtube.h"

#define DOWNLOAD_IMPLEMENTATION
#include "../libs/download.h"

char *next(int *argc, char ***argv) {
  if((*argc) == 0) return NULL;
  char *res = *argv[0];
  (*argc)--;
  (*argv)++;
  return res;
}

String_Buffer sb = {0};

typedef enum{
	     STATE_NONE,
	     STATE_AUDIO,
	     STATE_VIDEO,
}State;

//TODO: implement buildHLS
//    https://github.com/hyugogirubato/pydash2hls/

//TODO: this forgets the last stream. For now it is not important
bool nextStream(Youtube_Info *info, size_t *acc, size_t *index, string *tag, string *type) {

  size_t i = *acc;
  *index = *acc;
  
  while(i<info->strings->count) {
    *tag = *(string *) arr_get(info->strings, i++);
    *type = *(string *) arr_get(info->strings, i++);

    string next = *(string *) arr_get(info->strings, i++);
    (void) next;
    while(i<info->strings->count && !string_eq(next, YOUTUBE_INFO_FRAME)) {
      string value = *(string *) arr_get(info->strings, i++);
      (void) value;
      if(i == info->strings->count) return false;
      next = *(string *) arr_get(info->strings, i++);
    }

    *acc = i;
    return true;
  }

  return false;
}

int getStream(string tag) {
  (void) tag;
  return -1;
}

bool getRange(Youtube_Info *info, const char *range_name, size_t stream, string *start, string *end) {
  size_t i = stream;

  *start = (string) {0};
  *end = (string) {0};
  while(i<info->strings->count) {
    i+=2;

    string next = *(string *) arr_get(info->strings, i++);
    while(i<info->strings->count && !string_eq(next, YOUTUBE_INFO_FRAME)) {
      string _value = *(string *) arr_get(info->strings, i++);

      if(string_eq_cstr(next, "start")) {
	*start = _value;
      } else if(string_eq_cstr(next, "end")) {
	*end = _value;
      } else if(string_eq_cstr(next, range_name)) {
	if(start->len && end->len) {
	  return true;
	} else {
	  *start = (string) {0};
	  *end = (string) {0};	  
	}
      } else {
	*start = (string) {0};
	*end = (string) {0};
      }
      
      if(i == info->strings->count) return false;
      next = *(string *) arr_get(info->strings, i++);      
    }
    return false;
  }
  
  return false;
 
}

bool getInitRange() {
  return false;
}

bool getAttribute(Youtube_Info *info, size_t stream, const char *key, string *value) {

  size_t i = stream;

  while(i<info->strings->count) {
    i+=2;

    string next = *(string *) arr_get(info->strings, i++);
    while(i<info->strings->count && !string_eq(next, YOUTUBE_INFO_FRAME)) {
      string _value = *(string *) arr_get(info->strings, i++);

      if(string_eq_cstr(next, key)) {
	*value = _value;
        return true;
      }
      
      if(i == info->strings->count) return false;
      next = *(string *) arr_get(info->strings, i++);      
    }
    return false;
  }
  
  return false;
}

bool splitType(string type, string *mime, string *codec) {
  //audio/mp4; codecs=\"mp4a.40.2\"

  *mime = (string) {0};
  *codec = (string) {0};
  string codec_wrapper = (string) {0};
  int i = 0;
  while(type.len) {
    string part = string_chop_by_delim(&type, ';');
    if(i == 0) *mime = part;
    else if(i == 1) codec_wrapper = part;
    i++;
  }

  if(!mime->len) return false;
  if(!codec_wrapper.len) return false;

  i = 0;
  while(codec_wrapper.len) {
    string part = string_chop_by_delim(&codec_wrapper, '=');
    if(i == 1) *codec = part;
    i++;
  }

  if(codec->len <= 4) return false;
  string_chop_left(codec, 2);
  string_chop_right(codec, 2);
  
  return true;
}

bool buildRanges(Youtube_Info *info, size_t stream, FILE *f) {

  string start, end;
  if(!getRange(info, "indexRange", stream, &start, &end)) {
    return false;
  }
  fprintf(f, "\t\t\t\t<SegmentBase indexRange=\"");
  fprintf(f, String_Fmt, String_Arg(start));
  fprintf(f, "-");
  fprintf(f, String_Fmt, String_Arg(end));
  fprintf(f, "\">\n");

  if(!getRange(info, "initRange", stream, &start, &end)) {
    return false;
  }
  fprintf(f, "\t\t\t\t\t<Initialization range=\"");
  fprintf(f, String_Fmt, String_Arg(start));
  fprintf(f, "-");
  fprintf(f, String_Fmt, String_Arg(end));
  fprintf(f, "\"/>\n");

  fprintf(f, "\t\t\t\t</SegmentBase>\n");
  
  return true;
}

void buildVideo(Youtube_Info *info, size_t stream, string tag, string type, Youtube_Decoder *decoder, String_Buffer *temp, duk_context *duk_ctx, FILE *f) {

  string width, height, fps, bitrate, signature;
  if(getAttribute(info, stream, "audioSampleRate", &width)) {
    return;
  }
  if(!getAttribute(info, stream, "width", &width)) {
    return;
  }
  if(!getAttribute(info, stream, "height", &height)) {
    return;
  }
  if(!getAttribute(info, stream, "fps", &fps)) {
    return;
  }
  if(!getAttribute(info, stream, "bitrate", &bitrate)) {
    return;
  }
  if(!getAttribute(info, stream, "sig", &signature)) {
    return;
  }

  const char *url;
  if(!youtube_decoder_decode(decoder, temp, duk_ctx, signature, &url)) {
    return;
  }

  string mime, codec;
  if(!splitType(type, &mime, &codec)) {
    return;
  }
  
  fprintf(f, "\t\t\t<Representation id=\"");
  fprintf(f, String_Fmt, String_Arg(tag));
  fprintf(f, "\" mimeType=\"");
  fprintf(f, String_Fmt, String_Arg(mime));
  fprintf(f, "\" codecs=\"");
  fprintf(f, String_Fmt, String_Arg(codec));
  fprintf(f, "\" width=\"");
  fprintf(f, String_Fmt, String_Arg(width));
  fprintf(f, "\" height=\"");
  fprintf(f, String_Fmt, String_Arg(height));
  fprintf(f, "\" frameRate=\"");
  fprintf(f, String_Fmt, String_Arg(fps));
  fprintf(f, "\" maxPlayoutRate=\"1\" bandwidth=\"");
  fprintf(f, String_Fmt, String_Arg(bitrate));
  fprintf(f, "\">\n");

  fprintf(f, "\t\t\t\t<BaseURL>");
  //fprintf(f, "%s", url);    
  size_t url_len = strlen(url);
  for(size_t i=0;i<url_len;i++) {
    char c = url[i];
    if(c == '&') {
      fprintf(f, "&#38;");
    } else {
      fprintf(f, "%c", c);
    }
  }
  fprintf(f, "</BaseURL>\n");

  if(!buildRanges(info, stream, f)) {
    return;
  }
  
  fprintf(f, "\t\t\t</Representation>\n");
}

void buildAudio(Youtube_Info *info, size_t stream, string tag, string type, Youtube_Decoder *decoder, String_Buffer *temp, duk_context *duk_ctx, FILE *f) {
  
  string audio_sample_rate, bitrate, signature;
  if(getAttribute(info, stream, "width", &audio_sample_rate)) {
    return;
  }
  if(!getAttribute(info, stream, "audioSampleRate", &audio_sample_rate)) {
    return;
  }
  if(!getAttribute(info, stream, "bitrate", &bitrate)) {
    return;
  }
  if(!getAttribute(info, stream, "sig", &signature)) {
    return;
  }

  const char *url;
  if(!youtube_decoder_decode(decoder, temp, duk_ctx, signature, &url)) {
    return;
  }

  string mime, codec;
  if(!splitType(type, &mime, &codec)) {
    return;
  }

  fprintf(f,"\t\t\t<Representation id=\"");
  fprintf(f, String_Fmt, String_Arg(tag));
  fprintf(f,"\" mimeType=\"");
  fprintf(f, String_Fmt, String_Arg(mime));
  fprintf(f,"\" codecs=\"");
  fprintf(f, String_Fmt, String_Arg(codec));
  fprintf(f,"\" audioSamplingRate=\"");
  fprintf(f, String_Fmt, String_Arg(audio_sample_rate));
  fprintf(f,"\" bandwidth=\"");
  fprintf(f, String_Fmt, String_Arg(bitrate));
  fprintf(f,"\" maxPlayoutRate=\"1\">\n");

  fprintf(f,"\t\t\t\t<AudioChannelConfiguration schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\"2\"/>\n");

  fprintf(f,"\t\t\t\t<BaseURL>");
  size_t url_len = strlen(url);
  for(size_t i=0;i<url_len;i++) {
    char c = url[i];
    if(c == '&') {
      fprintf(f, "&#38;");
    } else {
      fprintf(f, "%c", c);
    }
  }
  fprintf(f,"</BaseURL>\n");

  if(!buildRanges(info, stream, f)) {
    return;
  }

  
  fprintf(f, "\t\t\t</Representation>\n");
}

//TODO: unhardcode *duraction*
//TODO: unhardcode best_stream
void buildMpd(Youtube_Info *info, Youtube_Decoder *decoder, String_Buffer *temp, duk_context *duk_ctx, FILE *f) {
  
  size_t stream, acc=1;
  string tag, type;

  //tag:137 index:36 (for specific videoId)
  int best_stream = 36;

  string best_width, best_height, best_fps;
  if(!getAttribute(info, best_stream, "width", &best_width)) {
    fprintf(stderr, "ERROR: Can not find best width\n");
    return;
  }
  if(!getAttribute(info, best_stream, "height", &best_height)) {
    fprintf(stderr, "ERROR: Can not find best height\n");
    return;
  }
  if(!getAttribute(info, best_stream, "fps", &best_fps)) {
    fprintf(stderr, "ERROR: Can not find best fps\n");
    return;
  }

  fprintf(f, "<?xml version=\"1.0\"?>\n<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:sand=\"urn:mpeg:dash:schema:sand:2016\" xsi:schemaLocation=\"urn:mpeg:dash:schema:mpd:2011 DASH-MPD.xsd urn:mpeg:dash:schema:sand:2016 SAND-MPD.xsd\" minBufferTime=\"PT1.500000S\" type=\"static\" mediaPresentationDuration=\"");

  //Media Presentation
  fprintf(f, "PT0H2M0.00S");
  fprintf(f, "\" >\n");
  fprintf(f,"\t<BaseURL></BaseURL>\n\n");
  fprintf(f, "\t<Period id=\"\" start =\"0\" duration=\"");
  //Media Presentation
  fprintf(f, "PT0H2M0.00S");
  fprintf(f, "\">\n");

  //VIDEO
  fprintf(f, "\t\t<AdaptationSet segmentAlignment=\"true\" maxWidth=\"");
  fprintf(f, String_Fmt, String_Arg(best_width));
  fprintf(f, "\" startsWithSAP=\"1\" maxHeight=\"");
  fprintf(f, String_Fmt, String_Arg(best_height));
  fprintf(f, "\" maxFrameRate=\"");
  fprintf(f, String_Fmt, String_Arg(best_fps));
  fprintf(f, "\" par=\"16:9\" subsegmentStartsWithSAP=\"1\" scanType=\"progressive\">\n");
  
  size_t temp_len = temp->len;

  acc = 1;
  while(nextStream(info, &acc, &stream, &tag, &type)) {
    temp->len = temp_len;
    buildVideo(info, stream, tag, type, decoder, temp, duk_ctx, f);    
  }
  
  fprintf(f, "\t\t</AdaptationSet>\n");

  //AUDIO
  fprintf(f, "\t\t<AdaptationSet segmentAlignment=\"true\" startsWithSAP=\"1\" scanType=\"progressive\" >\n");

  acc = 1;
  while(nextStream(info, &acc, &stream, &tag, &type)) {
    temp->len = temp_len;
    buildAudio(info, stream, tag, type, decoder, temp, duk_ctx, f);
  } 
  
  fprintf(f, "\t\t</AdaptationSet>\n");


  fprintf(f, "\t</Period>\n\n\n");

  fprintf(f, "</MPD>");

  /*
  while(nextStream(info, &acc, &stream, &tag, &type)) {
    printf(String_Fmt"\n", String_Arg(tag));
    if(getAttribute(info, stream, "sig", &value)) {
      printf("\t"String_Fmt"\n", String_Arg(value));
    }
  }
  */  


}

int main() {

  const char *videoId_cstr = "tIstVOX-9Hw";

  string videoId = string_from(videoId_cstr, strlen(videoId_cstr));

  Http http;
  if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
    panic("http_init2");
  }
  string_buffer_reserve(&sb, 5 * 1024 * 1024);
  
  string response;
  if(!youtube_get_response(videoId, &http, &sb, &response)) {
    panic("youtube_get_response");
  }

  Youtube_Info info;
  if(!youtube_info_init(response, &info)) {
    panic("youtube_info_init");
  }

  String_Buffer temp = {0};
  Youtube_Decoder decoder;
  if(!youtube_decoder_init(response, &http, &temp, &decoder)) {
    panic("decoder");
  }

  duk_context *duk_ctx = duk_create_heap_default();

  FILE *f = fopen("trackies.mpd", "wb");
  if(!f) {
    panic("fopen");
  }
  
  buildMpd(&info, &decoder, &temp, duk_ctx, f);
  fclose(f);

    
  return 0;
}

int main2(int argc, char **argv) {
    
  State state = STATE_NONE;
  bool download = false;
  bool pack = false;
  const char *out_name = NULL;

  string itag = {0};
  const char *program = next(&argc, &argv);

  const char *in = next(&argc, &argv);  
  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide a searchterm or an url\n");
    fprintf(stderr, "USAGE: %s [-o <output_name>] (--pack / [-d] [ (-f <tag>) / (-audio/-video)]) <searchterm/url>\n", program);
    exit(1);
  }

  if(strcmp(in, "-o") == 0) {
    out_name = next(&argc, &argv);
    in = next(&argc, &argv);
  }

  if(strcmp(in, "--pack") == 0) {
    pack = true;
    state = STATE_AUDIO; //search for audio reguarly and video specially
    in = next(&argc, &argv);
  }

  if(!pack && strcmp(in, "-d") == 0) {
    download = true;
    in = next(&argc, &argv);
  }

  if(!pack && strcmp(in, "-f") == 0) {
    itag = string_from_cstr(next(&argc, &argv));
    in = next(&argc, &argv);
  } else if(strcmp(in, "-audio") == 0) {
    state = STATE_AUDIO;
    in = next(&argc, &argv);
  } else if(strcmp(in, "-video") == 0) {
    state = STATE_VIDEO;
    in = next(&argc, &argv);
  }
  
  if(in == NULL) {
    fprintf(stderr, "ERROR: Please provide a searchterm or an url\n");
    fprintf(stderr, "USAGE: %s [ (-f <tag>) / (-audio/-video) ] <searchterm/url>\n", program);
    exit(1);
  }

  const char *term = in;

  Http http;
  if(!http_init2(&http, YOUTUBE_HOSTNAME, strlen(YOUTUBE_HOSTNAME), true)) {
    panic("http_init2");
  }
  string_buffer_reserve(&sb, 5 * 1024 * 1024);

  string videoId;
  if(!youtube_get_videoId(term, &videoId)) {

    Youtube_Results results;
    if(!youtube_results_init(string_from_cstr(term), &http, &sb, &results)) {
      panic("youtube_results_init");
    }
    youtube_results_dump(&results);
	
    return 0;
  }

  string response;
  if(!youtube_get_response(videoId, &http, &sb, &response)) {
    panic("youtube_get_response");
  }

  Youtube_Info info;
  if(!youtube_info_init(response, &info)) {
    panic("youtube_info_init");
  }

  if(info.m3u8_source.len) {
    printf(String_Fmt"\n", String_Arg(info.m3u8_source));
    return 0;
  } else if(info.dash_source.len) {
    printf(String_Fmt"\n", String_Arg(info.dash_source));
    return 0;      
  }

  if(pack) {
    UNIMPLEMENTED();
  }

  string signature = {0};
  bool is_signature = false;
    
  if(itag.len) {	
    if(!youtube_info_find_stream(&info, itag, &signature, &is_signature)) {
      panic("youtube_info_find_stream");
    }
  } else if(state == STATE_AUDIO) {
    if(!youtube_info_find_stream(&info, STRING("140"), &signature, &is_signature)) {
      panic("youtube_info_find_audio");
    }
  } else if(state == STATE_VIDEO) {
    if(!youtube_info_find_video(&info, &signature, &is_signature)) {
      panic("youtube_info_find_video");
    }
  } else if(state == STATE_NONE) {
    youtube_info_dump(&info);
    return 0;
  }

  if(!is_signature) {

    string decoded_url = tsmap(&sb, signature, http_decodeURI);
    const char *url = tprintf(&sb, String_Fmt, String_Arg(decoded_url));
    if(download) {
      char *out_buffer;
      size_t out_buffer_size;
      printf("Url: '%s'\n", url);
      
      download3(url, true, &out_buffer, &out_buffer_size);

      if(!out_name) {
	out_name = "videoplayback.mp4";
      }
      io_write_file_len(out_name, out_buffer, out_buffer_size);
      printf("Saved: '%s'\n", out_name);
      return 0;
    }
    printf(String_Fmt"\n", String_Arg( decoded_url ));
	
    return 0;
  }

  String_Buffer temp = {0};
  Youtube_Decoder decoder;
  if(!youtube_decoder_init(response, &http, &temp, &decoder)) {
    panic("decoder");
  }

  duk_context *duk_ctx = duk_create_heap_default();

  const char* url;
  HttpHeader header;
  int i=0;
  for(;i<2;i++) {
    if(!youtube_decoder_decode(&decoder, &temp, duk_ctx, signature, &url)) {
      panic("youtube_decoder_decode");
    }

  check:
    if(!http_head(url, &header, NULL)) {
      panic("http_head");
    }
    
    int res = http_header_response_code(&header);
    if(res == 200) {
      break;
    } else if(res == 301 || res == 302 || res ==304 ) {

      string key = STRING_STATIC("Location");
      string value;
      if(!http_header_has(&header, key, &value)) {
	panic("Rediraction with code: %d has not Location", res);
      }

      url = tprintf(&temp, String_Fmt, String_Arg(value) );
      goto check;
    }
  }
  if(i == 2) {
    panic("Can not find url");
  }

  if(download) {
    char *out_buffer;
    size_t out_buffer_size;
    printf("Url: '%s'\n", url);
      
    download3(url, true, &out_buffer, &out_buffer_size);

    if(!out_name) {
      out_name = "videoplayback.mp4";
    }
    io_write_file_len(out_name, out_buffer, out_buffer_size);
    printf("Saved: '%s'\n", out_name);
    return 0;
  }

  printf("%s\n", url);

  return 0;
}
