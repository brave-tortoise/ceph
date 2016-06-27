rm -rf ~/.ssh
for((i=1; i<4; i++))
do
	sshpass -p 111111 scp -o StrictHostKeyChecking=no /etc/hosts node$i:/etc/hosts
	sshpass -p 111111 ssh node$i "rm -rf ~/.ssh && echo \"y\" | ssh-keygen -t rsa -P '' -f ~/.ssh/id_rsa"
done

echo "y" | ssh-keygen -t rsa -P '' -f ~/.ssh/id_rsa
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys

ssh -o StrictHostKeyChecking=no client "echo \"client host known\""
ssh -o StrictHostKeyChecking=no localhost "echo \"localhost known\""

for((i=1; i<4; i++))
do
	sshpass -p 111111 scp ~/.ssh/id_rsa.pub node$i:~/.ssh/id_rsa.pub.admin
	sshpass -p 111111 scp nopass-node.sh node$i:~
	sshpass -p 111111 ssh node$i "./nopass-node.sh node$i && rm -f nopass-node.sh"
done
