<<<<<<< HEAD
// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#include "test/librbd/test_fixture.h"
#include "test/librbd/test_support.h"
#include "librbd/ExclusiveLock.h"
=======
// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
#include "test/librbd/test_fixture.h"
#include "test/librbd/test_support.h"
>>>>>>> upstream/hammer
#include "librbd/ImageCtx.h"
#include "librbd/ImageWatcher.h"
#include "librbd/internal.h"
#include "librbd/ObjectMap.h"
#include "cls/rbd/cls_rbd_client.h"
#include <list>

void register_test_object_map() {
}

class TestObjectMap : public TestFixture {
public:
<<<<<<< HEAD

  int when_open_object_map(librbd::ImageCtx *ictx) {
    C_SaferCond ctx;
    librbd::ObjectMap<> object_map(*ictx, ictx->snap_id);
    object_map.open(&ctx);
    return ctx.wait();
  }
=======
>>>>>>> upstream/hammer
};

TEST_F(TestObjectMap, RefreshInvalidatesWhenCorrupt) {
  REQUIRE_FEATURE(RBD_FEATURE_OBJECT_MAP);

  librbd::ImageCtx *ictx;
  ASSERT_EQ(0, open_image(m_image_name, &ictx));
  ASSERT_FALSE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));

<<<<<<< HEAD
  C_SaferCond lock_ctx;
  {
    RWLock::WLocker owner_locker(ictx->owner_lock);
    ictx->exclusive_lock->try_acquire_lock(&lock_ctx);
  }
  ASSERT_EQ(0, lock_ctx.wait());

  std::string oid = librbd::ObjectMap<>::object_map_name(ictx->id, CEPH_NOSNAP);
  bufferlist bl;
  bl.append("corrupt");
  ASSERT_EQ(0, ictx->md_ctx.write_full(oid, bl));

  ASSERT_EQ(0, when_open_object_map(ictx));
=======
  {
    RWLock::WLocker owner_locker(ictx->owner_lock);
    ASSERT_EQ(0, ictx->image_watcher->try_lock());
  }

  std::string oid = librbd::ObjectMap::object_map_name(ictx->id, CEPH_NOSNAP);
  bufferlist bl;
  bl.append("corrupt");
  ASSERT_EQ(0, ictx->data_ctx.write_full(oid, bl));

  {
    RWLock::RLocker owner_locker(ictx->owner_lock);
    RWLock::WLocker snap_locker(ictx->snap_lock);
    ictx->object_map.refresh(CEPH_NOSNAP);
  }
>>>>>>> upstream/hammer
  ASSERT_TRUE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));
}

TEST_F(TestObjectMap, RefreshInvalidatesWhenTooSmall) {
  REQUIRE_FEATURE(RBD_FEATURE_OBJECT_MAP);

  librbd::ImageCtx *ictx;
  ASSERT_EQ(0, open_image(m_image_name, &ictx));
  ASSERT_FALSE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));

<<<<<<< HEAD
  C_SaferCond lock_ctx;
  {
    RWLock::WLocker owner_locker(ictx->owner_lock);
    ictx->exclusive_lock->try_acquire_lock(&lock_ctx);
  }
  ASSERT_EQ(0, lock_ctx.wait());
=======
  {
    RWLock::WLocker owner_locker(ictx->owner_lock);
    ASSERT_EQ(0, ictx->image_watcher->try_lock());
  }
>>>>>>> upstream/hammer

  librados::ObjectWriteOperation op;
  librbd::cls_client::object_map_resize(&op, 0, OBJECT_NONEXISTENT);

<<<<<<< HEAD
  std::string oid = librbd::ObjectMap<>::object_map_name(ictx->id, CEPH_NOSNAP);
  ASSERT_EQ(0, ictx->md_ctx.operate(oid, &op));

  ASSERT_EQ(0, when_open_object_map(ictx));
=======
  std::string oid = librbd::ObjectMap::object_map_name(ictx->id, CEPH_NOSNAP);
  ASSERT_EQ(0, ictx->data_ctx.operate(oid, &op));

  {
    RWLock::RLocker owner_locker(ictx->owner_lock);
    RWLock::WLocker snap_locker(ictx->snap_lock);
    ictx->object_map.refresh(CEPH_NOSNAP);
  }
>>>>>>> upstream/hammer
  ASSERT_TRUE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));
}

TEST_F(TestObjectMap, InvalidateFlagOnDisk) {
  REQUIRE_FEATURE(RBD_FEATURE_OBJECT_MAP);

  librbd::ImageCtx *ictx;
  ASSERT_EQ(0, open_image(m_image_name, &ictx));
  ASSERT_FALSE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));

<<<<<<< HEAD
  C_SaferCond lock_ctx;
  {
    RWLock::WLocker owner_locker(ictx->owner_lock);
    ictx->exclusive_lock->try_acquire_lock(&lock_ctx);
  }
  ASSERT_EQ(0, lock_ctx.wait());

  std::string oid = librbd::ObjectMap<>::object_map_name(ictx->id, CEPH_NOSNAP);
  bufferlist bl;
  bl.append("corrupt");
  ASSERT_EQ(0, ictx->md_ctx.write_full(oid, bl));

  ASSERT_EQ(0, when_open_object_map(ictx));
=======
  {
    RWLock::WLocker owner_locker(ictx->owner_lock);
    ASSERT_EQ(0, ictx->image_watcher->try_lock());
  }

  std::string oid = librbd::ObjectMap::object_map_name(ictx->id, CEPH_NOSNAP);
  bufferlist bl;
  bl.append("corrupt");
  ASSERT_EQ(0, ictx->data_ctx.write_full(oid, bl));

  {
    RWLock::RLocker owner_locker(ictx->owner_lock);
    RWLock::WLocker snap_locker(ictx->snap_lock);
    ictx->object_map.refresh(CEPH_NOSNAP);
  }
>>>>>>> upstream/hammer
  ASSERT_TRUE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));

  ASSERT_EQ(0, open_image(m_image_name, &ictx));
  ASSERT_TRUE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));
}

TEST_F(TestObjectMap, InvalidateFlagInMemoryOnly) {
  REQUIRE_FEATURE(RBD_FEATURE_OBJECT_MAP);

  librbd::ImageCtx *ictx;
  ASSERT_EQ(0, open_image(m_image_name, &ictx));
  ASSERT_FALSE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));

<<<<<<< HEAD
  std::string oid = librbd::ObjectMap<>::object_map_name(ictx->id, CEPH_NOSNAP);
  bufferlist valid_bl;
  ASSERT_LT(0, ictx->md_ctx.read(oid, valid_bl, 0, 0));

  bufferlist corrupt_bl;
  corrupt_bl.append("corrupt");
  ASSERT_EQ(0, ictx->md_ctx.write_full(oid, corrupt_bl));

  ASSERT_EQ(0, when_open_object_map(ictx));
  ASSERT_TRUE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));

  ASSERT_EQ(0, ictx->md_ctx.write_full(oid, valid_bl));
=======
  std::string oid = librbd::ObjectMap::object_map_name(ictx->id, CEPH_NOSNAP);
  bufferlist valid_bl;
  ASSERT_LT(0, ictx->data_ctx.read(oid, valid_bl, 0, 0));

  bufferlist corrupt_bl;
  corrupt_bl.append("corrupt");
  ASSERT_EQ(0, ictx->data_ctx.write_full(oid, corrupt_bl));

  {
    RWLock::RLocker owner_locker(ictx->owner_lock);
    RWLock::WLocker snap_locker(ictx->snap_lock);
    ictx->object_map.refresh(CEPH_NOSNAP);
  }
  ASSERT_TRUE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));

  ASSERT_EQ(0, ictx->data_ctx.write_full(oid, valid_bl));
>>>>>>> upstream/hammer
  ASSERT_EQ(0, open_image(m_image_name, &ictx));
  ASSERT_FALSE(ictx->test_flags(RBD_FLAG_OBJECT_MAP_INVALID));
}

