echo "set ts=4" >> ~/.vimrc
echo "alias vi=vim" >> ~/.bashrc

nic_org_name=enp8s0
nic_new_name=eth0
nic_new_cfg=ifcfg-$nic_new_name
ip_address=192.168.12.120
net_mask=255.255.0.0
gate_way=192.168.0.1
dns_server=8.8.8.8

cd /etc/sysconfig/network-scripts
mv ifcfg-$nic_org_name $nic_new_name	# NIC name
sed -i "s/BOOTPROTO=.*/BOOTPROTO=static/g" $nic_new_cfg
sed -i "s/NAME=.*/NAME=$nic_new_name/g" $nic_new_cfg
sed -i "s/ONBOOT=.*/ONBOOT=yes/g" $nic_new_cfg
echo "IPADDR=$ip_address" >> $nic_new_cfg	# ip address
echo "NETMASK=$net_mask" >> $nic_new_cfg	# net mask
echo "GATEWAY=$gate_way" >> $nic_new_cfg	# gateway
echo "DNS1=$dns_server" >> $nic_new_cfg		# dns

sed -i "s/quiet/quiet net.ifnames=0 biosdevname=0/g" /etc/default/grub
grub2-mkconfig -o /boot/grub2/grub.cfg

systemctl stop firewalld
systemctl disable firewalld
setenforce 0

sed -i "s/SELINUX=enforcing/SELINUX=disabled/g" /etc/selinux/config
ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime

echo "/usr/lib" >> /etc/ld.so.conf
echo "/usr/lib64" >> /etc/ld.so.conf
ldconfig

#reboot
