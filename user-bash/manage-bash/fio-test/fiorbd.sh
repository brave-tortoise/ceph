echo 3 > /proc/sys/vm/drop_caches
for((i=1; i<4; i++))
do
	ssh node$i "echo 3 > /proc/sys/vm/drop_caches"
done

fio $1
#fio rbd.fio
#fio rbd2.fio
