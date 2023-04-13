#ifndef XAUDIO_H_H_
#define XAUDIO_H_H_

#include <windows.h>
#include <xaudio2.h>

IXAudio2* xaudio;
IXAudio2MasteringVoice* xaudioMasterVoice;
IXAudio2SourceVoice* xaudioSourceVoice;
static HANDLE hSemaphore = 0;

int xaudio_init(const WAVEFORMATEX *pSourceFormat);
void xaudio_play(BYTE* data, UINT32 size);
void xaudio_wait();

#ifdef XAUDIO_IMPLEMENTATION

#pragma comment(lib, "ole32.lib")

void xaudio_OnBufferEnd(IXAudio2VoiceCallback* This, void* pBufferContext) {
  ReleaseSemaphore(hSemaphore, 1, NULL);
}

void xaudio_OnStreamEnd(IXAudio2VoiceCallback* This) {
}
void xaudio_OnVoiceProcessingPassEnd(IXAudio2VoiceCallback* This) { }
void xaudio_OnVoiceProcessingPassStart(IXAudio2VoiceCallback* This, UINT32 SamplesRequired) { }
void xaudio_OnBufferStart(IXAudio2VoiceCallback* This, void* pBufferContext) {}
void xaudio_OnLoopEnd(IXAudio2VoiceCallback* This, void* pBufferContext) { }
void xaudio_OnVoiceError(IXAudio2VoiceCallback* This, void* pBufferContext, HRESULT Error) { }

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

int xaudio_init(const WAVEFORMATEX *pSourceFormat) {
  HRESULT comResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (FAILED(comResult)) {
    return 0;
  }

  comResult = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
  if (FAILED(comResult)) {
    return 0;
  }

  comResult = xaudio->lpVtbl->CreateMasteringVoice(xaudio,
  //comResult = IXAudio2_CreateMasteringVoice(xaudio,
					    &xaudioMasterVoice,
					    pSourceFormat->nChannels,
					    pSourceFormat->nSamplesPerSec,
					    0,
					    0,
					    NULL,//effectChain,
					    AudioCategory_GameEffects
					    );
  if (FAILED(comResult)) {
    return 0;
  }


  comResult = xaudio->lpVtbl->CreateSourceVoice(xaudio,
  //comResult = IXAudio2_CreateSourceVoice(xaudio,
					 &xaudioSourceVoice,
					 pSourceFormat,
					 0,
					 1.0f,
					 &xaudio_xAudioCallbacks,
					 NULL,
					 NULL
					 );
  if(FAILED(comResult)) {
    return 0;
  }

  xaudioSourceVoice->lpVtbl->Start(xaudioSourceVoice, 0, XAUDIO2_COMMIT_NOW);
  //IXAudio2SourceVoice_Start(xaudioSourceVoice, 0, XAUDIO2_COMMIT_NOW);

  hSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
  ReleaseSemaphore(hSemaphore, 1, NULL);
  
  return 1;
}

int xaudio_init_source_voice(const WAVEFORMATEX *pSourceFormat, IXAudio2SourceVoice* sourceVoice) {
  if(FAILED(xaudio->lpVtbl->CreateSourceVoice(xaudio,
					      //comResult = IXAudio2_CreateSourceVoice(xaudio,
					      &sourceVoice,
					      pSourceFormat,
					      0,
					      1.0f,
					      &xaudio_xAudioCallbacks,
					      NULL,
					      NULL
					      ))) {
    return 0;
  }

  return 1;
}

void xaudio_play(BYTE* data, UINT32 size) {
  XAUDIO2_BUFFER xaudioBuffer = {0};
  
  xaudioBuffer.AudioBytes = size;
  xaudioBuffer.pAudioData = data;
  //xaudioBuffer.Flags = XAUDIO2_END_OF_STREAM;

  xaudioSourceVoice->lpVtbl->SubmitSourceBuffer(xaudioSourceVoice, &xaudioBuffer, NULL);
  //IXAudio2SourceVoice_SubmitSourceBuffer(xaudioSourceVoice, &xaudioBuffer, NULL);
  WaitForSingleObject(hSemaphore, INFINITE);
}

void xaudio_wait() {
  WaitForSingleObject(hSemaphore, INFINITE);
}

#endif //XAUDIO_IMPLEMENTATION

#endif //XAUDIO_H_H_
