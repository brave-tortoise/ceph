ceph osd pool delete rbd rbd --yes-i-really-really-mean-it

for((i=1; i<4; i++))
do
	scp ssd-deploy.sh node$i:~
	ssh node$i "./ssd-deploy.sh && rm -f ssd-deploy.sh"
done

for((i=1; i<4; i++))
do
	scp hdd-deploy.sh node$i:~
	ssh node$i "./hdd-deploy.sh && rm -f hdd-deploy.sh"
done
