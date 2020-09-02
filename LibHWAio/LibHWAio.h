#ifndef LIBHWAIO_H
#define LIBHWAIO_H

#include "LibHWAio_global.h"
#include <map>
#include <array>
#include "CommDef.h"
namespace AIO_DEVS {
const int MAGIC_IN = 0X2345;
const int MAGIC_OUT = 0X5432;
}
class LibHWAio
{
public:
    enum EnableStatus
    {
        UNABLE_STATUS = 0,
        ENABLE_STATUS = 1
    };
    enum PGIACONFIG
    {
        PGIA_24_48V ,
        PGIA_0_64V ,
        PGIA_1_28V ,
        PGIA_2_56V ,
        PGIA_5_12V ,
        PGIA_10_24V

    };
    enum WAVEFILE{
        WAVE1,
        WAVE2,
        WAVE3,
        WAVE4
    };
    enum DevOriention
    {
        DEV_INVALID = 0,
        DEV_IN,
        DEV_OUT,
        DEV_IN_OUT
    };
public:
    explicit LibHWAio(std::string strDev, DevOriention eOrient);
    virtual ~LibHWAio();
    ///设备初始
    int initDevs();
    int getVersion(std::string& str);
    int uninitDevs();

    uint32_t dmaRecv(uint32_t length, uint32_t addr);
    uint32_t dmaSend(uint32_t length, char *buffer, uint32_t addr);
    void registerWrite(uint32_t addr, uint32_t value);
    int registerRead(uint32_t addr);
    //input
    int setLevelInputConfig(PGIACONFIG pgia, int levelDelay, int levelAvg);
    int getLevelInput(int* arr);
    int getLevelInputInfo(InputInfo * info);
    //auto pgia
    int setlevelInputAutoConfigPgia();
    //wave
    int setWaveformConfig(PGIACONFIG pgia, uint32_t channel);
    int getWaveformInput(char* FilePath,  uint32_t length);
    int getWaveformInputStatus(uint32_t *buf_ddr_len, uint32_t *buf_total_len);
    //output
    int setLevelOutput(uint32_t channel, uint32_t vol);
    int setLevelOutputEnable(int is_en, int dac_num);
    int setOverCurrentProtect();
    //PPS
    int setPpsUpdateCount(uint32_t sec);
    int getWaveCaptureTime(uint64_t *mic_sec);
    int getPpsSecCount(uint32_t *sec);
    int setPpsExportEnable(int en);
    int setPpsEdgeSel(int edge);

    static bool verifyDevice(std::string strDes, std::string strSrc);
    int getMagicNum() const;

private:
    int m_iMagicNum = 0;
    std::map<int, int> m_mapDevsFD;
    int m_iConfigureFD = 0;
    int m_iDMAFD = 0;
    int m_iSlot = -1;
    int m_gain = 0;
    std::string m_strDev;
    uint32_t total_read = 0;
    uint32_t len_avail = 0;
    uint32_t arry[32] = {0};
    const int MAX_CHANNEL = 32;
    const int MCP_NUM = 4;
    char* map_user = NULL;
    char* map_dma = NULL;
    const int MAX_MAP_USER_SIZE = 65536;
    const int MAX_MAP_DMA_SIZE = 0x400000;
private:
    void operOverCurrentProtection();
};
#endif // LIBHWAIO_H
