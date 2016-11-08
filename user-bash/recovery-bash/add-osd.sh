data_dev=/dev/sdc1
journal_dev=/dev/sda1
ceph-disk prepare --cluster ceph --cluster-uuid ce927ce5-d33b-4d70-9427-ba5e65bec77a $data_dev $journal_dev
ceph-disk activate $data_dev
./crush-config.sh
