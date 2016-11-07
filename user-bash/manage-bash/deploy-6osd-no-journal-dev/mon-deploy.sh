monmaptool --create --generate -c /etc/ceph/ceph.conf /tmp/monmap --clobber
ceph-mon -i $1 --mkfs --monmap /tmp/monmap
ceph-mon -i $1
