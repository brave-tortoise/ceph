// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "librbd/ObjectMap.h"
#include "librbd/BlockGuard.h"
#include "librbd/ExclusiveLock.h"
#include "librbd/ImageCtx.h"
#include "librbd/object_map/RefreshRequest.h"
#include "librbd/object_map/ResizeRequest.h"
#include "librbd/object_map/SnapshotCreateRequest.h"
#include "librbd/object_map/SnapshotRemoveRequest.h"
#include "librbd/object_map/SnapshotRollbackRequest.h"
#include "librbd/object_map/UnlockRequest.h"
#include "librbd/object_map/UpdateRequest.h"
#include "librbd/Utils.h"
#include "common/dout.h"
#include "common/errno.h"
#include "common/WorkQueue.h"

#include "include/rados/librados.hpp"

#include "cls/lock/cls_lock_client.h"
#include "cls/rbd/cls_rbd_types.h"
#include "include/stringify.h"
#include "osdc/Striper.h"
#include <sstream>

#define dout_subsys ceph_subsys_rbd
#undef dout_prefix
#define dout_prefix *_dout << "librbd::ObjectMap: " << this << " " << __func__ \
                           << ": "

namespace librbd {

template <typename I>
ObjectMap<I>::ObjectMap(I &image_ctx, uint64_t snap_id)
  : m_image_ctx(image_ctx), m_snap_id(snap_id),
    m_update_guard(new UpdateGuard(m_image_ctx.cct)) {
}

template <typename I>
ObjectMap<I>::~ObjectMap() {
  delete m_update_guard;
}

template <typename I>
int ObjectMap<I>::aio_remove(librados::IoCtx &io_ctx, const std::string &image_id,
			     librados::AioCompletion *c) {
  return io_ctx.aio_remove(object_map_name(image_id, CEPH_NOSNAP), c);
}

template <typename I>
std::string ObjectMap<I>::object_map_name(const std::string &image_id,
				          uint64_t snap_id) {
  std::string oid(RBD_OBJECT_MAP_PREFIX + image_id);
  if (snap_id != CEPH_NOSNAP) {
    std::stringstream snap_suffix;
    snap_suffix << "." << std::setfill('0') << std::setw(16) << std::hex
		<< snap_id;
    oid += snap_suffix.str();
  }
  return oid;
}

<<<<<<< HEAD
template <typename I>
bool ObjectMap<I>::is_compatible(const file_layout_t& layout, uint64_t size) {
  uint64_t object_count = Striper::get_num_objects(layout, size);
  return (object_count <= cls::rbd::MAX_OBJECT_MAP_OBJECT_COUNT);
=======
uint8_t ObjectMap::operator[](uint64_t object_no) const
{
  assert(m_image_ctx.object_map_lock.is_locked());
  assert(object_no < m_object_map.size());
  return m_object_map[object_no];
}

bool ObjectMap::enabled() const
{
  RWLock::RLocker l(m_image_ctx.object_map_lock);
  return m_enabled;
>>>>>>> upstream/hammer
}

template <typename I>
ceph::BitVector<2u>::Reference ObjectMap<I>::operator[](uint64_t object_no)
{
  assert(m_image_ctx.object_map_lock.is_wlocked());
  assert(object_no < m_object_map.size());
  return m_object_map[object_no];
}

template <typename I>
uint8_t ObjectMap<I>::operator[](uint64_t object_no) const
{
  assert(m_image_ctx.object_map_lock.is_locked());
  assert(object_no < m_object_map.size());
  return m_object_map[object_no];
}

template <typename I>
bool ObjectMap<I>::object_may_exist(uint64_t object_no) const
{
  assert(m_image_ctx.snap_lock.is_locked());

  // Fall back to default logic if object map is disabled or invalid
  if (!m_image_ctx.test_features(RBD_FEATURE_OBJECT_MAP,
                                 m_image_ctx.snap_lock) ||
      m_image_ctx.test_flags(RBD_FLAG_OBJECT_MAP_INVALID,
                             m_image_ctx.snap_lock)) {
    return true;
  }

  RWLock::RLocker l(m_image_ctx.object_map_lock);
<<<<<<< HEAD
  uint8_t state = (*this)[object_no];
  bool exists = (state != OBJECT_NONEXISTENT);
  ldout(m_image_ctx.cct, 20) << "object_no=" << object_no << " r=" << exists
=======
  if (!m_enabled) {
    return true;
  }
  assert(object_no < m_object_map.size());

  uint8_t state = (*this)[object_no];
  bool exists = (state == OBJECT_EXISTS || state == OBJECT_PENDING);
  ldout(m_image_ctx.cct, 20) << &m_image_ctx << " object_may_exist: "
			     << "object_no=" << object_no << " r=" << exists
>>>>>>> upstream/hammer
			     << dendl;
  return exists;
}

<<<<<<< HEAD
template <typename I>
bool ObjectMap<I>::update_required(uint64_t object_no, uint8_t new_state) {
  assert(m_image_ctx.object_map_lock.is_wlocked());
  uint8_t state = (*this)[object_no];

  if ((state == new_state) ||
      (new_state == OBJECT_PENDING && state == OBJECT_NONEXISTENT) ||
      (new_state == OBJECT_NONEXISTENT && state != OBJECT_PENDING)) {
    return false;
=======
void ObjectMap::refresh(uint64_t snap_id)
{
  assert(m_image_ctx.snap_lock.is_wlocked());
  RWLock::WLocker l(m_image_ctx.object_map_lock);

  uint64_t features;
  m_image_ctx.get_features(snap_id, &features);
  if ((features & RBD_FEATURE_OBJECT_MAP) == 0 ||
      (m_image_ctx.snap_id == snap_id && !m_image_ctx.snap_exists)) {
    m_object_map.clear();
    m_enabled = false;
    return;
  }
  m_enabled = true;

  CephContext *cct = m_image_ctx.cct;
  ldout(cct, 10) << &m_image_ctx << " refreshing object map" << dendl;

  std::string oid(object_map_name(m_image_ctx.id, snap_id));
  int r = cls_client::object_map_load(&m_image_ctx.md_ctx, oid,
                                      &m_object_map);
  if (r < 0) {
    lderr(cct) << "error refreshing object map: " << cpp_strerror(r)
               << dendl;
    invalidate(false);
    m_object_map.clear();
    return;
  }

  ldout(cct, 20) << "refreshed object map: " << m_object_map.size()
                 << dendl;

  uint64_t num_objs = Striper::get_num_objects(
    m_image_ctx.layout, m_image_ctx.get_image_size(snap_id));
  if (m_object_map.size() < num_objs) {
    lderr(cct) << "object map smaller than current object count: "
               << m_object_map.size() << " != " << num_objs << dendl;
    invalidate(false);
  } else if (m_object_map.size() > num_objs) {
    // resize op might have been interrupted
    ldout(cct, 1) << "object map larger than current object count: "
                  << m_object_map.size() << " != " << num_objs << dendl;
>>>>>>> upstream/hammer
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
void ObjectMap<I>::open(Context *on_finish) {
  auto req = object_map::RefreshRequest<I>::create(
    m_image_ctx, &m_object_map, m_snap_id, on_finish);
  req->send();
=======
void ObjectMap::rollback(uint64_t snap_id) {
  assert(m_image_ctx.snap_lock.is_wlocked());
  int r;
  std::string oid(object_map_name(m_image_ctx.id, CEPH_NOSNAP));

  CephContext *cct = m_image_ctx.cct;
  ldout(cct, 10) << &m_image_ctx << " rollback object map" << dendl;

  uint64_t features;
  m_image_ctx.get_features(snap_id, &features);
  if ((features & RBD_FEATURE_OBJECT_MAP) == 0) {
    r = m_image_ctx.md_ctx.remove(oid);
    if (r < 0 && r != -ENOENT) {
      lderr(cct) << "unable to remove object map: " << cpp_strerror(r)
		 << dendl;
    }
    return;
  }

  RWLock::WLocker l(m_image_ctx.object_map_lock);
  if (!m_enabled) {
    return;
  }

  std::string snap_oid(object_map_name(m_image_ctx.id, snap_id));
  bufferlist bl;
  r = m_image_ctx.md_ctx.read(snap_oid, bl, 0, 0);
  if (r < 0) {
    lderr(cct) << "unable to load snapshot object map '" << snap_oid << "': "
	       << cpp_strerror(r) << dendl;
    invalidate(false);
    return;
  }

  librados::ObjectWriteOperation op;
  rados::cls::lock::assert_locked(&op, RBD_LOCK_NAME, LOCK_EXCLUSIVE, "", "");
  op.write_full(bl);

  r = m_image_ctx.md_ctx.operate(oid, &op);
  if (r < 0) {
    lderr(cct) << "unable to rollback object map: " << cpp_strerror(r)
	       << dendl;
    invalidate(true);
  }
>>>>>>> upstream/hammer
}

template <typename I>
void ObjectMap<I>::close(Context *on_finish) {
  if (m_snap_id != CEPH_NOSNAP) {
    m_image_ctx.op_work_queue->queue(on_finish, 0);
    return;
  }

  auto req = object_map::UnlockRequest<I>::create(m_image_ctx, on_finish);
  req->send();
}

<<<<<<< HEAD
template <typename I>
void ObjectMap<I>::rollback(uint64_t snap_id, Context *on_finish) {
  assert(m_image_ctx.snap_lock.is_locked());
  assert(m_image_ctx.object_map_lock.is_wlocked());

  object_map::SnapshotRollbackRequest *req =
    new object_map::SnapshotRollbackRequest(m_image_ctx, snap_id, on_finish);
  req->send();
}

template <typename I>
void ObjectMap<I>::snapshot_add(uint64_t snap_id, Context *on_finish) {
  assert(m_image_ctx.snap_lock.is_locked());
  assert((m_image_ctx.features & RBD_FEATURE_OBJECT_MAP) != 0);
  assert(snap_id != CEPH_NOSNAP);
=======
  int r;
  bufferlist bl;
  RWLock::WLocker l(m_image_ctx.object_map_lock);
  if (!m_enabled) {
    return;
  }
  std::string oid(object_map_name(m_image_ctx.id, CEPH_NOSNAP));
  r = m_image_ctx.md_ctx.read(oid, bl, 0, 0);
  if (r < 0) {
    lderr(cct) << "unable to load object map: " << cpp_strerror(r)
	       << dendl;
    invalidate(false);
    return;
  }

  std::string snap_oid(object_map_name(m_image_ctx.id, snap_id));
  r = m_image_ctx.md_ctx.write_full(snap_oid, bl);
  if (r < 0) {
    lderr(cct) << "unable to snapshot object map '" << snap_oid << "': "
	       << cpp_strerror(r) << dendl;
    invalidate(false);
  }
}

void ObjectMap::aio_resize(uint64_t new_size, uint8_t default_object_state,
			   Context *on_finish) {
  assert(m_image_ctx.test_features(RBD_FEATURE_OBJECT_MAP));
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.image_watcher != NULL);
  assert(m_image_ctx.image_watcher->is_lock_owner());
>>>>>>> upstream/hammer

  object_map::SnapshotCreateRequest *req =
    new object_map::SnapshotCreateRequest(m_image_ctx, &m_object_map, snap_id,
                                          on_finish);
  req->send();
}

template <typename I>
void ObjectMap<I>::snapshot_remove(uint64_t snap_id, Context *on_finish) {
  assert(m_image_ctx.snap_lock.is_wlocked());
  assert((m_image_ctx.features & RBD_FEATURE_OBJECT_MAP) != 0);
  assert(snap_id != CEPH_NOSNAP);

  object_map::SnapshotRemoveRequest *req =
    new object_map::SnapshotRemoveRequest(m_image_ctx, &m_object_map, snap_id,
                                          on_finish);
  req->send();
}

<<<<<<< HEAD
template <typename I>
void ObjectMap<I>::aio_save(Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.snap_lock.is_locked());
  assert(m_image_ctx.test_features(RBD_FEATURE_OBJECT_MAP,
                                   m_image_ctx.snap_lock));
  RWLock::RLocker object_map_locker(m_image_ctx.object_map_lock);

  librados::ObjectWriteOperation op;
  if (m_snap_id == CEPH_NOSNAP) {
    rados::cls::lock::assert_locked(&op, RBD_LOCK_NAME, LOCK_EXCLUSIVE, "", "");
=======
bool ObjectMap::aio_update(uint64_t start_object_no, uint64_t end_object_no,
			   uint8_t new_state,
                           const boost::optional<uint8_t> &current_state,
                           Context *on_finish)
{
  assert(m_image_ctx.snap_lock.is_locked());
  assert((m_image_ctx.features & RBD_FEATURE_OBJECT_MAP) != 0);
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.image_watcher != NULL);
  assert(m_image_ctx.image_watcher->is_lock_owner());
  assert(m_image_ctx.object_map_lock.is_wlocked());
  assert(start_object_no < end_object_no);
  
  CephContext *cct = m_image_ctx.cct;
  ldout(cct, 20) << &m_image_ctx << " aio_update: start=" << start_object_no
		 << ", end=" << end_object_no << ", new_state="
		 << static_cast<uint32_t>(new_state) << dendl;
  if (end_object_no > m_object_map.size()) {
    ldout(cct, 20) << "skipping update of invalid object map" << dendl;
    return false;
  }
  
  for (uint64_t object_no = start_object_no; object_no < end_object_no;
       ++object_no) {
    if ((!current_state || m_object_map[object_no] == *current_state) &&
        m_object_map[object_no] != new_state) {
      UpdateRequest *req = new UpdateRequest(m_image_ctx, start_object_no,
					     end_object_no, new_state,
					     current_state, on_finish);
      req->send();
      return true;
    }
>>>>>>> upstream/hammer
  }
  cls_client::object_map_save(&op, m_object_map);

  std::string oid(object_map_name(m_image_ctx.id, m_snap_id));
  librados::AioCompletion *comp = util::create_rados_safe_callback(on_finish);

  int r = m_image_ctx.md_ctx.aio_operate(oid, comp, &op);
  assert(r == 0);
  comp->release();
}

<<<<<<< HEAD
template <typename I>
void ObjectMap<I>::aio_resize(uint64_t new_size, uint8_t default_object_state,
			      Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.snap_lock.is_locked());
  assert(m_image_ctx.test_features(RBD_FEATURE_OBJECT_MAP,
                                   m_image_ctx.snap_lock));
  assert(m_image_ctx.image_watcher != NULL);
  assert(m_image_ctx.exclusive_lock == nullptr ||
         m_image_ctx.exclusive_lock->is_lock_owner());

  object_map::ResizeRequest *req = new object_map::ResizeRequest(
    m_image_ctx, &m_object_map, m_snap_id, new_size, default_object_state,
    on_finish);
  req->send();
}
=======
void ObjectMap::invalidate(bool force) {
  assert(m_image_ctx.snap_lock.is_wlocked());
  assert(m_image_ctx.object_map_lock.is_wlocked());
  uint64_t flags;
  m_image_ctx.get_flags(m_image_ctx.snap_id, &flags);
  if ((flags & RBD_FLAG_OBJECT_MAP_INVALID) != 0) {
    return;
  }
>>>>>>> upstream/hammer

template <typename I>
void ObjectMap<I>::detained_aio_update(UpdateOperation &&op) {
  CephContext *cct = m_image_ctx.cct;
  ldout(cct, 20) << dendl;

<<<<<<< HEAD
  assert(m_image_ctx.snap_lock.is_locked());
  assert(m_image_ctx.object_map_lock.is_wlocked());

  BlockGuardCell *cell;
  int r = m_update_guard->detain({op.start_object_no, op.end_object_no},
                                &op, &cell);
  if (r < 0) {
    lderr(cct) << "failed to detain object map update: " << cpp_strerror(r)
               << dendl;
    m_image_ctx.op_work_queue->queue(op.on_finish, r);
    return;
  } else if (r > 0) {
    ldout(cct, 20) << "detaining object map update due to in-flight update: "
                   << "start=" << op.start_object_no << ", "
		   << "end=" << op.end_object_no << ", "
                   << (op.current_state ?
                         stringify(static_cast<uint32_t>(*op.current_state)) :
                         "")
		   << "->" << static_cast<uint32_t>(op.new_state) << dendl;
    return;
=======
  // do not update on-disk flags if not image owner
  if (m_image_ctx.image_watcher == NULL ||
      (m_image_ctx.image_watcher->is_lock_supported(m_image_ctx.snap_lock) &&
       !m_image_ctx.image_watcher->is_lock_owner())) {
    return;
  }

  librados::ObjectWriteOperation op;
  if (m_image_ctx.snap_id == CEPH_NOSNAP && !force) {
    m_image_ctx.image_watcher->assert_header_locked(&op);
  }
  cls_client::set_flags(&op, m_image_ctx.snap_id, m_image_ctx.flags,
                        RBD_FLAG_OBJECT_MAP_INVALID);

  int r = m_image_ctx.md_ctx.operate(m_image_ctx.header_oid, &op);
  if (r == -EBUSY) {
    ldout(cct, 5) << "skipping on-disk object map invalidation: "
                  << "image not locked by client" << dendl;
  } else if (r < 0) {
    lderr(cct) << "failed to invalidate on-disk object map: " << cpp_strerror(r)
	       << dendl;
>>>>>>> upstream/hammer
  }

  ldout(cct, 20) << "in-flight update cell: " << cell << dendl;
  Context *on_finish = op.on_finish;
  Context *ctx = new FunctionContext([this, cell, on_finish](int r) {
      handle_detained_aio_update(cell, r, on_finish);
    });
  aio_update(CEPH_NOSNAP, op.start_object_no, op.end_object_no, op.new_state,
             op.current_state, ctx);
}

template <typename I>
void ObjectMap<I>::handle_detained_aio_update(BlockGuardCell *cell, int r,
                                              Context *on_finish) {
  CephContext *cct = m_image_ctx.cct;
<<<<<<< HEAD
  ldout(cct, 20) << "cell=" << cell << ", r=" << r << dendl;
=======
  ldout(cct, 20) << &m_image_ctx << " should_complete: r=" << r << dendl;

  switch (m_state)
  {
  case STATE_REQUEST:
    if (r == -EBUSY) {
      lderr(cct) << "object map lock not owned by client" << dendl;
      return invalidate();
    } else if (r < 0) {
      lderr(cct) << "failed to update object map: " << cpp_strerror(r)
		 << dendl;
      return invalidate();
    }
>>>>>>> upstream/hammer

  typename UpdateGuard::BlockOperations block_ops;
  m_update_guard->release(cell, &block_ops);

  {
    RWLock::RLocker snap_locker(m_image_ctx.snap_lock);
    RWLock::WLocker object_map_locker(m_image_ctx.object_map_lock);
    for (auto &op : block_ops) {
      detained_aio_update(std::move(op));
    }
  }

  on_finish->complete(r);
}

template <typename I>
void ObjectMap<I>::aio_update(uint64_t snap_id, uint64_t start_object_no,
                              uint64_t end_object_no, uint8_t new_state,
                              const boost::optional<uint8_t> &current_state,
                              Context *on_finish) {
  assert(m_image_ctx.snap_lock.is_locked());
  assert((m_image_ctx.features & RBD_FEATURE_OBJECT_MAP) != 0);
  assert(m_image_ctx.image_watcher != nullptr);
  assert(m_image_ctx.exclusive_lock == nullptr ||
         m_image_ctx.exclusive_lock->is_lock_owner());
  assert(snap_id != CEPH_NOSNAP || m_image_ctx.object_map_lock.is_wlocked());
  assert(start_object_no < end_object_no);

  CephContext *cct = m_image_ctx.cct;
  ldout(cct, 20) << "start=" << start_object_no << ", "
		 << "end=" << end_object_no << ", "
                 << (current_state ?
                       stringify(static_cast<uint32_t>(*current_state)) : "")
		 << "->" << static_cast<uint32_t>(new_state) << dendl;
  if (snap_id == CEPH_NOSNAP) {
    if (end_object_no > m_object_map.size()) {
      ldout(cct, 20) << "skipping update of invalid object map" << dendl;
      m_image_ctx.op_work_queue->queue(on_finish, 0);
      return;
    }

    uint64_t object_no;
    for (object_no = start_object_no; object_no < end_object_no; ++object_no) {
      if (update_required(object_no, new_state)) {
        break;
      }
    }
    if (object_no == end_object_no) {
      ldout(cct, 20) << "object map update not required" << dendl;
      m_image_ctx.op_work_queue->queue(on_finish, 0);
      return;
    }
  }

  auto req = object_map::UpdateRequest<I>::create(
    m_image_ctx, &m_object_map, snap_id, start_object_no, end_object_no,
    new_state, current_state, on_finish);
  req->send();
}

} // namespace librbd

template class librbd::ObjectMap<librbd::ImageCtx>;

