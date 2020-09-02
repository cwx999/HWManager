#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <getopt.h>
#include <fcntl.h>
#include "xpdma.h"
#include <sys/time.h> 
#include <sys/mman.h>

#define TEST_SIZE   1024 // 1KB test data
#define TEST_ADDR   0 // offset of DDR start address
#define BOARD_ID    0 // board number (for multiple boards)

#define min(a,b) (((a) < (b)) ? (a) : (b))

static int is_printable(char c) {
    // anything below ASCII code 32 is non-printing, 127 is DELETE
    if (c < 32 || c == 127) {
        return 0;
    }
    return 1;
}

static void print_bytes(uint32_t addr, uint8_t* data, size_t length)
{
    FILE* output = stdout;
    int row = 0;

    // formatted output
    for (row = 0; row < (int)(length / 16 + ((length % 16) ? 1 : 0));
        row++) {

        // Print address
        fprintf(output, "0x%04llX:  ", addr + row * 16);

        // Print bytes
        int column;
        for (column = 0; column < (int)min(16, length - (row * 16));
            column++) {
            fprintf(output, "%02x ", data[(row * 16) + column]);
        }
        for (; column < 16; column++) {
            fprintf(output, "   ");
        }

        // Print gutter
        fprintf(output, "    ");

        // Print characters
        for (column = 0; column < (int)min(16, length - (row * 16));
            column++) {
            fprintf(output, "%c", is_printable(data[(row * 16) + column]) ?
                (data[(row * 16) + column]) : '.');
        }
        for (; column < 16; column++) {
            fprintf(output, " ");
        }
        fprintf(output, "\n");
    }
}

void print_usage()
{
    printf("unsage:\n");
    printf("------------------------------\n");
    printf("./test_xpdma -d device -t type -a address -l length -s rand -v value\n");
    printf("device:\n");
    printf("    device name, such as /dev/xdma_5\n");
    printf("type :\n");
    printf("    0:dma read\n");
    printf("    1:dma write\n");
    printf("    2:reg read\n");
    printf("    3:reg write\n");
    printf("address:\n");
    printf("    board address\n");
    printf("length:\n");
    printf("    read or write length, <= 4M bytes\n");
    printf("rand:\n");
    printf("    0:not rand\n");
    printf("    1:rand\n");
    printf("value:\n");
    printf("    register value\n");
}

int main(int argc, char **argv)
{
    int result = -1;
    int type = -1;
    int is_rand = 1;
    uint32_t addr = 0;
    uint32_t length = 0;
    int fd_dma = 0;
    int fd_user = 0;
    char *map_dma = NULL; 
    char *map_user = NULL;
    uint32_t reg_value = 0;
    int i = 0;
    char devUser[64] = {0};
    char devDma[64] = {0};

    opterr = 0;  //使getopt不行stderr输出错误信息

    while ((result = getopt(argc, argv, "d:t:a:l:s:v:")) != -1 )
    {
        switch (result)
        {
            case 'd':
                sprintf(devUser, "%s_user", optarg);
                sprintf(devDma, "%s_dma", optarg);
                break;
                
            case 't':
                type = strtol(optarg, NULL, 0);
                break;
                
            case 'a':
                addr = strtol(optarg, NULL, 0);
                break;
                
            case 'l':
                length = min(4*1024*1024, strtol(optarg, NULL, 0));
                break;
                
            case 's':
                is_rand = strtol(optarg, NULL, 0);
                break;

            case 'v':
                reg_value = strtol(optarg, NULL, 0);
                break;
                
            case '?':
                print_usage();
                return;
                
            default:
                print_usage();
                return;
        }
    }
    
    if (type < 0 || strlen(devUser) == 0 || strlen(devDma) == 0) {
        print_usage();
        return;
    }
    
    printf("type %d, address 0x%X, length 0x%X, is rand %d\n", type, addr, length, is_rand);

    fd_dma = XdmaOpen(devDma);
    if (fd_dma < 0) {
        printf ("Failed to open xdma dma device\n");
        return 1;
    }
    
    fd_user = open(devUser, O_RDWR | O_SYNC);
    if (fd_user < 0) {
        printf("Failed to open %s\n", devUser);
        return 1;
    }
    
    map_dma = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dma, 0);
    map_user = mmap(NULL, 65536, PROT_READ | PROT_WRITE, MAP_SHARED, fd_user, 0);

    if (type == 0) {
        result = XdmaRecv(fd_dma, length, addr);
        if (result != 0) {
            printf("dma read error %d\n", result);
            goto exit;
        }
        
        printf("read buffer (length = %d) is :\n", length);
        print_bytes(addr, map_dma, length);
    }
    else if (type == 1) {        
        for (i = 0; i < length; i++) {
            map_dma[i] = is_rand ? (rand() % 256) : i;
        }
        
        result = XdmaSend(fd_dma, length, addr);
        if (result != 0) {
            printf("dma write error %d\n", result);
            goto exit;
        }
        printf("write buffer (length = %d) is:\n", length);
        print_bytes(addr, map_dma, length);
    }
    else if (type == 2) {
        reg_value = *((volatile uint32_t *)(map_user + addr));
        printf("read reg 0x%X, value is 0x%X\n", addr, reg_value);
    }
    else if (type == 3) {
        *((volatile uint32_t *)(map_user + addr)) = reg_value;
        printf("write reg 0x%X with value 0x%X\n", addr, reg_value);
    }
    
exit:
    if (NULL != map_dma) {
        munmap(map_dma, 4096);
    }
    
    if (NULL != map_user) {
        munmap(map_user, 16384);
    }
    XdmaClose(fd_dma);
    close(fd_user);
}
