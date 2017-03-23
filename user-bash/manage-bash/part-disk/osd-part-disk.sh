journal_devs=("/dev/sda" "/dev/sdb")
data_devs=("/dev/sdc" "/dev/sdd" "/dev/sde" "/dev/sdf" "/dev/sdg" "/dev/sdh")

for((i=0; i<2; i++))
do
	dd if=/dev/zero of=${journal_devs[$i]} bs=4M count=1 oflag=direct
	parted ${journal_devs[$i]} mklabel gpt &>/dev/null
	parted ${journal_devs[$i]} mkpart journal xfs 0% 20%
	parted ${journal_devs[$i]} mkpart journal xfs 20% 40%
	parted ${journal_devs[$i]} mkpart journal xfs 40% 60%
done

for((i=0; i<6; i++))
do
	dd if=/dev/zero of=${data_devs[$i]} bs=4M count=1 oflag=direct
	parted ${data_devs[$i]} mklabel gpt &>/dev/null
	parted ${data_devs[$i]} mkpart data xfs 0% 100%
done
