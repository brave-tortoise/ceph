for((i=1; i<4; i++))
do
	scp mon-deploy.sh node$i:~
	ssh node$i "./mon-deploy.sh node$i && rm -f mon-deploy.sh"
done
