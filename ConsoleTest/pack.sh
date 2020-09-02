#!/bin/sh  
exe="ConsoleTest" #你需要发布的程序名称
des="/home/ConsoleTest" #创建文件夹的位置
deplist=$(ldd $exe | awk  '{if (match($3,"/")){ printf("%s "),$3 } }')  
cp $deplist $des

