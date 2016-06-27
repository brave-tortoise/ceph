data_devs=("/dev/sdc1" "/dev/sdd1" "/dev/sde1" "/dev/sdf1" "/dev/sdg1" "/dev/sdh1" "/dev/sdi1" "/dev/sdj1" "/dev/sdk1")
journal_devs=("/dev/sda" "/dev/sdb")

for((i=0; i<2; i++))
do
	#echo ${data_devs[$i]} ${journal_devs[$i / 5]}`expr $i % 5 + 1`
	#echo ${journal_devs[$i]}1
	ceph-disk prepare --cluster ceph --cluster-uuid ce927ce5-d33b-4d70-9427-ba5e65bec77a ${data_devs[$i]} ${journal_devs[$i]}1
	ceph-disk activate ${data_devs[$i]}
done
