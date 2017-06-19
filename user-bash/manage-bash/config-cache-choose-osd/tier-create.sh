# 创建缓存池
ceph osd pool create ssd-pool 256 256
ceph osd pool set ssd-pool crush_ruleset 0
ceph osd tier add hdd-pool ssd-pool #--force-nonempty
ceph osd tier cache-mode ssd-pool writeback
ceph osd tier set-overlay hdd-pool ssd-pool
