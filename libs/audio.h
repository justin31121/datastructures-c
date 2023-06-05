#ifndef AUDIO_H_H
#define AUDIO_H_H

#ifdef AUDIO_IMPLEMENTATION
#  ifdef _WIN32
#    define WAV_IMPLEMENTATION
#  endif //_WIN32
#endif //AUDIO_IMPLEMENTATION

#ifdef _WIN32
#  include <libs/wav.h>
#endif

#ifdef _WIN32
typedef struct{
  int x;
}Audio;
#elif linux
typedef struct{
  int x;
}Audio;
#endif //_WIN32

#ifdef AUDIO_IMPLEMENTATION
#endif //AUDIO_IMPLEMENTATION

#endif //AUDIO_H_H
