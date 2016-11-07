x=0
service ceph stop osd.$x
ceph osd rm osd.$x
ceph osd crush remove osd.$x
#ceph auth list
ceph auth del osd.$x
umount /var/lib/ceph/osd/ceph-$x
