import ctypes
import ctypes.util

from SimPy.Simulation import *
import os, sys
from sys import argv

import imp
imp.load_dynamic("Iotrace","./Iotrace.so")
import Iotrace

MS_INTERVAL = 1172165502.415434 * 1000.0

class IO_Player(Process):
	def execute(self):
		print "%7.4f %s starts"%(now(), self.name)
		period = 0
		i = 0
		while 1:
			req = Iotrace.nextrequest()
			'''
			req[0]: devno
			req[1]: startbyte
			req[2]: IO size in byte
			req[3]: R/W type
			req[4]: req time, ms is the scale
			'''
			#just request to disk 1, in tpcc
			if req[0] != 0:
				continue
			
			i += 1
			L = LogIO("IO "+`i`)
			L.offset = req[1]
			L.length = req[2]
			L.rwType = req[3]

			temp = req[4] + period * MS_INTERVAL
			print "%7.4f"%(req[4])
			if temp < now():
				print "-----------------play trace next round"
				period += 1
				temp += MS_INTERVAL
			if temp > now():
				yield hold, self, temp - now()

			activate(L,L.execute())


class LogIO(Process):
	def execute(self):
		os.lseek(fd, self.offset, 0)
		if self.rwType == 'W':
			libc.write(ctypes.c_int(fd), buf, ctypes.c_int(self.length))
		else:
			libc.read(ctypes.c_int(fd), buf, ctypes.c_int(self.length))
		print "%s: now=%7.4f, start=%d, length=%d"%(self.rwType, now(), self.offset, self.length)
		yield hold, self, 100


def ctypes_alloc_aligned(size, alignment):
	buf_size = size + (alignment - 1)
	raw_memory = bytearray(buf_size)

	ctypes_raw_type = (ctypes.c_char * buf_size)
	ctypes_raw_memory = ctypes_raw_type.from_buffer(raw_memory)

	raw_address = ctypes.addressof(ctypes_raw_memory)
	offset = raw_address % alignment
	
	offset_to_aligned = (alignment - offset) % alignment
	ctypes_aligned_type = (ctypes.c_char * (buf_size - offset_to_aligned))
	
	ctypes_aligned_memory = ctypes_aligned_type.from_buffer(raw_memory, offset_to_aligned)
	return ctypes_aligned_memory


libc = ctypes.CDLL(ctypes.util.find_library('c'))
buf = ctypes_alloc_aligned(1024*1024, 512)
#libc.memset(buf, ctypes.c_int(0), ctypes.c_int(1024*1024))

fd = os.open("/dev/rbd0", os.O_RDWR|os.O_DIRECT)

if len(argv) < 2:
	Iotrace.initialize("Cambridge/prxy_0.trace")
else:
	Iotrace.initialize(argv[1])
	print argv[1]

initialize()

player = IO_Player('workload generator')
activate(player, player.execute())

simulate(until=1172166502415)

Iotrace.finalize()
os.close(fd)

