#ifndef COMMDEF_H
#define COMMDEF_H
namespace AUDIO_DEVS {
enum AUDIO_REG{
    SLOT = 0X000,         //	槽位号	000	R	5
    DATETIME = 0X004, // 	程序日期	004	R	32
    VERSION = 0X008,  //	程序版本	008	R	32
    RW_TEST = 0X400,	//读写测试	400	RW	32	0X1234ABCD
    ///音频采集相关寄存器
    TRANS_NUM = 0X480,
    ADC_REG_DATA = 0X484,
    ADC_REG_TRIG = 0X488,
    BIAS_CURR_EN = 0X4C0,
    BIAS_CURR_ERR = 0X4C4,
    IN_ENABLE = 0X800,
    IN_MODE = 0X804,
    IN_ATTEN = 0X808,
    IN_MULT_COEF1 = 0X80C,
    IN_MULT_COEF2 = 0X810,
    IN_MULT_COEF3 = 0X814,
    IN_MULT_COEF4 = 0X818,
    IN_PAUSE = 0X81C,
    DDR_HEAD_IN1 = 0X840,
    DDR_TAIL_IN1 = 0X844,
    DDR_FULL_IN1 = 0X848,

    DDR_HEAD_IN2 = 0X84C,
    DDR_TAIL_IN2 = 0X850,
    DDR_FULL_IN2 = 0X854,

    DDR_HEAD_IN3 = 0X858,
    DDR_TAIL_IN3 = 0X85C,
    DDR_FULL_IN3 = 0X860,

    DDR_HEAD_IN4 = 0X864,
    DDR_TAIL_IN4 = 0X868,
    DDR_FULL_IN4 = 0X86C,
    ///音频播放相关寄存器
    OUT_ENABLE = 0X900,
    OUT_MODE = 0X904,
    OUT_AMP = 0X908,
    OUT_SAMMPLE_RATE0 = 0X90C,
    OUT_SAMMPLE_RATE1 = 0X910,
    OUT_MULT_COEF1 = 0X914,
    OUT_MULT_COEF2 = 0X918,
    OUT_MULT_COEF3 = 0X91C,
    OUT_MULT_COEF4 = 0X920,
    OUT_REPEAT = 0X924,
    OUT_PAUSE = 0x928,
    DAC_REG_DATA0 = 0X48C,
    DAC_REG_TRIG0 = 0X490,
    DAC_REG_DATA1 = 0X494,
    DAC_REG_TRIG1 = 0X498,

    DDR_HEAD_OUT1 = 0X940,
    DDR_TAIL_OUT1 = 0X944,
    DDR_FULL_OUT1 = 0X948,

    DDR_HEAD_OUT2 = 0X94C,
    DDR_TAIL_OUT2 = 0X950,
    DDR_FULL_OUT2 = 0X954,

    DDR_HEAD_OUT3 = 0X958,
    DDR_TAIL_OUT3 = 0X95C,
    DDR_FULL_OUT3 = 0X960,

    DDR_HEAD_OUT4 = 0X964,
    DDR_TAIL_OUT4 = 0X968,
    DDR_FULL_OUT4 = 0X96C,

    ///A2B通信相关寄存器
    DDS_OUT_ENABLE = 0X49C,
    DDS_CFG_DATA = 0X4A0,
    DDS_CFG_TRIG = 0X4A4,
    A2B_MODE = 0XA00,
    A2B_TX_EN = 0XA04,
    A2B_RX_EN = 0XA08,
    A2B_IRQ = 0XA0C,
    A2B_HEAD_RX = 0XA10,
    A2B_TAIL_RX = 0XA14,
    A2B_FULL_RX = 0XA18,
    A2B_HEAD_TX = 0XA1C,
    A2B_TAIL_TX = 0XA20,
    A2B_FULL_TX = 0XA24,
    A2B_SAMPLE_RATE = 0XA28,
    A2B_SLAVE_BUSY = 0XA2C,
    A2B_BCLK_CNT = 0XA30,
    A2B_SYNC_CNT = 0XA34,
    A2B_RX_MULT_COEF = 0XA38,
    A2B_TX_MULT_COEF = 0XA3C,


    ///A2B配置相关寄存器
    ///

    A2B_BASE_ADDR = 0X6E, //0x1dc
    A2B_BUS_ADDR = 0X6F,  //0x1de
    A2B_CHIP = 0X00,
    A2B_NODEADDR = 0X01,
    A2B_VENDOR = 0X02,
    A2B_PRODUCT = 0X03,
    A2B_VERSION = 0X04,
    A2B_CAPABILITY = 0X05,
    A2B_SWCTL = 0X09,
    A2B_LDNSLOTS = 0X0B,
    A2B_LUPSLOTS = 0X0C,
    A2B_DNSLOTS = 0X0D,
    A2B_UPSLOTS = 0X0E,
    A2B_RESPCYS = 0XF,

    A2B_SLOTFMT = 0X10,
    A2B_DATCTL = 0X11,
    A2B_CONTROL = 0X12,
    A2B_DISCVRY = 0X13,
    A2B_INTSRC = 0X16,
    A2B_INTTYPE = 0X17,
    A2B_INTPND2 = 0X1A,
    A2B_INTMASK0 = 0X1B,
    A2B_INTMASK1 = 0X1C,
    A2B_INTMASK2 = 0X1D,
    A2B_BECCTL = 0X1E,
    //
    A2B_I2CCFG = 0X3F,
    A2B_I2SGCFG = 0X41,
    A2B_I2SCFG = 0X42,
    A2B_PDMCTL = 0X47,
    A2B_GPIODAT = 0X4A,
    A2B_GPIOOEN = 0X4D,
    A2B_PINCFG = 0X52,
    A2B_CLK2CFG = 0X5A,
    A2B_UPMASK0 = 0X60,
    A2B_DNMASK0 = 0X65

};

enum StatusCode
{
    FAILURE_RET = -1,
    SUCCESS_RET = 0
};
}
#endif // COMMDEF_H
