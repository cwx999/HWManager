#include "LibHWDio_global.h"
#include <LibHWDio.h>
#include <string>
#include <string.h>

#define GET_IN_DEV(handle)                              \
    LibHWDio *dev = (LibHWDio *)(handle);             \
    if (NULL == (dev) || DIO_DEVS::MAGIC_IN != (dev)->getMagicNum()) \
    {                                                     \
        return -1;                    \
    }

#define GET_OUT_DEV(handle)                              \
    LibHWDio *dev = (LibHWDio *)(handle);             \
    if (NULL == (dev) || DIO_DEVS::MAGIC_OUT != (dev)->getMagicNum()) \
    {                                                     \
        return -1;                    \
    }

#define GET_IN_OUT_DEV(handle)                              \
    LibHWDio *dev = (LibHWDio *)(handle);             \
    if (NULL == (dev) || (!(DIO_DEVS::MAGIC_IN == (dev)->getMagicNum()|| DIO_DEVS::MAGIC_OUT == (dev)->getMagicNum()))) \
    {                                                     \
        return -1;                    \
    }

int GetDioVersion(void* handle, char* pStr, int iLength, int* pActLength)
{
    int status = 0;
    std::string str;

    GET_IN_OUT_DEV(handle);

    dev->getVersion(str);
    if ( str.size() <= iLength)
    {
        strcpy(pStr, str.c_str());
        *pActLength = str.size();
    }
    else if (str.size() > iLength)
    {
        strcpy(pStr, str.substr(0, iLength).c_str());
        *pActLength = iLength;
    }
    return status;
}
int CloseDio(void *handle)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);

    delete dev;
    dev = NULL;
    handle = NULL;

    return status;
}

int SetDoChannelEnable(void* handle, int iChannel, EnableStatus eStatus)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setEnableStatus(iChannel, eStatus);

    return status;
}

int SetDoMode(void* handle, int iChannel, OutputMode eMode)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setOutputMode(iChannel, eMode);

    return status;
}

int SetDiReferenceVoltage(void* handle, int iChannelGroup, int iVoltage)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setInputReferenceVoltageByGroup(iChannelGroup, iVoltage);
    return status;
}

int GetDiLevel(void* handle, int iChannel, int *iVoltage)
{
    int status = 0;
    GET_IN_DEV(handle);

    std::map<int, VoltageLevel> mapLevel;
    status = dev->getInputLevel(mapLevel);
    auto iter = mapLevel.find(iChannel);
    if (iter!=mapLevel.end())
        *iVoltage = iter->second;

    return status;
}

int GetDiAllLevel(void* handle, int* iLevel)
{
    int status = 0;
    GET_IN_DEV(handle);

    status = dev->getInputLevel(*iLevel);

    return status;
}

int SetDoLevelConfigure(void* handle, int iChannel, DOLevelConfigure stDOLevelCfg)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setOutputLevelConfigure(iChannel, stDOLevelCfg);

    return status;
}

int SetDoLevel(void* handle, int iChannel, VoltageLevel eLevel)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setOutputLevel(iChannel, eLevel);

    return status;
}

int SetDiPWMCaptureEnable(void* handle, int iChannel, EnableStatus eStatus)
{
    int status = 0;
    GET_IN_DEV(handle);

    status = dev->setPWMCaptureEnableStatus(iChannel, eStatus);

    return status;
}

int SetDoPWMConfigure(void* handle, int iChannel, DOPWMConfigure stDOPWMCfg)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setOutputPWMConfigure(iChannel, stDOPWMCfg);

    return status;
}

int GetDiPWMAutoCapture(void *handle, int iRefChannel, PWMProperty arrPWMProper[])
{
    int status = 0;
    GET_IN_DEV(handle);
    std::array<PWMProperty, 8> arr = {0};
    status = dev->getInputPWMPropertyAuto(iRefChannel, arr);
    for (int i = 0; i < DIO_CHANNEL_PER_GROUP_NUM; i++)
    {
        arrPWMProper[i].dDuty = arr[i].dDuty;
        arrPWMProper[i].dFreq = arr[i].dFreq;
    }

    return status;
}

int SetDiPWMConfigure(void* handle, int iChannel, DIPWMConfigure stDIPWMCfg)
{
    int status = 0;
    GET_IN_DEV(handle);

    status = dev->setInputPWMConfigure(iChannel, stDIPWMCfg);

    return status;
}

int GetDiPWMCapture(void* handle, PWMProperty arrPWMProper[])
{
    int status = 0;
    GET_IN_DEV(handle);

    std::array<PWMProperty, 32> arr = {0};
    status = dev->getInputPWMProperty(arr);
    for (int i = 0; i < DIO_MAX_CHANNEL_NUM; i++)
    {
        arrPWMProper[i].dDuty = arr[i].dDuty;
        arrPWMProper[i].dFreq = arr[i].dFreq;
    }

    return status;
}



int SetDoAllChannelEnable(void* handle, unsigned int iStatus)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setEnableStatus(iStatus);

    return status;
}

int SetDiPWMAllCaptureEnable(void* handle, unsigned int iStatus)
{
    int status = 0;
    GET_IN_DEV(handle);

    status = dev->setPWMCaptureEnableStatus(iStatus);

    return status;
}

int GetDoAllChannelEnable(void* handle, unsigned int *pStatus)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->getEnableStatus(*pStatus);

    return status;
}


int GetDoAllMode(void* handle, unsigned int *pHiMode, unsigned int *pLoMode)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->getOutputMode(*pHiMode, *pLoMode);

    return status;
}

int GetDiPWMAllCaptureEnable(void* handle, unsigned int *pStatus)
{
    int status = 0;
    GET_IN_DEV(handle);

    status = dev->getPWMCaptureEnableStatus(*pStatus);

    return status;
}

int ClearDoOverProtectionStatus(void* handle)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->clearOverCurrentProtectionStatus();

    return status;
}

int SetDoBitConfigure(void* handle, int iChannel, DOBitConfigure stDOBitCfg)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setOutputBitConfigure(iChannel, stDOBitCfg);

    return status;
}

void swapr(uint32_t &a, uint32_t &b) {
    a ^= b;
    b ^= a;
    a ^= b;
}

int WriteDoBitData(void* handle, uint32_t *pArr, int iLength, int* iActLen)
{
    int status = 0;
    GET_OUT_DEV(handle);
#if 0
    int len = iLength/2;
    for (int i = 0; i<len;i+=2)
    {
        swapr(pArr[i], pArr[i+1]);
    }
    status = dev->writeBITData((uint64_t *)pArr, iLength/2);
#else
    status = dev->writeBitData(pArr, iLength, iActLen);
#endif
    return status;
}

int SetDoDeadZone(void* handle, unsigned int iPosNs, unsigned int iNegNs)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setOutputDeadZone(iPosNs, iNegNs);

    return status;
}

int SetDoPWMDutyCalibration(void* handle, int iChannel, OutputReferenceVoltage eRef, int32_t iValue)
{
    int status = 0;
    GET_OUT_DEV(handle);

    status = dev->setOutputPWMDutyCalibration(iChannel, eRef ,iValue);

    return status;
}

int OpenDio(const char *strDev, void **handle)
{
    int ret = 0;
    if (NULL == strDev)
    {
        return -1;
    }
    LibHWDio* pDev = NULL;
    if (LibHWDio::verifyDevice("^/dev/dio_([1-2][0-9]|[0-9])_in$", strDev))
    {
        pDev = new LibHWDio(strDev, LibHWDio::DEV_IN);
    }else if (LibHWDio::verifyDevice("^/dev/dio_([1-2][0-9]|[0-9])_out$", strDev))
    {
        pDev = new LibHWDio(strDev, LibHWDio::DEV_OUT);
    }
    if (NULL != pDev)
    {
        ret = pDev->initDevs();
    }
    if (ret != 0)
    {
        delete pDev;
        pDev = NULL;
        *handle = NULL;
    }
    else
    {
        *handle = pDev;
    }
    return ret;
}

int SetDioPpsUpdateCount(void* handle, uint32_t sec)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->setPpsUpdateCount(sec);
    return status;
}

int GetDioBitStreamSendTime(void* handle, uint64_t *mic_sec)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->getBitStreamSendTime(mic_sec);
    return status;
}

int GetDioPpsSecCount(void* handle, uint32_t *sec)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->getPpsSecCount(sec);
    return status;
}

int SetDioPpsExportEnable(void *handle, int en)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->setPpsExportEnable(en);
    return status;
}

int SetDioPpsEdgeSel(void *handle, int edge)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->setPpsEdgeSel(edge);
    return status;
}

int WriteDoBitFile(void *handle, const char *strFile)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->writeBitFile(strFile);
    return status;
}
