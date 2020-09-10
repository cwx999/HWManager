#include "LibHWAudio_global.h"
#include "LibHWAudio.h"
#include "Wave.h"
#include <string>
#include <string.h>

#define GET_IN_DEV(handle)                              \
    LibHWAudio *dev = (LibHWAudio *)(handle);             \
    if (NULL == (dev) || AUDIO_DEVS::MAGIC_IN != (dev)->getMagicNum()) \
    {                                                     \
        return -1;                    \
    }

#define GET_OUT_DEV(handle)                              \
    LibHWAudio *dev = (LibHWAudio *)(handle);             \
    if (NULL == (dev) || AUDIO_DEVS::MAGIC_OUT != (dev)->getMagicNum()) \
    {                                                     \
        return -1;                    \
    }

#define GET_IN_OUT_DEV(handle)                              \
    LibHWAudio *dev = (LibHWAudio *)(handle);             \
    if (NULL == (dev) || (!(AUDIO_DEVS::MAGIC_IN == (dev)->getMagicNum()|| AUDIO_DEVS::MAGIC_OUT == (dev)->getMagicNum()))) \
    {                                                     \
        return -1;                    \
    }

#define GET_IN_OUT_A2B_DEV(handle)                              \
    LibHWAudio *dev = (LibHWAudio *)(handle);             \
    if (NULL == (dev) || (!(AUDIO_DEVS::MAGIC_IN == (dev)->getMagicNum()|| AUDIO_DEVS::MAGIC_OUT == (dev)->getMagicNum() || AUDIO_DEVS::MAGIC_A2B == (dev)->getMagicNum()))) \
    {                                                     \
        return -1;                    \
    }

#define GET_A2B_DEV(handle)                              \
    LibHWAudio *dev = (LibHWAudio *)(handle);             \
    if (NULL == (dev) || AUDIO_DEVS::MAGIC_A2B != (dev)->getMagicNum()) \
    {                                                     \
        return -1;                    \
    }

int OpenAudio(const char* strDev, void** handle)
{
    int ret = 0;
    if (NULL == strDev)
    {
        return -1;
    }
    LibHWAudio* pDev = NULL;
    int index = -1;
    if (LibHWAudio::verifyDevice("^/dev/audio_([1-2][0-9]|[0-9])_in([1-4])$", strDev, index))
    {
        pDev = new LibHWAudio(strDev, index, LibHWAudio::DEV_IN);
    }else if (LibHWAudio::verifyDevice("^/dev/audio_([1-2][0-9]|[0-9])_out([1-4])$", strDev, index))
    {
        pDev = new LibHWAudio(strDev, index, LibHWAudio::DEV_OUT);
    }
    else if (LibHWAudio::verifyDevice("^/dev/audio_([1-2][0-9]|[0-9])_A2B$", strDev, index))
    {
        pDev = new LibHWAudio(strDev, index, LibHWAudio::DEV_A2B);
    }
    if (NULL != pDev)
    {
        ret = pDev->initAudioDevs();
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


int CloseAudio(void* handle)
{
    int status = 0;
    GET_IN_OUT_A2B_DEV(handle);

    delete dev;
    dev = NULL;
    handle = NULL;

    return status;
}

int SetAudiAllRate(void *handle, SAMPLING_RATE samp)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setInputRate(samp);
    return status;
}

int SetAudiMode(void* handle, int mode)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setAudioInputMode(dev->getChlIndex(), mode);
    return status;
}

int SetAudiEnable(void* handle, int en)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setAudioInputEnable(dev->getChlIndex(), en, dev->getInputMode());
    return status;
}

int GetAudiStatus(void* handle, int *fileSize, int *total_len)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->getAudioInputStatus(dev->getChlIndex(), fileSize, total_len);
    return status;
}

int GetAudiChannelInfo(void* handle, AudInChlInfo *info)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->getAudioInputChannelInfo(dev->getChlIndex(), info);
    return status;
}

int GetAudioVersion(void *handle, char *pStr, int iLength, int *pActLength)
{
    int status = 0;
    std::string str;

    GET_IN_OUT_A2B_DEV(handle);

    dev->getAudioVersion(str);
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

int SetAudiAtten(void *handle, int atten)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setInputAtten(dev->getChlIndex(), atten);
    return status;
}

int SetAudoEnable(void* handle, int en)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setOutputEnable(dev->getChlIndex(), en, dev->getOutputMode());
    return status;
}

int SetAudoMode(void* handle, int mode)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setOutputMode(dev->getChlIndex(), mode);
    return status;
}

int SetAudoAMP(void* handle, int amp)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setOutputAMP(dev->getChlIndex(), amp);
    return status;
}

int SetAudoLoopPlay(void *handle, int isLoop)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setLoopPlay(dev->getChlIndex(), isLoop);
    return status;
}

int SetA2BTxMode(void* handle, A2BTXMODE mode, A2BRATE mRate)
{
    int status = 0;
    GET_A2B_DEV(handle);
    status = dev->A2BTXMode(mode, mRate);
    return status;
}

int SetA2BBoardMode(void* handle, int mode)
{
    int status = 0;
    GET_A2B_DEV(handle);
    status = dev->A2BBoardMode(mode);
    return status;
}

int SendAudoData(void *handle, char *pBuf, int iLength, int *pActLength)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->sendAudioData(dev->getChlIndex(), pBuf, iLength, pActLength);
    return status;
}

int RecvAudiData(void *handle, char *pBuf, int iLength, int *iActLen)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->recvAudioData(dev->getChlIndex(), pBuf, iLength, iActLen);
    return status;
}

int SetAudiCfg(void *handle, AudInCfg *cfg)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setAudioInputCfg(dev->getChlIndex(), cfg);
    return status;
}

int SetAudoCfg(void *handle, AudOutCfg *cfg)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setAudioOutputCfg(dev->getChlIndex(), cfg);
    return status;
}

int RecvAudiFile(void *handle, const char *strFile, int sec)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->recvAudioUniqueFile(dev->getChlIndex(), strFile, sec);
    return status;
}

int SendAudoFile(void *handle, const char *strFile)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->sendAudioUniqueFile(dev->getChlIndex(), strFile);
    return status;
}

int SetAudiSuspend(void *handle, int isPause)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setAudioInputSuspend(dev->getChlIndex(), isPause);
    if (!isPause)
    {
        status = dev->recvAudioUniqueContinueFile(dev->getChlIndex());
    }
    return status;
}

int SetAudoSuspend(void *handle, int isPause)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setAudioOutputSuspend(dev->getChlIndex(), isPause);
    if (!isPause)
    {
        status = dev->sendAudioUniqueContinueFile(dev->getChlIndex());
    }
    return status;
}

int SetAudiDataType(void *handle, int isInt)
{
    int status = 0;
    GET_IN_DEV(handle);
    status = dev->setAudioInputDataType(dev->getChlIndex(), isInt == 0 ? false : true);
    return status;
}

int SetAudoDataType(void *handle, int isInt)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->setOutptDataType(dev->getChlIndex(), isInt == 0 ? false : true);
    return status;
}

int SetA2BRecvCfg(void *handle, int isInt, float coef)
{
    int status = 0;
    GET_A2B_DEV(handle);
    status = dev->setA2BRecvcfg(isInt == 0 ? false : true, coef);
    return status;
}

int SetA2BSendCfg(void *handle, int isInt, float coef, SAMPLING_RATE rate)
{
    int status = 0;
    GET_A2B_DEV(handle);
    status = dev->setA2BSendcfg(isInt == 0 ? false : true, coef, rate);
    return status;
}

int SendA2BData(void *handle, char *pBuf, int iLength, int *iActLen)
{
    int status = 0;
    GET_A2B_DEV(handle);
    status = dev->sendA2BData(pBuf, iLength, iActLen);
    return status;
}

int RecvA2Bdata(void *handle, char *pBuf, int iLength, int *iActLen)
{
    int status = 0;
    GET_A2B_DEV(handle);
    status = dev->recvA2Bdata(pBuf, iLength, iActLen);
    return status;
}

int RecvA2BFile(void *handle, const char *strFile, int sec)
{
    int status = 0;
    GET_A2B_DEV(handle);
    status = dev->A2BRecvFile(strFile, sec);
    return status;
}

int SendA2BFile(void *handle, const char *strFile)
{
    int status = 0;
    GET_A2B_DEV(handle);
    status = dev->A2BSendFile(strFile);
    return status;
}

int GetAudoChannelInfo(void *handle, AudOutChlInfo *info)
{
    int status = 0;
    GET_OUT_DEV(handle);
    status = dev->getAudioOutputChannelInfo(dev->getChlIndex(), info);
    return status;
}
