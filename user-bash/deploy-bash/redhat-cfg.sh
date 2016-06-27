# edit hostname
echo "set ts=4" >> ~/.vimrc
echo "alias vi=vim" >> ~/.bashrc
cd /etc/sysconfig/network-scripts
mv ifcfg-enp8s0 ifcfg-eth0
sed -i "s/BOOTPROTO=.*/BOOTPROTO=static/g" ifcfg-eth0
sed -i "s/NAME=.*/NAME=eth0/g" ifcfg-eth0
#sed -i "s/ONBOOT=.*/ONBOOT=yes/g" ifcfg-eth0
echo "IPADDR=172.23.12.120" >> ifcfg-eth0
echo "NETMASK=255.255.0.0" >> ifcfg-eth0
echo "GATEWAY=172.23.0.1" >> ifcfg-eth0
echo "DNS1=166.111.8.28" >> ifcfg-eth0
sed -i "s/quiet/quiet net.ifnames=0 biosdevname=0/g" /etc/default/grub
grub2-mkconfig -o /boot/grub2/grub.cfg
#reboot
