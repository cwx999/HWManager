#ifndef LIBHWDIO_H
#define LIBHWDIO_H
#include <vector>
#include <map>
#include <array>
#include <CommDef.h>
#include <LibHWDio_global.h>
namespace DIO_DEVS {
const int MAGIC_IN = 0X1234;
const int MAGIC_OUT = 0X4321;
}
///
/// \brief The DIO card
///
class LibHWDio
{
public:
    ///输入配置
    struct DIConfigure
    {

    };
    ///输出配置
    struct DOConfigure
    {

    };
    enum DevOriention
    {
        DEV_INVALID = 0,
        DEV_IN,
        DEV_OUT,
        DEV_IN_OUT
    };
public:
    explicit LibHWDio(std::string strDev, DevOriention eOrient);
    virtual ~LibHWDio();
    ///设备初始
    int initDevs();
    int getVersion(std::string& str);
    int uninitDevs();
    ///使能
    int setEnableStatus(int iChannel, EnableStatus eStatus = UNABLE_STATUS);
    int setEnableStatus(unsigned int iStatus);
    int getEnableStatus(unsigned int &iStatus);
    ///输出模式
    int setOutputMode(int iChannel, OutputMode eMode);
    int getOutputMode(unsigned int &iHiMode, unsigned int &iLoMode);
    int getOutputModes(std::vector<OutputMode>& arrMode);
    ///输入电平
    int setInputReferenceVoltage(int iChannel, int iVoltage);
    int setInputReferenceVoltageByGroup(int iGroup, int iVoltage);
    int getInputLevel(std::map<int, VoltageLevel>& mapVoltage);
    int getInputLevel(int& iLevel);
    ///输出电平
    int setOutputLevelConfigure(int iChannel, DOLevelConfigure stDOLevelCfg);
    int setOutputLevel(int iChannel, VoltageLevel eLevel);
    ///输入PWM
    int setPWMCaptureEnableStatus(int iChannel, EnableStatus eStatus = UNABLE_STATUS);
    int setPWMCaptureEnableStatus(unsigned int iStatus);
    int getPWMCaptureEnableStatus(unsigned int& iStatus);
    int setInputPWMConfigure(int iChannel, DIPWMConfigure stDIPWMCfg);
    int getInputPWMPropertyAuto(int iChannel, std::array<PWMProperty, 8> &arrPWMProper);
    int getInputPWMProperty(std::array<PWMProperty, 32>& arrPWMProper);
    ///输出PWM
    int setOutputPWMConfigure(int iChannel, DOPWMConfigure stDOPWMCfg);
    ///
    int clearOverCurrentProtectionStatus();
    ///输出BIT
    int setOutputBitConfigure(int iChannel, DOBitConfigure stDOBitCfg);
    int writeBITData(uint64_t* pArr, int iLength);
    int writeBITFile(std::string strFile);
    int writeBitData(uint32_t* pArr, int iLength, int *iActLen);
    int writeBitFile(const char* strFile);
    ///
    int getInputConfigure(int iChannel, DIConfigure& stCfg);
    int getOuputConfigure(int iChannel, DOConfigure& stCfg);
    ///
    int setOutputDeadZone(unsigned int iPosNs, unsigned int iNegNs);
    int setOutputPWMDutyCalibration(int iChannel, OutputReferenceVoltage eRef, int32_t iValue);
    ///PPS
    int setPpsUpdateCount(uint32_t sec);
    int getBitStreamSendTime(uint64_t *mic_sec);
    int getPpsSecCount(uint32_t *sec);
    int setPpsExportEnable(int en);
    int setPpsEdgeSel(int edge);
    ///
    double getClkCalCoef();
    ///
    int getMagicNum() const;
    static bool verifyDevice(std::string strDes, std::string strSrc);

private:
    void operOverCurrentProtection();
private:
    void setZero2Indication(unsigned int& reg, int pos);
    void setOne2Indication(unsigned int& reg, int pos);
    void setTwoBit2Indication(unsigned int& reg, int pos, int value);
    bool isOutputChannelInvalid(int iChannel);
    bool isInputChannelInvalid(int iChannel);
    std::pair<int, int> calculateHighLowLevelCount(double dFreq, double dDuty);
    int read(int fd, unsigned int* pValue, unsigned int addr);
    int write(int fd, unsigned int* pValue, unsigned int addr);
    bool waitForBytesWritten(int msec);
    int writeData(unsigned int addr, uint8_t *pArr, int iLength);
    int readData(unsigned int addr, uint8_t* pArr, int iLength);
    void initDutyCalibration();
    void initDefaultConfigure();
    void saveDutyCalibration();
    void saveDefaultConfigure();
    bool isEEPRomReadable();
    bool isEEPRomWritable();

private:
    int m_iMagicNum = 0;
    std::map<int, int> m_mapDevsFD;
    int m_iConfigureFD = 0;
    int m_iDMAFD = 0;
    int m_iSlot = -1;
    std::string m_strDev;
    const int MAX_INPUT_CHANNEL_NUM = 32;
    const int MAX_OUTPUT_CHANNEL_NUM = 24;
    const int MCP_NUM = 4;
    char * map_user = NULL;
    char * map_dma = NULL;
    const int MAX_MAP_USER_SIZE = 65536;
    const int MAX_MAP_DMA_SIZE = 0x20000;
    int32_t arrDutyCalibrationVol5[24] = {0};
    int32_t arrDutyCalibrationVol12[24] = {0};
    int32_t arrDutyCalibrationVolRef[24] = {0};
    unsigned int uiDeadZone = 0;
};
#endif // LIBHWDIO_H
