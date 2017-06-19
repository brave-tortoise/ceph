#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os, sys
import mmap

# 打开文件
#fd = os.open( "foo.txt", os.O_RDWR|os.O_CREAT )
#fd = os.open("/dev/rbd0", os.O_RDWR|os.O_DIRECT)
fd = os.open("foo3.txt", os.O_RDWR|os.O_CREAT|os.O_DIRECT)

# 写入字符串
#os.write(fd, 'a'*512)
m = mmap.mmap(-1, 1024)
buf = ' ' * 1024
m.write(buf)
os.write(fd, m)

# 关闭文件
os.close( fd )

print "关闭文件成功!!"
