#ifndef WAV_H_H
#define WAV_H_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char riffHeader[4];
    int fileSize;
    char waveHeader[4];
    char fmtHeader[4];
    int fmtChunkSize;
    short audioFormat;
    short numChannels;
    int sampleRate;
    int byteRate;
    short blockAlign;
    short bitsPerSample;
} WavHeader;

bool wav_load(const char *file_path,
	      WAVEFORMATEX *waveFormat,
	      char **data,
	      int *data_size);
bool wav_load_format(const char *file_path,
		     WAVEFORMATEX *waveFormat,
		     int *data_size,
		     FILE **file);
void wav_write(const char* file_path, const WAVEFORMATEX* format, const uint8_t* buffer, const uint32_t buffer_size);

#ifdef WAV_IMPLEMENTATION

bool wav_load(const char *file_path,
	      WAVEFORMATEX *waveFormat,
	      char **data,
	      int *data_size) {
  FILE* wavFile;
  WavHeader header;

  // Open the WAV file
  wavFile = fopen(file_path, "rb");
  if (!wavFile) {
    printf("Error opening WAV file.\n");
    return false;
  }

  // Read the WAV file header
  fread(&header, sizeof(WavHeader), 1, wavFile);

  waveFormat->wFormatTag = WAVE_FORMAT_PCM; // or header.audioFormat for non-PCM formats
  waveFormat->nChannels = header.numChannels;
  waveFormat->nSamplesPerSec = header.sampleRate;
  waveFormat->nAvgBytesPerSec = header.byteRate;
  waveFormat->nBlockAlign = header.blockAlign;
  waveFormat->wBitsPerSample = header.bitsPerSample;
  waveFormat->cbSize = 0;

  fseek(wavFile, sizeof(WavHeader), SEEK_SET); // Skip the header
  *data_size = header.fileSize - sizeof(WAVEFORMATEX);
  *data = (char*) malloc(*data_size);
  if (!(*data)) {
    printf("Error allocating memory.\n");
    fclose(wavFile);
    return false;
  }
  fread(*data, *data_size, 1, wavFile);

  // Close the WAV file
  fclose(wavFile);
  return true;
}

bool wav_load_format(const char *file_path,
		     WAVEFORMATEX *waveFormat,
		     int *data_size,
		     FILE **file) {
  *file = fopen(file_path, "rb");
  if (!(*file)) {
    printf("Error opening WAV file.\n");
    return false;
  }

  WavHeader header;
  fread(&header, sizeof(WavHeader), 1, *file);

  waveFormat->wFormatTag = WAVE_FORMAT_PCM; // or header.audioFormat for non-PCM formats
  waveFormat->nChannels = header.numChannels;
  waveFormat->nSamplesPerSec = header.sampleRate;
  waveFormat->nAvgBytesPerSec = header.byteRate;
  waveFormat->nBlockAlign = header.blockAlign;
  waveFormat->wBitsPerSample = header.bitsPerSample;
  waveFormat->cbSize = 0;

  fseek(*file, sizeof(WavHeader), SEEK_SET); // Skip the header

  *data_size = header.fileSize - sizeof(WAVEFORMATEX);

  return true;
}

void wav_write(const char* file_path, const WAVEFORMATEX* format, const uint8_t* buffer, const uint32_t buffer_size) {
    // Open the file for writing
    FILE* file = fopen(file_path, "wb");
    if (file == NULL) {
        perror("Failed to open file for writing");
        return;
    }
    
    // Write the WAV file header
    char header[44] = {0};
    
    memcpy(header, "RIFF", 4);
    *((uint32_t*)(header + 4)) = 36 + buffer_size;
    memcpy(header + 8, "WAVE", 4);
    
    memcpy(header + 12, "fmt ", 4);
    *((uint32_t*)(header + 16)) = 16;
    *((uint16_t*)(header + 20)) = format->wFormatTag;
    *((uint16_t*)(header + 22)) = format->nChannels;
    *((uint32_t*)(header + 24)) = format->nSamplesPerSec;
    *((uint32_t*)(header + 28)) = format->nAvgBytesPerSec;
    *((uint16_t*)(header + 32)) = format->nBlockAlign;
    *((uint16_t*)(header + 34)) = format->wBitsPerSample;
    
    memcpy(header + 36, "data", 4);
    *((uint32_t*)(header + 40)) = buffer_size;
    
    // Write the WAV file header to the file
    fwrite(header, 1, sizeof(header), file);
    
    // Write the audio data to the file
    fwrite(buffer, 1, buffer_size, file);
    
    // Close the file
    fclose(file);
}

#endif //WAV_IMPLEMENTATION

#endif //WAV_H_H
