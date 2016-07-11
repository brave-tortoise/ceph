rbd rm test1 -p hdd-pool
rbd rm test2 -p hdd-pool
rbd rm test3 -p hdd-pool
rbd rm test4 -p hdd-pool
#rados -p ssd-pool cache-flush-evict-all
