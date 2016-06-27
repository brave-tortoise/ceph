service ceph -a restart mon
for((i=1; i<4; i++))
do
    ssh node$i service ceph restart osd
done
