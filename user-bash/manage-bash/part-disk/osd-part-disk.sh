data_devs=("/dev/sda" "/dev/sdb" "/dev/sdc" "/dev/sdd" "/dev/sde" "/dev/sdf" "/dev/sdg" "/dev/sdh")

for((i=0; i<8; i++))
do
	dd if=/dev/zero of=${data_devs[$i]} bs=4M count=1 oflag=direct
	parted ${data_devs[$i]} mklabel gpt &>/dev/null
	parted ${data_devs[$i]} mkpart data 0% 100%
done
