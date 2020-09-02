#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "xpdma.h"
#include <stdio.h>
#define IOC_MAGIC 0xa5
typedef struct {
    uint32_t count;
    uint32_t addr;
    uint8_t isBusyBlock;
    uint8_t iChl;
} Buffer_t;
int XdmaOpen(const char *devName) 
{ 
    int fd = 0;
    
    fd = open(devName, O_RDWR | O_SYNC);
    
    if (fd < 0) {
        printf("open %s failed\n", devName);
    }
    
    return fd;
}

void XdmaClose(int fd)
{
    close(fd);
}

int XdmaAudioSend(int fd, unsigned int count, unsigned int addr, uint8_t isBusyBlock, uint8_t iChl)
{
    if (addr % 4) {
        printf("addr error 0x%X\n", addr);
        return -1;
    }

    printf("send to 0x%X %d bytes, fd %d\n", addr, count, fd);
    
    Buffer_t buffer = {count, addr, isBusyBlock, iChl};

    ioctl(fd, _IOW(IOC_MAGIC, 1, 8), &buffer);

    return 0;
}

int XdmaAudioRecv(int fd, unsigned int count, unsigned int addr, uint8_t isBusyBlock, uint8_t iChl)
{
    if (addr % 4)
        return -1;

    printf("recv from 0x%X %d bytes, fd %d\n", addr, count, fd);

    Buffer_t buffer = {count, addr, isBusyBlock, iChl};
    
    ioctl(fd, _IOR(IOC_MAGIC, 0, 8), &buffer);
    return 0;
}