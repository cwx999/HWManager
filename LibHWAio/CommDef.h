#ifndef COMMDEF_H
#define COMMDEF_H

namespace AIO_DEVS {
enum AIO_REG{
    //通用寄存器
    SLOT = 0X000,
    DATETIME = 0X004,
    VERSION = 0X008,
    RW_TEST = 0X400,
    //AI相关寄存器
    AI_MODE = 0X824,    // 模拟输入模式   0:电平模式  1：波形模式
    AI_LEVEL0 = 0X500,  // 通道0输入电平  bit[15:0]代表ADC采样值
    AI_LEVEL1 = 0X504,
    AI_LEVEL2 = 0X508,
    AI_LEVEL3 = 0X50C,
    AI_LEVEL4 = 0X510,
    AI_LEVEL5 = 0X514,
    AI_LEVEL6 = 0X518,
    AI_LEVEL7 = 0X51C,
    AI_LEVEL8 = 0X520,
    AI_LEVEL9 = 0X524,
    AI_LEVEL10 = 0X528,
    AI_LEVEL11 = 0X52C,
    AI_LEVEL12 = 0X530,
    AI_LEVEL13 = 0X534,
    AI_LEVEL14 = 0X538,
    AI_LEVEL15 = 0X53C,
    AI_LEVEL16 = 0X540,
    AI_LEVEL17 = 0X544,
    AI_LEVEL18 = 0X548,
    AI_LEVEL19 = 0X54C,
    AI_LEVEL20 = 0X550,
    AI_LEVEL21 = 0X554,
    AI_LEVEL22 = 0X558,
    AI_LEVEL23 = 0X55C,
    AI_LEVEL24 = 0X560,
    AI_LEVEL25 = 0X564,
    AI_LEVEL26 = 0X568,
    AI_LEVEL27 = 0X56C,
    AI_LEVEL28 = 0X570,
    AI_LEVEL29 = 0X574,
    AI_LEVEL30 = 0X578,
    AI_LEVEL31 = 0X57C,
    AI_LEVEL_DELAY = 0X580,
    AI_LEVEL_AVGEXP = 0X584,

    ADC_RESET = 0X800,      //ADC复位     0：正常模式  1：复位
    ADC_CONFIG_EN = 0X804,  //ADC配置使能   1：配置生效  0：忽略
    ADC_CONFIG = 0X808,     //基础配置寄存器   需要在ADC复位信号为1时，才能配置
    ADC_ASR0 = 0X80C,       //ASR0配置寄存器,需要在ADC复位信号为1时，才能配置
    ADC_CLK_DIVIDER = 0X810,    //ADC时钟分频计数器,需要在ADC复位信号为1时，才能配置
    ADC_SW0 = 0X814,        //输入通道0~7八选一切换开关。
    ADC_SW1 = 0X818,
    ADC_SW2 = 0X81C,
    ADC_SW3 = 0X820,

    DDR_HEAD = 0X880,       //DDR环形缓冲区头部（写DDR地址）
    DDR_TAIL = 0X884,       //DDR环形缓冲区尾部（读DDR地址）
    DDR_TRANS_NUM = 0X888,  //DDR每次传输长度
    OVER_CURRENT = 0X8c0,
    OVER_CURRENT_CLEAR = 0X8C4,
    OVER_CURRENT_TIME = 0X8C8,
    DAC_ENABLE = 0X8CC,
    //PPS相关寄存器
    PPS_SOURCE = 0X900,
    PPS_EDGE_SEL = 0X904,
    PPS_TRIG_SEL = 0X908,
    PPS_SECOND_SET = 0X90C,
    PPS_SECOND_VALID = 0X910,
    PPS_EXPORT_EN = 0X914,
    PPS_SECONDS = 0X918,
    PPS_NANO_SECONDS = 0X91C,
    PPS_COUNTER = 0X920,
    WAVE_SECONDS = 0X840,
    WAVE_NANO_SECONDS = 0X844,
    CAL_COUNTER = 0X940,
    CAL_DONE = 0X944
    };
enum StatusCode{
    FAILURE_RET = -1,
    sUCCESS_RET = 0
    };
}

#endif // COMMDEF_H
