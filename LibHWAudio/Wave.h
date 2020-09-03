#ifndef WAVE_H
#define WAVE_H

//#include <QMainWindow>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <random>

template <char ch0, char ch1, char ch2, char ch3>
struct MakeFOURCC{
    enum { value = (ch0 << 0) + (ch1 << 8) + (ch2 << 16) + (ch3 << 24) };
};

typedef struct WAV_RIFF{
    /* chunk "riff" */
       char ChunkID[4];     /* "RIFF" */
       /* sub-chunk-size */
       uint32_t ChunkSize;  /* 36 + Subchunk2Size */
       /* sub-chunk-data */
       char Format[4];      /* "WAVE" */
}RIFF_t;

typedef struct WAV_FMT{
    /* sub-chunk "fmt" */
       char Subchunk1ID[4];    /* "fmt " */
       /* sub-chunk-size */
       uint32_t Subchunk1Size; /* 16 for PCM */
       /* sub-chunk-data */
       uint16_t AudioFormat;   /* PCM = 1*/
       uint16_t NumChannels;   /* Mono = 1, Stereo = 2, etc. */
       uint32_t SampleRate;    /* 8000, 44100, etc. */
       uint32_t ByteRate;      /* = SampleRate * NumChannels * BitsPerSample/8 */
       uint16_t BlockAlign;    /* = NumChannels * BitsPerSample/8 */
       uint16_t BitsPerSample; /* 8bits, 16bits, etc. */
}FMT_t;

typedef struct WAV_DATA{
    /* sub-chunk "data" */
    char Subchunk2ID[4];       /* "data" */
    /* sub-chunk-size */
    uint32_t Subchunk2Size;     /* data size */
}DATA_t;

typedef struct WAV_FOTMAT{
    RIFF_t riff;
    FMT_t fmt;
    DATA_t data;
}Wav;

class Wave
{

public:
    explicit Wave();
    ~Wave();

    long getFileSize(char* fileName);
    int dataToWav(const char *dataPath, int channels, int sample_rate, const char* wavePath);
    int waveToData(char* waveName, char *dataName);
    void getWavHead(int mode, int rate, int length, Wav *wav, bool isInt);
    int filecopy(char * file_in, char * file_out);
    int WavToMono(char * file_in);
    char *GetFileName(const char* fullPathName);

    int offsetToData(int* fp);

private:

};

#endif // WAVE_H
