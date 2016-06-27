cd
#git clone https://github.com/ceph/ceph.git
#git clone git@github.com:brave-tortoise/ceph.git
cd ceph
#git checkout -f -b wugy v0.94.2
#git submodule update --init --recursive
#sed -i "s/RedHatEnterpriseServer/RedHatEnterpriseServer|n\/a/g" install-deps.sh
./install-deps.sh
./autogen.sh
./configure --prefix=/usr --sbindir=/sbin --localstatedir=/var --sysconfdir=/etc --without-radosgw
make -j8 && make install
cp ~/ceph/src/sample.ceph.conf /etc/ceph/ceph.conf
cp ~/ceph/src/init-ceph /etc/init.d/ceph
mkdir /var/lib/ceph/mon
mkdir /var/lib/ceph/osd
echo "/usr/lib" >> /etc/ld.so.conf
echo "/usr/lib64" >> /etc/ld.so.conf
ldconfig
