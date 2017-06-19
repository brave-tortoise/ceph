from gevent import monkey
#monkey.patch_socket()
import gevent
from time import sleep

def f(n):
	for i in range(n):
		print gevent.getcurrent(), i
		sleep(3)

print "1"
g1 = gevent.spawn(f, 5)
g1.join()
print "2"
g2 = gevent.spawn(f, 5)
g2.join()
g3 = gevent.spawn(f, 5)
g3.join()
