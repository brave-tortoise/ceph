for((i=1; i<4; i++))
do
	scp undeploy.sh node$i:~
	ssh node$i "./undeploy.sh && rm -f undeploy.sh"
done
