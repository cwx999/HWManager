#include "LibHWAio_global.h"
#include "LibHWAio.h"
#include <string>
#include <string.h>

#define GET_IN_DEV(handle)                              \
    LibHWAio *dev = (LibHWAio *)(handle);             \
    if (NULL == (dev) || AIO_DEVS::MAGIC_IN != (dev)->getMagicNum()) \
    {                                                     \
        return -1;                    \
    }

#define GET_OUT_DEV(handle)                              \
    LibHWAio *dev = (LibHWAio *)(handle);             \
    if (NULL == (dev) || AIO_DEVS::MAGIC_OUT != (dev)->getMagicNum()) \
    {                                                     \
        return -1;                    \
    }

#define GET_IN_OUT_DEV(handle)                              \
    LibHWAio *dev = (LibHWAio *)(handle);             \
    if (NULL == (dev) || (!(AIO_DEVS::MAGIC_IN == (dev)->getMagicNum()|| AIO_DEVS::MAGIC_OUT == (dev)->getMagicNum()))) \
    {                                                     \
        return -1;                    \
    }

int SetAiLevelConfig(void *handle, PgiaConfig pgia, int levelDelay, int levelAvg)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setLevelInputConfig((LibHWAio::PGIACONFIG)pgia, levelDelay, levelAvg);
    return status;
}

int GetAiLevel(void *handle, int arr[])
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->getLevelInput(arr);
    return status;
}

int SetAiWaveFormConfig(void *handle, PgiaConfig pgia, uint32_t channel)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setWaveformConfig((LibHWAio::PGIACONFIG)pgia, channel);
    return status;
}

int SetAiWaveFormFileName(void *handle, char *FilePath ,  uint32_t length)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->getWaveformInput(FilePath, length);
    return status;
}

int GetAiWaveFormStatus(void *handle, uint32_t *buf_ddr_len, uint32_t *buf_total_read)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->getWaveformInputStatus(buf_ddr_len, buf_total_read);
    return status;
}

int SetAoLevel(void *handle,  uint32_t channel,  uint32_t vol)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setLevelOutput( channel, vol);
    return status;
}

int ClearAoOverProtectionStatus(void *handle)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setOverCurrentProtect();
    return status;
}
int GetAioVersion(void *handle, char* pStr, int iLength, int* pActLength)
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

int OpenAio(const char *strDev, void **handle)
{
    int ret = 0;
    if (NULL == strDev)
    {
        return -1;
    }
    LibHWAio* pDev = NULL;
    if (LibHWAio::verifyDevice("^/dev/aio_([1-2][0-9]|[0-9])_in$", strDev))
    {
        pDev = new LibHWAio(strDev, LibHWAio::DEV_IN);
    }else if (LibHWAio::verifyDevice("^/dev/aio_([1-2][0-9]|[0-9])_out$", strDev))
    {
        pDev = new LibHWAio(strDev, LibHWAio::DEV_OUT);
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

int CloseAio(void *handle)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);

    delete dev;
    dev = NULL;
    handle = NULL;

    return status;
}

int SetAoEnable(void* handle, int is_en, int dac_num)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setLevelOutputEnable(is_en, dac_num);
    return status;
}

int GetAiLevelInfo(void* handle, InputInfo *info)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->getLevelInputInfo(info);
    return status;
}

int SetAioPpsUpdateCount(void* handle, uint32_t sec)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->setPpsUpdateCount(sec);
    return status;
}

int GetAioWaveCaptureTime(void* handle, uint64_t *mic_sec)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->getWaveCaptureTime(mic_sec);
    return status;
}

int GetAioPpsSecCount(void* handle, uint32_t *sec)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->getPpsSecCount(sec);
    return status;
}

int SetAioPpsExportEnable(void* handle, int en)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->setPpsExportEnable(en);
    return status;
}

int SetAioPpsEdgeSel(void* handle, int edge)
{
    int status = 0;
    GET_IN_OUT_DEV(handle);
    status = dev->setPpsEdgeSel(edge);
    return status;
}

int SetAiLevelAutoConfigPgia(void *handle)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setlevelInputAutoConfigPgia();
    return status;
}
