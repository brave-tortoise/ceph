for((i=1; i<4; i++))
do
	scp osd-part-disk.sh node$i:~
	ssh node$i "./osd-part-disk.sh && rm -f osd-part-disk.sh"
done
