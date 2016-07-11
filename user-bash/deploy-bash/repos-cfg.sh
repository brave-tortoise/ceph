rpm -qa | grep yum | xargs rpm -e --nodeps
#wget -N http://mirrors.163.com/centos/RPM-GPG-KEY-CentOS-7
#wget -N http://mirrors.163.com/centos/7/os/x86_64/Packages/yum-3.4.3-132.el7.centos.0.1.noarch.rpm
#wget -N http://mirrors.163.com/centos/7/os/x86_64/Packages/yum-metadata-parser-1.1.4-10.el7.x86_64.rpm
#wget -N http://mirrors.163.com/centos/7/os/x86_64/Packages/yum-plugin-fastestmirror-1.1.31-34.el7.noarch.rpm
rpm --import RPM-GPG-KEY-CentOS-7
rpm -ivh yum-*
cp epel.repo /etc/yum.repos.d/
cp deploy-ceph.repo /etc/yum.repos.d/
cd /etc/yum.repos.d
wget -N http://mirrors.163.com/.help/CentOS7-Base-163.repo
sed -i "s/\$releasever/7/g" CentOS7-Base-163.repo
yum clean all
yum makecache
yum -y install tmux git.x86_64 redhat-lsb.x86_64 sshpass.x86_64 gdisk.x86_64
yum -y update
systemctl stop firewalld
systemctl disable firewalld
setenforce 0
sed -i "s/SELINUX=enforcing/SELINUX=disabled/g" /etc/selinux/config
ln -sf /usr/share/zoneinfo/Asia/Shanghai /etc/localtime
reboot
