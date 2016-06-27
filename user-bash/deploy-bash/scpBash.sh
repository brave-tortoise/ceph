for((i=1; i<4; i++))
do
	#sshpass -p 111111 scp -o StrictHostKeyChecking=no -r ~/deploy-bash node$i:~
	scp -r ~/cetune-bash node$i:~
done
