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
static double MS_PER_FRAME = (double) 1000.0 / (double) 60.0;
static double now = 0.0;

#define PRE_CALC_BUFFER 12

void demux(bool is_video) {
    if(is_video) {
	if( video_index + PRE_CALC_BUFFER >
	    demuxer_video.video_buffer_count
	    //&& demuxer_audio.video_buffer_count - video_index < demuxer_audio.video_buffer_cap
	    ) {
	    demuxer_decode(&demuxer_video, &is_video);
	}
    } else {
	if(audio_index + PRE_CALC_BUFFER >
	   demuxer_audio.audio_buffer_count
	   //&& demuxer_audio.audio_buffer_count - audio_index < demuxer_audio.audio_buffer_cap
	    ) {
	    demuxer_decode(&demuxer_audio, &is_video) ;
	}

    }
}

void *demuxing_thread(void *arg) {
    (void) arg;

    do{
	int64_t audio_diff = demuxer_audio.audio_buffer_count - (audio_index + PRE_CALC_BUFFER);
	int64_t video_diff = demuxer_video.video_buffer_count - (video_index + PRE_CALC_BUFFER);

	if(audio_diff > video_diff) {
	    demux(false);
	    demux(true);
	} else {
	    demux(true);
	    demux(false);
	}

	thread_sleep(5);

    }while(1);
    
    return NULL;
}

void *audio_thread(void *arg) {
    (void) arg;

    int index = audio_index % demuxer_audio.audio_buffer_cap;
    double audio_ms = (double) (av_rescale_q(demuxer_audio.audio_ptss[index], demuxer_audio.audio_time_base, AV_TIME_BASE_Q) * 1000.0 / AV_TIME_BASE);
    unsigned char *buffer_base = demuxer_audio.audio_buffer;

    unsigned char *buffer = buffer_base + index * demuxer_audio.audio_buffer_size;

    unsigned int buffer_stride = demuxer_audio.audio_buffer_size;
    unsigned int buffer_size = (unsigned int) ((int64_t) SAMPLES * (int64_t) 4);

    unsigned int cap = demuxer_audio.audio_buffer_cap;

    mutex_lock(mutex_now);
    double local_now;
    mutex_release(mutex_now);

    while(1) {
	mutex_lock(mutex_now);
	local_now = now;
	mutex_release(mutex_now);

	if(local_now >= audio_ms) {
	    
	    audio_play(&audio, buffer, buffer_size);
	    buffer = buffer_base + index * buffer_stride;
	    audio_index++;
	    index = audio_index % cap;
	    audio_ms = (double)
		(av_rescale_q(demuxer_audio.audio_ptss[index],
			      demuxer_audio.audio_time_base, AV_TIME_BASE_Q)
		 * 1000.0 / AV_TIME_BASE);
	}

    }
    
    return NULL;
}

int main(int argc, char **argv) {

#ifdef _MSC_VER
#pragma comment(lib,"winmm.lib")
#endif //_MSC_VER
    timeBeginPeriod(1);

    if(!mutex_create(&mutex_now)) {
	panic("mutex_create");
    }
    
    const char *filepath = "rsc/videoplayback.mp4";

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
    while(demuxer_audio.audio_buffer_count < demuxer_audio.audio_buffer_cap - 2) {
	demuxer_decode(&demuxer_audio, &is_video);
    }

    is_video = true;
    while(demuxer_video.video_buffer_count < demuxer_video.video_buffer_cap - 2) {
	demuxer_decode(&demuxer_video, &is_video);
    }
    
    /* Thread demuxing_video_thread_id; */
    /* if(!thread_create(&demuxing_video_thread_id, demuxing_video_thread, NULL)) { */
    /* 	panic("thread_create"); */
    /* } */

    /* Thread demuxing_audio_thread_id; */
    /* if(!thread_create(&demuxing_audio_thread_id, demuxing_audio_thread, NULL)) { */
    /* 	panic("thread_create"); */
    /* } */

    Thread demuxing_thread_id;
    if(!thread_create(&demuxing_thread_id, demuxing_thread, NULL)) {
	panic("thread_create");
    }

    Thread audio_thread_id;
    if(!thread_create(&audio_thread_id, audio_thread, NULL)) {
	panic("thread_create");
    }

    bool paused = false;
    int width, height;
    Gui_Event event;
    double local_now;
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


	mutex_lock(mutex_now);
	local_now = now;
	mutex_release(mutex_now);

	int index = video_index % demuxer_video.video_buffer_cap;
	double video_ms = (double) (av_rescale_q(demuxer_video.video_ptss[index], demuxer_video.video_time_base, AV_TIME_BASE_Q) * 1000 / AV_TIME_BASE);	
	if(local_now >= video_ms) {
	    video_index++;
	    glTexSubImage2D(GL_TEXTURE_2D,
			    0,
			    0,
			    0,
			    video_width,
			    video_height,
			    GL_RGB,
			    GL_UNSIGNED_BYTE,
			    demuxer_video.video_buffer + index * demuxer_video.video_buffer_size);
	}
		
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

	mutex_lock(mutex_now);
	if(!paused) now += MS_PER_FRAME;
	mutex_release(mutex_now);
	
	gui_swap_buffers(&gui);

    }
    
    return 0;
}
