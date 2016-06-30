cd ~/ceph
make -j32 && make install
cp -f ~/ceph/src/init-ceph /etc/init.d/ceph
