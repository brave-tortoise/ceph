#/usr/bin/evn python
# _*_ coding:UTF-8

import sys
import time
import os

import ctypes
import ctypes.util

def ctypes_alloc_aligned(size, alignment):
	buf_size = size + (alignment - 1)
	#先使用bytearray函数分配一块内存
	
	raw_memory = bytearray(buf_size)
	
	#然后从raw_memory创建一个ctypes对象
	ctypes_raw_type = (ctypes.c_char * buf_size)
	ctypes_raw_memory = ctypes_raw_type.from_buffer(raw_memory)
	#通过ctypes对象的addressof获得内存指针的值
	raw_address = ctypes.addressof(ctypes_raw_memory)
	offset = raw_address % alignment
	
	#通过内存地址可以得出，对齐内存的偏移量
	offset_to_aligned = (alignment - offset) % alignment
	ctypes_aligned_type = (ctypes.c_char * (buf_size - offset_to_aligned))
	
	#通过内存的偏移量，创建对齐内存的ctype对象
	ctypes_aligned_memory = ctypes_aligned_type.from_buffer(raw_memory, offset_to_aligned)
	return ctypes_aligned_memory


libc = ctypes.CDLL(ctypes.util.find_library('c'))

#获得一块4k对齐的内存

buf =ctypes_alloc_aligned(1024*1024, 4096)
print len(buf)

#direct io的方式打开块设备文件
fd = os.open('foo6.txt', os.O_RDWR|os.O_CREAT|os.O_DIRECT)
err_code = libc.read(ctypes.c_int(fd), buf, ctypes.c_int(1024*1024))
#把directIO读出的数据放到python的一个字符串变量中：
print err_code
data = buf.raw[0:1024]
#os.lseek(fd, 1024, 0)
#写数据
libc.memset(buf, ctypes.c_int(0), ctypes.c_int(1024*1024))
libc.write(ctypes.c_int(fd), buf, ctypes.c_int(512))
os.close(fd)

