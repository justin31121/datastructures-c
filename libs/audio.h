#ifndef AUDIO_H_H
#define AUDIO_H_H

#include <stdint.h>

#ifndef AUDIO_DEF
# define AUDIO_DEF static inline
#endif //AUDIO_DEF

#ifdef AUDIO_IMPLEMENTATION
#  ifdef _WIN32
#    define XAUDIO_IMPLEMENTATION
#  endif //_WIN32
#endif //AUDIO_IMPLEMENTATION

#ifdef _WIN32
#  include "xaudio.h"
#elif linux
#  include <alsa/asoundlib.h>
#endif

#ifdef _WIN32
typedef struct{
  XAudio2Device device;
}Audio;
#elif linux
typedef struct{
  snd_pcm_t *device;
}Audio;
#endif //_WIN32

//FOR NOW ONLY "S16LE" support
AUDIO_DEF bool audio_init(Audio *audio, int channels, int sample_rate);
AUDIO_DEF void audio_play(Audio *audio, unsigned char *data, unsigned int data_size);

#ifdef AUDIO_IMPLEMENTATION

#ifdef _WIN32
static bool audio_xaudio2_inited = false;

//FOR NOW ONLY "S16LE" support
AUDIO_DEF bool audio_init(Audio *audio, int channels, int sample_rate) {
  if(!audio_xaudio2_inited) {
    if(!xaudio_init(channels, sample_rate)) {
      return false;
    }
    audio_xaudio2_inited = true;
  }

  WAVEFORMATEX waveFormat;
  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  waveFormat.nChannels = (WORD) channels;
  waveFormat.nSamplesPerSec = sample_rate;
  waveFormat.wBitsPerSample = 16;
  waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
  waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
  waveFormat.cbSize = 0;

  if(!xaudio_device_init(&audio->device, &waveFormat)) {
    return false;
  }

  return true;
}

AUDIO_DEF void audio_play(Audio *audio, unsigned char *data, unsigned int data_size) {
  xaudio_device_play_async( &audio->device, data, data_size );
}
#endif //_WIN32

#ifdef linux
//FOR NOW ONLY "S16LE" support
AUDIO_DEF bool audio_init(Audio *audio, int channels, int sample_rate) {
  if(snd_pcm_open(&audio->device, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
    return false;
  }
  if(snd_pcm_set_params(audio->device, SND_PCM_FORMAT_S16_LE, //TODO unhardcode everything
			SND_PCM_ACCESS_RW_INTERLEAVED, channels,
			sample_rate, 0, sample_rate / 4) < 0) {
    return false;
  }
  snd_pcm_uframes_t buffer_size = 0;
  snd_pcm_uframes_t period_size = 0;
  if(snd_pcm_get_params(audio->device, &buffer_size, &period_size) < 0) {
    return false;
  }
  snd_pcm_prepare(audio->device);

  return true;
}

AUDIO_DEF void audio_play(Audio *audio, unsigned char *data, unsigned int data_size) {
  data_size = data_size / (unsigned int) 4;

  while(data_size > 0) {
    int ret = snd_pcm_writei(audio->device, data, data_size);
    if(ret <= 0) {
      snd_pcm_recover(audio->device, ret, 1);
      //snd_pcm_prepare(audio->device);
    } else {
      data_size -= (unsigned int) ret;
    }
  }
}

#endif //linux

#endif //AUDIO_IMPLEMENTATION

#endif //AUDIO_H_H
