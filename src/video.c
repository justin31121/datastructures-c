#define DEMUXER_IMPLEMENTATION
#include "../libs/demuxer.h"

#define GUI_IMPLEMENTATION
#define GUI_CONSOLE
#define GUI_OPENGL
#include "../libs/gui.h"

#define AUDIO_IMPLEMENTATION
#include "../libs/audio.h"

#define THREAD_IMPLEMENTATION
#include "../libs/thread.h"

#define IO_IMPLEMENTATION
#include "../libs/io.h"

double MS_PER_FRAME = (double) 1000.0 / (double) 60.0;

//24 bits per pixel

typedef struct{
    int64_t *ptss;
    char *data;
    int64_t count;
    int64_t cap;

    int64_t sample_rate;
    int64_t samples;
}Samples;

typedef struct{
    Samples *samples;
    bool stop;
}Meta;

void save_decode(Demuxer *demuxer, int64_t new_pts, int out_samples, void *arg) {

    Meta *meta = (Meta *) arg;

    if(out_samples < 0) {
    
    } else {
	Samples *samples = meta->samples;
	int64_t index = samples->count % samples->cap;
    
	memcpy(samples->data + (index * samples->samples * (int64_t) 4), demuxer->buffer, samples->samples * (int64_t) 4);
	samples->ptss[index] = new_pts;
  
	samples->count++;
    }
}

static Demuxer demuxer_audio;
static Demuxer demuxer_video;
static Audio audio = {0};

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
    double audio_ms = (double) (av_rescale_q(samples->ptss[index], demuxer_audio.time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE); 

    mutex_lock(mutex);
    double local_now = now;
    mutex_release(mutex);
  
    while(!meta.stop) {

	mutex_lock(mutex);
	local_now = now;
	mutex_release(mutex);

	if(local_now >= audio_ms) {

	    printf("AUDIO: %lf >= %lf\n", local_now, audio_ms); fflush(stdout);
	    audio_play(&audio, data, buffer_size);
      
	    data = buffer_data + index * buffer_stride;
	    audio_pos++;
	    index = audio_pos % samples->cap;
	    audio_ms = (double) (av_rescale_q(samples->ptss[index], demuxer_audio.time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
	}

    }
    
    return NULL;
}

void *decoding_audio_thread(void *arg) {
    (void) arg;
  
    while(!meta.stop) {
        int audio_slot = audio_pos % meta.samples->cap;
	int audio_index = meta.samples->count % meta.samples->cap;

	while( (audio_slot != audio_index) && (meta.samples->count - audio_pos< meta.samples->cap)) {
	    demuxer_decode(&demuxer_audio, save_decode, &meta);
	}

	thread_sleep(10);

    }
    return NULL;
}

void *decoding_video_thread(void *arg) {
    (void) arg;
  
    while(!meta.stop) {

	int video_slot = video_pos % demuxer_video.cap;
	int video_index = demuxer_video.count % demuxer_video.cap;

	while( (video_slot != video_index) && (demuxer_video.count - video_pos < demuxer_video.cap)) {
	    demuxer_decode(&demuxer_video, NULL, &meta);
	}
    
	thread_sleep(10);
	//fprintf(stderr, "video_pos: %lld, Frames.count: %lld (%lld)\n", video_pos, meta.frames->count, meta.frames->count - video_pos); fprintf(stderr, "audio_pos: %lld, Samples.count: %lld (%lld)\n", audio_pos, meta.samples->count, meta.samples->count - audio_pos); fflush(stderr);
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

    demuxer_audio.is_audio = true;
    demuxer_init(&demuxer_audio, filepath);
    demuxer_video.is_audio = false;
    demuxer_init(&demuxer_video, filepath);
  
    printf("%lldsec\n", demuxer_video.duration);
    float total = (float) demuxer_video.duration / 60.f;
    float secs = floorf(total);
    float comma = (total - secs) * 60.0f / 100.0f;
  
    printf("%fmin\n", secs + comma);

    if(!audio_init(&audio, 2, demuxer_audio.sample_rate)) {
	return 1;
    }

    ////////////////////////////////////////////////////////////////////////// 
 
    int64_t bytes_per_sample = 4;

    Samples samples = {0};
    samples.sample_rate = demuxer_audio.sample_rate;
    samples.samples = SAMPLES;
    samples.cap = 100;
  
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

    meta = (Meta) {&samples, false};

    for(int i=0;i<40;i++) demuxer_decode(&demuxer_video, NULL, &meta);
    for(int i=0;i<20;i++) demuxer_decode(&demuxer_audio, save_decode, &meta);

    printf("video_count: %lld, audio_count: %lld\n", demuxer_video.count, samples.count); 
    fflush(stdout);

    for(int i=0;i<19;i++) {
	printf("video_pts: %lld\n", demuxer_video.ptss[i % demuxer_video.cap]);
    }

    for(int i=0;i<19;i++) {
	printf("audio_pts: %lld\n", samples.ptss[i % samples.cap]);
    }
	
    /////////////////////////////////////////////////////////////////////////
   
    Gui gui;
    Gui_Canvas canvas = {(unsigned int) 500, (unsigned int) 300, NULL};
    if(!gui_init(&gui, &canvas, "video")) {
	return 1;
    }
    gui_use_vsync(1);

    int64_t bytes_per_pixel = 3;
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
		 (GLsizei) demuxer_video.buffer_width,
		 (GLsizei) demuxer_video.buffer_height,
		 0,
		 GL_RGB,
		 GL_UNSIGNED_BYTE,
		 NULL);

    glTexSubImage2D(GL_TEXTURE_2D,
		    0,
		    0,
		    0,
		    (GLsizei) demuxer_video.buffer_width,
		    (GLsizei) demuxer_video.buffer_height,
		    GL_RGB,
		    GL_UNSIGNED_BYTE,
		    demuxer_video.buffer + ((video_pos++ % demuxer_video.cap) * demuxer_video.buffer_width * demuxer_video.buffer_height * bytes_per_pixel));

    bool paused = false;

    Thread audio_thread_id;
    Thread decoding_video_thread_id, decoding_audio_thread_id;
    if(!thread_create(&decoding_video_thread_id, decoding_video_thread, NULL)) {
	return 1;
    }
    if(!thread_create(&decoding_audio_thread_id, decoding_audio_thread, NULL)) {
	return 1;
    }

    bool bar_clicked = false;

    if(!thread_create(&audio_thread_id, audio_thread, &samples)) {
	return 1;
    }

    float width = 0.f, height = 0.f;
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
	if(!paused) {
	    now += MS_PER_FRAME;
	}
	mutex_release(mutex);

	mutex_lock(mutex);
	double local_now = now;
	mutex_release(mutex);

	int64_t index = (video_pos) % demuxer_video.cap;
    
	double video_ms = (double) (av_rescale_q(demuxer_video.ptss[index], demuxer_video.time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
	if(local_now >= video_ms) {
	    if(video_pos == 0) {
	    }
	    //printf("%lf ms, video_pos: %lld, index: %ld\n", now, video_pos, index); fflush(stdout);
	    video_pos++;
	    printf("VIDEO: %lf >= %lf\n", local_now, video_ms); fflush(stdout);
	    glTexSubImage2D(GL_TEXTURE_2D,
			    0,
			    0,
			    0,
			    (GLsizei) demuxer_video.buffer_width,
			    (GLsizei) demuxer_video.buffer_height,
			    GL_RGB,
			    GL_UNSIGNED_BYTE,
			    demuxer_video.buffer + (index * demuxer_video.buffer_width * demuxer_video.buffer_height * bytes_per_pixel));
	    if(video_pos >= demuxer_video.count - 1) {
		gui.running = false;
		printf("exited fine\n"); fflush(stdout);
		break;
	    }

	}
    
	gui_get_window_sizef(&gui, &width, &height);

	float mouse_x = (float) event.mousex;
	float mouse_y = (float) event.mousey;
	gui_mouse_to_screenf(width, height, &mouse_x, &mouse_y);

	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);
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
	    thread_join(decoding_video_thread_id);
	    thread_join(decoding_audio_thread_id);
	    thread_join(audio_thread_id);

	    //SEEK
	    demuxer_seek(&demuxer_video, current);
	    demuxer_seek(&demuxer_audio, current);

	    demuxer_video.count = 0;
	    samples.count = 0;
	    audio_pos = 0;
	    video_pos = 0;
	    meta.stop = false;

	    while(demuxer_video.count < demuxer_video.cap) {
		if(samples.count == samples.cap) break;
		demuxer_decode(&demuxer_audio, save_decode, &meta);
		demuxer_decode(&demuxer_video, save_decode, &meta);
	    }

	    double frame_start = (double) (av_rescale_q(demuxer_video.ptss[0], demuxer_video.time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
	    double sample_start = frame_start;
	    if(samples.count) {
		sample_start = (double) (av_rescale_q(samples.ptss[0], demuxer_audio.time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
	    }
	    now = frame_start > sample_start ? frame_start : sample_start;

	    if(!thread_create(&decoding_video_thread_id, decoding_video_thread, NULL)) {
		return 1;
	    }
      
	    if(!thread_create(&decoding_audio_thread_id, decoding_audio_thread, NULL)) {
		return 1;
	    }

	    if(!thread_create(&audio_thread_id, audio_thread, &samples)) {
		return 1;
	    }

	}
    
	float actual_cursor_x = (float) now * width / 1000.f / (float) demuxer_video.duration;
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
    thread_join(decoding_video_thread_id);
    thread_join(decoding_audio_thread_id);
  
    return 0;
}
