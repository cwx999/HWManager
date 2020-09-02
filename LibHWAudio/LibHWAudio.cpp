#include "LibHWAudio.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cmath>
#include <xpdma.h>
#include <string.h>
#include "Wave.h"

#define min(a, b)   (((a) < (b)) ? (a) : (b))

Wave* wave = new Wave;
LibHWAudio::LibHWAudio(std::string strDev, int index, LibHWAudio::DevOriention eOrient)
    : m_strDev(strDev),
      m_index(index - 1)

{
    switch (eOrient) {
    case DEV_IN:
        m_iMagicNum = AUDIO_DEVS::MAGIC_IN;
        break;
    case DEV_OUT:
        m_iMagicNum = AUDIO_DEVS::MAGIC_OUT;
        break;
    case DEV_A2B:
        m_iMagicNum = AUDIO_DEVS::MAGIC_A2B;
        break;
    default:
        break;
    }
}

LibHWAudio::~LibHWAudio()
{
    if (NULL != map_user &&  AUDIO_DEVS::MAGIC_A2B != m_iMagicNum)
    {
        munmap(map_user, MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE);
    }
    if (NULL != map_user && AUDIO_DEVS::MAGIC_A2B == m_iMagicNum)
    {
        munmap(map_user, MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE * 2);
    }
    uninitAudioDevs();
}

int LibHWAudio::initAudioDevs()
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
    switch (m_iMagicNum) {
    case AUDIO_DEVS::MAGIC_IN:
        map_user = (char*) mmap(NULL, MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_iConfigureFD, m_index*4096);
        break;
    case AUDIO_DEVS::MAGIC_OUT:
        map_user = (char*) mmap(NULL, MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_iConfigureFD, (m_index+4)*4096);
        break;
    case AUDIO_DEVS::MAGIC_A2B:
//TODO::
        map_user = (char*) mmap(NULL, MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE * 2, PROT_READ | PROT_WRITE, MAP_SHARED, m_iConfigureFD, 8*4096);
        break;
    default:
        break;
    }


    ///2
    m_iDMAFD = m_iConfigureFD;
    if (AUDIO_DEVS::MAGIC_A2B != m_iMagicNum)
    {
        map_dma = (char*) (map_user + MAX_MAP_USER_SIZE);
    }else
    {
        map_A2B_dma_recv = (char*) (map_user + MAX_MAP_USER_SIZE );
        map_A2B_dma_send = (char*) (map_user + MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE );
    }

    A2BBoardMode(0);
    return ret;
}

int LibHWAudio::getAudioVersion(std::string &str)
{
    int ret = 0;
    if(m_iConfigureFD == 0)
        ret = -1;
    unsigned int buf = 0;
    buf = registerRead(AUDIO_DEVS::DATETIME);
    char arr [10] = {0};
    sprintf(arr,"%x", buf);
    str = std::string(arr, 8);

    buf = registerRead(AUDIO_DEVS::VERSION);
    sprintf(arr,"%x", buf);
    str.append(std::string(arr,8));
    return ret;
}

int LibHWAudio::uninitAudioDevs()
{
    int ret = 0;
    for(auto it = m_mapDevsFD.begin(); it != m_mapDevsFD.end(); it++)
        close(it->second);
    close(m_iConfigureFD);
    XdmaClose(m_iDMAFD);
    return ret;
}

uint32_t LibHWAudio::dmaRecv(uint32_t length, uint32_t addr)
{
    int ret = -1;
    ret = XdmaAudioRecv(m_iDMAFD, length, addr, 1, 0);
    if(ret !=0)
    {
        printf("dma read error %d\n", ret);

    }
    printf("read buffer (length = %d)\n", length);
}

uint32_t LibHWAudio::dmaSend(uint32_t length, char *buffer, uint32_t addr)
{
    int ret = -1;
    int i = 0;
    for(i = 0; i < length; i++)
    {
        map_dma[i] = buffer[i];
    }
    ret = XdmaAudioSend(m_iDMAFD, length, addr, 1, 0);
}

void LibHWAudio::registerWrite(uint32_t addr, uint32_t value)
{
    *((volatile uint32_t *)(map_user + addr)) = value;
}

int LibHWAudio::registerRead(uint32_t addr)
{
    int reg_value = 0;
    reg_value = *((volatile uint32_t*) (map_user + addr));
    return reg_value;
}

void LibHWAudio::regWriteFloat(uint32_t addr, float value)
{
    *((volatile float *)(map_user + addr)) = value;
}

float LibHWAudio::regReadFloat(uint32_t addr)
{
    float reg_value = 0;
    reg_value = *((volatile float*) (map_user + addr));
    return reg_value;
}

int LibHWAudio::setInputAtten(int channel, int atten)
{
    int iAtten = 0;
    int tmp = 1;
    setAudioInputEnable(channel, 0, m_inputMode);
    iAtten = registerRead(AUDIO_DEVS::IN_ATTEN);
    //单声道
    if(m_inputMode == 1){       //立体声
        tmp = 0x3;
    }
    switch (atten) {
    case 0:
        registerWrite(AUDIO_DEVS::IN_ATTEN, (~(tmp << channel) ) & iAtten);
        break;
    case 1:
        registerWrite(AUDIO_DEVS::IN_ATTEN,  (tmp << channel) | iAtten);
        break;
    default:
        break;
    }
    return 0;
}

int LibHWAudio::setInputRate(SAMPLING_RATE sampling_rate)
{
    ///输入共用同一采样
    registerWrite(AUDIO_DEVS::IN_ENABLE, 0);
    sleep(1);
    switch (sampling_rate) {
    case F48kHz:
        registerWrite(AUDIO_DEVS::ADC_REG_DATA, 0X2237);
        m_audioSam = 48000;
        break;
    case F96kHz:
        registerWrite(AUDIO_DEVS::ADC_REG_DATA, 0X221F);
        m_audioSam = 96000;
        break;
    case F192kHz:
        registerWrite(AUDIO_DEVS::ADC_REG_DATA, 0X2207);
        m_audioSam = 192000;
        break;
    default:
        break;
    }
    return 0;
}

int LibHWAudio::setAudioInputMode(uint32_t channel, int mode)
{
    m_inputMode = mode;
    setAudioInputEnable(channel, 0, mode);
    unsigned int buf = registerRead(AUDIO_DEVS::IN_MODE);
    switch(channel){
    case 0:
    case 2:
        if(mode == 0){
            buf = (~(0x03 << (channel))) & buf;
        }else{
            buf = (0x03 << channel) | buf;
        }
        break;
    case 1:
    case 3:
        if(mode == 0){
            buf = (~(0x03 << (channel - 1))) & buf;
        }else{
            return INPUT_MODE_ERR;
        }
        break;
    default:
        break;
    }
    registerWrite(AUDIO_DEVS::IN_MODE, buf);
    return 0;
}

int LibHWAudio::setAudioInputDataType(uint32_t channel, bool isInt)
{
    setAudioInputEnable(channel, 0, m_inputMode);
    uint32_t buf = registerRead(AUDIO_DEVS::IN_MODE) ;
    m_isInt = isInt;
    if(isInt){
        switch(channel){
            case 0:
            case 2:
                if(m_inputMode == 0){
                    buf = buf | (0x10 << channel);
                }else{
                    buf = buf | (0x30 << channel);
                }
                break;
            case 1:
            case 3:
                if(m_inputMode == 0){
                    buf = buf | (0x10 << channel);
                }else{
                    return INPUT_MODE_ERR;
                }
                break;

            default:
                break;
            }
            registerWrite(AUDIO_DEVS::IN_MODE, buf);

    }else{
        switch(channel){
        case 0:
        case 2:
        {
            if(m_inputMode == 0){
                buf = buf & (~(0x10 << channel));
            }else{
                buf = buf & (~(0x30 << channel));
            }
        }
            break;
        case 1:
        case 3:
        {
            if(m_inputMode == 0){
                buf = buf & (~(0x10 << channel));
            }
            else{
                return INPUT_MODE_ERR;
            }
        }
            break;
        default:
            break;
        }
        registerWrite(AUDIO_DEVS::IN_MODE, buf);
    }
    return 0;

}

int LibHWAudio::setAudioInputEnable(uint32_t channel, int en, int mode)
{
    int m_en = 0;
    bool isDelay = false;
    m_en = registerRead(AUDIO_DEVS::IN_ENABLE);
    (m_en >> channel & 0x01) == en ? isDelay = false : isDelay = true;
    switch (en) {
    case 0:
        if (mode == 0)
            registerWrite(AUDIO_DEVS::IN_ENABLE, (~(0x01 << channel) ) & m_en);
        else
        {
            if (channel == 0 || channel == 2)
                registerWrite(AUDIO_DEVS::IN_ENABLE, (~(0x03 << channel) ) & m_en);
        }
        break;
    case 1:
        if (mode == 0)
            registerWrite(AUDIO_DEVS::IN_ENABLE, (0x01 << channel) | m_en);
        else
        {
            if (channel == 0 || channel == 2)
                registerWrite(AUDIO_DEVS::IN_ENABLE, (0x03 << channel) | m_en);
        }
        break;
    default:
        break;
    }
    if (isDelay)
        usleep(10000);
    return 0;

}

int LibHWAudio::getAudioInputStatus(uint32_t channel, int *dataSize, int *total_len) const
{
    if(m_index == channel){
        *dataSize = total_read[channel];
        *total_len = ilen_avail[channel];
    }else{
        *dataSize = 0;
        *total_len = 0;
    }
    return 0;
}

int LibHWAudio::getAudioInputChannelInfo(uint32_t channel, AudInChlInfo *info)
{
    int samp = 0;
    int mode = 0;
    int pause = 0;
    int iEn = 0;
    mode = (uint16_t)registerRead(AUDIO_DEVS::IN_MODE);
    iEn = registerRead(AUDIO_DEVS::IN_ENABLE);
    pause = registerRead(AUDIO_DEVS::IN_PAUSE);
    info->mode = mode >> channel & 0x01;
    info->en = iEn >> channel & 0x01;
    info->atten = (uint16_t)registerRead(AUDIO_DEVS::IN_ATTEN);
    samp = registerRead(AUDIO_DEVS::ADC_REG_DATA);
    if(samp == 0x2237){
        info->sampling = 48000;
    }
    else if(samp == 0x221F){
        info->sampling = 96000;
    }
    else if(samp == 0x2207){
        info->sampling = 192000;
    }else {
        info->sampling = 48000;
    }

    if(((pause >> channel) & 0x01) ==1) {
        info->status = PAUSE;
    }
    else if(total_read[channel] > 0 && total_read[channel] < m_stRecvFileStatus[channel].iTotalLength) {
        info->status = COLLECTION;
    }
    else {
        info->status = FREE;
    }
    if(m_index == channel){
        info->fileSize = total_read[channel];
        info->total_len = ilen_avail[channel];
    }else{
        info->fileSize = 0;
        info->total_len = 0;
    }

    strcpy(info->filename, wave->GetFileName(m_stRecvFileStatus[channel].strFile.c_str()));
    return 0;
}

int LibHWAudio::setAudioInputSuspend(uint32_t channel, int isPause)
{
    int iPause = 0;
    int tmp = 1;
    iPause = registerRead(AUDIO_DEVS::IN_PAUSE);
    if(m_inputMode == 1){
        tmp = 0x3;
    }
    switch(isPause){
    case 0:
        registerWrite(AUDIO_DEVS::IN_PAUSE, (~(tmp << channel) ) & iPause);
        break;
    case 1:
        registerWrite(AUDIO_DEVS::IN_PAUSE, (tmp << channel) | iPause);
        break;
    default:
        break;
    }
    setRecvTransStatus(channel, isPause);
    return 0;
}

int LibHWAudio::setOutputEnable(uint32_t channel, int en, int mode)
{
    int m_en = 0;
    bool isDelay = false;
    m_en = registerRead(AUDIO_DEVS::OUT_ENABLE);
    (m_en >> channel & 0x01) == en ? isDelay = false : isDelay = true;
    switch (en) {
    case 0:
        if (mode == 0)
            registerWrite(AUDIO_DEVS::OUT_ENABLE, (~(0x01 << channel) ) & m_en);
        else
        {
            if (channel == 0 || channel == 2)
                registerWrite(AUDIO_DEVS::OUT_ENABLE, (~(0x03 << channel) ) & m_en);
        }
        break;
    case 1:
        if (mode == 0)
            registerWrite(AUDIO_DEVS::OUT_ENABLE, (0x01 << channel) | m_en);
        else
        {
            if (channel == 0 || channel == 2)
                registerWrite(AUDIO_DEVS::OUT_ENABLE, (0x03 << channel) | m_en);
        }
        break;
    default:
        break;
    }
    if (isDelay)
        usleep(2000000);
    return 0;
}

int LibHWAudio::setOutputMode(uint32_t channel, int mode)
{
    m_outputMode = mode;
    setOutputEnable(channel, 0, mode);
    unsigned int buf = registerRead(AUDIO_DEVS::OUT_MODE);
    switch(channel){
    case 0:
    case 2:
        if(mode == 0){
            buf = (~(0x03 << (channel))) & buf;
        }else{
            buf = (0x03 << channel) | buf;
        }
        break;
    case 1:
    case 3:
        if(mode == 0){
            buf = (~(0x03 << (channel - 1))) & buf;
        }else{
            return OUTPUT_MODE_ERR;
        }
        break;
    default:
        break;
    }
    registerWrite(AUDIO_DEVS::OUT_MODE, buf);
    return 0;
}

int LibHWAudio::setOutptDataType(uint32_t channel, bool isInt)
{
    setOutputEnable(channel, 0, m_outputMode);
    unsigned int buf = registerRead(AUDIO_DEVS::OUT_MODE);
    if(isInt){
        switch(channel){
            case 0:
            case 2:
                if(m_outputMode == 0){
                    buf = buf | (0x10 << channel);
                }else{
                    buf = buf | (0x30 << channel);
                }
                break;
            case 1:
            case 3:
                if(m_outputMode == 0){
                    buf = buf | (0x10 << channel);
                }else{
                    return OUTPUT_MODE_ERR;
                }
                break;
            default:
                break;
            }
            registerWrite(AUDIO_DEVS::OUT_MODE, buf);
    }else{
        switch (channel) {
        case 0:
        case 2:
        {
            if(m_outputMode == 0){
                buf = buf & (~(0x10 << channel));
            }else{
                buf = buf & (~(0x30 << channel));
            }
        }
            break;
        case 1:
        case 3:
        {
            if(m_outputMode == 0){
                buf = buf & (~(0x10 << channel));
            }
            else{
                return OUTPUT_MODE_ERR;
            }
        }
            break;
        default:
            break;
        }
        registerWrite(AUDIO_DEVS::OUT_MODE, buf);
    }
    return 0;
}

int LibHWAudio::setOutputAMP(uint32_t channel, int amp)
{
    setOutputEnable(channel, 0, m_outputMode);
    unsigned int buf = registerRead(AUDIO_DEVS::OUT_AMP);
    unsigned int tmp = 1;
    if(m_outputMode == 1){          //立体声
        tmp = 0x3;
    }
    switch(amp){
    case 0:
        buf = (~(tmp << channel)) & buf;
        registerWrite(AUDIO_DEVS::OUT_AMP, buf);
        break;
    case 1:
        buf = (tmp << channel) | buf;
        registerWrite(AUDIO_DEVS::OUT_AMP, buf);
        break;
    default:
        break;
    }
    return 0;
}

int LibHWAudio::setLoopPlay(int channel, int isLoop)
{
    int iLoop = 0;
    int tmp = 1;
    if(m_outputMode == 1){
        tmp = 0x3;
    }
    iLoop = registerRead(AUDIO_DEVS::OUT_REPEAT);
    switch (isLoop) {
    case 0:
        registerWrite(AUDIO_DEVS::OUT_REPEAT, (~(tmp << channel) ) & iLoop);
        break;
    case 1:
        registerWrite(AUDIO_DEVS::OUT_REPEAT, (tmp << channel) | iLoop);
    default:
        break;
    }
}

int LibHWAudio::setAudioOutputSuspend(uint32_t channel, int isPause)
{
    int iPause = 0;
    int tmp = 1;
    iPause = registerRead(AUDIO_DEVS::OUT_PAUSE);
    if(m_outputMode == 1){
        tmp = 0x3;
    }
    switch(isPause){
    case 0:
        registerWrite(AUDIO_DEVS::OUT_PAUSE, (~(tmp << channel) ) & iPause);
        break;
    case 1:
        registerWrite(AUDIO_DEVS::OUT_PAUSE, (tmp << channel) | iPause);
        break;
    default:
        break;
    }
    setSendTransStatus(channel, isPause);
    return 0;
}

int LibHWAudio::getAudioOutputChannelInfo(uint32_t channel, AudOutChlInfo *info)
{
    int samp = 0;
    int mode = 0;
    int pause = 0;
    int iEn = 0;
    mode = (uint16_t)registerRead(AUDIO_DEVS::OUT_MODE);
    iEn = registerRead(AUDIO_DEVS::OUT_ENABLE);
    pause = registerRead(AUDIO_DEVS::OUT_PAUSE);
    info->mode = mode >> channel & 0x01;
    info->en = iEn >> channel & 0x01;
    info->atten = (uint16_t)registerRead(AUDIO_DEVS::OUT_AMP);
    if(channel < 2) {
        samp = registerRead(AUDIO_DEVS::OUT_SAMMPLE_RATE0);
    }else {
        samp = registerRead(AUDIO_DEVS::OUT_SAMMPLE_RATE1);
    }
    if(samp == 0){
        info->sampling = 48000;
    }
    else if(samp == 1){
        info->sampling = 96000;
    }
    else if(samp == 2){
        info->sampling = 192000;
    }else {
        info->sampling = 48000;
    }

    if(((pause >> channel) & 0x01) ==1) {
        info->status = PAUSE;
    }
    else if(total_write[channel] > 0 && total_write[channel] < m_stSendFileStatus[channel].iTotalLength
            && m_stSendFileStatus[channel].getAvalidLen() > 0) {
        info->status = PLAY;
    }
    else {
        info->status = FREE;
    }
    if(m_index == channel){
        info->fileSize = total_write[channel];
        info->total_len = olen_avail[channel];
    }else{
        info->fileSize = 0;
        info->total_len = 0;
    }
    strcpy(info->filename, wave->GetFileName(m_stSendFileStatus[channel].strFile.c_str()));
    return 0;
}

int LibHWAudio::A2BBaseRead(uint32_t addr)
{
    uint32_t offset = 0x4108;
    registerWrite(0x4100, 2);
    registerWrite(0x4100, 1);
    registerRead(0x4104);
    registerWrite(offset, 0x01DC);
    registerWrite(offset, addr);
    registerWrite(offset, 0x1DD);
    registerWrite(offset, 0x200);
    usleep(10000);
    return registerRead( 0x410C);
}

void LibHWAudio::A2BBaseWrite(uint32_t addr, int value)
{
    uint32_t offset = 0x4108;
    registerWrite(0x4100, 2);
    registerWrite(0x4100, 1);
    registerRead(0x4104);
    registerWrite(offset, 0x01DC);
    registerWrite(offset, addr);
    registerWrite(offset, value);
    registerWrite(offset, 0x200);
    usleep(10000);
}

int LibHWAudio::A2BBusRead(uint32_t addr)
{
    uint32_t offset = 0x4108;
    registerWrite(0x4100, 2);
    registerWrite(0x4100, 1);
    registerRead(0x4104);
    registerWrite(offset, 0x01DE);
    registerWrite(offset, addr);
    registerWrite(offset, 0x1DD);
    registerWrite(offset, 0x200);
    usleep(10000);
    return registerRead( 0x410C);
}

void LibHWAudio::A2BBusWrite(uint32_t addr, int value)
{
    uint32_t offset = 0x4108;
    registerWrite(0x4100, 2);
    registerWrite(0x4100, 1);
    registerRead(0x4104);
    registerWrite(offset, 0x01DE);
    registerWrite(offset, addr);
    registerWrite(offset, value);
    registerWrite(offset, 0x200);
    usleep(10000);
}

//Master 唤醒
int LibHWAudio::A2BMasterAwaken()
{
    if((0xAD == A2BBaseRead(AUDIO_DEVS::A2B_VENDOR)) &&
       (0x28 == A2BBaseRead(AUDIO_DEVS::A2B_PRODUCT))){
           A2BBaseWrite(AUDIO_DEVS::A2B_CONTROL, 0X84);
           registerWrite(AUDIO_DEVS::A2B_MODE, 1);
        }
        if((0x80 == A2BBaseRead(AUDIO_DEVS::A2B_INTSRC)) &&
           (0xFF == A2BBaseRead(AUDIO_DEVS::A2B_INTTYPE))){
            return 0;  //success
        }else
            return AWAKEN_ERR; //awaken error
}

int LibHWAudio::A2BBoardMode(int mode)
{
    if (AUDIO_DEVS::MAGIC_A2B != m_iMagicNum)
        return -1;
    if(0 == mode){
        registerWrite(AUDIO_DEVS::A2B_MODE, 0);
        A2BBaseWrite(AUDIO_DEVS::A2B_CONTROL, 0);
    }
    else
       registerWrite(AUDIO_DEVS::A2B_MODE, 1);
    return 0;
}
//配置mask
int LibHWAudio::A2BConfigMask()
{
    //配置mask
    A2BBaseWrite(AUDIO_DEVS::A2B_INTMASK0, 0x77);
    A2BBaseWrite(AUDIO_DEVS::A2B_INTMASK1, 0x78);
    A2BBaseWrite(AUDIO_DEVS::A2B_INTMASK2, 0x0F);
    A2BBaseWrite(AUDIO_DEVS::A2B_BECCTL, 0xEF);
    A2BBaseWrite(AUDIO_DEVS::A2B_INTPND2, 0x01);
    //判断slave0是否激活
    A2BBaseWrite(AUDIO_DEVS::A2B_RESPCYS, 0x7B);//time
    A2BBaseWrite(AUDIO_DEVS::A2B_CONTROL, 0x81);
    A2BBaseWrite(AUDIO_DEVS::A2B_I2SGCFG, 0x20);
    A2BBaseWrite(AUDIO_DEVS::A2B_SWCTL, 0x01);
    A2BBaseWrite(AUDIO_DEVS::A2B_DISCVRY, 0x7B); //time
    A2BBaseRead(AUDIO_DEVS::A2B_INTSRC);
    A2BBaseRead(AUDIO_DEVS::A2B_INTTYPE);
    //调整switch mode 给slave编号
    A2BBaseWrite(AUDIO_DEVS::A2B_SWCTL, 0x21);
    return 0;

}

//init slave0
int LibHWAudio::A2BInitSlave( A2BRATE mRate)
{
    A2BBaseWrite(AUDIO_DEVS::A2B_NODEADDR, 0x0);
    if((0xAD == A2BBusRead(AUDIO_DEVS::A2B_VENDOR)) &&
        (0x28 == A2BBusRead(AUDIO_DEVS::A2B_PRODUCT)) &&
        (0x00 == A2BBusRead(AUDIO_DEVS::A2B_VERSION)) &&
        (0x01 == A2BBusRead(AUDIO_DEVS::A2B_CAPABILITY)) //满足条件可以初始化从1
        )
        printf("success\n");
    A2BBusWrite(AUDIO_DEVS::A2B_LDNSLOTS, 0x80);  //ldnSlot
    A2BBusWrite(AUDIO_DEVS::A2B_LUPSLOTS, 0x02);  //lupSlot

    //set frame rate & data rate
    switch(mRate){
    case R48k_400KHZ:
        A2BBusWrite(AUDIO_DEVS::A2B_I2CCFG, 0X01);
        break;
    case R48k_100KHZ:
        A2BBusWrite(AUDIO_DEVS::A2B_I2CCFG, 0X00);
        break;
    case R44_1k_400KHZ:
        A2BBusWrite(AUDIO_DEVS::A2B_I2CCFG, 0X05);
        break;
    case R44_1k_100KHZ:
        A2BBusWrite(AUDIO_DEVS::A2B_I2CCFG, 0X04);
        break;
    default:
        A2BBusWrite(AUDIO_DEVS::A2B_I2CCFG, 0X01);
        break;
    }
    //Drive SYNC Pin for I2S Operation TDM2 32bit
    A2BBusWrite(AUDIO_DEVS::A2B_I2SGCFG, 0x0020);
    //使能RX0且在BCLK下降沿采数据上升沿发数据
    A2BBusWrite(AUDIO_DEVS::A2B_I2SCFG, 0x0019);
    A2BBusWrite(AUDIO_DEVS::A2B_PDMCTL, 0x0018);

    A2BBusWrite(AUDIO_DEVS::A2B_GPIODAT, 0x0010);
    A2BBusWrite(AUDIO_DEVS::A2B_GPIOOEN, 0x0010);
    A2BBusWrite(AUDIO_DEVS::A2B_PINCFG, 0x0000);
    A2BBusWrite(AUDIO_DEVS::A2B_CLK2CFG, 0x00C1);

    A2BBusWrite(AUDIO_DEVS::A2B_DNMASK0, 0x03);

    A2BBusWrite(AUDIO_DEVS::A2B_INTMASK0, 0x0067);
    A2BBusWrite(AUDIO_DEVS::A2B_INTMASK1, 0x007F);
    A2BBusWrite(AUDIO_DEVS::A2B_BECCTL, 0x00EF);
    return 0;
}

int LibHWAudio::A2BInitMaster()
{
    A2BBaseWrite(AUDIO_DEVS::A2B_SWCTL, 1);
    A2BBaseWrite(AUDIO_DEVS::A2B_DNSLOTS, 2); //dnSlot
    A2BBaseWrite(AUDIO_DEVS::A2B_UPSLOTS, 2);//upSlot
    A2BBaseWrite(AUDIO_DEVS::A2B_I2SCFG, 0x19);
    A2BBaseWrite(AUDIO_DEVS::A2B_PINCFG, 0x00);
    A2BBaseWrite(AUDIO_DEVS::A2B_INTMASK0, 0x77);
    A2BBaseWrite(AUDIO_DEVS::A2B_INTMASK1, 0x78);
    A2BBaseWrite(AUDIO_DEVS::A2B_INTMASK2, 0x0F);
    A2BBaseWrite(AUDIO_DEVS::A2B_BECCTL, 0x0EF);
    A2BBaseWrite(AUDIO_DEVS::A2B_RESPCYS, 0x7B);//time
    A2BBaseWrite(AUDIO_DEVS::A2B_SLOTFMT, 0x044);
    A2BBaseWrite(AUDIO_DEVS::A2B_DATCTL, 0x003);
    //主模式设置new structure=1
    A2BBaseWrite(AUDIO_DEVS::A2B_CONTROL, 0x081);
    A2BBaseRead(AUDIO_DEVS::A2B_INTSRC);
    A2BBaseWrite(AUDIO_DEVS::A2B_CONTROL, 0x082);
    A2BBaseWrite(AUDIO_DEVS::A2B_NODEADDR, 0x00);

    return 0;

}

int LibHWAudio::A2BOneSlave(A2BRATE mRate)
{
    if(0 == registerRead(AUDIO_DEVS::A2B_SLAVE_BUSY)){
        A2BMasterAwaken();
        A2BConfigMask();
        A2BInitSlave(mRate);
        A2BInitMaster();
    }else{
        printf("err! aleady  one master!\n");
        return A2B_CONFIG_ERR;
    }
 return 0;
}

int LibHWAudio::A2BTXMode(A2BTXMODE mode, A2BRATE mRate)
{
    A2BOneSlave(mRate);
}

void LibHWAudio::setA2BTxEnable(uint32_t en)
{
    registerWrite(AUDIO_DEVS::A2B_TX_EN, en);
    usleep(1000);
}

void LibHWAudio::setA2BRxEnable(uint32_t en)
{
    registerWrite(AUDIO_DEVS::A2B_RX_EN, en);
    usleep(1000);
}

int LibHWAudio::setA2BRecvcfg(bool isInt, float coef)
{
    uint32_t iMode = 0;
    iMode = registerRead(AUDIO_DEVS::A2B_MODE);
    setA2BRxEnable(0);
    m_a2bisInt = isInt;
    if(isInt){
        iMode = 0x2 | iMode;
        registerWrite(AUDIO_DEVS::A2B_MODE, iMode);
    }
    if(1 == registerRead(AUDIO_DEVS::DDS_OUT_ENABLE)){
        regWriteFloat(AUDIO_DEVS::A2B_RX_MULT_COEF, 0.0000000298); //dDDS
    }else{
        regWriteFloat(AUDIO_DEVS::A2B_RX_MULT_COEF, 0.00000000046566);//DATA
    }
    setA2BRxEnable(1);
    return 0;
}

int LibHWAudio::setA2BSendcfg(bool isInt, float coef, SAMPLING_RATE rate)
{
    float mult_coef = 0.0;
    float coef_div = 1.0;
    uint32_t iMode = 0;
    uint32_t iRate = 48000;
    uint32_t blck_cnt = 0;
    uint32_t sync_cnt = 0;
    uint16_t bits_sam = 0;
    iMode = registerRead(AUDIO_DEVS::A2B_MODE);
    blck_cnt = registerRead(AUDIO_DEVS::A2B_BCLK_CNT);
    sync_cnt = registerRead(AUDIO_DEVS::A2B_SYNC_CNT);
    usleep(1000);
    if((blck_cnt != 0) && (sync_cnt !=0)){
        bits_sam = blck_cnt / (sync_cnt * 2);
        iRate = sync_cnt * 100;
    }else{
        printf("A2Bconfig err\n");
        //return A2B_CONFIG_ERR;
    }
    setA2BTxEnable(0);

    coef_div = coef;
    mult_coef = regReadFloat(AUDIO_DEVS::A2B_TX_MULT_COEF);
    mult_coef = (float)(0x80000000) / coef_div; //33554432 // 0x80000000
    regWriteFloat(AUDIO_DEVS::A2B_TX_MULT_COEF, mult_coef);
    if(isInt){
        iMode = 0x2 | iMode;
        registerWrite(AUDIO_DEVS::A2B_MODE, iMode);
    }
    setA2BTxEnable(1);

    if(1 == (registerRead(AUDIO_DEVS::A2B_MODE) & 0x1)){
        switch (rate){
            case F48kHz:
                registerWrite(AUDIO_DEVS::A2B_SAMPLE_RATE, 0X0);
                break;
            case F96kHz:
                registerWrite(AUDIO_DEVS::A2B_SAMPLE_RATE, 0X1);
                break;
            case F192kHz:
                registerWrite(AUDIO_DEVS::A2B_SAMPLE_RATE, 0X2);
                break;
            default:
                break;
            }
    }else{
        switch (iRate){
            case 48000:
                registerWrite(AUDIO_DEVS::A2B_SAMPLE_RATE, 0X0);
                break;
            case 96000:
                registerWrite(AUDIO_DEVS::A2B_SAMPLE_RATE, 0X1);
                break;
            case 192000:
                registerWrite(AUDIO_DEVS::A2B_SAMPLE_RATE, 0X2);
                break;
            default:
            registerWrite(AUDIO_DEVS::A2B_SAMPLE_RATE, 0X0);
                break;
            }
        }
    return 0;
}

int LibHWAudio::sendA2BData(char *pBuf, int iLength, int *iActLen)
{
    int fd = 0;
    int ret = 0;
    uint32_t len = 0;
    long length = 0;
    uint32_t min_data = 8;
    uint32_t ddr_head = 0;
    uint32_t ddr_tail = 0;
    uint32_t ddr_full = 0;
    uint32_t trans_num = 0;
    //
    uint32_t total_read = 0;
    uint32_t len_avail = 0;
    *iActLen = 0;
    length = iLength;
    length = min(0x1FFFFFFF, length) & (~0x7);

    trans_num = registerRead(AUDIO_DEVS::TRANS_NUM);
    // 读取环形缓冲区
    while(total_read < length){
        ddr_head = registerRead(AUDIO_DEVS::A2B_HEAD_TX);
        ddr_tail = registerRead(AUDIO_DEVS::A2B_TAIL_TX);
        ddr_full = registerRead(AUDIO_DEVS::A2B_FULL_TX);

        if(ddr_full){
            sleep(1);
            continue;
        }
        len_avail = (ddr_tail - ddr_head + 0x7FFFFFF) & 0x7FFFFFF;
        if(len_avail < min_data){
            usleep(1000);
            continue;
        }
        len = min(len_avail, length - total_read);
        len = min(0x100000,len)& (~0x7);
        if(len > 0){
            memcpy(map_A2B_dma_send, pBuf, len);
            pBuf+=len;
         }
		 XdmaAudioSend(m_iDMAFD, len, ddr_head + 0x38000000, 1, 9);
         registerWrite(AUDIO_DEVS::A2B_HEAD_TX , (ddr_head + len) & 0x7FFFFFF);
         total_read += len;
        }
    *iActLen = total_read;
    return 0;
}

int LibHWAudio::recvA2Bdata(char *pBuf, int iLength, int *iActLen)
{
    int fd = 0;
    int ret = 0;
    uint32_t len = 0;
    uint32_t min_data = 8;
    uint32_t ddr_head = 0;
    uint32_t ddr_tail = 0;
    uint32_t trans_num = 0;
    uint32_t total_read = 0;
    uint32_t len_avail = 0;
    long length = iLength;
    char* pBufTmp = pBuf;
    *iActLen = 0;

    trans_num = registerRead(AUDIO_DEVS::TRANS_NUM);
    // 读取环形缓冲区
    int flag = 0;
    while(total_read < length){
        ddr_head = registerRead(AUDIO_DEVS::A2B_HEAD_RX);
        ddr_tail = registerRead(AUDIO_DEVS::A2B_TAIL_RX);

        len_avail = (ddr_head - ddr_tail + 0x8000000)&0x7FFFFFF;

        if(len_avail < min_data){
            usleep(1000);
            flag++;
            if(flag > 10000){
                printf("no data \n");
                return DDR_EMPTY_ERR;
            }
            continue;
        }
        len = (ddr_head >= ddr_tail) ? (ddr_head - ddr_tail) : (0x8000000 - ddr_tail) & 0x7FFFFFF;
        len = min(len, length - total_read);
        len = min(0x100000, len) & (~0x7);
        if(len > 0){
			XdmaAudioRecv(m_iDMAFD, len, ddr_tail + 0x30000000, 1, 8);
            memcpy(pBufTmp, map_A2B_dma_recv, len);
            pBufTmp+=len;
            }
            registerWrite(AUDIO_DEVS::A2B_TAIL_RX, (ddr_tail + len) & 0x7FFFFFF);
            total_read +=len;
        }

    *iActLen = total_read;
    return 0;
}

int LibHWAudio::A2BRecvFile(const char *strFile, int sec)
{
    int status = 0;
    Wav wav;
    FILE* fp = fopen(strFile, "wb");
    if(fp == NULL)
    {
        printf("open error\n");
        return OPEN_ERR;
    }
    long length = sec * 48000 * 4 * 2;
    wave->getWavHead(2, 48000,length, &wav, m_a2bisInt);
    fwrite(&wav, sizeof(wav), 1, fp);
    char *arr = (char *)malloc(20480);
    int len = 0;
    do{
        recvA2Bdata(arr, 20480, &len);
        fwrite(arr, 1, len, fp);
    }
    while(len && (ftell(fp) <= length));
    free(arr);
    fclose(fp);
    return status;
}

int LibHWAudio::A2BSendFile(const char *strFile)
{
    int status = 0;
    int fd_out = open(strFile, O_RDWR | O_SYNC, 0666);
    if(fd_out < 0)
    {
        printf("open error\n");
        return OPEN_ERR;
    }
    wave->offsetToData(&fd_out);
    char *arr  = (char *) malloc(20480);
    int len = 0;
    int iActLen = 0;
    while(len = read(fd_out, arr, 20480))
    {
        sendA2BData(arr, len, &iActLen);
    }
    free(arr);
    close(fd_out);
    return status;
}

int LibHWAudio::getMagicNum() const
{
    return m_iMagicNum;
}

int LibHWAudio::getChlIndex() const
{
    return m_index;
}
#include <regex.h>
/// e.g. "^audio_([0-8])_in([0-3])"
bool LibHWAudio::verifyDevice(std::string strDes, std::string strSrc, int &index)
{
    bool isOk = false;
    ///1
    regex_t reg;
    regcomp(&reg, strDes.c_str(), REG_EXTENDED);
    const size_t nmatch = 3;
    regmatch_t pmatch [nmatch];

    int status = 0;
    ///2 进行匹配
    status = regexec(&reg, strSrc.c_str(), nmatch, pmatch, 0);
    if (status == REG_NOMATCH)
        isOk = false;
    else if(status == REG_NOERROR)
    {
        isOk = true;
        if (pmatch[2].rm_so >= 0)
        {
            std::string strTmp = strSrc.substr(pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
            if (!strTmp.empty())
                index = atoi(strTmp.c_str());
        }
    }
    ///3
    regfree(&reg);
    return isOk;
}

int LibHWAudio::setAudioInputCfg(int channel, AudInCfg *cfg)
{
    int ret = 0;
    float coef_default = 0.00000000046566;
    float coef = 1.0;
    coef = cfg->in_coef * coef_default;
    int m_en = 0;

    setAudioInputEnable(channel, 0, m_inputMode);

    unsigned int buf = 0;
    registerWrite(AUDIO_DEVS::BIAS_CURR_EN, buf);
    buf = registerRead(AUDIO_DEVS::BIAS_CURR_EN);
    if(cfg->IS_IEPE == 1){
        buf = (buf | (1 << channel));

    }
    else
        buf = (buf &(~(1<< channel)));
    registerWrite(AUDIO_DEVS::BIAS_CURR_EN, buf);

    regWriteFloat(AUDIO_DEVS::IN_MULT_COEF1 + channel *4, coef_default);
    registerWrite(AUDIO_DEVS::ADC_REG_TRIG, 1 );
    return ret;
}

int LibHWAudio::setAudioOutputCfg(int channel, AudOutCfg *cfg)
{
    int ret = 0;
    float coef_div = 1.0;
    coef_div = cfg->out_coef;
    float mult_coef = 0.0;
    int m_samp = 0;
    int reg_data = 0x200F;
    setOutputEnable(channel, 0, m_outputMode);

    mult_coef = regReadFloat(AUDIO_DEVS::OUT_MULT_COEF1 + channel * 4);
    mult_coef = (float)(0x80000000) * coef_div;
    regWriteFloat(AUDIO_DEVS::OUT_MULT_COEF1 + channel * 4, mult_coef);
    if(channel <2){
        registerWrite(AUDIO_DEVS::DAC_REG_DATA0, reg_data);
        registerWrite(AUDIO_DEVS::DAC_REG_TRIG0, 1);
        switch (cfg->rate) {
        case F48kHz:
            registerWrite(AUDIO_DEVS::OUT_SAMMPLE_RATE0,0);
            m_samp = 48000;
            break;
        case F96kHz:
            registerWrite(AUDIO_DEVS::OUT_SAMMPLE_RATE0,1);
            m_samp = 96000;
            break;
        case F192kHz:
            registerWrite(AUDIO_DEVS::OUT_SAMMPLE_RATE0,2);
            m_samp = 192000;
            break;
        default:
            registerWrite(AUDIO_DEVS::OUT_SAMMPLE_RATE0,0);
            m_samp = 48000;
            break;
        }
    }else{
        registerWrite(AUDIO_DEVS::DAC_REG_DATA1, reg_data);
        registerWrite(AUDIO_DEVS::DAC_REG_TRIG1, 1);
        switch (cfg->rate) {
        case F48kHz:
            registerWrite(AUDIO_DEVS::OUT_SAMMPLE_RATE1,0);
            m_samp = 48000;
            break;
        case F96kHz:
            registerWrite(AUDIO_DEVS::OUT_SAMMPLE_RATE1,1);
            m_samp = 96000;
            break;
        case F192kHz:
            registerWrite(AUDIO_DEVS::OUT_SAMMPLE_RATE1,2);
            m_samp = 192000;
            break;
        default:
            registerWrite(AUDIO_DEVS::OUT_SAMMPLE_RATE1,0);
            m_samp = 48000;
            break;
        }
    }
    int samp = registerRead(AUDIO_DEVS::OUT_SAMMPLE_RATE0);
    samp = registerRead(AUDIO_DEVS::OUT_SAMMPLE_RATE1);
    return ret;
}

int LibHWAudio::sendAudioData(int channel, char *pBuf, int iLength, int* iActLen)
{
    int fd_out = 0;
    int ret = 0;
    long length = 0;
    uint32_t len = 0;
    uint32_t min_data = 0x100;
    uint32_t ddr_head = 0;
    uint32_t ddr_tail = 0;
    uint32_t ddr_full = 0;
    uint32_t trans_num = 0;
    uint32_t totalWrite = 0;
    olen_avail[channel] = 0;
    *iActLen = 0;
    const int MAX_DATA_SIZE = 0x200000;
    if (iLength >= MAX_DATA_SIZE)
        length = MAX_DATA_SIZE;
    else
        length = iLength;

    length = min(0x3FFFFFFF, length) & (~0x7);
    // 读取环形缓冲
    while(totalWrite < length){
        ddr_head = registerRead(AUDIO_DEVS::DDR_HEAD_OUT1 + channel * 12);
        ddr_tail = registerRead(AUDIO_DEVS::DDR_TAIL_OUT1 + channel * 12);
        ddr_full = registerRead(AUDIO_DEVS::DDR_FULL_OUT1 + channel * 12);

        if(ddr_full){
            usleep(1000);
            continue;
        }

        olen_avail[channel] = (ddr_tail - ddr_head +  0x7FFFFFF) & 0x7FFFFFF;
        if(olen_avail[channel] < min_data){
            usleep(1000);
            continue;
        }
        len = min(olen_avail[channel], length - totalWrite);
        len = min(0x100000, len) & (~0x7);
        if(len > 0){

            memcpy(map_dma, pBuf, len);
            XdmaAudioSend(m_iDMAFD, len, ddr_head + 0x10000000 + 0x8000000 * channel, 1, m_index +4);
            pBuf+=len;
//            ret = read(fd_out, map_dma, len);
//            if(ret <0){
//                close(fd_out);
//                return READ_ERR;
         }
         registerWrite(AUDIO_DEVS::DDR_HEAD_OUT1 + channel *12, (ddr_head + len) & 0x7FFFFFF);
         totalWrite += len;
         total_write[channel] += len;
        }
    *iActLen = totalWrite;
//    close(fd_out);
    return 0;
}

int LibHWAudio::recvAudioData(int channel, char *pBuf, int iLength, int *iActLen)
{
    uint32_t len = 0;
    uint32_t min_data = 8;
    uint32_t ddr_head = 0;
    uint32_t ddr_tail = 0;
    uint32_t trans_num = 0;
    long length = iLength & (~0x07);
    char* pBufTmp = pBuf;
    uint32_t totalRead = 0;
    ilen_avail[channel] = 0;
    int flag = 0;
    int pause = -1;
    *iActLen = 0;
    // 读取环形缓冲
    while(totalRead < length){
        ddr_head = registerRead(AUDIO_DEVS::DDR_HEAD_IN1 + channel * 12);
        ddr_tail = registerRead(AUDIO_DEVS::DDR_TAIL_IN1 + channel * 12);
//        pause = registerRead(AUDIO_DEVS::IN_PAUSE);

        ilen_avail[channel] = (ddr_head - ddr_tail + 1 + 0x3FFFFFF) & 0x3FFFFFF;
        if(ilen_avail[channel] < min_data){
            usleep(1000);
            flag++;
            if(flag > 1000 /*&& (((pause >> channel) & 0x1) == 0) */){
                printf("no data \n");
                return DDR_EMPTY_ERR;
            }
            continue;
        }

        len = (ddr_head >= ddr_tail) ? (ddr_head - ddr_tail) : (0x4000000 - ddr_tail) & 0x3FFFFFF;
        //len = min(len_avail, length - total_read);
        len = min(len, length - totalRead);
        len = min(0x100000, len) & (~0x7);
        if(len > 0){
            XdmaAudioRecv(m_iDMAFD, len, ddr_tail + 0x4000000 * channel, 1, m_index);
            memcpy(pBufTmp, map_dma, len);
            pBufTmp+=len;
        }
        registerWrite(AUDIO_DEVS::DDR_TAIL_IN1 + channel * 12, (ddr_tail + len) & 0x3FFFFFF);
        totalRead += len;
        total_read[channel] +=len;
    }
    *iActLen = totalRead;
    return 0;
}

int LibHWAudio::sendAudioFile(int iChannel, const char *strFile)
{
    int status = 0;
    total_write[iChannel] = 0;
    olen_avail[iChannel] = 0;
    int fd_out = open(strFile, O_RDWR | O_SYNC, 0666);
    if(fd_out < 0)
    {
        printf("open error\n");
        return OPEN_ERR;
    }
    ///Suspend false
    setAudioOutputSuspend(iChannel, 0);
    ///文件大小
    long maxLend = lseek(fd_out, 0, SEEK_END);
    ///偏移数据区
    wave->offsetToData(&fd_out);
    ///文件头部大小
    long headLen = lseek(fd_out, 0, SEEK_CUR);

    m_stSendFileStatus[iChannel].strFile = std::string(strFile);
    m_stSendFileStatus[iChannel].iActOperLen = headLen;
    m_stSendFileStatus[iChannel].iTotalLength = maxLend;

    char *arr  = (char *) malloc(204800);
    int len = 0;
    int iActLen = 0;
    int totalTrans = 0;
    for(;(len = read(fd_out, arr, 204800)) != 0
        && getSendTransStatus(iChannel) == DevTransStatus::DATA_IDLE;
        totalTrans += len)
    {
        sendAudioData(iChannel, arr, len, &iActLen);
    }
    m_stSendFileStatus[iChannel].iActOperLen += totalTrans;
    free(arr);
    close(fd_out);
    return status;
}

int LibHWAudio::recvAudioFile(int iChannel, const char *strFile, int sec)
{
    int status = 0;
    Wav wav;
#if 0
    FILE* fp = fopen(strFile, "w");
    if(fp == NULL)
    {
        printf("open error\n");
        return OPEN_ERR;
    }
    total_read[iChannel] = 0;
    ilen_avail[iChannel] = 0;
    ///Suspend false
    setAudioInputSuspend(iChannel, 0);
    int samp = registerRead(AUDIO_DEVS::ADC_REG_DATA);
    if(samp == 0x2237){
        m_audioSam = 48000;
    }
    else if(samp == 0x221F){
        m_audioSam = 96000;
    }
    else if(samp == 0x2207){
        m_audioSam = 192000;
    }else {
        m_audioSam = 48000;
    }

    long length = sec * m_audioSam * 4 * (m_inputMode +1); //(m_inputMode);
    wave->getWavHead(m_inputMode + 1, m_audioSam, length , &wav, m_isInt);
    fwrite(&wav, sizeof(wav), 1, fp);

    m_stRecvFileStatus[iChannel].strFile = std::string(strFile);
    m_stRecvFileStatus[iChannel].iTotalLength = length + sizeof (Wav);
    m_stRecvFileStatus[iChannel].iActOperLen = sizeof (Wav);

    char *arr  = (char *) malloc(204800);
    int len = length;
    int tolRead = 0;
    int bufSize = 0;
    for(;len && tolRead <= length
        && getRecvTransStatus(iChannel) == DATA_IDLE;){
        bufSize = min(length - tolRead, 204800);
        recvAudioData(iChannel, arr, bufSize, &len);
        fwrite(arr, 1, len, fp);
        tolRead += len;
    }

    m_stRecvFileStatus[iChannel].iActOperLen += tolRead;
    fflush(fp);
    registerWrite(AUDIO_DEVS::BIAS_CURR_EN, 0x0);
    free(arr);
    fclose(fp);
#else
    int fp = open(strFile, O_CREAT | O_RDWR | O_SYNC | O_TRUNC, 0666);
    if(fp < 0)
    {
        printf("open error\n");
        return OPEN_ERR;
    }
    total_read[iChannel] = 0;
    ilen_avail[iChannel] = 0;
    ///Suspend false
    setAudioInputSuspend(iChannel, 0);
    int samp = registerRead(AUDIO_DEVS::ADC_REG_DATA);
    if(samp == 0x2237){
        m_audioSam = 48000;
    }
    else if(samp == 0x221F){
        m_audioSam = 96000;
    }
    else if(samp == 0x2207){
        m_audioSam = 192000;
    }else {
        m_audioSam = 48000;
    }

    long length = sec * m_audioSam * 4 * (m_inputMode +1); //(m_inputMode);
    wave->getWavHead(m_inputMode + 1, m_audioSam, length , &wav, m_isInt);
    write(fp, &wav, sizeof(wav));

    m_stRecvFileStatus[iChannel].strFile = std::string(strFile);
    m_stRecvFileStatus[iChannel].iTotalLength = length + sizeof (Wav);
    m_stRecvFileStatus[iChannel].iActOperLen = sizeof (Wav);

    char *arr  = (char *) malloc(204800);
    int len = length;
    int tolRead = 0;
    int bufSize = 0;
    for(;len && tolRead <= length
        && getRecvTransStatus(iChannel) == DATA_IDLE;){
        bufSize = min(length - tolRead, 204800);
        recvAudioData(iChannel, arr, bufSize, &len);
        write(fp, arr, len);
        tolRead += len;
    }

    m_stRecvFileStatus[iChannel].iActOperLen += tolRead;
    registerWrite(AUDIO_DEVS::BIAS_CURR_EN, 0x0);
    free(arr);
    close(fp);
#endif
    return status;
}

int LibHWAudio::recvAudioUniqueFile(int iChannel, const char *strFile, int sec)
{
    int status = 0;
    if (m_recvlock[iChannel].test_and_set())
        return -1;
    status = recvAudioFile(iChannel, strFile, sec);
    m_recvlock[iChannel].clear();
    return status;
}

int LibHWAudio::sendAudioUniqueFile(int iChannel, const char *strFile)
{
    int status = 0;
    if (m_sendlock[iChannel].test_and_set())
        return -1;
    status = sendAudioFile(iChannel, strFile);
    m_sendlock[iChannel].clear();
    return status;
}

int LibHWAudio::sendAudioContinueFile(int iChannel)
{
    int status = 0;
    std::string strFile = m_stSendFileStatus[iChannel].strFile;
    if (strFile.empty())
        return OPEN_ERR;
    if (m_stSendFileStatus[iChannel].getAvalidLen() <= 0)
        return OPEN_ERR;
    int fd_out = open(strFile.c_str(), O_RDWR | O_SYNC, 0666);
    if(fd_out < 0)
    {
        printf("open error\n");
        return OPEN_ERR;
    }

    ///偏移数据区
    long currPos = m_stSendFileStatus[iChannel].iActOperLen;
    lseek(fd_out, currPos, SEEK_SET);
    ///文件头部大小
    long headLen = lseek(fd_out, 0, SEEK_CUR);

    char *arr  = (char *) malloc(204800);
    int len = 0;
    int iActLen = 0;
    int totalTrans = 0;
    for(;(len = read(fd_out, arr, 204800)) != 0
        && getSendTransStatus(iChannel) == DevTransStatus::DATA_IDLE;
        totalTrans += len)
    {
        sendAudioData(iChannel, arr, len, &iActLen);
        printf("%s_______ \n", __FUNCTION__);
    }
    m_stSendFileStatus[iChannel].iActOperLen += totalTrans;

    free(arr);
    close(fd_out);
    return status;
}
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)
int LibHWAudio::recvAudioContinueFile(int iChannel)
{
    int status = 0;

    std::string strFile = m_stRecvFileStatus[iChannel].strFile;
    if (strFile.empty())
        return OPEN_ERR;
    int length = m_stRecvFileStatus[iChannel].getAvalidLen();
    if (length <= 0)
        return OPEN_ERR;
#if 0
    FILE* fp = fopen(m_stRecvFileStatus[iChannel].strFile.c_str(), "ab+");
    if(fp == NULL)
    {
        printf("open error\n");
        return OPEN_ERR;
    }

    char *arr  = (char *) malloc(204800);

    int len = length;
    int tolRead = 0;
    int bufSize = 0;
    for(;len && tolRead <= length
        && getRecvTransStatus(iChannel) == DATA_IDLE;){
        bufSize = min(length - tolRead, 204800);
        recvAudioData(iChannel, arr, bufSize, &len);
        fwrite(arr, 1, len, fp);
        tolRead += len;
    }
    m_stRecvFileStatus[iChannel].iActOperLen += tolRead;

    registerWrite(AUDIO_DEVS::BIAS_CURR_EN, 0x0);
    fflush(fp);
    free(arr);
    fclose(fp);
#else
    int fd = open(m_stRecvFileStatus[iChannel].strFile.c_str(),
                  O_RDWR | O_SYNC, 0666);
    if (fd < 0)
    {
        return OPEN_ERR;
    }
    lseek(fd, m_stRecvFileStatus[iChannel].iActOperLen, SEEK_SET);
    char *arr  = (char *) malloc(204800);

    int len = length;
    int tolRead = 0;
    int bufSize = 0;
    for(;len && tolRead <= length
        && getRecvTransStatus(iChannel) == DATA_IDLE;){
        bufSize = min(length - tolRead, 204800);
        recvAudioData(iChannel, arr, bufSize, &len);
        write(fd, arr, len);
        tolRead += len;
        printf("%s__[%d][%d][%d][%d][%d]\n", __FUNCTION__, gettid(), total_read, tolRead, length, m_stRecvFileStatus[iChannel].iActOperLen);
    }
    m_stRecvFileStatus[iChannel].iActOperLen += tolRead;

    registerWrite(AUDIO_DEVS::BIAS_CURR_EN, 0x0);
    free(arr);
    close(fd);

#endif
    return status;
}

int LibHWAudio::getInputMode() const
{
    return m_inputMode;
}

int LibHWAudio::getOutputMode() const
{
    return m_outputMode;
}

int LibHWAudio::recvAudioUniqueContinueFile(int iChannel)
{
    int status = 0;
    if (m_recvlock[iChannel].test_and_set())
        return -1;
    status = recvAudioContinueFile(iChannel);
    m_recvlock[iChannel].clear();
    return status;
}

int LibHWAudio::sendAudioUniqueContinueFile(int iChannel)
{
    int status = 0;
    if (m_sendlock[iChannel].test_and_set())
        return -1;
    status = sendAudioContinueFile(iChannel);
    m_sendlock[iChannel].clear();
    return status;
}

void LibHWAudio::setSendTransStatus(int iChannel, int isPause)
{
    switch(isPause){
    case 0:
        if(m_outputMode == 1 && (iChannel == 0 || iChannel == 2)){
            m_stSendStatus[iChannel +1] = DATA_IDLE;
        }
        m_stSendStatus[iChannel] = DATA_IDLE;
        break;
    case 1:
        if(m_outputMode == 1 && (iChannel == 0 || iChannel == 2)){
            m_stSendStatus[iChannel +1] = DATA_STOP;
        }
        m_stSendStatus[iChannel] = DATA_STOP;
        break;
    default:
        break;
    }
}

void LibHWAudio::setRecvTransStatus(int iChannel, int isPause)
{
    switch(isPause){
    case 0:
        if(m_inputMode == 1 && (iChannel == 0 || iChannel == 2)){
            m_stRecvStatus[iChannel +1] = DATA_IDLE;
        }
        m_stRecvStatus[iChannel] = DATA_IDLE;
        break;
    case 1:
        if(m_inputMode == 1 && (iChannel == 0 || iChannel == 2)){
            m_stRecvStatus[iChannel +1] = DATA_STOP;
        }
        m_stRecvStatus[iChannel] = DATA_STOP;
        break;
    default:
        break;
    }
}

LibHWAudio::DevTransStatus LibHWAudio::getSendTransStatus(int iChannel)
{
    return m_stSendStatus[iChannel];
}

LibHWAudio::DevTransStatus LibHWAudio::getRecvTransStatus(int iChannel)
{
    return m_stRecvStatus[iChannel];
}

