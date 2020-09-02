#ifndef LIBHWAUDIO_H
#define LIBHWAUDIO_H

#include "LibHWAudio_global.h"
#include <map>
#include <array>
#include <atomic>
#include "CommDef.h"
namespace AUDIO_DEVS {
const int MAGIC_IN = 0X3456;
const int MAGIC_OUT = 0X6543;
const int MAGIC_A2B = 0X4567;
}
///文件信息
struct FileOperStatus
{
    std::string strFile = "";
    int iTotalLength = 0;
    int iActOperLen = 0;
    ///剩余长度
    int getAvalidLen() const
    {
        return iTotalLength - iActOperLen;
    }
};
class  LibHWAudio
{
public:
    enum DevOriention
    {
        DEV_INVALID = 0,
        DEV_IN,
        DEV_OUT,
        DEV_IN_OUT,
        DEV_A2B
    };
    enum DevTransStatus
    {
        DATA_IDLE = 0,
        DATA_STOP,
//        DATA_TRANSACTION
    };
public:
    LibHWAudio(std::string strDev, int index, DevOriention eOrient);
    ~LibHWAudio();
    int initAudioDevs();
    int getAudioVersion(std::string& str);
    int uninitAudioDevs();

    uint32_t dmaRecv(uint32_t length, uint32_t addr);
    uint32_t dmaSend(uint32_t length, char *buffer, uint32_t addr);
    void registerWrite(uint32_t addr, uint32_t value);
    int registerRead(uint32_t addr);
    void regWriteFloat(uint32_t addr, float value);
    float regReadFloat(uint32_t addr);
    //input
    int setInputAtten(int channel, int atten);
    int setInputRate(SAMPLING_RATE sampling_rate);
    int setAudioInputMode(uint32_t channel, int mode);
    int setAudioInputDataType(uint32_t channel, bool isInt);
    int setAudioInputEnable(uint32_t channel, int en, int mode = 0);
    int getAudioInputStatus(uint32_t channel, int * dataSize, int* total_len) const;
    int getAudioInputChannelInfo(uint32_t channel, AudInChlInfo *info);
    int setAudioInputSuspend(uint32_t channel, int isPause);
    //output
    int setOutputEnable(uint32_t channel, int en, int mode = 0);
    int setOutputMode(uint32_t channel, int mode);
    int setOutptDataType(uint32_t channel, bool isInt);
    int setOutputAMP(uint32_t channel, int amp);
    int setLoopPlay(int channel, int isLoop);
    int setAudioOutputSuspend(uint32_t channel, int isPause);
    int getAudioOutputChannelInfo(uint32_t channel, AudOutChlInfo *info);
    //A2B
    int A2BBaseRead(uint32_t addr);
    void A2BBaseWrite(uint32_t addr, int value);
    int A2BBusRead(uint32_t addr);
    void A2BBusWrite(uint32_t addr, int value);
    //
    int A2BMasterAwaken();
    int A2BBoardMode(int mode);
    int A2BConfigMask();
    int A2BInitSlave(A2BRATE mRate);
    int A2BInitMaster();

    int A2BOneSlave(A2BRATE mRate);
    int A2BTXMode(A2BTXMODE mode, A2BRATE mRate);
    //A2BTRANS
    void setA2BTxEnable(uint32_t en);
    void setA2BRxEnable(uint32_t en);
    int setA2BRecvcfg(bool isInt, float coef);
    int setA2BSendcfg(bool isInt, float coef, SAMPLING_RATE rate);
    int sendA2BData(char *pBuf, int iLength, int *iActLen);
    int recvA2Bdata(char *pBuf, int iLength, int *iActLen);
    int A2BRecvFile(const char* strFile, int sec);
    int A2BSendFile(const char* strFile);
    ///
    int getMagicNum() const;
    int getChlIndex() const;
    static bool verifyDevice(std::string strDes, std::string strSrc, int& index);
    ///
    int setAudioInputCfg(int iChannel, AudInCfg *cfg);
    int setAudioOutputCfg(int channel, AudOutCfg* cfg);
    int sendAudioData(int iChannel, char *pBuf, int iLength, int *iActLen);
    int recvAudioData(int channel, char *pBuf, int iLength, int* iActLen);
    int sendAudioFile(int iChannel, const char* strFile);
    int recvAudioFile(int iChannel, const char* strFile, int sec);
    int recvAudioUniqueFile(int iChannel, const char* strFile, int sec);
    int sendAudioUniqueFile(int iChannel, const char* strFile);
    int sendAudioContinueFile(int iChannel);
    int recvAudioContinueFile(int iChannel);
    int getInputMode() const;
    int getOutputMode() const;
    int recvAudioUniqueContinueFile(int iChannel);
    int sendAudioUniqueContinueFile(int iChannel);
private:
    void setSendTransStatus(int iChannel, int isPause);
    void setRecvTransStatus(int iChannel, int isPause);
    DevTransStatus getSendTransStatus(int iChannel);
    DevTransStatus getRecvTransStatus(int iChannel);
private:
    int m_iMagicNum = 0;
    std::map<int, int> m_mapDevsFD;
    int m_iConfigureFD = 0;
    int m_iDMAFD = 0;
    int m_iSlot = -1;
    int m_gain = 0;
    int m_audioSam = 0;
    int m_inputMode = -1;   ///1 立体声 0 单声道
    int m_outputMode = -1;  ///1 立体声 0 单声道
    uint32_t total_read[5] = {0};
    uint32_t total_write[5] = {0};
    uint32_t ilen_avail[5] = {0};
    uint32_t olen_avail[5] = {0};
    const int MAX_CHANNEL = 32;
    const int MCP_NUM = 4;
    char* map_user = NULL;
    char* map_dma = NULL;
    char* map_A2B_dma_send = NULL;
    char* map_A2B_dma_recv = NULL;
    const int MAX_MAP_USER_SIZE = 65536;
    const int MAX_MAP_DMA_SIZE = 0x100000;
    bool m_isInt = false;
    bool m_a2bisInt = false;
    int m_index = -1;
    std::string m_strDev;
    FileOperStatus m_stRecvFileStatus[5];
    FileOperStatus m_stSendFileStatus[5];

    DevTransStatus m_stSendStatus[5] = {DATA_IDLE};
    DevTransStatus m_stRecvStatus[5] = {DATA_IDLE};
    std::atomic_flag m_recvlock[5] = {ATOMIC_FLAG_INIT};
    std::atomic_flag m_sendlock[5] = {ATOMIC_FLAG_INIT};
};
#endif // LIBHWAUDIO_H
