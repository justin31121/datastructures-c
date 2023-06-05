#define THREAD_IMPLEMENTATION
#include "../libs/thread.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#define GUI_IMPLEMENTATION
#define GUI_CONSOLE
#define GUI_OPENGL
#include "../libs/gui.h"

#define AUDIO_IMPLEMENTATION
#include "../libs/audio.h"

#define DEMUXER_IMPLEMENTATION
#include "../libs/demuxer.h"

#include <alsa/asoundlib.h>

#define PRE_CALC_BUFFER 6
double MS_PER_FRAME = (double) 1000.0 / (double) 60.0;

//24 bits per pixel
typedef struct{
  int64_t *ptss;
  char *data;
  int64_t cap;
  int64_t count;

  int64_t width;
  int64_t height;
}Frames;

typedef struct{
  int64_t *ptss;
  char *data;
  int64_t count;
  int64_t cap;

  int64_t sample_rate;
  int64_t samples;
}Samples;

typedef struct{
  Frames *frames;
  Samples *samples;
  bool stop;
}Meta;

void save_decode(Demuxer *demuxer, int64_t new_pts, int out_samples, void *arg) {

  Meta *meta = (Meta *) arg;

  if(out_samples < 0) {
    Frames *frames = meta->frames;
    int64_t index = frames->count % frames->cap;
    
    memcpy(frames->data + (index * frames->width * frames->height * (int64_t) 3), demuxer->video_rgb_buffer, frames->width * frames->height * (int64_t) 3);
    frames->ptss[index] = new_pts;
  
    frames->count++;
    
  } else {
    Samples *samples = meta->samples;
    int64_t index = samples->count % samples->cap;
    
    memcpy(samples->data + (index * samples->samples * (int64_t) 4), demuxer->audio_buffer, samples->samples * (int64_t) 4);
    samples->ptss[index] = new_pts;
  
    samples->count++;
  }
}

static Demuxer demuxer;

static snd_pcm_t *device;
//static XAudio2Device device;

static Meta meta;
static double now = 0;
static Mutex mutex;

static int64_t audio_pos = 0;
static int64_t video_pos = 0;

void *audio_thread(void *arg) {
  Samples *samples = (Samples *) arg;

  unsigned int buffer_size = (unsigned int) (samples->samples * (int64_t) 4);
  unsigned char *buffer_data  = (unsigned char *) samples->data;
  unsigned int buffer_stride = (unsigned int) (samples->samples * (int64_t) 4) ;

  unsigned char *data = buffer_data;
  (void) buffer_size;
  
  //int64_t samples_per_sec = (int64_t) demuxer.audio_time_base.den;
  //int64_t ms_per_buffer = (int64_t) SAMPLES * (int64_t) 1000 / samples_per_sec;

  int64_t index = audio_pos % samples->cap;
  double audio_ms = (double) (av_rescale_q(samples->ptss[index], demuxer.audio_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE); 

  mutex_lock(mutex);
  double local_now = now;
  mutex_release(mutex);
  
  while(!meta.stop) {

    mutex_lock(mutex);
    local_now = now;
    mutex_release(mutex);

    if(local_now >= audio_ms) {      
      //xaudio_device_play_async( &device, data, buffer_size );
      int ret = snd_pcm_writei(device, data, samples->samples);
      if(ret < 0) {
	snd_pcm_recover(device, ret, 1);
	snd_pcm_prepare(device);
	snd_pcm_writei(device, data, samples->samples);
      }
      
      data = buffer_data + index * buffer_stride;
      audio_pos++;
      index = audio_pos % samples->cap;
      audio_ms = (double) (av_rescale_q(samples->ptss[index], demuxer.audio_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
    }

  }
    
  return NULL;
}

void *decoding_thread(void *arg) {
  (void) arg;
  
  while(!meta.stop) {    

    while( (video_pos + PRE_CALC_BUFFER >= meta.frames->count) ||
	   (audio_pos + PRE_CALC_BUFFER >= meta.samples->count)) {
      if(meta.stop) break;
      if(!demuxer_decode(&demuxer, save_decode, &meta)) break;
    }
    
    thread_sleep(5);
  }
  return NULL;
}

int main(int argc, char **argv) {

  if(!mutex_create(&mutex)) {
    return 1;
  }
  
  //////////////////////////////////////////////////////////////////////////

  const char* filepath = "videoplayback.mp4";
  if(argc > 1) {
    filepath = argv[1];
  }
  demuxer_init(&demuxer, filepath);
  printf("%ldsec\n", demuxer.duration);

  float total = (float) demuxer.duration / 60.f;
  float secs = floorf(total);
  float comma = (total - secs) * 60.0f / 100.0f;
  
  printf("%fmin\n", secs + comma);

  //////////////////////////////////////////////////////////////////////////
  
  int64_t bytes_per_pixel = 3;
    
  Frames frames = {0};
  frames.width = demuxer.video_rgb_buffer_stride / 3;
  frames.height = demuxer.video_rgb_buffer_height;
  frames.cap = 20;

  int64_t size = frames.width * frames.height
    * bytes_per_pixel * frames.cap;

  frames.data = (char *) malloc(size);
  if(!frames.data) {
    fprintf(stderr, "ERROR: Can not allocate enough memory\n");
    return 1;
  }

  frames.ptss = (int64_t *) malloc(sizeof(int64_t) * frames.cap);
  if(!frames.ptss) {
    fprintf(stderr, "ERROR: Can not allocate enough memory\n");
    return 1;
  }

  //////////////////////////////////////////////////////////////////////////
 
  int64_t bytes_per_sample = 4;

  Samples samples = {0};
  samples.sample_rate = demuxer.audio_sample_rate;
  samples.samples = SAMPLES;
  samples.cap = 400;
  
  samples.ptss = (int64_t *) malloc(sizeof(int64_t) * samples.cap);
  if(!samples.ptss) { 
    fprintf(stderr, "ERROR: Can not allocate enough memory\n");
    return 1;
  }

  int64_t samples_size = samples.samples * samples.cap * bytes_per_sample;

  samples.data = (char *) malloc(samples_size);
  if(!samples.data) {
    fprintf(stderr, "ERROR: Can not allocate enough memory\n");
    return 1;
  }

  samples.ptss = (int64_t *) malloc(sizeof(int64_t) * samples.cap);
  if(!samples.ptss) { 
    fprintf(stderr, "ERROR: Can not allocate enough memory\n");
    return 1;
  }

  /////////////////////////////////////////////////////////////////////////

  meta = (Meta) {&frames, &samples, false};

  for(int i=0;i<PRE_CALC_BUFFER*2;i++) {
    if(!demuxer_decode(&demuxer, save_decode, &meta)) break;
  }

  Thread decoding_thread_id;
  if(!thread_create(&decoding_thread_id, decoding_thread, NULL)) {
    return 1;
  }

  if(snd_pcm_open(&device, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
    return 1;
  }
  int sample_rate = demuxer.audio_sample_rate;
  if(snd_pcm_set_params(device, SND_PCM_FORMAT_S16_LE, //TODO unhardcode everything
			SND_PCM_ACCESS_RW_INTERLEAVED, 2,
			sample_rate, 0, sample_rate / 4) < 0) {
    return 1;
  }
  snd_pcm_uframes_t buffer_size = 0;
  snd_pcm_uframes_t period_size = 0;
  if(snd_pcm_get_params(device, &buffer_size, &period_size) < 0) {
    return 1;
  }
  snd_pcm_prepare(device);

  /*
  if(!xaudio_init(2, demuxer.audio_sample_rate)) {
    return 1;
  }

  WAVEFORMATEX waveFormat;
  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  waveFormat.nChannels = 2;
  waveFormat.nSamplesPerSec = demuxer.audio_sample_rate;
  waveFormat.wBitsPerSample = 16;
  waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
  waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
  waveFormat.cbSize = 0;

  if(!xaudio_device_init(&device, &waveFormat)) {
    return 1;
  }
  */

  /////////////////////////////////////////////////////////////////////////
   
  Gui gui;
  Gui_Canvas canvas = {(unsigned int) 1080, (unsigned int) 512, NULL};
  if(!gui_init(&gui, &canvas, "video")) {
    return 1;
  }
  gui_use_vsync(1);

  GLuint textures;
  glGenTextures(1, &textures);
  glBindTexture(GL_TEXTURE_2D, textures);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D,
	       0,
	       GL_RGB,
	       (GLsizei) frames.width,
	       (GLsizei) frames.height,
	       0,
	       GL_RGB,
	       GL_UNSIGNED_BYTE,
	       NULL);

  glTexSubImage2D(GL_TEXTURE_2D,
		  0,
		  0,
		  0,
		  (GLsizei) frames.width,
		  (GLsizei) frames.height,
		  GL_RGB,
		  GL_UNSIGNED_BYTE,
		  frames.data + ((video_pos % frames.cap) * frames.width * frames.height * bytes_per_pixel));

  bool paused = false;

  Thread audio_thread_id;
  if(!thread_create(&audio_thread_id, audio_thread, &samples)) {
    return 1;
  }

  bool bar_clicked = false;

  float width, height;
  Gui_Event event;
  while(gui.running) {
    bool left_click = false;
    bool release = false;
	
    while(gui_peek(&gui, &event)) {
      if(event.type == GUI_EVENT_KEYPRESS) {
	if(event.as.key == 'Q') gui.running = false;
	if(event.as.key == 'K') paused = !paused;
      } else if(event.type == GUI_EVENT_MOUSEPRESS) {
	if(event.as.key == 'L') {
	  left_click = true;
	}
      } else if(event.type == GUI_EVENT_MOUSERELEASE) {
	if(bar_clicked) release = true;
	bar_clicked = false;
      }
    }

    mutex_lock(mutex);
    double local_now = now;
    mutex_release(mutex);

    int64_t index = (video_pos) % frames.cap;
    
    double video_ms = (double) (av_rescale_q(frames.ptss[index], demuxer.video_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
    if(local_now >= video_ms) {
      //printf("%lld ms\n", now);
      video_pos++;
      if(video_pos >= frames.count - 2) {
	gui.running = false;
	break;
      }      
      glTexSubImage2D(GL_TEXTURE_2D,
		      0,
		      0,
		      0,
		      (GLsizei) frames.width,
		      (GLsizei) frames.height,
		      GL_RGB,
		      GL_UNSIGNED_BYTE,
		      frames.data + (index * frames.width * frames.height * bytes_per_pixel));
    }
    
    gui_get_window_sizef(&gui, &width, &height);

    float mouse_x = (float) event.mousex;
    float mouse_y = (float) event.mousey;
    gui_mouse_to_screenf(width, height, &mouse_x, &mouse_y);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    
    glViewport(0, 0, (int) width, (int) height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(.7f, 0, 0, 1);

    glColor4f(1.0f,1.0f,1.0f,1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, 1);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(1, 1); glVertex2f(1, -1);
    glTexCoord2f(0, 1); glVertex2f(-1, -1);
    glEnd();

    float padding = 0.025f * width;
    float bar_height = 0.02f * height;    
    
    float cursor_width = 0.02f * width;

    if(release) {
      float current = mouse_x;
      if(current < padding) current = padding;
      if(current >= width - padding) current = width - padding;

      current = (current - padding) / (width - 2.0f*padding);

      //STOP
      meta.stop = true;
      thread_join(decoding_thread_id);
      thread_join(audio_thread_id);

      //SEEK
      demuxer_seek(&demuxer, current);

      frames.count = 0;
      samples.count = 0;
      audio_pos = 0;
      video_pos = 0;
      meta.stop = false;

      while(frames.count < PRE_CALC_BUFFER) {
	demuxer_decode(&demuxer, save_decode, &meta);
      }

      double sample_start = 0.0;
      if(samples.count) {
	sample_start = (double) (av_rescale_q(samples.ptss[audio_pos], demuxer.audio_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);	
      }
      double frame_start = (double) (av_rescale_q(frames.ptss[video_pos], demuxer.video_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
      now = frame_start;

      while(samples.ptss[audio_pos] < frames.ptss[video_pos]) {
	audio_pos++;
      }

      if(!thread_create(&decoding_thread_id, decoding_thread, NULL)) {
	return 1;
      }

      if(!thread_create(&audio_thread_id, audio_thread, &samples)) {
	return 1;
      }

    }
    
    float actual_cursor_x = (float) now * width / 1000.f / (float) demuxer.duration;
    float cursor_x = actual_cursor_x;
    float cursor_y = padding;

    if(left_click) {
      if(actual_cursor_x + cursor_width/2 <= mouse_x && mouse_x <= actual_cursor_x + cursor_width/2 + cursor_width &&
	 cursor_y <= mouse_y && mouse_y <= cursor_y + bar_height) {
	bar_clicked = true;
      } 
    }
    cursor_x += padding;
    actual_cursor_x += padding;

    if(bar_clicked) {
      cursor_x = mouse_x;
      if(cursor_x < padding) cursor_x = padding;
      if(cursor_x >= width - padding) cursor_x = width - padding;
    }
    
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glColor4f(0.3f,0.3f,0.3f,1.0f);
    glVertex2f(padding/width*2.0-1.0, (bar_height+padding)/height*2.0-1.0);
    glVertex2f((actual_cursor_x)/width*2.0-1.0, (bar_height+padding)/height*2.0-1.0); 
    glVertex2f((actual_cursor_x)/width*2.0-1.0, padding/height*2.0-1.0);
    glVertex2f(padding/width*2.0-1.0, padding/height*2.0-1.0);

    ////////////////////////

    glColor4f(0.09f,0.09f,0.09f,1.0f);    
    glVertex2f(actual_cursor_x/width*2.0-1.0, (bar_height+padding)/height*2.0-1.0); 
    glVertex2f((width - padding)/width*2.0-1.0, (bar_height+padding)/height*2.0-1.0); 
    glVertex2f((width - padding)/width*2.0-1.0, padding/height*2.0-1.0);
    glVertex2f(actual_cursor_x/width*2.0-1.0, padding/height*2.0-1.0);

    ///////////////////////////////////////////////

    if(bar_clicked) {
      glColor4f(0.6f,0.6f,0.6f,1.0f);
    } else {
      glColor4f(1.0f,1.0f,1.0f,1.0f);
    }
    
    glVertex2f(( - cursor_width/2 + cursor_x )/width*2.0-1.0, (bar_height+padding)/height*2.0-1.0);
    glVertex2f(( cursor_width/2 + cursor_x )/width*2.0-1.0, (bar_height+padding)/height*2.0-1.0); 
    glVertex2f(( cursor_width/2 + cursor_x )/width*2.0-1.0, padding/height*2.0-1.0);
    glVertex2f(( - cursor_width/2 + cursor_x )/width*2.0-1.0, padding/height*2.0-1.0);
    
    glEnd();
    
    mutex_lock(mutex);
    if(!paused) {
      now += MS_PER_FRAME;
    }
    mutex_release(mutex);

    double seconds = now / 1000;
    double minutes = (double) ((int64_t) (seconds / 60) % 10);
    double fractional = roundf(seconds - minutes * 60.0) / 100.0;
    
    if(fractional == 0.60) {
      fractional = 0.0;
      minutes += 1.0;
    }
    //printf("%f\n", minutes + fractional);

    gui_swap_buffers(&gui);
  }

  meta.stop = true;
  thread_join(decoding_thread_id);
  
  return 0;
}
