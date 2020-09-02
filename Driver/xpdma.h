#ifndef XPDMA_CONTROL_H
#define XPDMA_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int XdmaOpen(const char *devName);
void XdmaClose(int fd);
int XdmaSend(int fd, unsigned int count, unsigned int addr);
int XdmaRecv(int fd, unsigned int count, unsigned int addr);

#ifdef __cplusplus
}
#endif

#endif 
