#define DEMUXER_IMPLEMENTATION
#include "../libs/demuxer.h"

#define THREAD_IMPLEMENTATION
#include "../libs/thread.h"

#define AUDIO_IMPLEMENTATION
#include "../libs/audio.h"

#define GUI_IMPLEMENTATION
#include "../libs/gui.h"

static Demuxer demuxer_video;
static Demuxer demuxer_audio;
static Audio audio;
static Gui gui;

static int64_t audio_index = 0;
static int64_t video_index = 0;

static Mutex mutex_now;
static double MS_PER_FRAME;
static double now = 0.0;

void *demuxing_video_thread(void *arg) {
    (void) arg;
    bool is_video = true;

    do{
	while(demuxer_video.video_buffer_count - video_index < demuxer_video.video_buffer_cap) {
	    /* printf("video_index: %lld, count: %lld, diff: %lld, cap: %d\n", video_index, demuxer_video.video_buffer_count, demuxer_video.video_buffer_count - video_index, demuxer_video.video_buffer_cap); fflush(stdout); */
	    if(!demuxer_decode(&demuxer_video, &is_video)) {
		return NULL;
	    }
	}

	thread_sleep(10);
    }while(1);
    
    return NULL;
}

void *demuxing_audio_thread(void *arg) {
    (void) arg;
    bool is_video = false;

    do{
	while(demuxer_audio.audio_buffer_count - audio_index < demuxer_audio.audio_buffer_cap) {
	    /* printf("audio_index: %lld, count: %lld, diff: %lld, cap: %d\n", audio_index, demuxer_audio.audio_buffer_count, demuxer_audio.audio_buffer_count - audio_index, demuxer_audio.audio_buffer_cap); fflush(stdout); */
	    if(!demuxer_decode(&demuxer_audio, &is_video)) {
		return NULL;
	    }
	}

	thread_sleep(10);
    }while(1);
    
    return NULL;
}

void *audio_thread(void *arg) {
    (void) arg;

    int index = audio_index % demuxer_audio.audio_buffer_cap;
    double audio_ms = (double) (av_rescale_q(demuxer_audio.audio_ptss[index], demuxer_audio.audio_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
    unsigned char *buffer = demuxer_audio.audio_buffer + index * demuxer_audio.audio_buffer_size;
    
    double local_now;

    while(1) {
	mutex_lock(mutex_now);
	local_now = now;
	mutex_release(mutex_now);

	if(audio_index < demuxer_audio.audio_buffer_count && local_now >= audio_ms) {
	    audio_play(&audio, buffer, SAMPLES * 4);


	    audio_index++;
	    index = audio_index % demuxer_audio.audio_buffer_cap;
	    buffer = demuxer_audio.audio_buffer + index * demuxer_audio.audio_buffer_size;
	    audio_ms = (double) (av_rescale_q(demuxer_audio.audio_ptss[index], demuxer_audio.audio_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);
	}

    }
    
    return NULL;
}

int main(int argc, char **argv) {

    MS_PER_FRAME = 1000.0 / 60.0;

    if(!mutex_create(&mutex_now)) {
	panic("mutex_create");
    }
    
    const char *filepath = "videoplayback.mp4";

    if(argc > 1) {
	filepath = argv[1];
    }
 
    if(!demuxer_init(&demuxer_video, filepath)) {
	panic("demuxer_video_init");
    }
    if(!demuxer_init(&demuxer_audio, filepath)) {
	panic("demuxer_audio_init");
    }
    printf("sample_rate: %d\n", demuxer_audio.audio_sample_rate);

    int video_width = demuxer_video.video_width;
    int video_height = demuxer_video.video_height;

    Gui_Canvas canvas = {video_width, video_height, NULL};
    if(!gui_init(&gui, &canvas, "video")) {
	panic("gui_init");
    }
    gui_use_vsync(1);

    ////////////////////////////////////////////
    
    if(!audio_init(&audio, 2, demuxer_video.audio_sample_rate)) {
	panic("audio_init");
    }

    ////////////////////////////////////////////

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
		 video_width,
		 video_height,
		 0,
		 GL_RGB,
		 GL_UNSIGNED_BYTE,
		 NULL);


    glEnable(GL_TEXTURE_2D);

    bool is_video = false;
    while(demuxer_audio.audio_buffer_count < demuxer_audio.audio_buffer_cap - 1) {
	demuxer_decode(&demuxer_audio, &is_video);
    }
    is_video = true;
    while(demuxer_video.video_buffer_count < demuxer_video.video_buffer_cap - 1) {
	demuxer_decode(&demuxer_video, &is_video);
    }

    Thread demuxing_video_thread_id;
    if(!thread_create(&demuxing_video_thread_id, demuxing_video_thread, NULL)) {
	panic("thread_create");
    }

    Thread demuxing_audio_thread_id;
    if(!thread_create(&demuxing_audio_thread_id, demuxing_audio_thread, NULL)) {
	panic("thread_create");
    }

    Thread audio_thread_id;
    if(!thread_create(&audio_thread_id, audio_thread, NULL)) {
	panic("thread_create");
    }

    bool paused = false;
    int width, height;
    Gui_Event event;
    while(gui.running) {
		
	while(gui_peek(&gui, &event)) {
	    if(event.type == GUI_EVENT_KEYPRESS) {
		if(event.as.key == 'Q') {
		    gui.running = false;		    
		} else if(event.as.key == 'K') {
		    paused = !paused;
		}
	    }	    
	}

	int index = video_index % demuxer_video.video_buffer_cap;
	double video_ms = (double) (av_rescale_q(demuxer_video.video_ptss[index], demuxer_video.video_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);	
	if(now >= video_ms) {
	    glTexSubImage2D(GL_TEXTURE_2D,
			    0,
			    0,
			    0,
			    video_width,
			    video_height,
			    GL_RGB,
			    GL_UNSIGNED_BYTE,
			    demuxer_video.video_buffer + index * demuxer_video.video_buffer_size);
	    video_index++;
	}
	
	mutex_lock(mutex_now);
	if(!paused) now += MS_PER_FRAME;
	mutex_release(mutex_now);
	
	gui_get_window_size(&gui, &width, &height);
	
	glViewport(0, 0, width, height);
	glClearColor(1, 0, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(-1, 1);
	glTexCoord2f(1, 0); glVertex2f(1, 1);
	glTexCoord2f(1, 1); glVertex2f(1, -1);
	glTexCoord2f(0, 1); glVertex2f(-1, -1);
	glEnd();
	
	gui_swap_buffers(&gui);

    }
    
    return 0;
}
