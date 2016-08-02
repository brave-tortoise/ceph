cd ~/ceph
./configure --prefix=/usr --sbindir=/sbin --localstatedir=/var --sysconfdir=/etc --without-radosgw --without-debug
make -j8 && make install
\cp -f ~/ceph/src/init-ceph /etc/init.d/ceph
