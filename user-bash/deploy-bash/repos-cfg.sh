rpm -qa | grep yum | xargs rpm -e --nodeps
#wget -N http://mirrors.163.com/centos/RPM-GPG-KEY-CentOS-7
#wget -N http://mirrors.163.com/centos/7/os/x86_64/Packages/yum-3.4.3-132.el7.centos.0.1.noarch.rpm
#wget -N http://mirrors.163.com/centos/7/os/x86_64/Packages/yum-metadata-parser-1.1.4-10.el7.x86_64.rpm
#wget -N http://mirrors.163.com/centos/7/os/x86_64/Packages/yum-plugin-fastestmirror-1.1.31-34.el7.noarch.rpm
rpm --import RPM-GPG-KEY-CentOS-7
rpm -ivh yum-*
#wget -N http://mirrors.163.com/.help/CentOS7-Base-163.repo
#sed -i "s/\$releasever/7/g" CentOS7-Base-163.repo
#cp CentOS7-Base-163.repo /etc/yum.repos.d/
cp CentOS7-Base-aliyun.repo /etc/yum.repos.d/
cp epel.repo /etc/yum.repos.d/
yum clean all
yum makecache
yum -y install tmux git.x86_64 redhat-lsb.x86_64 sshpass.x86_64 gdisk.x86_64
yum -y update
