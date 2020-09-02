#ifndef LIBHWDIO_GLOBAL_H
#define LIBHWDIO_GLOBAL_H
#include <stdint.h>
#ifdef __WIN__
#define DLL_API_EXPORT
#ifdef DLL_API_EXPORT
#define HWCARD_API    __declspec(dllexport)
#else
#define HWCARD_API    __declspec(dllimport)
#endif
#else
#define HWCARD_API
#endif

typedef enum _EnableStatus
{
    UNABLE_STATUS = 0,
    ENABLE_STATUS = 1
} EnableStatus;
///输出模式
typedef enum _OutputMode{
    NULL_OUTPUT_MODE = 0x00,
    LEVEL_OUTPUT_MODE = 0x01,
    PWM_OUTPUT_MODE = 0x02,
    BIT_OUTPUT_MODE = 0x03
} OutputMode;
///输出阻抗模式
typedef enum _OutputImpedanceMode
{
    HIGH_IMPEDANCE_MODE = 0,
    PULL_MODE = 0X01,
    PUSH_MODE = 0X02,
    PUSH_AND_PULL_MODE = 0X03
} OutputImpedanceMode;
///电平
typedef enum _VoltageLevel
{
    LOW_LEVEL = 0x00,
    HIGH_LEVEL = 0x01,
    INVALID_LEVEL
} VoltageLevel;
///输出参考电压
typedef enum _OutputReferenceVoltage
{
    OUTPUT_5V_REF = 0X01,
    OUTPUT_12V_REF = 0X02,
    OUTPUT_OUTSIDE_REF = 0X03
} OutputReferenceVoltage;
typedef enum _ReferenceClock
{
    REFCLK_100_MHZ = 0,
    REFCLK_20_MHZ = 1
} ReferenceClock;
///输出电平配置结构
typedef struct _DOLevelConfigure
{
    OutputImpedanceMode eMode;
    OutputReferenceVoltage eRef;
} DOLevelConfigure;
typedef struct _DOPWMConfigure
{
    double dFreq;
    double dDuty;
    OutputImpedanceMode eMode;
    OutputReferenceVoltage eRef;
} DOPWMConfigure;
typedef struct _DIPWMConfigure
{
    double dDurationTime;
    ReferenceClock eRefClk;
} DIPWMConfigure;
typedef struct _PWMProperty
{
    double dFreq;
    double dDuty;
} PWMProperty;
typedef struct _DOBitConfigure
{
    OutputImpedanceMode eMode;
    long lRate;
    OutputReferenceVoltage eRef;
} DOBitConfigure;
const int DIO_MAX_CHANNEL_NUM = 32;
const int DIO_CHANNEL_PER_GROUP_NUM = 8;
#ifdef __cplusplus
extern "C" {
#endif

int OpenDio(const char* strDev, void **handle);
int GetDioVersion(void *handle, char* pStr, int iLength, int* pActLength);
int CloseDio(void* handle);

int SetDoChannelEnable(void *handle, int iChannel, EnableStatus eStatus);
int SetDoAllChannelEnable(void *handle, unsigned int iStatus);
int GetDoAllChannelEnable(void *handle, unsigned int* pStatus);
int SetDoMode(void *handle, int iChannel, OutputMode eMode);
int GetDoAllMode(void *handle, unsigned int* pHiMode, unsigned int* pLoMode);
int SetDiReferenceVoltage(void *handle, int iChannelGroup, int iVoltage);
int GetDiLevel(void *handle, int iChannel, int* iVoltage);
int GetDiAllLevel(void *handle, int* iLevel);
int SetDoLevelConfigure(void *handle, int iChannel, DOLevelConfigure stDOLevelCfg);
int SetDoLevel(void *handle, int iChannel, VoltageLevel eLevel);
int SetDiPWMCaptureEnable(void *handle, int iChannel, EnableStatus eStatus);
int SetDiPWMAllCaptureEnable(void *handle, unsigned int iStatus);
int GetDiPWMAllCaptureEnable(void *handle, unsigned int* pStatus);
int SetDoPWMConfigure(void *handle, int iChannel, DOPWMConfigure stDOPWMCfg);

int GetDiPWMAutoCapture(void *handle, int iRefChannel, PWMProperty arrPWMProper[8]);
int SetDiPWMConfigure(void *handle, int iChannel, DIPWMConfigure stDIPWMCfg);
int GetDiPWMCapture(void *handle, PWMProperty arrPWMProper[32]);

int ClearDoOverProtectionStatus(void *handle);

int SetDoBitConfigure(void *handle, int iChannel, DOBitConfigure stDOBitCfg);
int WriteDoBitData(void *handle, uint32_t* pArr, int iLength, int *iActLen);
int WriteDoBitFile(void *handle, const char* strFile);

int SetDoDeadZone(void *handle, unsigned int iPosNs, unsigned int iNegNs);

int SetDoPWMDutyCalibration(void *handle, int iChannel, OutputReferenceVoltage eRef, int32_t iValue);

//pps
int SetDioPpsUpdateCount(void* handle, uint32_t sec);
int GetDioBitStreamSendTime(void* handle, uint64_t *mic_sec);
int GetDioPpsSecCount(void* handle, uint32_t *sec);
int SetDioPpsExportEnable(void* handle, int en);
int SetDioPpsEdgeSel(void* handle, int edge);

#ifdef __cplusplus
}
#endif

#endif // LIBHWDIO_GLOBAL_H
