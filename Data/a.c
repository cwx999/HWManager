#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	uint32_t buff[1024] = {0};
	int fd = 0;
	int i = 0;
	int j = 1;
	int a = 0x01;
	fd = open("test_1k.bin", O_CREAT | O_RDWR,0644);
	while(j--){
	//for(i = 0; i < 1024; i++)
	//{
	//	buff[i] = a ++;	
	//}
buff[0] = 0x1FFFF;
buff[1] = 0x1FFFF;
buff[2] = 0x00000;
buff[3] = 0x1FFFF;
buff[4] = 0x00000;
buff[5] = 0x1FFFF;
buff[6] = 0x00000;
buff[7] = 0x00000;
buff[8] = 0x1FFFF;
buff[9] = 0x00000;
buff[10] = 0x1FFFF;
	write(fd,buff,1024);
	}
	close(fd);
	return 0;

}
