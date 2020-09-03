#include "Wave.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include<sys/stat.h>

#define SIZE 1024
Wave::Wave()
{

    //waveToData("/home/music.wav", "/home/data");
    //usleep(10000);
    //onverToWav("/home/48k_0_9v", 1, 48000,"/home/test.wav");
    //usleep(1000);
    //waveToData("/home/test.wav", "/home/data");
}

Wave::~Wave()
{
}

long Wave::getFileSize(char *fileName)
{
    FILE *fp = fopen(fileName, "r");
    if(!fp)
        return -1;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fclose(fp);
    return size;
}

int Wave::dataToWav(const char *dataPath, int channels, int sample_rate, const char *wavePath)
{
    int bits = 16;
    RIFF_t m_riff;
    FMT_t m_fmt;
    DATA_t m_data;
    Wav wav;
    FILE *fp, *fpout;

    fp = fopen(dataPath, "rb");
    if(NULL == fp){
        perror("file open failed\n");
        return -1;
    }
    long dataSize = getFileSize((char*)dataPath);
    //WAVE_RIFF
    memcpy(m_riff.ChunkID, "RIFF", strlen("RIFF"));
    m_riff.ChunkSize = 44 + dataSize;
    memcpy(m_riff.Format, "WAVE", strlen("WAVE"));

    fpout = fopen(wavePath, "wb");
    if(NULL == fpout){
        perror("file open failed\n");
        return -1;
    }
    //fwrite(&m_riff,sizeof(m_riff),1, fpout);
    wav.riff = m_riff;

    //WAVE_FMT
    m_fmt.NumChannels = channels;
    m_fmt.SampleRate = sample_rate;
    m_fmt.BitsPerSample = bits;
    m_fmt.ByteRate = sample_rate * channels * m_fmt.BitsPerSample / 8;
    memcpy(m_fmt.Subchunk1ID, "fmt ", strlen("fmt "));
    m_fmt.Subchunk1Size = 16;
    m_fmt.AudioFormat = 3;
    m_fmt.BlockAlign = channels * m_fmt.BitsPerSample / 8;
    //fwrite(&m_fmt, sizeof(m_fmt), 1, fpout);
    wav.fmt = m_fmt;

    //WAVE_DATA
    memcpy(m_data.Subchunk2ID, "data", strlen("data"));
    m_data.Subchunk2Size = dataSize;
    //fwrite(&m_data, sizeof(m_data), 1, fpout);
    wav.data = m_data;
    fwrite(&wav, 1, sizeof(wav), fpout);
    char *buff = (char*)malloc(512);
    int len;
    while((len = fread(buff, sizeof(char), 512,fp)) != 0)
    {
        fwrite(buff, sizeof(char),len,fpout);
    }
    free(buff);
    fclose(fp);
    fclose(fpout);
    return 0;
}

int Wave::waveToData(char *waveName, char* dataName)
{
    FILE *fp = NULL;
    FILE *fpout = NULL;
    Wav wav;
    RIFF_t riff;
    FMT_t fmt;
    DATA_t data;

    fp = fopen(waveName, "rb");
    if (!fp) {
            printf("can't open audio file\n");
            exit(1);
        }
    fpout = fopen(dataName, "wb");
    if (!fpout) {
            printf("can't open audio file\n");
            exit(1);
        }
    fread(&wav,1,sizeof(wav), fp);

    riff = wav.riff;
    fmt = wav.fmt;
    data = wav.data;

    /**
    *  RIFF
    */

    printf("ChunkID \t\t%c%c%c%c\n", riff.ChunkID[0], riff.ChunkID[1], riff.ChunkID[2], riff.ChunkID[3]);
    printf("ChunkSize \t\t%d\n", riff.ChunkSize);
    printf("Format \t\t\t%c%c%c%c\n", riff.Format[0], riff.Format[1], riff.Format[2], riff.Format[3]);

    printf("\n");

    /**
    * fmt
    */
    printf("Subchunk1ID \t%c%c%c%c\n", fmt.Subchunk1ID[0], fmt.Subchunk1ID[1], fmt.Subchunk1ID[2], fmt.Subchunk1ID[3]);
    printf("Subchunk1Size \t%d\n", fmt.Subchunk1Size);
    printf("AudioFormat \t%d\n", fmt.AudioFormat);
    printf("NumChannels \t%d\n", fmt.NumChannels);
    printf("SampleRate \t\t%d\n", fmt.SampleRate);
    printf("ByteRate \t\t%d\n", fmt.ByteRate);
    printf("BlockAlign \t\t%d\n", fmt.BlockAlign);
    printf("BitsPerSample \t%d\n", fmt.BitsPerSample);

    printf("\n");

    /*
     * data
     */
    printf("blockID \t\t%c%c%c%c\n", data.Subchunk2ID[0], data.Subchunk2ID[1], data.Subchunk2ID[2], data.Subchunk2ID[3]);
    printf("blockSize \t\t%d\n", data.Subchunk2Size);

    printf("\n");
    printf("duration \t\t%d\n", data.Subchunk2Size / fmt.ByteRate);

    /////
//   char *buff = (char*)malloc(1024);
//   int buff[1024] = {0};
    float buff = 0;
    int buf = 0;
    int len = 0;
    while((len = fread(&buff, sizeof(float), 1,fp)) != 0)
    {    
        buf = (int)buff;
        fwrite(&buf, sizeof(int),len,fpout);
       // memset(buff,0,1024);
    }
    //free(buff);
    fclose(fp);
    fclose(fpout);
    return 0;
}

void Wave::getWavHead(int mode, int rate, int length, Wav* wav, bool isInt)
{
    wav->data.Subchunk2Size = length;
    wav->riff.ChunkSize = 36 + wav->data.Subchunk2Size;//36
    memcpy(wav->riff.Format, "WAVE", 4);
    memcpy(wav->riff.ChunkID, "RIFF", 4);
    memcpy(wav->fmt.Subchunk1ID, "fmt ", 4);
    wav->fmt.Subchunk1Size = 16;
    if(isInt){
        wav->fmt.AudioFormat = 1;
    }else{
        wav->fmt.AudioFormat = 3;
    }
    wav->fmt.NumChannels = mode;
    wav->fmt.SampleRate = rate;
    wav->fmt.BitsPerSample = 32;
    wav->fmt.ByteRate = (wav->fmt.SampleRate) * (wav->fmt.NumChannels) * (wav->fmt.BitsPerSample) / 8;
    wav->fmt.BlockAlign = (wav->fmt.NumChannels) * (wav->fmt.BitsPerSample) / 8;
    memcpy(wav->data.Subchunk2ID, "data", 4);
}

int Wave::filecopy(char *file_in, char *file_out)
{
    FILE* fp1 = fopen(file_in, "rb");
        FILE* fp2 = fopen(file_out, "wb");

        if (!fp1 || !fp2) {
            printf("复制文件出错\n");
            return -2;
        }
        char* temp = NULL;
        int size = 0;
        struct stat st;
        stat(file_in, &st);

        //根据文件实际大小开辟空间
        if (st.st_size > SIZE) {
          temp = (char*)malloc(sizeof(char)* SIZE);
          size = SIZE;
        } else {
           temp = (char*)malloc(sizeof(char)* st.st_size +10);
           size = st.st_size + 10;
        }

        int count = 0;
        while (!feof(fp1)) {
            memset(temp, 0, size);
          count = fread(temp, sizeof(char), size, fp1);
            fwrite(temp, sizeof(char), count, fp2);
        }

        free(temp);
        fclose(fp2);
        fclose(fp1);

    return 0;
}

int Wave::WavToMono(char *file_in)
{
    int i = 0;
        int j = 0;
        char File_in[64] = {0};
        char file_out[64] = {0};
        sprintf(File_in, "%s", file_in);
        sprintf(file_out, "%s", "data");
        FILE* fp1 = fopen(file_in, "rb");
        FILE* fp2 = fopen(file_out, "wb");

        if (!fp1 || !fp2) {
            printf("复制文件出错\n");
            return -2;
        }
        char* temp = NULL;
        char* arr = NULL;
        char *head = (char*)malloc(44);
        long int tmp = 0;
        int size = 0;
        struct stat st;
        stat(file_in, &st);

        //根据文件实际大小开辟空间
        if (st.st_size > SIZE) {
          temp = (char*)malloc(sizeof(char)* SIZE);
          arr = (char*)malloc(sizeof(char)* SIZE);
          size = SIZE;
        } else {
           temp = (char*)malloc(sizeof(char)* st.st_size +10 - 44);
           arr = (char*)malloc(sizeof(char)* st.st_size  +10 - 44);
           size = st.st_size + 10;
        }
        int count = 0;
        count = fread(head, sizeof(char), 44, fp1);
        fwrite(head, sizeof(char), 44, fp2);
        while (!feof(fp1)) {
            memset(temp, 0, size);
            memset(arr, 0, size);
            j = 0;
            count = fread(temp, sizeof(char), size, fp1);
            for(i = 0; i < count; i++){
                if(i % 8 < 4){
                arr[j] = temp[i];
                j++;
                }
            }
            fwrite(arr, sizeof(char), j, fp2);
        }
        free(head);
        free(temp);
        fclose(fp2);
        fclose(fp1);
        filecopy(file_out, File_in);
    return 0;

}

char *Wave::GetFileName(const char *fullPathName)
{
    char* saveName, *pos;
    int nameLen = -1;
    nameLen = strlen(fullPathName);
    pos = (char*)fullPathName + nameLen;
    while(*pos != '/' && pos != fullPathName)
        pos --;
    if (pos == fullPathName)
    {
        saveName = (char*)fullPathName +1;
        return saveName;
    }
    nameLen = nameLen - (pos - fullPathName);
    saveName = (char*)malloc(nameLen + 1);
    memcpy(saveName, pos+1, nameLen);
    return saveName;
}

int Wave::offsetToData(int *fp)
{
    if (fp == NULL)
        return -1;
    uint32_t chunkId = 0;
    uint32_t chunkIdSize = 0;
    int maxLen = lseek(*fp, 0, SEEK_END);
    lseek(*fp, 12, SEEK_SET);
    read(*fp, (char*)&chunkId, sizeof (uint32_t));
    uint32_t chunkId_Data = MakeFOURCC<'d','a','t','a'>::value;
    int iLen = 0;
    ///search "data" chunkid
    for(;
        chunkId != chunkId_Data && iLen <= maxLen;
        read(*fp, (char*)&chunkId, sizeof (uint32_t)))
    {
        read(*fp, (char*)&chunkIdSize, sizeof (uint32_t));
        iLen = lseek(*fp, chunkIdSize, SEEK_CUR);
    }
    if (chunkId == chunkId_Data)
    {
        lseek(*fp, 8, SEEK_CUR);
    }
}
