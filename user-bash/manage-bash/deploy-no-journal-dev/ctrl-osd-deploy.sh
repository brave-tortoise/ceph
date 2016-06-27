for((i=1; i<4; i++))
do
	scp osd-deploy.sh node$i:~
	ssh node$i "./osd-deploy.sh && rm -f osd-deploy.sh"
done
