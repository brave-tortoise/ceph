cd ~/ceph
make -j8 && make install
\cp -f ~/ceph/src/init-ceph /etc/init.d/ceph
