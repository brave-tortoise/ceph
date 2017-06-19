from SimPy.Simulation import *
from random import Random,expovariate,uniform
from math import sqrt
from sys import argv
import imp
imp.load_dynamic("Iotrace","./Iotrace.so")
imp.load_dynamic("Location","./Location.so")

import Iotrace
from  disks_py import *
import Location
from  parameters import *

import time

'''Simulation of the process of online stripe widen
- DISKS = original Number of disks
- DISKS_ADD =  Number of disks to add
- EXTENT = number of blocks in an extent
- EXTENTS_PER_DISK = number of extents in a disk
- SLIDING_WINDOW = size of sliding window
'''

__version__='\nModel: the process of online RAID scaling: XSCALE'

#MS_INTERVAL = 43712.242187 * 1000.0
MS_INTERVAL = 14.63 * 4 * 1000.0

IOPS_MARK = 100

# Reader & Writer lock
class rwLock(object):
	def __init__(self):
		self.mutex = Resource(capacity=1)
		self.nWriter=0
		self.nReader=0
		self.waitQ=[]
		self.rORw=[]


class PhsIO(Process):
	def execute(self):
		#print "%s: dev=%d, start=%d, length=%d, now=%d, name=%s"%(self.rwType, self.devno, self.startblkno, self.bytecount, now(), issuer.name)
		mydisk.request(self.rwType,self.devno,self.startblkno,self.bytecount,now(),self)
		yield passivate,self	# wait for IO completion
		self.compEvent.signal()
	
	
class IO_Player(Process):
	def execute(self):
		global iopsQ
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
			L.startblkno = req[1] / BLOCK_SIZE;
			L.startblkno = L.startblkno % Ori_Sum_Blocks
			L.bytecount = req[2] + (req[1] % BLOCK_SIZE)
			align = L.bytecount % BLOCK_SIZE
			if align != 0:
				L.bytecount = L.bytecount + BLOCK_SIZE - align
			L.rwType = req[3]

			temp = req[4] * 4 + period * MS_INTERVAL
			if temp < now():
				print "---------------------------round"
				period += 1
				temp += MS_INTERVAL
			if temp > now():
				yield hold, self, temp - now()

			#print "%s: now=%d, start=%d, length=%d, io=%d"%(L.rwType, temp, L.startblkno, L.bytecount, i)
			iopsQ.append(now())
			activate(L,L.execute())


class LogIO(Process):
	 def execute(self):
		global moving_region, total_disks, rwlocks, lock_size, s_track, t

		#self.trace("starts ")
		IO_start = now()
		extent_first = self.startblkno / EXTENT
		extent_last = (self.startblkno + (self.bytecount - 1) / BLOCK_SIZE) / EXTENT
		#print "first: %d, last: %d\n"%(extent_first, extent_last)

		locks = []
		event_list = []
		left_bytecount = self.bytecount

		for extent_cur in range(extent_first, extent_last + 1):
			location = Location.fmap(t-1, extent_cur)
			disk_no = location[0]
			phy_extent = location[1]
			log_extent = Location.phy2Log(phy_extent, t-1)
			new_log_extent = Location.phy2Log(phy_extent, t)
			io_square_index = new_log_extent / (total_disks - 2)
			
			#print "%d, %d"%(io_square_index, moving_region)

			# acquire needed lock
			if io_square_index == moving_region:
				myRW = rwlocks[disk_no]
				yield request,self,myRW.mutex
				# when any mover, IO will wait
				if(myRW.nWriter != 0 or myRW.waitQ != []):
					myRW.waitQ.append(self)
					myRW.rORw.append('R')
					yield release,self,myRW.mutex
					yield passivate,self
					print "--------------------reactive,%d,%d"%(io_square_index, moving_region)
					continue
				else:
					locks.append(disk_no)
					myRW.nReader += 1
					yield release,self,myRW.mutex
			
			stripe_lines = DISKS
			# stripe has been moved, IO should be issued to new location
			if io_square_index < moving_region:
				location = Location.fmap(t, extent_cur)
				disk_no = location[0]
				phy_extent = location[1]
				log_extent = new_log_extent
				stripe_lines = total_disks

			# sometimes do not need read/write the whole extent
			phy_startblkno = phy_extent * EXTENT
			if extent_cur == extent_first:
				phy_startblkno += self.startblkno % EXTENT

			cur_slot = (EXTENT - phy_startblkno % EXTENT) * BLOCK_SIZE
			phy_bytecount = cur_slot
			if left_bytecount < cur_slot:
				phy_bytecount = left_bytecount
				left_bytecount = 0
			else:
				left_bytecount -= phy_bytecount
			
			# begin IO
			P_disk_no = 0
			P_phy_startblkno = 0
			Q_disk_no = 0
			Q_phy_startblkno = 0
			
			if self.rwType == 'W':
				# data
				r_phy_io = PhsIO()
				r_phy_io.rwType = 'R'
				r_phy_io.devno = disk_no
				r_phy_io.startblkno = phy_startblkno
				r_phy_io.bytecount = phy_bytecount
				r_phy_io.compEvent = SimEvent()
				event_list.append(r_phy_io.compEvent)
				activate(r_phy_io, r_phy_io.execute(), delay=0)
				# parity P
				P_phy_extent = log_extent / (stripe_lines - 2) * s_track[0] + s_track[0] - 2
				P_disk_no = (disk_no + (stripe_lines - 2) - (log_extent % (stripe_lines - 2))) % stripe_lines
				P_phy_startblkno = P_phy_extent * EXTENT
				if extent_cur == extent_first:
					P_phy_startblkno += self.startblkno % EXTENT
				P_phy_io = PhsIO()
				P_phy_io.rwType = 'R'
				P_phy_io.devno = P_disk_no
				P_phy_io.startblkno = P_phy_startblkno
				P_phy_io.bytecount = phy_bytecount
				P_phy_io.compEvent = SimEvent()
				event_list.append(P_phy_io.compEvent)
				activate(P_phy_io, P_phy_io.execute(), delay=0)
				# parity Q
				Q_phy_extent = P_phy_extent + 1
				Q_disk_no = (disk_no + 2 + (log_extent % (stripe_lines - 2))) % stripe_lines
				Q_phy_startblkno = Q_phy_extent * EXTENT
				if extent_cur == extent_first:
					Q_phy_startblkno += self.startblkno % EXTENT
				Q_phy_io = PhsIO()
				Q_phy_io.rwType = 'R'
				Q_phy_io.devno = Q_disk_no
				Q_phy_io.startblkno = Q_phy_startblkno
				Q_phy_io.bytecount = phy_bytecount
				Q_phy_io.compEvent = SimEvent()
				event_list.append(Q_phy_io.compEvent)
				activate(Q_phy_io, Q_phy_io.execute(), delay=0)
			
			# issue a physical IO
			phy_io = PhsIO()
			phy_io.rwType = self.rwType
			phy_io.devno = disk_no
			phy_io.startblkno = phy_startblkno
			phy_io.bytecount = phy_bytecount
			phy_io.compEvent = SimEvent()
			event_list.append(phy_io.compEvent)
			activate(phy_io, phy_io.execute(), delay=0)
			
			if self.rwType == 'W':
				# parity P
				P_phy_io = PhsIO()
				P_phy_io.rwType = 'W'
				P_phy_io.devno = P_disk_no
				P_phy_io.startblkno = P_phy_startblkno
				P_phy_io.bytecount = phy_bytecount
				P_phy_io.compEvent = SimEvent()
				event_list.append(P_phy_io.compEvent)
				activate(P_phy_io, P_phy_io.execute(), delay=0)
				# parity Q
				Q_phy_io = PhsIO()
				Q_phy_io.rwType = 'W'
				Q_phy_io.devno = Q_disk_no
				Q_phy_io.startblkno = Q_phy_startblkno
				Q_phy_io.bytecount = phy_bytecount
				Q_phy_io.compEvent = SimEvent()
				event_list.append(Q_phy_io.compEvent)
				activate(Q_phy_io, Q_phy_io.execute(), delay=0)

		# wait for the completion of IOs
		left_events = len(event_list)
		while left_events > 0: 
			yield waitevent,self,event_list
			left_events -= len(self.eventsFired)

		# release all the locks
		for lock_index in locks:
			print "------------io lock release"
			myRW = rwlocks[lock_index]
			yield request,self,myRW.mutex
			myRW.nReader -= 1
			if(myRW.nReader == 0):	# final active application IO
				if myRW.waitQ != [] and myRW.rORw[0] == 'W':
					writer = myRW.waitQ.pop(0)
					myRW.rORw.pop(0)
					if(writer.passive()):
						reactivate(writer)
			yield release,self,myRW.mutex

		print "read/write complete: %d"%(io_square_index)
		response = now() - IO_start
		Monitor_response.observe(response)
		latency = repr(IO_start) + ', ' + repr(response) + '\r\n'
		perffile.write(latency)
		perffile.flush()
   
	 def trace(self,message):
		if TRACING:
			print "%7.4f %6s %10s "%(now(),self.name,message)


class ShardMover(Process):
	def execute(self):
		global write_events
		
		# read from old disks
		read_events = []
		phy_no = 0
		for disk_no in range(DISKS):
			phy_io = PhsIO()
			phy_io.rwType = 'R'
			phy_io.devno = disk_no
			phy_io.startblkno = self.startblkno
			phy_io.bytecount = self.bytecount
			phy_io.compEvent = SimEvent()
			read_events.append(phy_io.compEvent)
			activate(phy_io, phy_io.execute(), delay=0)
			phy_no += 1
		
		left_events = phy_no
		while left_events > 0:
			yield waitevent,self,read_events
			left_events -= len(self.eventsFired)

		#yield hold, self, 0
		#time.sleep(5)

		# write to newly added disks
		for dd in range(DISKS_ADD):
			disk_no = DISKS + dd
			phy_io = PhsIO()
			phy_io.rwType = 'W'
			phy_io.devno = disk_no
			phy_io.startblkno = self.startblkno
			phy_io.bytecount = self.bytecount
			phy_io.compEvent = SimEvent()
			write_events.append(phy_io.compEvent)
			activate(phy_io, phy_io.execute(), delay=0)

		self.compEvent.signal()


class Mover(Process):
	def execute(self):
		global moving_region, total_disks, stripes, rwlocks, lock_size, s_track, t, iopsQ, write_events

		self.trace("starts ")
		move_start = now()

		complete = 0
		while not complete:
			#print "---Refcount of None: ", sys.getrefcount(None)
			if moving_region % 5 == 0:
				bar = moving_region * 100.0 / stripes
				print>>progressbarfile, "%2.4f %% completed at %.6f\r\n"%(bar,now())
				progressbarfile.flush()

			# rate control
			highLoad = True
			while highLoad:
				# calculate IOPS
				while iopsQ != [] and iopsQ[0] < now()-1.0*1000:
					iopsQ.pop(0)
				#print "HIGH WORKLOAD: IOPS = %7.4f"%(len(iopsQ))
				if len(iopsQ) > IOPS_MARK:	#check number of tasks in the last second
					#print "high workload"
					yield hold, self, 1.0 *1000
				else:
					highLoad = False

			# read data for moving and parity calculation
			# write to the new locations
			start_log_extent = moving_region * (total_disks - 2)
			start_phy_extent = Location.log2Phy(start_log_extent, t)
			start_ext_idx = start_phy_extent % DISKS
			if(start_ext_idx >= DISKS - 2):
				start_phy_extent += (DISK - start_ext_idx)
				
			end_log_extent = start_log_extent + total_disks - 2 - 1
			end_phy_extent = Location.log2Phy(end_log_extent, t)
			end_ext_idx = end_phy_extent % DISKS
			if(end_ext_idx >= DISKS - 2):
				end_phy_extent -= (end_ext_idx - (DISKS - 2) + 1)

			ext_num = end_phy_extent - start_phy_extent + 1

			# acquire the locks first!!!
			for lock_index in range(lock_size):
				myRW = rwlocks[lock_index]
				yield request,self,myRW.mutex
				if(myRW.nReader != 0):
					myRW.waitQ.append(self)
					myRW.rORw.append('W')
					# release all gotten locks
					for idx in range(0, lock_index):
						gotRW = rwlocks[idx]
						yield request,self,gotRW.mutex
						gotRW.nWrite -= 1
						while gotRW.waitQ != [] and gotRW.rORw[0] == 'R':
							reader = gotRW.waitQ.pop(0)
							gotRW.rORw.pop(0)
							if(reader.passive()):
								reactivate(reader)
						yield release,self,gotRW.mutex
					yield release,self,myRW.mutex
					yield passivate,self
					lock_index = 0
				else:
					myRW.nWriter += 1
					yield release,self,myRW.mutex
			
			print "begin to move"

			# begin to move
			write_events = []	# record the write operations (data & parity)
			
			read_lists = []
			while ext_num > 0:
				startblkno = start_phy_extent * EXTENT
				submit_num = min(ext_num, DISKS - 2 - start_phy_extent % DISKS)
				shard_io = ShardMover()
				shard_io.startblkno = startblkno
				shard_io.bytecount = submit_num * EXTENT * BLOCK_SIZE
				shard_io.compEvent = SimEvent()
				read_lists.append(shard_io.compEvent)
				activate(shard_io, shard_io.execute(), delay=0)
				start_phy_extent += (submit_num + 2)
				ext_num -= (submit_num + 2)

			#print "issue read"

			# all read complete
			left_events = len(read_lists)
			while left_events > 0:
				yield waitevent,self,read_lists
				left_events -= len(self.eventsFired)
		
			#print "read done"

			# write parity extents
			parity_phy_extent = moving_region * s_track[0] + s_track[0] - 2
			startblkno = parity_phy_extent * EXTENT
			ext_num = 2
			for disk_no in range(total_disks):
				phy_io = PhsIO()
				phy_io.rwType = 'W'
				phy_io.devno = disk_no
				phy_io.startblkno = startblkno
				phy_io.bytecount = ext_num * EXTENT * BLOCK_SIZE
				phy_io.compEvent = SimEvent()
				write_events.append(phy_io.compEvent)
				activate(phy_io, phy_io.execute(), delay=0)

			#print "issue write"

			# all write complete (data & parity)
			left_events = len(write_events)
			while left_events > 0:
				yield waitevent,self,write_events
				left_events -= len(self.eventsFired)
			
			#print "write done"

			# check the end
			moving_region += 1
			if moving_region >= stripes:
				complete = 1
		
			print "move complete: %d"%(moving_region)

			# finally release the locks and reactivate the waiting IO
			for lock_index in range(lock_size):
				myRW = rwlocks[lock_index]
				yield request,self,myRW.mutex
				myRW.nWriter -= 1
				while myRW.waitQ != [] and myRW.rORw[0] == 'R':
					reader = myRW.waitQ.pop(0)
					myRW.rORw.pop(0)
					if(reader.passive()):
						reactivate(reader)
				yield release,self,myRW.mutex
			
		print>>progressbarfile, "mover costs %.10f ms \r\n" %(now()-move_start)
		progressbarfile.flush()			   
		print "mover costs %.4f ms" %(now() - move_start)
		stopSimulation()
	
	def trace(self,message):
		if TRACING:
			print "%7.4f %6s %10s "%(now(),self.name,message)
	
			
class Pusher(Process):
	def execute(self):
		while true:
			mydisk.tickaway(now())
			yield passivate, self	# wait for event completion


# main procedure begin
TRACING = 1
NR_SAMPLE = 2000

Monitor_response = Monitor()	# monitor the response time

s_track = [HISTORY[0]]
t = len(HISTORY) - 1
for i in range(t):
	s_track.append(s_track[i] + HISTORY[i + 1])
chunks = EXTENTS_PER_DISK
DISKS = s_track[t-1]
DISKS_ADD = HISTORY[t]
total_disks = s_track[t]

set_size = DISKS * total_disks
stripes_sets = chunks / set_size
stripes = stripes_sets * DISKS

usedS = chunks - (chunks % set_size)
Ori_Sum_Extents = Location.initialize(t, s_track, chunks)
Ori_Sum_Blocks = Ori_Sum_Extents * EXTENT

print t, DISKS,DISKS_ADD,usedS

iopsQ =[]
moving_region = 0 #stripes * 99 / 100 # record the moving location of square chunks

rwlocks = []
lock_size = DISKS
for i in range(lock_size):
	myRW = rwLock()
	myRW.__init__()
	rwlocks.append(myRW)

if len(argv) < 2:
	Iotrace.initialize("tpcc.trace")
else:
	Iotrace.initialize(argv[1])
	print argv[1]

if len(argv) <3:
	perffile=open("xscale_latency.txt","w")
else:
	perffile=open(argv[2],"w")
perffile.write('starttime, latency (in ms)\r\n')
progressbarfile=open("xscale_progressbar_io.txt","w")

initialize()
print __version__

mydisk = Disks_py()
mydisk.initialize("ibm36z15_jbod.parv", "ibm36z15_fastscale.out", total_disks)

pusher = Pusher()
activate(pusher, pusher.execute())

mover = Mover('data mover')
activate(mover, mover.execute())

player = IO_Player('workload generator')
activate(player, player.execute())

simulate(until=12000000.0)
mydisk.finalize(now())
Iotrace.finalize()
Location.finalize()

perffile.close()
progressbarfile.close()

#print sample time & response time
resp_time = Monitor_response.yseries()
samp_time = Monitor_response.tseries()
sub_len = Monitor_response.count() / NR_SAMPLE
if sub_len ==0:
	print "SAMPLE TIME SERIES------------>"
	for i in range(Monitor_response.count()):
		print samp_time[i]
	print "RESPONSE TIME SERIES------------>"
	for i in range(Monitor_response.count()):
		print resp_time[i]
	print "------------SERIES over------------>"
else:
	print "SAMPLE TIME SERIES------------>"
	sampled_resp=[]
	cnt =0
	for i in range(NR_SAMPLE):
		max_resp =0.0
		mon_index = 0
		for j in range(sub_len):
			if resp_time[cnt]>max_resp:
				max_resp = resp_time[cnt]
				mon_index = cnt
			cnt+=1
		print samp_time[mon_index]
		sampled_resp.append(resp_time[mon_index])
	print samp_time[Monitor_response.count()-1]
	sampled_resp.append(resp_time[Monitor_response.count()-1])
	print "RESPONSE TIME SERIES------------>"
	for i in range(NR_SAMPLE+1):
		print sampled_resp[i]
	print "------------SERIES over------------>"


Monitor_response.yseries()
print "Average response time is %.4f ms" %(Monitor_response.mean(),)
print "std dev of response time is %.4f" %(sqrt(Monitor_response.var()),)

print "max response time is %.4f ms" %(max(Monitor_response.yseries()),)
print "min response time is %.4f ms" %(min(Monitor_response.yseries()),)
