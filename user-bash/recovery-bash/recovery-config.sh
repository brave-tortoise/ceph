for((i=0; i<18; i++))
do
	ceph tell osd.$i injectargs --osd_recovery_tick_interval 0.01
done
