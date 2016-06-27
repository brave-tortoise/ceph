for((i=1; i<4; i++))
do
	scp /etc/ceph/ceph.conf node$i:/etc/ceph/ceph.conf
done
