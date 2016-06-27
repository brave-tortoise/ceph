data_devs=("/dev/sda1" "/dev/sdb1")

for((i=0; i<2; i++))
do
	#echo ${data_devs[$i]} ${journal_devs[$i / 5]}`expr $i % 5 + 1`
	ceph-disk prepare --cluster ceph --cluster-uuid ce927ce5-d33b-4d70-9427-ba5e65bec77a ${data_devs[$i]}
	ceph-disk activate ${data_devs[$i]}
done
