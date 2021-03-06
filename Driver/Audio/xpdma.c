#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "xpdma.h"
#include <stdio.h>
#include "../driver/xpdma_driver.h"

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

int XdmaSend(int fd, unsigned int count, unsigned int addr)
{
    if (addr % 4) {
        printf("addr error 0x%X\n", addr);
        return -1;
    }

    printf("send to 0x%X %d bytes, fd %d\n", addr, count, fd);
    
    Buffer_t buffer = {count, addr};

    ioctl(fd, _IOW(IOC_MAGIC, 1, 8), &buffer);

    return 0;
}

int XdmaRecv(int fd, unsigned int count, unsigned int addr)
{
    if (addr % 4)
        return -1;

    printf("recv from 0x%X %d bytes, fd %d\n", addr, count, fd);

    Buffer_t buffer = {count, addr};
    
    ioctl(fd, _IOR(IOC_MAGIC, 0, 8), &buffer);
    return 0;
}
