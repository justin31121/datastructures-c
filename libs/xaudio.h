#ifndef XAUDIO_H_H_
#define XAUDIO_H_H_

#ifdef _WIN32
#include <windows.h>
#include <xaudio2.h>
#include <stdbool.h>

#ifndef XAUDIO_DEF
#define XAUDIO_DEF static inline
#endif //XAUDIO_DEF

typedef struct{
  IXAudio2SourceVoice* sourceVoice;
  HANDLE semaphore;
}XAudio2Device;

IXAudio2* xaudio = NULL;
IXAudio2MasteringVoice *xaudioMasteringVoice = NULL;

XAUDIO_DEF bool xaudio_init(int channels, int sample_rate);
XAUDIO_DEF bool xaudio_device_init(XAudio2Device *device, const WAVEFORMATEX *pSourceFormat);
XAUDIO_DEF void xaudio_device_free(XAudio2Device *device);
XAUDIO_DEF void xaudio_device_play(XAudio2Device *device, unsigned char* data, UINT32 size);

#ifdef XAUDIO_IMPLEMENTATION

#ifdef _MSC_VER
# pragma comment(lib, "ole32.lib")
#endif //_MCS_VER

XAUDIO_DEF void xaudio_OnBufferEnd(IXAudio2VoiceCallback* This, void* pBufferContext) {
  XAudio2Device *device = (XAudio2Device *) pBufferContext;
  ReleaseSemaphore(device->semaphore, 1, NULL);
}

XAUDIO_DEF void xaudio_OnStreamEnd(IXAudio2VoiceCallback* This) {}
XAUDIO_DEF void xaudio_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback* This) { }
XAUDIO_DEF void xaudio_OnVoiceProcessingPassStart(IXAudio2VoiceCallback* This, UINT32 SamplesRequired) { }
XAUDIO_DEF void xaudio_OnBufferStart(IXAudio2VoiceCallback* This, void* pBufferContext) {}
XAUDIO_DEF void xaudio_OnLoopEnd(IXAudio2VoiceCallback* This, void* pBufferContext) { }
XAUDIO_DEF void xaudio_OnVoiceError(IXAudio2VoiceCallback* This, void* pBufferContext, HRESULT Error) { }

IXAudio2VoiceCallback xaudio_xAudioCallbacks = {
  .lpVtbl = &(IXAudio2VoiceCallbackVtbl) {
    .OnStreamEnd = xaudio_OnStreamEnd,
    .OnVoiceProcessingPassEnd = xaudio_OnVoiceProcessingPassEnd,
    .OnVoiceProcessingPassStart = xaudio_OnVoiceProcessingPassStart,
    .OnBufferEnd = xaudio_OnBufferEnd,
    .OnBufferStart = xaudio_OnBufferStart,
    .OnLoopEnd = xaudio_OnLoopEnd,
    .OnVoiceError = xaudio_OnVoiceError
  }
};

XAUDIO_DEF bool xaudio_init(int channels, int sample_rate) {
  HRESULT comResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (FAILED(comResult)) {
    return false;
  }
  comResult = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
  if (FAILED(comResult)) {
    return false;
  }

  comResult = xaudio->lpVtbl->CreateMasteringVoice(xaudio,
						   &xaudioMasteringVoice,
						   channels,
						   sample_rate,
						   0,
						   0,
						   NULL,
						   AudioCategory_GameEffects
						   );
  if (FAILED(comResult)) {
    return false;
  }

  return true;
}

XAUDIO_DEF bool xaudio_device_init(XAudio2Device *device, const WAVEFORMATEX *pSourceFormat) {

  HRESULT  comResult;
  *device = (XAudio2Device) {0};
  
  comResult = xaudio->lpVtbl->CreateSourceVoice(xaudio,
					 &device->sourceVoice,
					 pSourceFormat,
					 0,
					 1.0f,
					 &xaudio_xAudioCallbacks,
					 NULL,
					 NULL
					 );
  if(FAILED(comResult)) {
    return false;
  }

  device->sourceVoice->lpVtbl->Start(device->sourceVoice, 0, XAUDIO2_COMMIT_NOW);
  //IXAudio2SourceVoice_Start(xaudioSourceVoice, 0, XAUDIO2_COMMIT_NOW);

  device->semaphore = CreateSemaphore(NULL, 0, 1, NULL);
  ReleaseSemaphore(device->semaphore, 1, NULL);
  
  return true;
}

XAUDIO_DEF void xaudio_device_play(XAudio2Device* device, unsigned char* data, UINT32 size) {
  XAUDIO2_BUFFER xaudioBuffer = {0};
  
  xaudioBuffer.AudioBytes = size;
  xaudioBuffer.pAudioData = data;
  xaudioBuffer.pContext = device;
  //xaudioBuffer.Flags = XAUDIO2_END_OF_STREAM;

  device->sourceVoice->lpVtbl->SubmitSourceBuffer(device->sourceVoice, &xaudioBuffer, NULL);
  //IXAudio2SourceVoice_SubmitSourceBuffer(xaudioSourceVoice, &xaudioBuffer, NULL);
  WaitForSingleObject(device->semaphore, INFINITE);
}

XAUDIO_DEF void xaudio_device_free(XAudio2Device *device) {

  CloseHandle(device->semaphore);
  
  device->sourceVoice->lpVtbl->Stop(device->sourceVoice, 0, 0);
  device->sourceVoice->lpVtbl->DestroyVoice(device->sourceVoice);
 
}
#endif //XAUDIO_IMPLEMENTATION

#endif //_WIN32
#endif //XAUDIO_H_H_
