#ifndef XPDMA_CONTROL_H
#define XPDMA_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int XdmaOpen(const char *devName);
void XdmaClose(int fd);
int XdmaAudioSend(int fd, unsigned int count, unsigned int addr, uint8_t isBusyBlock, uint8_t iChl);
int XdmaAudioRecv(int fd, unsigned int count, unsigned int addr, uint8_t isBusyBlock, uint8_t iChl);

#ifdef __cplusplus
}
#endif

#endif 
