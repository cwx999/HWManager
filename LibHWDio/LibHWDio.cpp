#include "LibHWDio.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cmath>
#include <xpdma.h>
#include <string.h>
using namespace DIO_DEVS;
using namespace std;

LibHWDio::LibHWDio(std::string strDev, DevOriention eOrient) : m_strDev(strDev)
{
    switch (eOrient) {
    case DEV_IN:
        m_iMagicNum = MAGIC_IN;
        break;
    case DEV_OUT:
        m_iMagicNum = MAGIC_OUT;
        break;
    default:
        break;
    }
}

LibHWDio::~LibHWDio()
{
    saveDutyCalibration();
    saveDefaultConfigure();
    if (NULL != map_user)
    {
        munmap(map_user, MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE);
    }
    uninitDevs();
}

int LibHWDio::initDevs()
{
    int ret = 0;
    ///1
    if (m_strDev.empty())
    {
        return -1;
    }

    m_iConfigureFD = open(m_strDev.c_str(), O_RDWR | O_SYNC);
    if (m_iConfigureFD < 0)
    {
        return -1;
    }

    map_user = (char*) mmap(NULL, MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_iConfigureFD, 0);

    ///2
    m_iDMAFD = m_iConfigureFD;
    map_dma = (char*) (map_user + MAX_MAP_USER_SIZE);

    ///3
    operOverCurrentProtection();
    ///4
    initDutyCalibration();
    initDefaultConfigure();
    return ret;
}
#include <stdio.h>
int LibHWDio::getVersion(std::string &str)
{
    int ret = 0;
    if (m_iConfigureFD == 0)
        ret = -1;
    unsigned int buf = 0;
    ret = read(m_iConfigureFD, &buf, DATETIME);
    char arr [10] = {0};
    sprintf(arr,"%x", buf);
    str = std::string(arr, 8);

    ret = read(m_iConfigureFD, &buf, VERSION);
    sprintf(arr,"%x", buf);
    str.append(std::string(arr,8));
    return ret;
}

int LibHWDio::uninitDevs()
{
    int ret = 0;
    ret = close (m_iConfigureFD);
    m_iConfigureFD = 0;
    m_iDMAFD = 0;
    return ret;
}
///
/// \brief 通道使能状态
/// \param iChannel
/// \param eStatus
/// \return
///
int LibHWDio::setEnableStatus(int iChannel, EnableStatus eStatus)
{
    int status = SUCCESS_RET;
    unsigned int buf = 0;
    status = read(m_iConfigureFD, &buf, DO_EN);

    if (isOutputChannelInvalid(iChannel))
    {
        return FAILURE_RET;
    }
    switch (eStatus) {

    case ENABLE_STATUS:
    {
        setOne2Indication(buf, iChannel);
    }
        break;
    case UNABLE_STATUS:
    default:
    {
        setZero2Indication(buf, iChannel);
    }
        break;
    }
    status = write(m_iConfigureFD, &buf, DO_EN);
    return status;
}

int LibHWDio::setEnableStatus(unsigned int iStatus)
{
    int status = SUCCESS_RET;
    status = write(m_iConfigureFD, &iStatus, DO_EN);
    return status;
}

int LibHWDio::getEnableStatus(unsigned int &iStatus)
{
    int status = SUCCESS_RET;
    status = read(m_iConfigureFD, &iStatus, DO_EN);
    return status;
}
///
/// \brief 输出模式配置
/// \param iChannel
/// \param eMode
/// \return
///
int LibHWDio::setOutputMode(int iChannel, OutputMode eMode)
{
    int status = SUCCESS_RET;
    if (isOutputChannelInvalid(iChannel))
        return FAILURE_RET;
    setEnableStatus(iChannel);
    unsigned int buf = 0;
    if (iChannel > 15)
    {
        read(m_iConfigureFD, &buf, DO_MODE2);
        setTwoBit2Indication(buf, (iChannel - 16)*2, eMode);
        write(m_iConfigureFD, &buf, DO_MODE2);
    }
    else
    {
        read(m_iConfigureFD, &buf, DO_MODE1);
        setTwoBit2Indication(buf, iChannel *2, eMode);
        write(m_iConfigureFD, &buf, DO_MODE1);
    }
    return status;
}

int LibHWDio::getOutputMode(unsigned int &iHiMode, unsigned int &iLoMode)
{
    int status = SUCCESS_RET;
    status = read(m_iConfigureFD, &iHiMode, DO_MODE2);
    status = read(m_iConfigureFD, &iLoMode, DO_MODE1);
    return status;
}

int LibHWDio::getOutputModes(std::vector<OutputMode> &arrMode)
{
    int status = SUCCESS_RET;
    unsigned int hiMode = 0;
    unsigned int loMode = 0;
    int iMode = 0;
    status = read(m_iConfigureFD, &hiMode, DO_MODE2);
    status = read(m_iConfigureFD, &loMode, DO_MODE1);
    int iLength = arrMode.size() > MAX_OUTPUT_CHANNEL_NUM? MAX_OUTPUT_CHANNEL_NUM : arrMode.size();
    for (int i = 0; i < iLength; i++)
    {
        if (i > 15)
            iMode = hiMode >> (i- 16)*2 & 0x3;
        else
            iMode = loMode >> (i*2) & 0x3;
        arrMode.at(i) = (OutputMode)iMode;
    }
    return status;
}
///
/// \brief LibHWDio::setInputReferenceVoltage
///        4张MCP片子
/// \param iChannel
/// \return
///
int LibHWDio::setInputReferenceVoltage(int iChannel, int iVoltage)
{
    return setInputReferenceVoltageByGroup((int)iChannel/8, iVoltage);
}

int LibHWDio::setInputReferenceVoltageByGroup(int iGroup, int iVoltage)
{
    int status = SUCCESS_RET;
    unsigned int buf = 0;

    ///NOTE::按照组号设置电压
    buf = 0x02;
    write(m_iConfigureFD, &buf, 0x4100);

    buf = 0x01;
    write(m_iConfigureFD, &buf, 0x4100);
    ///BYTE1
    buf = 0x1c0;
    write(m_iConfigureFD, &buf, 0x4108);
    ///BYTE2
    switch(iGroup)
    {
    case 0:
    buf = 0x58;
        break;
    case 1:
    buf = 0x5a;
        break;
    case 2:
    buf = 0x5c;
        break;
    case 3:
    buf = 0x5e;
        break;
    }
    write(m_iConfigureFD, &buf, 0x4108);
    int bufCtl = round((double)iVoltage /(12));

    ///BYTE3
    buf = ((bufCtl& 0xF00) >> 8) | 0X90;
//    buf = 0x81;
    write(m_iConfigureFD, &buf, 0x4108);
    ///BYTE4
    buf = (bufCtl & 0xFF);
//    buf = 0x00;
    write(m_iConfigureFD, &buf, 0x4108);

    buf = 0x200;
    write(m_iConfigureFD, &buf, 0x4108);

    status = read(m_iConfigureFD, &buf, 0X4104);

    status = read(m_iConfigureFD, &buf, 0x410c);
    usleep(30000);  ///用以生效
    return status;
}

///
/// \brief 获取电平
/// \param mapVoltage
/// \return
///
int LibHWDio::getInputLevel(std::map<int, VoltageLevel> &mapVoltage)
{
    int status  = SUCCESS_RET;
    unsigned int buf = 0;
    read(m_iConfigureFD, &buf, DI_IN_LEVEL);
    for (int i = 0; i< MAX_INPUT_CHANNEL_NUM; i++)
    {
        VoltageLevel eLevel = INVALID_LEVEL;
        if (((buf >> i) & 0x01) == 0)
        {
            eLevel = LOW_LEVEL;
        }
        else
        {
            eLevel = HIGH_LEVEL;
        }
        mapVoltage.insert(std::pair<int, VoltageLevel> (i, eLevel));
    }
    return status;
}

int LibHWDio::getInputLevel(int &iLevel)
{
    int status  = SUCCESS_RET;
    status = read(m_iConfigureFD, (unsigned int*)&iLevel, DI_IN_LEVEL);
    return status;
}
///
/// \brief 输出电平配置
/// \param stDOLevelCfg
/// \return
///
int LibHWDio::setOutputLevelConfigure(int iChannel, DOLevelConfigure stDOLevelCfg)
{
    int status = SUCCESS_RET;
    if (isOutputChannelInvalid(iChannel))
    {
        return FAILURE_RET;
    }
    unsigned int buf = 0;
    ///1
    setEnableStatus(iChannel);

    ///2 Obsolote配置默认拉低电平保护输出删除

    ///3
    if (iChannel > 15)
    {
        status = read(m_iConfigureFD, &buf, DO_PULL_PUSH2);

        setTwoBit2Indication(buf, (iChannel-16)*2, stDOLevelCfg.eMode);

        status = write(m_iConfigureFD, &buf, DO_PULL_PUSH2);
    }
    else
    {
        status = read(m_iConfigureFD, &buf, DO_PULL_PUSH1);

        setTwoBit2Indication(buf, (iChannel)*2, stDOLevelCfg.eMode);

        status = write(m_iConfigureFD, &buf, DO_PULL_PUSH1);
    }

    ///4
    buf = stDOLevelCfg.eRef & 0X3;
    status = write(m_iConfigureFD, &buf, DO_REF_VOL0 + (iChannel >> 3) *4);
    return status;
}

int LibHWDio::setOutputLevel(int iChannel, VoltageLevel eLevel)
{
    int status = SUCCESS_RET;
    if (isOutputChannelInvalid(iChannel))
    {
        return FAILURE_RET;
    }
    unsigned int buf = 0;
    read(m_iConfigureFD, &buf, DO_LEVEL);

    switch (eLevel) {
    case LOW_LEVEL:
        setZero2Indication(buf, iChannel);
        break;
    case HIGH_LEVEL:
        setOne2Indication(buf, iChannel);
        break;
    default:
        break;
    }
    status = write(m_iConfigureFD, &buf, DO_LEVEL);
}
///
/// \brief PWM捕获使能
/// \param iChannel
/// \param eStatus
/// \return
///
int LibHWDio::setPWMCaptureEnableStatus(int iChannel, EnableStatus eStatus)
{
    int status = SUCCESS_RET;

    if (isInputChannelInvalid(iChannel))
    {
        return FAILURE_RET;
    }
    unsigned int buf = 0;
    status = read(m_iConfigureFD, &buf, PWM_CAP_EN);
    switch (eStatus) {

    case ENABLE_STATUS:
    {
        setOne2Indication(buf, iChannel);
    }
        break;
    case UNABLE_STATUS:
    default:
    {
        setZero2Indication(buf, iChannel);
    }
        break;
    }
    status = write(m_iConfigureFD, &buf, PWM_CAP_EN);
    return status;
}

int LibHWDio::setPWMCaptureEnableStatus(unsigned int iStatus)
{
    int status = SUCCESS_RET;
    status = write(m_iConfigureFD, &iStatus, PWM_CAP_EN);
    return status;
}

int LibHWDio::getPWMCaptureEnableStatus(unsigned int &iStatus)
{
    int status = SUCCESS_RET;
    status = read(m_iConfigureFD, &iStatus, PWM_CAP_EN);
    return status;
}

int LibHWDio::setOutputPWMConfigure(int iChannel, DOPWMConfigure stDOPWMCfg)
{
    int status = SUCCESS_RET;
    if (isOutputChannelInvalid(iChannel))
        return FAILURE_RET;
    ///NOTE::PWM CONFIGURE
    unsigned int buf = 0;
    /// 1
    setEnableStatus(iChannel);

    ///2
    read(m_iConfigureFD, &buf, DO_PWM_CLKSEL);
    if (stDOPWMCfg.dFreq > 10e3)    ///125MHZ
    {
        setZero2Indication(buf, iChannel);
    }
    else/// 125/4MHZ
    {
        setOne2Indication(buf, iChannel);
    }
    write(m_iConfigureFD, &buf, DO_PWM_CLKSEL);

    ///3
    std::pair<int, int> pairPulse = calculateHighLowLevelCount(stDOPWMCfg.dFreq, stDOPWMCfg.dDuty);
    int highTmp = 0;
    switch (stDOPWMCfg.eRef) {
    case OUTPUT_5V_REF:
        highTmp = arrDutyCalibrationVol5[iChannel];
        break;
    case OUTPUT_12V_REF:
        highTmp = arrDutyCalibrationVol12[iChannel];
        break;
    case OUTPUT_OUTSIDE_REF:
        highTmp = arrDutyCalibrationVolRef[iChannel];
        break;
    default:
        break;
    }
    buf = pairPulse.first + highTmp;
    write(m_iConfigureFD, &buf, PWM_OUT_HI0 + (4*iChannel));

    buf = pairPulse.second - highTmp;
    write(m_iConfigureFD, &buf, PWM_OUT_LO0 + (4*iChannel));

    ///4
    if (iChannel > 15)
    {
        status = read(m_iConfigureFD, &buf, DO_PULL_PUSH2);

        setTwoBit2Indication(buf, (iChannel-16)*2, stDOPWMCfg.eMode);

        status = write(m_iConfigureFD, &buf, DO_PULL_PUSH2);
    }
    else
    {
        status = read(m_iConfigureFD, &buf, DO_PULL_PUSH1);

        setTwoBit2Indication(buf, (iChannel)*2, stDOPWMCfg.eMode);

        status = write(m_iConfigureFD, &buf, DO_PULL_PUSH1);
    }
    ///4
    buf = stDOPWMCfg.eRef & 0X3;
    status = write(m_iConfigureFD, &buf, DO_REF_VOL0 + (iChannel >> 3) *4);
    return status;
}
///
/// \brief 过流保护状态清除
/// \return
///
int LibHWDio::clearOverCurrentProtectionStatus()
{
    int status = SUCCESS_RET;
    unsigned int buf = 0x07;
    status = write(m_iConfigureFD, &buf, DO_OVER_VOL);
    return status;
}
///
/// \brief LibHWDio::setOutputBitConfigure
/// \param stDOBitCfg
/// \return
///
int LibHWDio::setOutputBitConfigure(int iChannel, DOBitConfigure stDOBitCfg)
{
    int status = SUCCESS_RET;
    if (isOutputChannelInvalid(iChannel))
        return FAILURE_RET;
    ///NOTE::BIT CONFIGURE
    unsigned int buf = 0;
    /// 1
    setEnableStatus(iChannel);

    ///2
    if (iChannel > 15)
    {
        status = read(m_iConfigureFD, &buf, DO_PULL_PUSH2);

        setTwoBit2Indication(buf, (iChannel-16)*2, stDOBitCfg.eMode);

        status = write(m_iConfigureFD, &buf, DO_PULL_PUSH2);
    }
    else
    {
        status = read(m_iConfigureFD, &buf, DO_PULL_PUSH1);

        setTwoBit2Indication(buf, (iChannel)*2, stDOBitCfg.eMode);

        status = write(m_iConfigureFD, &buf, DO_PULL_PUSH1);
    }
    ///3
    if (stDOBitCfg.lRate != 0)
    {
        double clk_coef = getClkCalCoef();
        buf = 125e6 * clk_coef/stDOBitCfg.lRate - 1; //125Mhz
        write(m_iConfigureFD, &buf, BITS_CLK_COUNTER);
    }
    ///4
    buf = stDOBitCfg.eRef & 0X3;
    status = write(m_iConfigureFD, &buf, DO_REF_VOL0 + (iChannel >> 3) *4);
    return status;
}

int LibHWDio::writeBITData(uint64_t *pArr, int iLength)
{
    unsigned int buf = 0;
    if (iLength*8 <= MAX_MAP_DMA_SIZE)
    {
        ///1
        memcpy(map_dma, (char*)pArr, iLength * 8);
        ///2
        XdmaSend(m_iDMAFD, iLength * 8, 0);
        ///3
        buf = iLength * 8;
        write(m_iConfigureFD, &buf, STREAM_TX_DATA_LENGTH);

        buf = 0x01;
        write(m_iConfigureFD, &buf, STREAM_TX_START);
        ///4
        while (!waitForBytesWritten(500)) {
            usleep(10);
        }
        buf = 0x01;
        write(m_iConfigureFD, &buf, STREAM_TX_IRQ_CLEAR);
        return 0;
    }
    else
    {
        ///1
        strncpy(map_dma, (char*)pArr, MAX_MAP_DMA_SIZE);
        ///2
        XdmaSend(m_iDMAFD, MAX_MAP_DMA_SIZE, 0);
        ///3
        buf = MAX_MAP_DMA_SIZE;
        write(m_iConfigureFD, &buf, STREAM_TX_DATA_LENGTH);

        buf = 0x01;
        write(m_iConfigureFD, &buf, STREAM_TX_START);
        ///4
        while (!waitForBytesWritten(500)) {
            usleep(10);
        }
        writeBITData(pArr+MAX_MAP_DMA_SIZE, iLength - MAX_MAP_DMA_SIZE/8);
    }
}
///
/// \brief 获取所有输入配置信息
/// \return
///
int LibHWDio::getInputConfigure(int iChannel, DIConfigure& stCfg)
{
    int status = SUCCESS_RET;
    unsigned int buf = 0;
    status = read(m_iConfigureFD, &buf, PWM_CAP_EN);
    return status;
}
///
/// \brief 获取所有输出配置信息
/// \return
///
int LibHWDio::getOuputConfigure(int iChannel, DOConfigure &stCfg)
{
    int status = SUCCESS_RET;
    unsigned int buf = 0;
    status = read(m_iConfigureFD, &buf, DO_PULL_PUSH1);
    status = read(m_iConfigureFD, &buf, DO_PULL_PUSH2);
    return status;
}
///
/// \brief LibHWDio::setOutputDeadZone
/// \param iNanoSecond
/// \return
///
int LibHWDio::setOutputDeadZone(unsigned int iPosNs, unsigned int iNegNs)
{
    int status = SUCCESS_RET;
    unsigned int pos = 0;
    unsigned int neg = 0;
    unsigned int buf = 0;
    ///步进8ns 极限2048ns
    pos = iPosNs / 8 - 1;
    if (pos >=255)   ///MAX value limit
        pos = 255;
    neg = iNegNs / 8 - 1;
    if (neg >= 255)
        neg = 255;
    buf = (pos << 8) | (neg);
    status = write(m_iConfigureFD, &buf, DO_DEADZONE);
    uiDeadZone = buf;
    return status;
}

int LibHWDio::setOutputPWMDutyCalibration(int iChannel, OutputReferenceVoltage eRef, int32_t iValue)
{
    int status = SUCCESS_RET;
    if (isOutputChannelInvalid(iChannel))
    {
        status = FAILURE_RET;
        return status;
    }
    switch (eRef) {
    case OUTPUT_5V_REF:
        arrDutyCalibrationVol5[iChannel] = iValue;
        break;
    case OUTPUT_12V_REF:
        arrDutyCalibrationVol12[iChannel] = iValue;
        break;
    case OUTPUT_OUTSIDE_REF:
        arrDutyCalibrationVolRef[iChannel] = iValue;
        break;
    default:
        break;
    }
    return status;
}
///
/// \brief PWM捕获配置
/// \param stDIPWMCfg
/// \return
///
int LibHWDio::setInputPWMConfigure(int iChannel, DIPWMConfigure stDIPWMCfg)
{
    int status = SUCCESS_RET;
    if (isInputChannelInvalid(iChannel))
        return FAILURE_RET;
    unsigned int buf = 0;
    ///1
    setPWMCaptureEnableStatus(iChannel);

    ///2 Obsolete PWM捕获时钟固定为100MHz
#if 0
    switch (stDIPWMCfg.eRefClk) {
    case REFCLK_100_MHZ:
        setZero2Indication(buf, iChannel);
        break;
    case REFCLK_20_MHZ:
    default:
        setOne2Indication(buf, iChannel);
        break;
    }
    write(m_iConfigureFD, &buf, PWM_CAP_CLKSEL);
#endif /// 100Mhz 6.25Mhz时钟自动判断
    ///3
#if 0
    switch (stDIPWMCfg.eRefClk) {
    case REFCLK_100_MHZ:
        buf = stDIPWMCfg.dDurationTime/10 * 1e6;
        break;
    case REFCLK_20_MHZ:
        buf = stDIPWMCfg.dDurationTime/50 * 1e6;
    default:
        break;
    }
#else
    read(m_iConfigureFD, &buf, PWM_CAP_CLKSEL);
    if ((buf>>iChannel)&0x01 != 0)/// 125M/64= 1.953125M
    {
        buf = (double)stDIPWMCfg.dDurationTime/(512*16384) * 1e6;
    }
    else///125Mhz
    {
        buf = (double)stDIPWMCfg.dDurationTime/(8*16384) * 1e6;
    }
#endif /// 100Mhz 1.953125M时钟自动判断
    write(m_iConfigureFD, &buf, PWM_CAP_DURATION0 + 4 * iChannel);
#if 0
    if (stDIPWMCfg.dDurationTime > 100)
    {
        //1~15
        buf = 15;

    }
    else {
        buf = 4;
    }
    write(m_iConfigureFD, &buf, DI_FILITER);
#endif /// 自动滤波 1~15 2^15-1=65535
    return status;
}

///
/// \brief
/// \param
/// \return
///
#define DELAY_US 5000
#define RETRY_TIMES 8
#define CAP_DURATIONG_TIME 30
int LibHWDio::getInputPWMPropertyAuto(int iChannel, std::array<PWMProperty, 8> &arr)
{
    int status = SUCCESS_RET;
    double clk_coef = getClkCalCoef();
    unsigned int bufHI = 0;
    unsigned int buf = 0;
    unsigned int bufPeriod = 0;
    unsigned int bufCycle = 0;
    unsigned int bufReferenceClk = 0;
    int iRefCk = 0;
    float lastVoltage = 25;
    float currentVoltage = 25;
    int i = 0, j = 0, z = 0;
    unsigned int iGroup = iChannel / 8;

    // set duration time in group
    read(m_iConfigureFD, &bufReferenceClk, PWM_CAP_CLKSEL);
    buf = bufReferenceClk;
    if ((buf >> iChannel) & 0x01 != 0)/// 125M/64= 1.953125M
    {
        buf = (double)CAP_DURATIONG_TIME / (512 * 16384) * 1e6;
    }
    else///125Mhz
    {
        buf = (double)CAP_DURATIONG_TIME / (8 * 16384) * 1e6;
    }
    for(int k = iGroup * 8; k < iGroup * 8 + 8; k++)
    {
        write(m_iConfigureFD, &buf, PWM_CAP_DURATION0 + 4 * k);
    }
    buf = 0;

    // pwm level > 25 ?
    setPWMCaptureEnableStatus(iChannel, UNABLE_STATUS);
    setInputReferenceVoltage(iChannel, currentVoltage * 1000);
    setPWMCaptureEnableStatus(iChannel, ENABLE_STATUS);
    write(m_iConfigureFD, &bufHI, PWM_CAP_HI0 + (4 * iChannel)); // clear PWM_CAP_HI0
    for(int a = 0; a < RETRY_TIMES; a++)
    {
        usleep(DELAY_US);
        read(m_iConfigureFD, &buf, PWM_CAP_DONE);
        if(((buf >> iChannel) & 0x01) == 0)
        {
            continue;
        }
        else
        {
            for(int k = iGroup * 8; k < iGroup * 8 + 8; k++)
            {
                if( (buf >> k) & 0x01 != 0)
                {
                    status = read(m_iConfigureFD, &bufHI, PWM_CAP_HI0 + (4 * k));
                    status = read(m_iConfigureFD, &bufPeriod, PWM_CAP_PERIOD0 + (4 * k));
                    status = read(m_iConfigureFD, &bufCycle, PWM_CAP_CYCLES0 + (4 * k));
                    if (bufPeriod != 0)
                        arr[k].dDuty = (double)bufHI / bufPeriod;
                    if (((bufReferenceClk >> k) & 0x01) != 0)///125M/64=  1.953125M
                    {
                        iRefCk = 64;
                    }
                    else ///125MHZ
                    {
                        iRefCk = 1;
                    }
                    if (bufPeriod != 0  && iRefCk != 0)
                        arr[k].dFreq = (double)125e6/(iRefCk*clk_coef*(double)bufPeriod)*bufCycle;///? 频偏校准
                    printf(">25v channel:%d freq:%lf, duty:%lf\n", k, arr[k].dFreq, arr[k].dDuty);
                }
            }
            break;
        }
    }

    if(bufHI > 0)
    {
        printf("channel:%d PWM level > 25v, set reference voltage:%0.2fv, buf:%u\n",
               iChannel, currentVoltage, bufHI);
    }
    else // pwm level < 25v, find reference voltage
    {
        for(i = 0; i < 5; i++) // 5v
        {
            currentVoltage -= 5;
            if(0 == currentVoltage)
            {
                currentVoltage = 2;
            }
            bufHI = 0;
            setPWMCaptureEnableStatus(iChannel, UNABLE_STATUS);
            setInputReferenceVoltage(iChannel, currentVoltage * 1000);
            setPWMCaptureEnableStatus(iChannel, ENABLE_STATUS);
//            usleep(DELAY_US_R);
            for(int a = 0; a < RETRY_TIMES; a++)
            {
                usleep(DELAY_US);
                read(m_iConfigureFD, &buf, PWM_CAP_DONE);
                if(((buf >> iChannel) & 0x01) == 0)
                {
                    continue;
                }
                else
                {
                    status = read(m_iConfigureFD, &bufHI, PWM_CAP_HI0 + (4 * iChannel));
//                    printf("1 cur:%0.2f  last:%0.2f  buf:%u \n", currentVoltage, lastVoltage, bufHI);
                    break;
                }
            }

            if(bufHI > 0)
            {
                currentVoltage = lastVoltage;
                for(j = 0; j < 5; j++) // 1v
                {
                    currentVoltage -= 1;
                    bufHI = 0;
                    setPWMCaptureEnableStatus(iChannel, UNABLE_STATUS);
                    setInputReferenceVoltage(iChannel, currentVoltage * 1000);
                    setPWMCaptureEnableStatus(iChannel, ENABLE_STATUS);
                    if(currentVoltage < 3)
                    {
                        printf("error! pwm level < 3v\n");
                        return FAILURE_RET;
                    }
                    for(int a = 0; a < RETRY_TIMES; a++)
                    {
                        usleep(DELAY_US);
                        read(m_iConfigureFD, &buf, PWM_CAP_DONE);
                        if(((buf >> iChannel) & 0x01) == 0)
                        {
                            continue;
                        }
                        else
                        {
                            status = read(m_iConfigureFD, &bufHI, PWM_CAP_HI0 + (4 * iChannel));
//                            printf("2 cur:%0.2f  last:%0.2f  buf:%u \n", currentVoltage, lastVoltage, bufHI);
                            break;
                        }
                    }

                    if(bufHI > 0)
                    {
                        currentVoltage = lastVoltage;
                        for(z = 0; z < 10; z++) // 0.1v
                        {
                            currentVoltage -= 0.1;
                            bufHI = 0;
                            setPWMCaptureEnableStatus(iChannel, UNABLE_STATUS);
                            setInputReferenceVoltage(iChannel, currentVoltage * 1000);
                            setPWMCaptureEnableStatus(iChannel, ENABLE_STATUS);
                            for(int a = 0; a < RETRY_TIMES; a++)
                            {
                                usleep(DELAY_US);
                                read(m_iConfigureFD, &buf, PWM_CAP_DONE);
                                if(((buf >> iChannel) & 0x01) == 0)
                                {
                                    continue;
                                }
                                else
                                {
//                                    printf("a = %d\n", a);
                                    status = read(m_iConfigureFD, &bufHI, PWM_CAP_HI0 + (4 * iChannel));
//                                    printf("3 cur:%0.2f  last:%0.2f  buf:%u \n", currentVoltage, lastVoltage, bufHI);
                                    break;
                                }
                            }

                            if(bufHI > 0)
                            {
                                bufHI = 0;
                                setPWMCaptureEnableStatus(iChannel, UNABLE_STATUS);
                                setInputReferenceVoltage(iChannel, currentVoltage * 1000 / 2); // set voltage
                                setPWMCaptureEnableStatus(iChannel, ENABLE_STATUS);
                                printf("channel:%d get pwm level:%0.2fv, set reference voltage:%0.2fv\n",
                                       iChannel, currentVoltage, currentVoltage * 1000 / 2);
                                for(int a = 0; a < RETRY_TIMES; a++)
                                {
                                    usleep(DELAY_US);
                                    read(m_iConfigureFD, &buf, PWM_CAP_DONE);
                                    if(((buf >> iChannel) & 0x01) == 0)
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        for(int k = iGroup * 8; k < iGroup * 8 + 8; k++)
                                        {
                                            if( (buf >> k) & 0x01 != 0)
                                            {
                                                status = read(m_iConfigureFD, &bufHI, PWM_CAP_HI0 + (4 * k));
                                                status = read(m_iConfigureFD, &bufPeriod, PWM_CAP_PERIOD0 + (4 * k));
                                                status = read(m_iConfigureFD, &bufCycle, PWM_CAP_CYCLES0 + (4 * k));
//                                                printf("CHL::%u--HI::%u--PER::%u--CYC::%u \n", k, bufHI, bufPeriod, bufCycle);
                                                if (bufPeriod != 0)
                                                    arr[k].dDuty = (double)bufHI / bufPeriod;
                                                if (((bufReferenceClk >> k) & 0x01) != 0)///125M/64=  1.953125M
                                                {
                                                    iRefCk = 64;
                                                }
                                                else ///125MHZ
                                                {
                                                    iRefCk = 1;
                                                }
                                                if (bufPeriod != 0  && iRefCk != 0)
                                                    arr[k].dFreq = (double)125e6/(iRefCk*clk_coef*(double)bufPeriod)*bufCycle;///? 频偏校准
                                                printf("channel:%d freq:%lf, duty:%lf\n", k, arr[k].dFreq, arr[k].dDuty);
                                            }
                                        }
                                        break;
                                    }
                                }
                                break;
                            }
                            else
                            {
                                lastVoltage = currentVoltage;
                            }
                        }
                        break;
                    }
                    else
                    {
                        lastVoltage = currentVoltage;
                    }
                }
                break;
            }
            else
            {
                lastVoltage = currentVoltage;
            }
        }
    }
    setPWMCaptureEnableStatus(iChannel, ENABLE_STATUS);
    if((5 == i) || (5 == j) || (10 == z))
    {
        printf("error! ch:%d i:%d j:%d z:%d\n", iChannel, i, j, z);
        return FAILURE_RET;
    }
    printf("---------------------------------------\n");
    return status;
}

#include <stdio.h>
int LibHWDio::getInputPWMProperty(std::array<PWMProperty, 32> &arrPWMProper)
{
    int status = SUCCESS_RET;
    double clk_coef = getClkCalCoef();
    unsigned int buf = 0;
    unsigned int bufHI = 0;
    unsigned int bufPeriod = 0;
    unsigned int bufCycle = 0;
    unsigned int bufReferenceClk = 0;
    int iRefCk = 0;
    read(m_iConfigureFD, &bufReferenceClk, PWM_CAP_CLKSEL);

    read(m_iConfigureFD, &buf, PWM_CAP_DONE);
    for (int i = 0; i < MAX_INPUT_CHANNEL_NUM; i++)
    {
        if( (buf >> i) & 0x01 != 0)
        {
            status = read(m_iConfigureFD, &bufHI, PWM_CAP_HI0 + (4*i));
            status = read(m_iConfigureFD, &bufPeriod, PWM_CAP_PERIOD0 + (4*i));
            status = read(m_iConfigureFD, &bufCycle, PWM_CAP_CYCLES0 + (4*i));
//            printf("CHL::%u--HI::%u--PER::%u--CYC::%u \n", i, bufHI, bufPeriod, bufCycle);
            if (bufPeriod != 0)
                arrPWMProper[i].dDuty = (double)bufHI /bufPeriod;
            if ( (bufReferenceClk>>i)&0x01 !=0)///125M/64=  1.953125M
            {
                iRefCk = 64;
            }
            else ///125MHZ
            {
                iRefCk = 1;
            }
            if (bufPeriod != 0  && iRefCk != 0)
                arrPWMProper[i].dFreq = (double)125e6/(iRefCk*clk_coef*(double)bufPeriod)*bufCycle;///? 频偏校准
        }
    }

    return status;
}
///
/// \brief 过流保护
///
void LibHWDio::operOverCurrentProtection()
{
    ///TODO::片子选择
    /// PCA0
    unsigned int buf = 0;
    ///1、初始化RX_FIFO_PIRQ
    buf = 0xf;
    write(m_iConfigureFD, &buf, 0x6120);

    ///2、复位TX_FIFO
    buf = 0x2;
    write(m_iConfigureFD, &buf, 0x6100);

    ///3、使能IIC、撤销TX_FIFO复位,禁止General Call
    buf = 0x1;
    write(m_iConfigureFD, &buf, 0x6100);
    ///NOTE::??
    read(m_iConfigureFD, &buf, 0x6104);
    ///4、发送器件地址+START+WRITE
    buf = 0x140;
    write(m_iConfigureFD, &buf, 0x6108);///（3片的PCA9555地址不一样，需要改为实际的地址）

    ///5、发送COMMAND BYTE
    buf = 0;
    write(m_iConfigureFD, &buf, 0x6108); ///(低边8位command为0，高边8位command为1)

    ///6、发送器件地址+START+READ
    buf = 0x141;
    write(m_iConfigureFD, &buf, 0x6108);///（3片的PCA9555地址不一样，需要改为实际的地址）

    ///7、发送STOP
    buf = 0x200;
    write(m_iConfigureFD, &buf, 0x6108);

    ///8、检查RX_FIFO是否为空(2 : BB; 3: SRW; 4 : TX_FIFO_FULL; 5 : RX_FIFO_FULL; 6 : TX_FIFO_EMPTY; 7 : RX_FIFO_EMPTY)
    read(m_iConfigureFD, &buf, 0x6104);

    ///9、读取RX_FIFO
    read(m_iConfigureFD, &buf, 0x610C);
    usleep(500);
    ///PCA1
    ///1、初始化RX_FIFO_PIRQ
    buf = 0xf;
    write(m_iConfigureFD, &buf, 0x6120);

    ///2、复位TX_FIFO
    buf = 0x2;
    write(m_iConfigureFD, &buf, 0x6100);

    ///3、使能IIC、撤销TX_FIFO复位,禁止General Call
    buf = 0x1;
    write(m_iConfigureFD, &buf, 0x6100);
    ///NOTE::??
    read(m_iConfigureFD, &buf, 0x6104);
    ///4、发送器件地址+START+WRITE
    buf = 0x140;
    write(m_iConfigureFD, &buf, 0x6108);///（3片的PCA9555地址不一样，需要改为实际的地址）

    ///5、发送COMMAND BYTE
    buf = 1;
    write(m_iConfigureFD, &buf, 0x6108); ///(低边8位command为0，高边8位command为1)

    ///6、发送器件地址+START+READ
    buf = 0x141;
    write(m_iConfigureFD, &buf, 0x6108);///（3片的PCA9555地址不一样，需要改为实际的地址）

    ///7、发送STOP
    buf = 0x200;
    write(m_iConfigureFD, &buf, 0x6108);

    ///8、检查RX_FIFO是否为空(2 : BB; 3: SRW; 4 : TX_FIFO_FULL; 5 : RX_FIFO_FULL; 6 : TX_FIFO_EMPTY; 7 : RX_FIFO_EMPTY)
    read(m_iConfigureFD, &buf, 0x6104);

    ///9、读取RX_FIFO
    read(m_iConfigureFD, &buf, 0x610C);
    usleep(500);
    ///CLEAN STATUS
    buf = 0X7;
    write(m_iConfigureFD, &buf, 0X4B0);
}

///
/// \brief 指定位置置0
/// \param reg
/// \param pos
///
void LibHWDio::setZero2Indication(unsigned int &reg, int pos)
{
    reg = (~( ((int)0x01) << pos)) & reg;
}

///
/// \brief 指定位置置1
/// \param reg
/// \param pos
///
void LibHWDio::setOne2Indication(unsigned int &reg, int pos)
{
    reg = ( ((int)0x01) << pos) | reg;
}
///
/// \brief 配置两位bit数据位到指定位置
/// \param reg
/// \param pos 起始位置
/// \param value
///
void LibHWDio::setTwoBit2Indication(unsigned int &reg, int pos, int value)
{
    unsigned int tmp =( ~(((int)0x03) << pos)) & reg;
    reg = ((int)(value & 0x03) << pos) | tmp;
}

bool LibHWDio::isOutputChannelInvalid(int iChannel)
{
    if (iChannel >= MAX_OUTPUT_CHANNEL_NUM || iChannel < 0)
        return true;
    else
        return false;
}

///
/// \brief 通道是否有效
/// \param iChannel
/// \return
///
bool LibHWDio::isInputChannelInvalid(int iChannel)
{
    if (iChannel >= MAX_INPUT_CHANNEL_NUM || iChannel < 0)
        return true;
    else
        return false;
}

std::pair<int, int> LibHWDio::calculateHighLowLevelCount(double dFreq, double dDuty)
{
    int iRef = 0;
    double clk_coef = getClkCalCoef();
    if (dFreq > 10e3)
    {
        iRef = 125e6;
    }
    else
    {
        iRef = 125e6/4;
    }
    return std::pair<int, int>( clk_coef*iRef *dDuty /dFreq -1, clk_coef*iRef*(1-dDuty)/dFreq -1);//<HI,LOW>
}


int LibHWDio::read(int fd, unsigned int *pValue, unsigned int addr)
{
    int status = 0;
    *pValue = *((volatile uint32_t *)(map_user + addr));
    return status;
}

int LibHWDio::write(int fd, unsigned int*pValue, unsigned int addr)
{
    int status = 0;
    *((volatile uint32_t *)(map_user + addr)) = *pValue;
    return status;
}
#include <time.h>
bool LibHWDio::waitForBytesWritten(int msec)
{
    unsigned int buf = 0;
    read(m_iConfigureFD, &buf, STREAM_TX_IRQ);
    if ( (buf & 0x01) == 1)
        return true;
    else
    {
        static time_t t = time(0);
        time_t k = time(0);
        if (k - t > 1)
        {
            t = time(0);
            return true;
        }
        return false;
    }
}
///
/// \brief EEPROM DATA TRANS
/// \param addr 0X00 - 0X1FFF
/// \param pArr
/// \param iLength
/// \return
///
#include <stdio.h>
int LibHWDio::writeData(unsigned int addr, uint8_t *pArr, int iLength)
{
    usleep(5000);
    ///TODO::32bit page write
    unsigned int buf = 0;

    buf = 0x02;
    write(m_iConfigureFD, &buf, 0x2100);

    buf = 0x01;
    write(m_iConfigureFD, &buf, 0x2100);

    read(m_iConfigureFD, &buf, 0x2104);

    buf = 0x1A0;
    write(m_iConfigureFD, &buf, 0x2108);
    ///Addr High
    buf = 0x1F & (addr >> 8);
    write(m_iConfigureFD, &buf, 0x2108);
    ///Addr Low
    buf = 0xFF & addr;
    write(m_iConfigureFD, &buf, 0x2108);
    if (iLength < 13)
    {
        ///Data
        for (int i = 0; i < iLength - 1; i++)
        {
            read(m_iConfigureFD, &buf, 0x2100);
            printf("addr 0x%X value 0x%X\t",0x2100, buf);
            read(m_iConfigureFD, &buf, 0x2104);
            printf("addr 0x%X value 0x%X\t",0x2104, buf);
            read(m_iConfigureFD, &buf, 0x2114);
            printf("addr 0x%X value 0x%X\t",0x2114, buf);
            read(m_iConfigureFD, &buf, 0x2118);
            printf("addr 0x%X value 0x%X\t",0x2118, buf);
            read(m_iConfigureFD, &buf, 0x2120);
            printf("addr 0x%X value 0x%X\n",0x2120, buf);
            while (!isEEPRomWritable()) {
                usleep(50000);
            }
            buf = *(pArr+i);
            write(m_iConfigureFD, &buf, 0x2108);
        }
        ///Data Last
        buf = *(pArr+iLength - 1);
        buf = (*(pArr+iLength - 1) | 0x200);
        write(m_iConfigureFD, &buf, 0x2108);
    }
    else if (iLength >=13)
    {
        for (int i = 0; i < 12; i++)
        {
            read(m_iConfigureFD, &buf, 0x2100);
            printf("addr 0x%X value 0x%X\t",0x2100, buf);
            read(m_iConfigureFD, &buf, 0x2104);
            printf("addr 0x%X value 0x%X\t",0x2104, buf);
            read(m_iConfigureFD, &buf, 0x2114);
            printf("addr 0x%X value 0x%X\t",0x2114, buf);
            read(m_iConfigureFD, &buf, 0x2118);
            printf("addr 0x%X value 0x%X\t",0x2118, buf);
            read(m_iConfigureFD, &buf, 0x2120);
            printf("addr 0x%X value 0x%X\n",0x2120, buf);
            while (!isEEPRomWritable()) {
                usleep(50000);
            }
            buf = *(pArr+i);
            write(m_iConfigureFD, &buf, 0x2108);
        }
        buf = (*(pArr+12) | 0x200);
        write(m_iConfigureFD, &buf, 0x2108);
        writeData(addr + 0x0D, (pArr+13), iLength - 13);
    }
}

///
/// \brief EEPROM DATA ACQUIRE
/// \param pArr
/// \param iLength
/// \return
///
int LibHWDio::readData(unsigned int addr, uint8_t *pArr, int iLength)
{
    unsigned int buf = 0;

    buf = 0x0F;
    write(m_iConfigureFD, &buf, 0x2120);

    buf = 0x02;
    write(m_iConfigureFD, &buf, 0x2100);

    buf = 0x01;
    write(m_iConfigureFD, &buf, 0x2100);
    read(m_iConfigureFD, &buf, 0x2104);

    buf = 0x1A0;
    write(m_iConfigureFD, &buf, 0x2108);

    ///Addr High
    buf = 0x1F & (addr >> 8);
    write(m_iConfigureFD, &buf, 0x2108);
    ///Addr Low
    buf = 0xFF & addr;
    write(m_iConfigureFD, &buf, 0x2108);

    int index = 0;
    static time_t t1 = time(0); ///超时
    time_t k;
    for (; index < iLength; index++)
    {
        buf = 0x1A1;
        write(m_iConfigureFD, &buf, 0x2108);
        while (!isEEPRomReadable()) {

            usleep(50000);
            k = time(0);
            if (k-t1 >2)
            {
                t1 = time(0);
                buf = 0x200;
                write(m_iConfigureFD, &buf, 0x2108);
                return index;
            }
        }

        read(m_iConfigureFD, &buf, 0x210C);
        *(pArr+index) = buf;
        t1 = time(0);

    }
    buf = 0x200;
    write(m_iConfigureFD, &buf, 0x2108);
    return index;
}

void LibHWDio::initDutyCalibration()
{
    if (m_iConfigureFD > 0)
    {
        readData(0x0000, (uint8_t*)arrDutyCalibrationVol5, 24);
        readData(0x0020, (uint8_t*)arrDutyCalibrationVol12, 24);
        readData(0x0040, (uint8_t*)arrDutyCalibrationVolRef, 24);
    }
}

void LibHWDio::initDefaultConfigure()
{
    if (m_iConfigureFD > 0)
    {
        ///dead zone time
        readData(0x0060,(uint8_t*)&uiDeadZone, sizeof(uiDeadZone));
        write(m_iConfigureFD, &uiDeadZone, DO_DEADZONE);
    }
}

void LibHWDio::saveDutyCalibration()
{
    if (m_iConfigureFD > 0)
    {
        writeData(0x0000, (uint8_t*)arrDutyCalibrationVol5, 24);
        writeData(0x0020, (uint8_t*)arrDutyCalibrationVol12, 24);
        writeData(0x0040, (uint8_t*)arrDutyCalibrationVolRef, 24);
    }
}

void LibHWDio::saveDefaultConfigure()
{
    if (m_iConfigureFD > 0)
        writeData(0x0060, (uint8_t*)&uiDeadZone, sizeof(uiDeadZone));
}

bool LibHWDio::isEEPRomReadable()
{
    bool bStatus = false;
    unsigned int buf = 0;
    read(m_iConfigureFD, &buf, 0x2104);

    if ((0x40 & buf) == 0)
        bStatus = true;
    return bStatus;
}

bool LibHWDio::isEEPRomWritable()
{
    bool bStatus = false;
    unsigned int buf = 0;
    read(m_iConfigureFD, &buf, 0x2104);
    if ((0x10 & buf) != 0)
    {
        bStatus = false;
    }
    else
    {
        bStatus = true;
    }
    return bStatus;
}

int LibHWDio::getMagicNum() const
{
    return m_iMagicNum;
}

#include <regex.h>
/// e.g. "^dio_([0-8])_in"
bool LibHWDio::verifyDevice(std::string strDes, std::string strSrc)
{
    bool isOk = false;
     ///1
    regex_t reg;
    regcomp(&reg, strDes.c_str(), REG_EXTENDED);
    const size_t nmatch = 2;
    regmatch_t pmatch [nmatch];

    int status = 0;
    ///2 进行匹配
    status = regexec(&reg, strSrc.c_str(), nmatch, pmatch, 0);
    if (status == REG_NOMATCH)
        isOk = false;
    else if(status == REG_NOERROR)
        isOk = true;
    ///3
    regfree(&reg);
    return isOk;
}

int LibHWDio::setPpsUpdateCount(uint32_t sec)
{
    int status = 0;
    unsigned int buf = 0;
    status = write(m_iConfigureFD, &sec, DIO_DEVS::PPS_SECOND_SET);
    buf = 1;
    status = write(m_iConfigureFD, &buf, DIO_DEVS::PPS_SECOND_VALID);
    return 0;
}

int LibHWDio::getBitStreamSendTime(uint64_t *mic_sec)
{
    int status = 0;
    unsigned int m_sec = 0;
    unsigned int m_na_sec = 0;
    double clk_coef = getClkCalCoef();
    status = read(m_iConfigureFD, &m_sec, DIO_DEVS::BITS_SECONDS);
    status = read(m_iConfigureFD, &m_na_sec, DIO_DEVS::BITS_NANO_SECONDS);
    m_na_sec = m_na_sec*clk_coef;
    if (m_na_sec >= 999999999)
        m_na_sec = 999999999;
    *mic_sec = (uint64_t)m_sec * 1e6 + (uint64_t)m_na_sec / 1000;
    return 0;
}

int LibHWDio::getPpsSecCount(uint32_t *sec)
{
    int status = 0;
    status = read(m_iConfigureFD, sec, DIO_DEVS::PPS_COUNTER);
    return 0;
}

int LibHWDio::setPpsExportEnable(int en)
{
    int status = 0;
    unsigned int buf = 0;
    if (en != 0)
        buf = 0x1;
    status = write(m_iConfigureFD, &buf, DIO_DEVS::PPS_EXPORT_EN);
    return 0;
}
///
/// \brief LibHWDio::setPpsEdgeSel
/// \param edge 0:下降沿有效 1:上升沿有效
/// \return
///
int LibHWDio::setPpsEdgeSel(int edge)
{
    int status = 0;
    unsigned int buf = 0;
    if (edge != 0)
        buf = 0x1;
    status = write(m_iConfigureFD, &buf, DIO_DEVS::PPS_EDGE_SEL);
    return 0;
}

double LibHWDio::getClkCalCoef()
{
    unsigned int buf = 0;
    read(m_iConfigureFD, &buf, DIO_DEVS::CAL_DONE);
    read(m_iConfigureFD, &buf, DIO_DEVS::CAL_COUNTER);
    double clk_coef = 125e6/ buf;
    return clk_coef;
}

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
int LibHWDio::writeBITFile(std::string strFile)
{
    int ret = 1;
    FILE* fd = NULL;
    uint32_t total_read = 0;
    uint32_t ddr_head = 0;
    uint32_t ddr_tail = 0;
    uint32_t ddr_full = 0;
    uint32_t min_data = 0x100;
    uint32_t len_avail = 0;
    uint32_t len = 0;
    unsigned int buf = 0;
    long length = 0;

    if (!strFile.empty())
        fd = fopen(strFile.c_str(), "rb");
    if (NULL == fd) {
        return -1;
    }
    fseek(fd, 0L, SEEK_END);
    length = ftell(fd);
    fseek(fd, 0L, SEEK_SET);
    length = min(0x7FFFF, length) & (~0x7);
    // 读取环形缓冲
    while (total_read < length) {
        read(m_iDMAFD, &ddr_head, BIT_FIFO_HEAD);
        read(m_iDMAFD, &ddr_tail, BIT_FIFO_TAIL);
        read(m_iDMAFD, &ddr_full, BIT_FIFO_FULL);

        if (ddr_full) {
            printf("ddr full\n");
            usleep(10000);
            continue;
        }

        len_avail = (ddr_tail - ddr_head + 0x7FFFF) & 0x7FFFF;
        if (len_avail < min_data) {
            printf("len_avail %d, min_data %d\n", len_avail, min_data);
            usleep(1000);
            continue;
        }

        len = min(len_avail, length - total_read);
        len = min(0x400000, len) & (~0x7);

        if (len > 0) {
            printf("len:%X, ddr_head 0x%X, ddr_tail 0x%X\n", len, ddr_head, ddr_tail);

            ret = fread((void*)map_dma, 1, len, fd);
            if (ret <= 0) {
                return -1;
            }

            ret = XdmaSend(m_iDMAFD, len, ddr_head + 0x00000000);
            if (ret != 0) {
                printf("read from 0x%X, %d bytes failed\n", ddr_tail, len);
                return -1;
            }

            printf("write to 0x%X, %d bytes\n", ddr_tail + 0x00000000, len);
            buf = (ddr_head + len) & 0x7FFFF;
            write(m_iConfigureFD, &buf, BIT_FIFO_HEAD);
            total_read += len;
        }
    }

    read(m_iConfigureFD, &ddr_head, BIT_FIFO_HEAD);
    read(m_iConfigureFD, &ddr_tail, BIT_FIFO_TAIL);
    printf("write %d bytes, head %X, tail %X\n", total_read, ddr_head, ddr_tail);
    ret = 0;
    if (NULL != fd)
        fclose(fd);
    return ret;
}

int LibHWDio::writeBitData(uint32_t* pArr, int iLength, int* iActLen)
{
    int ret = 0;
    uint32_t total_read = 0;
    uint32_t ddr_head = 0;
    uint32_t ddr_tail = 0;
    uint32_t ddr_full = 0;
    uint32_t min_data = 0x100;
    uint32_t len_avail = 0;
    uint32_t len = 0;
    char* pBuf = (char*)pArr;
    unsigned int buf = 0;
    long length = 0;

    length = iLength * sizeof (uint32_t);
    length = min(0x1FFFF, length) & (~0x7);
    // 读取环形缓冲
    while (total_read < length) {
        read(m_iDMAFD, &ddr_head, BIT_FIFO_HEAD);
        read(m_iDMAFD, &ddr_tail, BIT_FIFO_TAIL);
        read(m_iDMAFD, &ddr_full, BIT_FIFO_FULL);

        if (ddr_full) {
            usleep(10000);
            continue;
        }

        len_avail = (ddr_tail - ddr_head + 0x1FFFF) & 0x1FFFF;
        if (len_avail < min_data) {
            usleep(1000);
            continue;
        }

        len = min(len_avail, length - total_read);
        len = min(0x20000, len) & (~0x7);

        if (len > 0) {
            memcpy((void*)map_dma, pBuf, len);
            pBuf += len;

            ret = XdmaSend(m_iDMAFD, len, ddr_head + 0x00000000);
            if (ret != 0) {
                printf("read from 0x%X, %d bytes failed\n", ddr_tail, len);
                return -1;
            }

            buf = (ddr_head + len) & 0x1FFFF;
            write(m_iConfigureFD, &buf, BIT_FIFO_HEAD);
            total_read += len;
        }
    }

    read(m_iConfigureFD, &ddr_head, BIT_FIFO_HEAD);
    read(m_iConfigureFD, &ddr_tail, BIT_FIFO_TAIL);
    *iActLen = total_read / sizeof (uint32_t);
    return ret;
}

int LibHWDio::writeBitFile(const char *strFile)
{
    int status = 0;
    FILE* fd = NULL;
    long length = 0;
    fd = fopen(strFile, "rb");
    if (NULL == fd) {
        return -1;
    }

    fseek(fd, 0L, SEEK_END);
    length = ftell(fd);
    fseek(fd, 0L, SEEK_SET);

    char* arr = (char* ) malloc(20480);
    int len = 0;
    int iActLen = 0;
    while(len = fread(arr, sizeof(uint32_t), 20480 / sizeof(uint32_t), fd))
    {
        writeBitData((uint32_t*)arr, len, &iActLen);
    }
    free(arr);
    fclose(fd);
    return status;
}
