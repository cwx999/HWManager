#include <QCoreApplication>
#include <LibHWDio_global.h>
#include <iostream>
#include <string.h>
#include <QString>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    std::string str;
    int iSlot = -1;
    int iChannel = -1;
    int i = 0;
    #if 0
    while (1) {
        std::cout << "input SLOT:"<< std::endl;
        std::cin >> iSlot;

        std::cout << "input Channel:"<< std::endl;
        std::cin >> iChannel;

        std::cout  << "command input :" <<std::endl;
        std::cin >> str;

        char arrayTmp [20];
        int iA = 0;

        if ("init" == str)
        {
            i = initDIO(iSlot);
            std::cout << "initDIO  RET:" << i;
        }

        if ("getVersion" == str)
        {
            i = getVersion(iSlot, arrayTmp, 20, &iA);
            std::cout << "getVersion  RET:" << i;
            std::cout << "version" << std::string(arrayTmp)<<std::endl;
        }

        if ("ReferenceVoltage" == str)
        {
            int iVol = 0;
            std::cout  << "Refvol Input :" <<std::endl;
            std::cin >> iVol;
            setDIOInputReferenceVoltage(iSlot, iChannel, iVol);
        }

        if ("Mode" == str)
        {
            int iMode = 0;
            std::cout  << "ModeType Input :" <<std::endl;
            std::cin >> iMode;
            setDIOOutputMode(iSlot, iChannel, (OutputMode)iMode);
        }
        if ("getVoltage" == str)
        {
            int iVol = -1;
            getInputLevel(iSlot, iChannel, &iVol);
            std::cout  <<"SLOT:" << iSlot<< "CHANNEL:"<< iChannel<<"Voltage :"<< iVol <<std::endl;
        }
        if ("pwmtest" == str)
        {
            DIPWMConfigure stCfg;
            stCfg.dDurationTime = 10;
            stCfg.eRefClk = REFCLK_20_MHZ;
            stCfg.iChannel = iChannel;
            setInputPWMConfigure(iSlot, stCfg);
            setPWMCaptureEnableStatus(iSlot, iChannel, ENABLE_STATUS);
        }
        if ("pwmget" == str)
        {
            PWMProperty arr[32];
            getInputPWMProperty(iSlot, arr);
            std::cout << "Freq:" << arr[iChannel].dFreq << std::endl;
            std::cout << "Duty:" << arr[iChannel].dDuty << std::endl;
        }
        if ("ditest" == str)
        {
            setDIOInputReferenceVoltage(iSlot, iChannel, 0);
        }
        if ("diget" == str)
        {
            int i = -1;
            getInputLevel(iSlot, iChannel, &i);
            std::cout << "voltage:" << i;
            getAllInputLevel(iSlot, &i);
            std::cout << "all vol :" << i;
        }
        if ("exit" == str)
        {
            i = uninitDIO(iSlot);
            std::cout << "uninitDIO  RET:" << i << std::endl;
            break;
        }
    }
    #endif
    return a.exec();
}
