#!/bin/bash

./test_xpdma -d /dev/dio_5 -t 3 -a 0x6100  -v 2
./test_xpdma -d /dev/dio_5 -t 3 -a 0x6100  -v 1
./test_xpdma -d /dev/dio_5 -t 2 -a 0x6104

./test_xpdma -d /dev/dio_5 -t 3 -a 0x6108  -v 0x140
./test_xpdma -d /dev/dio_5 -t 3 -a 0x6108  -v 0
./test_xpdma -d /dev/dio_5 -t 3 -a 0x6108  -v 0x141
./test_xpdma -d /dev/dio_5 -t 3 -a 0x6108  -v 0x200

./test_xpdma -d /dev/dio_5 -t 2 -a 0x6104
./test_xpdma -d /dev/dio_5 -t 2 -a 0x610c

