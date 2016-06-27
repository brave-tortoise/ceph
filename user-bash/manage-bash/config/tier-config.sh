# 配置缓存池
ceph osd pool set ssd-pool hit_set_type bloom
ceph osd pool set ssd-pool hit_set_period 1800
ceph osd pool set ssd-pool target_max_bytes 100000000000
ceph osd pool set ssd-pool cache_target_dirty_ratio 0.4
ceph osd pool set ssd-pool cache_target_warm_ratio 0.6
ceph osd pool set ssd-pool cache_target_full_ratio 0.8
ceph osd pool set ssd-pool cache_min_flush_age 600
ceph osd pool set ssd-pool cache_min_evict_age 1200
