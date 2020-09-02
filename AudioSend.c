#include "LibHWAudio/LibHWAudio_global.h"
#include <stddef.h>
#include <stdio.h>
int main()
{
    int ret = 0;
    const char* strDev = "/dev/audio_5_out1";
    void * pHandle = NULL;
    ret = OpenAudio(strDev, &pHandle);
    ret = SetAudoMode(pHandle, 0);
    ret = SetAudoAMP(pHandle, 0);
    ret = SetAudoDataType(pHandle, 0);
    AudOutCfg st = {0x200F, 1.0, F48kHz};
    ret = SetAudoCfg(pHandle, &st);
    ret = SetAudoEnable(pHandle, 1);
    ret = SendAudoFile(pHandle, "/home/audiotest1.wav");
//    ret = SetAudoEnable(pHandle, 0);
    ret = CloseAudio(pHandle);
    return 0;
}
