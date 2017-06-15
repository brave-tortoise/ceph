cd ../..
./install-deps.sh
./autogen.sh
./configure --prefix=/usr --sbindir=/sbin --localstatedir=/var --sysconfdir=/etc --without-radosgw --without-debug
make && make install
\cp -f src/init-ceph /etc/init.d/ceph

cd user-bash/deploy-bash
mkdir /etc/ceph
\cp -f ceph.conf /etc/ceph/ceph.conf
mkdir -p /var/lib/ceph/mon
mkdir -p /var/lib/ceph/osd
