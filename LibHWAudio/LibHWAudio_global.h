#ifndef LIBHWAUDIO_GLOBAL_H
#define LIBHWAUDIO_GLOBAL_H

#include <stdint.h>

typedef enum _SAMPLING_RATE{
    F48kHz ,
    F96kHz  ,
    F192kHz
}SAMPLING_RATE;

typedef enum _A2BRATE{
    R48k_400KHZ,
    R48k_100KHZ,
    R44_1k_400KHZ,
    R44_1k_100KHZ
}A2BRATE;

typedef enum _A2BTXMODE{
    SINGLE,
    MASTERT,
    SLAVE0,
    SLAVE1
}A2BTXMODE;

typedef enum _ERRORTYPE{
    OPEN_ERR = -1,
    WRITE_ERR = -2,
    READ_ERR = -3,
    DDR_EMPTY_ERR = -4,
    INPUT_MODE_ERR = -5,
    OUTPUT_MODE_ERR = -6,
    AWAKEN_ERR = -7,
    A2B_CONFIG_ERR = -8,
} ERRORTYPE;

typedef enum _AudChlStatus{
    FREE,
    PAUSE,
    COLLECTION,
    PLAY,
}ChlStatus;

typedef struct _AudChlInfo{
  int mode;
  int atten;
  int sampling;
  int en;
  int fileSize;
  int total_len;
  char filename[64];
  ChlStatus status;
} AudInChlInfo, AudOutChlInfo;

typedef struct _AudInCfg
{
    float in_coef;
    int IS_IEPE;
} AudInCfg;

typedef struct _AudOutCfg
{
    uint32_t regData;
    float out_coef;
    SAMPLING_RATE rate;
} AudOutCfg;
#ifdef __cplusplus
extern "C" {
#endif
int OpenAudio(const char* strDev, void** handle);
int GetAudioVersion(void* handle, char* pStr, int iLength, int* pActLength);
int CloseAudio(void* handle);
//input
int SetAudiAtten(void* handle, int atten);
int SetAudiAllRate(void* handle, SAMPLING_RATE samp);
int SetAudiMode(void *handle, int mode);
int SetAudiDataType(void *handle, int isInt);
int SetAudiEnable(void *handle, int en);
int GetAudiStatus(void *handle, int*fileSize, int* total_len);
int GetAudiChannelInfo(void *handle, AudInChlInfo *info);
int RecvAudiData(void *handle, char* pBuf, int iLength, int* iActLen);
int RecvAudiFile(void *handle, const char* strFile, int sec);
int SetAudiCfg(void* handle, AudInCfg* cfg);
int SetAudiSuspend(void *handle, int isPause);
//output
int SetAudoEnable(void *handle, int en);
int SetAudoMode(void *handle, int mode);
int SetAudoDataType(void *handle, int isInt);
int SetAudoAMP(void *handle, int amp);
int SetAudoLoopPlay(void *handle, int isLoop);
int SendAudoData(void* handle, char* pBuf, int iLength, int*pActLength);
int SendAudoFile(void* handle, const char* strFile);
int SetAudoCfg(void* handle, AudOutCfg* cfg);
int SetAudoSuspend(void *handle, int isPause);
int GetAudoChannelInfo(void *handle, AudOutChlInfo *info);
//A2B
int SetA2BBoardMode(void *handle, int mode);
int SetA2BTxMode(void *handle, A2BTXMODE mode, A2BRATE mRate);

//STREAM
int SetA2BRecvCfg(void *handle, int isInt, float coef);
int SetA2BSendCfg(void *handle, int isInt, float coef, SAMPLING_RATE rate);
int SendA2BData(void *handle, char *pBuf, int iLength, int *iActLen);
int RecvA2Bdata(void *handle, char *pBuf, int iLength, int *iActLen);
int RecvA2BFile(void *handle, const char* strFile, int sec);
int SendA2BFile(void *handle, const char* strFile);

#ifdef __cplusplus
}
#endif



#endif // LIBHWAUDIO_GLOBAL_H
