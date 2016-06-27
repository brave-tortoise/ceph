cat ~/.ssh/id_rsa.pub.admin >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
rm -f ~/.ssh/id_rsa.pub.admin

for((i=1; i<4; i++))
do
	sshpass -p 111111 scp -o StrictHostKeyChecking=no ~/.ssh/id_rsa.pub node$i:~/.ssh/id_rsa.pub.$1
	sshpass -p 111111 ssh node$i "cat ~/.ssh/id_rsa.pub.$1 >> ~/.ssh/authorized_keys && rm -f ~/.ssh/id_rsa.pub.$1"
done

ssh -o StrictHostKeyChecking=no localhost "echo \"localhost known\""
