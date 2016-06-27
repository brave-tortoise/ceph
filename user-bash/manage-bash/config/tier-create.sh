# 创建缓存池
ceph osd pool create ssd-pool 256 256
ceph osd pool set ssd-pool crush_ruleset 0
ceph osd pool create hdd-pool 512 512
ceph osd pool set hdd-pool crush_ruleset 1
ceph osd tier add hdd-pool ssd-pool
ceph osd tier cache-mode ssd-pool writeback
ceph osd tier set-overlay hdd-pool ssd-pool
