ceph osd tier cache-mode ssd-pool forward

# wait for minutes
rados -p ssd-pool ls
rados -p ssd-pool cache-flush-evict-all
ceph osd tier remove-overlay hdd-pool
ceph osd tier remove hdd-pool ssd-pool
