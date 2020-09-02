#include "LibHWAio.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <cmath>
#include "xpdma.h"
#include <string.h>

#define min(a, b)   (((a) < (b)) ? (a) : (b))

LibHWAio::LibHWAio(std::string strDev, DevOriention eOrient) : m_strDev(strDev)
{
    switch (eOrient) {
    case DEV_IN:
        m_iMagicNum = AIO_DEVS::MAGIC_IN;
        break;
    case DEV_OUT:
        m_iMagicNum = AIO_DEVS::MAGIC_OUT;
        break;
    default:
        break;
    }
}

LibHWAio::~LibHWAio()
{
    if (NULL != map_user)
    {
        munmap(map_user, MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE);
    }
    uninitDevs();
}

int LibHWAio::initDevs()
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
    map_user = (char*)mmap(NULL,MAX_MAP_USER_SIZE + MAX_MAP_DMA_SIZE,   PROT_READ | PROT_WRITE, MAP_SHARED, m_iConfigureFD,0);

    ///2
    m_iDMAFD = m_iConfigureFD;
    map_dma = (char*) (map_user + MAX_MAP_USER_SIZE);

    registerWrite(AIO_DEVS::OVER_CURRENT_TIME, 1000);
    operOverCurrentProtection();
    for(int i = 0; i < 4; i++){
        setLevelOutputEnable(1, i);
    }
    return ret;
}
#include <stdio.h>

int LibHWAio::getVersion(std::string &str)
{
    int ret = 0;
    if(m_iConfigureFD == 0)
        ret = -1;
    unsigned int buf = 0;
    buf = registerRead(AIO_DEVS::DATETIME);
    char arr [10] = {0};
    sprintf(arr,"%x", buf);
    str = std::string(arr, 8);

    buf = registerRead(AIO_DEVS::VERSION);
    sprintf(arr,"%x", buf);
    str.append(std::string(arr,8));
    return ret;
}

int LibHWAio::uninitDevs()
{
    int ret = 0;
    ret = close(m_iConfigureFD);

    m_iConfigureFD = 0;
    m_iDMAFD = 0;
    return ret;
}

uint32_t LibHWAio::dmaRecv( uint32_t length, uint32_t addr)
{
    int ret = -1;
    ret = XdmaRecv(m_iDMAFD, length, addr);
    if(ret !=0)
    {
        printf("dma read error %d\n", ret);

    }
    printf("read buffer (length = %d)\n", length);
}

uint32_t LibHWAio::dmaSend(uint32_t length, char *buffer, uint32_t addr)
{
    int ret = -1;
    int i = 0;
    for(i = 0; i < length; i++)
    {
        map_dma[i] = buffer[i];
    }
    ret = XdmaSend(m_iDMAFD, length, addr);
}

void LibHWAio::registerWrite(uint32_t addr, uint32_t value)
{
    *((volatile uint32_t *)(map_user + addr)) = value;
}

int LibHWAio::registerRead(uint32_t addr)
{
    int reg_value = 0;
    reg_value = *((volatile uint32_t*) (map_user + addr));
    return reg_value;
}

int LibHWAio::setLevelInputConfig(PGIACONFIG pgia, int levelDelay, int levelAvg)
{
    m_gain = 0;
    registerWrite(AIO_DEVS::ADC_RESET, 0x1);
    usleep(1000);
    registerWrite(AIO_DEVS::AI_MODE, 0);
    switch(pgia)
    {
    case PGIA_24_48V:
        registerWrite(AIO_DEVS::ADC_CONFIG, 0XF7FE);
        m_gain = 2048;
        break;
    case PGIA_0_64V:
        registerWrite(AIO_DEVS::ADC_CONFIG, 0XF6FE);
        m_gain = 64;
        break;
    case PGIA_1_28V:
        registerWrite(AIO_DEVS::ADC_CONFIG, 0XF67E);
        m_gain = 128;
        break;
    case PGIA_2_56V:
        registerWrite(AIO_DEVS::ADC_CONFIG, 0XF5FE);
        m_gain = 256;
        break;
    case PGIA_5_12V:
        registerWrite(AIO_DEVS::ADC_CONFIG, 0XF57E);
        m_gain = 512;
        break;
    case PGIA_10_24V:
        registerWrite(AIO_DEVS::ADC_CONFIG, 0XF4FE);
        m_gain = 1024;
        break;
    default:
        break;
    }
    registerWrite(AIO_DEVS::AI_LEVEL_DELAY,levelDelay);
    registerWrite(AIO_DEVS::AI_LEVEL_AVGEXP, levelAvg);
    registerWrite(AIO_DEVS::ADC_ASR0, 0xffff);

    registerWrite(AIO_DEVS::ADC_RESET, 0x0);
    usleep(1000);
    if(registerRead(AIO_DEVS::ADC_RESET) != 0){
        registerWrite(AIO_DEVS::ADC_RESET, 0x0);
    }
    registerWrite(AIO_DEVS::ADC_CONFIG_EN, 0xffff);
    usleep(50000);
    return 0;
}

int LibHWAio::getLevelInput(int* arr)
{
    int i = 0;
    int iVol = 0;
    int tmp = 0;
    memset(arr, 0, 32);
    for(i = 0; i < 32; i++)
    {

       tmp =  (int16_t)registerRead(AIO_DEVS:: AI_LEVEL0 + i*4);
       iVol = ((double)tmp / 32727) * m_gain * 7.8125 /100*1000;
       arr[i] = iVol;

    }

    return 0;
}

int LibHWAio::setWaveformConfig(PGIACONFIG pgia, uint32_t channel)
{

    registerWrite(AIO_DEVS::ADC_RESET, 0x01);
        switch(pgia)
        {
        case PGIA_24_48V:
            registerWrite(AIO_DEVS::ADC_CONFIG, 0XF7FE);
            break;
        case PGIA_0_64V:
            registerWrite(AIO_DEVS::ADC_CONFIG, 0XF6FE);
            break;
        case PGIA_1_28V:
            registerWrite(AIO_DEVS::ADC_CONFIG, 0XF67E);
            break;
        case PGIA_2_56V:
            registerWrite(AIO_DEVS::ADC_CONFIG, 0XF5FE);
            break;
        case PGIA_5_12V:
            registerWrite(AIO_DEVS::ADC_CONFIG, 0XF57E);
            break;
        case PGIA_10_24V:
            registerWrite(AIO_DEVS::ADC_CONFIG, 0XF4FE);
            break;
        default:
            break;
        }
        registerWrite(AIO_DEVS::ADC_ASR0, 0x7FFF);
        if(channel < 8){
            registerWrite(AIO_DEVS::ADC_SW0, channel);
        }
        else if(channel >=8 && channel <16){
            registerWrite(AIO_DEVS::ADC_SW1, (channel % 8));
        }
        else if(channel >=16 && channel < 24){
            registerWrite(AIO_DEVS::ADC_SW2, (channel % 8));
        }
        else if(channel >=24 && channel < 32){
            registerWrite(AIO_DEVS::ADC_SW3, (channel % 8));
        }
        registerWrite(AIO_DEVS::ADC_RESET, 0x0);
        registerWrite(AIO_DEVS::ADC_CONFIG_EN, 0x1);
        registerWrite(AIO_DEVS::AI_MODE, 0x1);
        usleep(1000);
        return 0;
}

int LibHWAio::getWaveformInput(char *FilePath, uint32_t length)
{
    int fd = -1;
    int ret = 0;
    uint32_t len  = 0;
    uint32_t ddr_head = 0;
    uint32_t ddr_tail = 0;
    uint32_t trans_num = 0;
    uint32_t min_data = 8;
    len_avail = 0;
    total_read = 0;

    fd = open(FilePath, O_CREAT | O_TRUNC | O_RDWR | O_SYNC, 0666);
    if(fd < 0){
        printf("open error\n");
        return -1;
    }
    registerWrite(AIO_DEVS::ADC_CONFIG_EN, 1);      //enable

    trans_num = registerRead(AIO_DEVS::DDR_TRANS_NUM);      //trans len
    length &= (~0x7);
    while(total_read < length){
        ddr_head = registerRead(AIO_DEVS::DDR_HEAD);
        ddr_tail = registerRead(AIO_DEVS::DDR_TAIL);

        len_avail = (ddr_head - ddr_tail +1 + 0x3FFFFFFF) & 0x3FFFFFFF;
        if(len_avail < min_data){
            usleep(1000);
            continue;
        }

        len = min(len_avail, length - total_read);
        len = min(0x400000, len) & (~0x7);

        if(len > 0){
            XdmaRecv(m_iDMAFD, len, ddr_tail);
            ret = write(fd, map_dma, len);
            if(ret < 0){
                printf("write error\n");
                close(fd);
                return -2;
            }
            registerWrite(AIO_DEVS::DDR_TAIL, (ddr_tail + len) & 0x3FFFFFFF);
            total_read += len;
        }
    }
    close(fd);
    return 0;
}

int LibHWAio::getWaveformInputStatus(uint32_t *buf_ddr_len, uint32_t *buf_total_len)
{
    *buf_ddr_len = len_avail;
    *buf_total_len = total_read;
    return 0;
}

int LibHWAio::setLevelOutput( uint32_t channel, uint32_t vol)
{

    uint32_t offset = 0x4108;
    int m_vol = 0;
    //registerWrite(AIO_DEVS::DAC_ENABLE, 0);
    m_vol = (((double)vol / 1000) * 65536 / 5 / 5);
    registerWrite(AIO_DEVS::OVER_CURRENT_TIME, 1000);

    registerWrite(0x4100, 2);
    registerWrite(0x4100, 1);

    //data
    if(channel < 8){
        registerWrite(offset, 0x118);
        registerWrite(offset, (0x30 + channel));
       // registerWrite(AIO_DEVS::DAC_ENABLE, 1 );
    }
    else if(channel >=8 && channel <16){
        registerWrite(offset, 0x11a);
        registerWrite(offset, (0x30 + (channel % 8)));
       // registerWrite(AIO_DEVS::DAC_ENABLE, 2);
    }
    else if(channel >=16 && channel < 24){
        registerWrite(offset, 0x11c);
        registerWrite(offset, (0x30 + (channel % 8)));
        //registerWrite(AIO_DEVS::DAC_ENABLE, 4);
    }
    else if(channel >=24 && channel < 32){
        registerWrite(offset, 0x11e);
        registerWrite(offset, (0x30 + (channel % 8)));
        //registerWrite(AIO_DEVS::DAC_ENABLE, 8);
    }

    registerWrite(offset, (m_vol >>8) & 0xff);
    registerWrite(offset, (0x200 | (m_vol & 0xff)));
    return 0;
}

int LibHWAio::setLevelOutputEnable(int is_en, int dac_num)
{
    int m_en = 0;
    m_en = registerRead(AIO_DEVS::DAC_ENABLE);
    switch (is_en) {
    case 0:
        registerWrite(AIO_DEVS::DAC_ENABLE, (~(1 << dac_num)) & m_en);
        break;
    case 1:
        registerWrite(AIO_DEVS::DAC_ENABLE, (1 << dac_num) | m_en);
        break;
    default:
        break;
    }
}

int LibHWAio::setOverCurrentProtect()
{
    //3
    int value = 0;
    value = registerRead(AIO_DEVS::OVER_CURRENT);
    if(value != 0){
        registerWrite(AIO_DEVS::OVER_CURRENT_CLEAR, 0x1);
    }
//    while(0 != registerRead(AIO_DEVS::OVER_CURRENT)){
//        registerWrite(AIO_DEVS::OVER_CURRENT_CLEAR, 0x1);
//        usleep(100);
//    }

    return 0;
}
#include <regex.h>
/// e.g. "^aio_([0-8])_in"
bool LibHWAio::verifyDevice(std::string strDes, std::string strSrc)
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

int LibHWAio::getMagicNum() const
{
    return m_iMagicNum;
}
void LibHWAio::operOverCurrentProtection()
{
    ///TODO:
}

int LibHWAio::getLevelInputInfo(InputInfo *info)
{
    int gain = 0;
    gain = registerRead(AIO_DEVS::ADC_CONFIG);
    switch(gain){
    case 0XF7FE:
        info->pgia = 20.48;
        break;
    case 0XF6FE:
        info->pgia = 0.64;
        break;
    case 0xF67E:
        info->pgia = 1.28;
        break;
    case 0XF5FE:
        info->pgia = 2.56;
        break;
    case 0XF57E:
        info->pgia = 5.12;
        break;
    case 0XF4FE:
        info->pgia = 10.24;
        break;
    default:
        break;
    }
}

int LibHWAio::setlevelInputAutoConfigPgia()
{
    int i = 0;
    uint32_t iGain = 0;
    int arr[32] = {0};
    float max = 0;
    float iMin = 0;

    getLevelInput(arr);
    max = arr[0];
    for(i = 1; i < 32; i++){
        if(arr[i] > max){
            max = arr[i];
        }
    }
    iMin = arr[0];
    for(i = 1; i < 32; i++){
        if(arr[i] < iMin){
            iMin = arr[i];
        }
    }
    if((iMin > (-5000)) &&  max < 5000){
        iGain = 1;
    }else if((5000 <= max && max < 10000)
             || ((iMin >= (-10000)) && (iMin <= (-5000)))){
        iGain = 2;
    }else if((10000 <= max && max < 20000)
             || ((iMin >= -20000) && (iMin < -10000))){
        iGain = 3;
    }else if((20000 <= max && max < 40000)
             || ((iMin >= -40000) && (iMin < -20000))){
        iGain = 4;
    }else if((40000 <= max && max < 80000)
             || ((iMin >= -80000) && (iMin < -40000))){
        iGain = 5;
    }else{
        iGain = 0;
    }
    setLevelInputConfig((PGIACONFIG)iGain, 7, 0);

    return 0;
}

int LibHWAio::setPpsUpdateCount(uint32_t sec)
{
    registerWrite(AIO_DEVS::PPS_SECOND_SET, sec);
    registerWrite(AIO_DEVS::PPS_SECOND_VALID, 1);
    return 0;
}

int LibHWAio::getWaveCaptureTime(uint64_t *mic_sec)
{
    uint32_t m_sec = 0;
    uint64_t m_na_sec = 0;
    uint64_t clk_coef = 0;
    uint64_t cal_counter = 0;
    registerRead(AIO_DEVS::CAL_DONE);
    cal_counter = registerRead(AIO_DEVS::CAL_COUNTER);
    m_sec = registerRead(AIO_DEVS::WAVE_SECONDS);
    m_na_sec = registerRead(AIO_DEVS::WAVE_NANO_SECONDS);

    clk_coef = 62.5e6 / cal_counter;
    m_na_sec = min(999999999, m_na_sec * clk_coef);
    *mic_sec = (uint64_t)m_sec * 1e6 + min(999999, m_na_sec / 1000);
    return 0;
}

int LibHWAio::getPpsSecCount(uint32_t *sec)
{
    *sec = registerRead(AIO_DEVS::PPS_COUNTER);
    return 0;
}

int LibHWAio::setPpsExportEnable(int en)
{
    unsigned int buf = 0;
    if (en != 0)
        buf = 0x1;
    registerWrite(AIO_DEVS::PPS_EXPORT_EN, buf);
    return 0;
}
///
/// \brief LibHWAio::setPpsEdgeSel
/// \param edge  0:下降沿有效 1:上升沿有效
/// \return
///
int LibHWAio::setPpsEdgeSel(int edge)
{
    unsigned int buf = 0;
    if (edge != 0)
        buf = 0x1;
    registerWrite(AIO_DEVS::PPS_EDGE_SEL, buf);
    return 0;
}
