ceph osd tier cache-mode ssd-pool forward
rados -p ssd-pool cache-flush-evict-all
ceph osd tier remove-overlay hdd-pool
ceph osd tier remove hdd-pool ssd-pool

ceph osd pool delete ssd-pool ssd-pool --yes-i-really-really-mean-it
ceph osd pool delete hdd-pool hdd-pool --yes-i-really-really-mean-it
