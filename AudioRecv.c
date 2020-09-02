#include "LibHWAudio/LibHWAudio_global.h"
#include <stddef.h>
#include <stdio.h>
int main()
{
    int ret = 0;
    const char* strDev = "/dev/audio_5_in1";
    void * pHandle = NULL;
    ret = OpenAudio(strDev, &pHandle);
    ret = SetAudiAllRate(pHandle, F48kHz);
    ret = SetAudiMode(pHandle, 0);
    ret = SetAudiAtten(pHandle, 0);
    ret = SetAudiDataType(pHandle, 0);
    AudInCfg st = {1.0,0};
    ret = SetAudiCfg(pHandle, &st);
    ret = SetAudiEnable(pHandle, 1);
    ret = RecvAudiFile(pHandle, "/home/audiotest1.wav", 5);
    ret = SetAudiEnable(pHandle, 0);
    ret = CloseAudio(pHandle);
    return 0;
}
