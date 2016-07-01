#ceph osd pool delete rbd rbd --yes-i-really-really-mean-it
#rados mkpool rbd
#rbd create test -p rbd --size 20480
#rbd map test -p rbd
#
#echo 3 > /proc/sys/vm/drop_caches
#for((i=1; i<4; i++))
#do
#	ssh node$i "echo 3 > /proc/sys/vm/drop_caches"
#done

dd if=/dev/zero of=/dev/rbd0 bs=4M count=5k
rbd unmap /dev/rbd0
