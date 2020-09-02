#ifndef LIBHWAIO_GLOBAL_H
#define LIBHWAIO_GLOBAL_H

#include <stdint.h>

typedef enum _PgiaConfig
{
    PGIA_24_48V,
    PGIA_0_64V,
    PGIA_1_28V,
    PGIA_2_56V,
    PGIA_5_12V,
    PGIA_10_24V
}PgiaConfig;

typedef struct _InputInfo{
    float pgia;
} InputInfo;

#ifdef __cplusplus
extern "C" {
#endif
int OpenAio(const char* strDev, void** handle);
int GetAioVersion(void* handle, char* pStr, int iLength, int* pActLength);
int CloseAio(void* handle);

int SetAiLevelConfig(void* handle, PgiaConfig pgia, int levelDelay, int levelAvg);
int GetAiLevel(void* handle, int arr[32]);
int GetAiLevelInfo(void* handle, InputInfo *info);
int SetAiLevelAutoConfigPgia(void *handle);

int SetAiWaveFormConfig(void* handle, PgiaConfig pgia, uint32_t channel);
int SetAiWaveFormFileName(void* handle, char* FilePath, uint32_t length);
int GetAiWaveFormStatus(void* handle, uint32_t* buf_ddr_len, uint32_t* buf_total_read);

int SetAoLevel(void* handle, uint32_t channel, uint32_t vol);
int SetAoEnable(void* handle, int is_en, int dac_num);
int ClearAoOverProtectionStatus(void* handle);
//pps
int SetAioPpsUpdateCount(void* handle, uint32_t sec);
int GetAioWaveCaptureTime(void* handle, uint64_t *mic_sec);
int GetAioPpsSecCount(void* handle, uint32_t *sec);
int SetAioPpsExportEnable(void* handle, int en);
int SetAioPpsEdgeSel(void* handle, int edge);
#ifdef __cplusplus
}
#endif

#endif // LIBHWAIO_GLOBAL_H
