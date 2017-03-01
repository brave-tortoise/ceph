// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "librbd/ImageWatcher.h"
#include "librbd/ExclusiveLock.h"
#include "librbd/ImageCtx.h"
<<<<<<< HEAD
#include "librbd/ImageState.h"
#include "librbd/internal.h"
#include "librbd/Operations.h"
=======
#include "librbd/internal.h"
#include "librbd/ObjectMap.h"
>>>>>>> upstream/hammer
#include "librbd/TaskFinisher.h"
#include "librbd/Utils.h"
#include "librbd/exclusive_lock/Policy.h"
#include "librbd/image_watcher/NotifyLockOwner.h"
#include "librbd/io/AioCompletion.h"
#include "include/encoding.h"
#include "common/errno.h"
#include "common/WorkQueue.h"
<<<<<<< HEAD
=======
#include <sstream>
>>>>>>> upstream/hammer
#include <boost/bind.hpp>

#define dout_subsys ceph_subsys_rbd
#undef dout_prefix
#define dout_prefix *_dout << "librbd::ImageWatcher: "

namespace librbd {

using namespace image_watcher;
using namespace watch_notify;
using util::create_async_context_callback;
using util::create_context_callback;
using util::create_rados_safe_callback;
using librbd::watcher::HandlePayloadVisitor;
using librbd::watcher::C_NotifyAck;

static const double	RETRY_DELAY_SECONDS = 1.0;

<<<<<<< HEAD
template <typename I>
ImageWatcher<I>::ImageWatcher(I &image_ctx)
  : Watcher(image_ctx.md_ctx, image_ctx.op_work_queue, image_ctx.header_oid),
    m_image_ctx(image_ctx),
    m_task_finisher(new TaskFinisher<Task>(*m_image_ctx.cct)),
    m_async_request_lock(util::unique_lock_name("librbd::ImageWatcher::m_async_request_lock", this)),
    m_owner_client_id_lock(util::unique_lock_name("librbd::ImageWatcher::m_owner_client_id_lock", this))
=======
ImageWatcher::ImageWatcher(ImageCtx &image_ctx)
  : m_image_ctx(image_ctx),
    m_watch_lock(unique_lock_name("librbd::ImageWatcher::m_watch_lock", this)),
    m_watch_ctx(*this), m_watch_handle(0),
    m_watch_state(WATCH_STATE_UNREGISTERED),
    m_lock_owner_state(LOCK_OWNER_STATE_NOT_LOCKED),
    m_task_finisher(new TaskFinisher<Task>(*m_image_ctx.cct)),
    m_async_request_lock(unique_lock_name("librbd::ImageWatcher::m_async_request_lock", this)),
    m_aio_request_lock(unique_lock_name("librbd::ImageWatcher::m_aio_request_lock", this)),
    m_owner_client_id_lock(unique_lock_name("librbd::ImageWatcher::m_owner_client_id_lock", this))
>>>>>>> upstream/hammer
{
}

template <typename I>
ImageWatcher<I>::~ImageWatcher()
{
  delete m_task_finisher;
<<<<<<< HEAD
}

template <typename I>
void ImageWatcher<I>::unregister_watch(Context *on_finish) {
  CephContext *cct = m_image_ctx.cct;
  ldout(cct, 10) << this << " unregistering image watcher" << dendl;
=======
  {
    RWLock::RLocker l(m_watch_lock);
    assert(m_watch_state != WATCH_STATE_REGISTERED);
  }
  {
    RWLock::RLocker l(m_image_ctx.owner_lock);
    assert(m_lock_owner_state == LOCK_OWNER_STATE_NOT_LOCKED);
  }
}

bool ImageWatcher::is_lock_supported() const {
  RWLock::RLocker l(m_image_ctx.snap_lock);
  return is_lock_supported(m_image_ctx.snap_lock);
}

bool ImageWatcher::is_lock_supported(const RWLock &) const {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.snap_lock.is_locked());
  return ((m_image_ctx.features & RBD_FEATURE_EXCLUSIVE_LOCK) != 0 &&
	  !m_image_ctx.read_only && m_image_ctx.snap_id == CEPH_NOSNAP);
}

bool ImageWatcher::is_lock_owner() const {
  assert(m_image_ctx.owner_lock.is_locked());
  return (m_lock_owner_state == LOCK_OWNER_STATE_LOCKED ||
          m_lock_owner_state == LOCK_OWNER_STATE_RELEASING);
}

int ImageWatcher::register_watch() {
  ldout(m_image_ctx.cct, 10) << this << " registering image watcher" << dendl;

  RWLock::WLocker l(m_watch_lock);
  assert(m_watch_state == WATCH_STATE_UNREGISTERED);
  int r = m_image_ctx.md_ctx.watch2(m_image_ctx.header_oid,
				    &m_watch_handle,
				    &m_watch_ctx);
  if (r < 0) {
    return r;
  }

  m_watch_state = WATCH_STATE_REGISTERED;
  return 0;
}

int ImageWatcher::unregister_watch() {
  ldout(m_image_ctx.cct, 10) << this << " unregistering image watcher" << dendl;

  {
    Mutex::Locker l(m_aio_request_lock);
    assert(m_aio_requests.empty());
  }
>>>>>>> upstream/hammer

  cancel_async_requests();

  FunctionContext *ctx = new FunctionContext([this, on_finish](int r) {
    m_task_finisher->cancel_all(on_finish);
  });
  Watcher::unregister_watch(ctx);
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::schedule_async_progress(const AsyncRequestId &request,
					      uint64_t offset, uint64_t total) {
  FunctionContext *ctx = new FunctionContext(
    boost::bind(&ImageWatcher<I>::notify_async_progress, this, request, offset,
                total));
  m_task_finisher->queue(Task(TASK_CODE_ASYNC_PROGRESS, request), ctx);
}

template <typename I>
int ImageWatcher<I>::notify_async_progress(const AsyncRequestId &request,
				           uint64_t offset, uint64_t total) {
  ldout(m_image_ctx.cct, 20) << this << " remote async request progress: "
			     << request << " @ " << offset
			     << "/" << total << dendl;

  send_notify(AsyncProgressPayload(request, offset, total));
  return 0;
}

template <typename I>
void ImageWatcher<I>::schedule_async_complete(const AsyncRequestId &request,
                                              int r) {
  FunctionContext *ctx = new FunctionContext(
    boost::bind(&ImageWatcher<I>::notify_async_complete, this, request, r));
  m_task_finisher->queue(ctx);
=======
int ImageWatcher::try_lock() {
  assert(m_image_ctx.owner_lock.is_wlocked());
  assert(m_lock_owner_state == LOCK_OWNER_STATE_NOT_LOCKED);

  while (true) {
    int r = lock();
    if (r != -EBUSY) {
      return r;
    }

    // determine if the current lock holder is still alive
    entity_name_t locker;
    std::string locker_cookie;
    std::string locker_address;
    uint64_t locker_handle;
    r = get_lock_owner_info(&locker, &locker_cookie, &locker_address,
			    &locker_handle);
    if (r < 0) {
      return r;
    }
    if (locker_cookie.empty() || locker_address.empty()) {
      // lock is now unlocked ... try again
      continue;
    }

    std::list<obj_watch_t> watchers;
    r = m_image_ctx.md_ctx.list_watchers(m_image_ctx.header_oid, &watchers);
    if (r < 0) {
      return r;
    }

    for (std::list<obj_watch_t>::iterator iter = watchers.begin();
	 iter != watchers.end(); ++iter) {
      if ((strncmp(locker_address.c_str(),
                   iter->addr, sizeof(iter->addr)) == 0) &&
	  (locker_handle == iter->cookie)) {
	Mutex::Locker l(m_owner_client_id_lock);
        set_owner_client_id(ClientId(iter->watcher_id, locker_handle));
	return 0;
      }
    }

    md_config_t *conf = m_image_ctx.cct->_conf;
    if (conf->rbd_blacklist_on_break_lock) {
      ldout(m_image_ctx.cct, 1) << this << " blacklisting client: " << locker
                                << "@" << locker_address << dendl;
      librados::Rados rados(m_image_ctx.md_ctx);
      r = rados.blacklist_add(locker_address,
			      conf->rbd_blacklist_expire_seconds);
      if (r < 0) {
        lderr(m_image_ctx.cct) << this << " unable to blacklist client: "
			       << cpp_strerror(r) << dendl;
        return r;
      }
    }

    ldout(m_image_ctx.cct, 5) << this << " breaking exclusive lock: " << locker
                              << dendl;
    r = rados::cls::lock::break_lock(&m_image_ctx.md_ctx,
                                     m_image_ctx.header_oid, RBD_LOCK_NAME,
                                     locker_cookie, locker);
    if (r < 0 && r != -ENOENT) {
      return r;
    }
  }
  return 0;
}

void ImageWatcher::request_lock(
    const boost::function<void(AioCompletion*)>& restart_op, AioCompletion* c) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_lock_owner_state == LOCK_OWNER_STATE_NOT_LOCKED);

  {
    Mutex::Locker l(m_aio_request_lock);
    bool request_pending = !m_aio_requests.empty();
    ldout(m_image_ctx.cct, 15) << this << " queuing aio request: " << c
			       << dendl;

    c->get();
    m_aio_requests.push_back(std::make_pair(restart_op, c));
    if (request_pending) {
      return;
    }
  }

  RWLock::RLocker l(m_watch_lock);
  if (m_watch_state == WATCH_STATE_REGISTERED) {
    ldout(m_image_ctx.cct, 15) << this << " requesting exclusive lock" << dendl;

    // run notify request in finisher to avoid blocking aio path
    FunctionContext *ctx = new FunctionContext(
      boost::bind(&ImageWatcher::notify_request_lock, this));
    m_task_finisher->queue(TASK_CODE_REQUEST_LOCK, ctx);
  }
>>>>>>> upstream/hammer
}

template <typename I>
void ImageWatcher<I>::notify_async_complete(const AsyncRequestId &request,
                                            int r) {
  ldout(m_image_ctx.cct, 20) << this << " remote async request finished: "
			     << request << " = " << r << dendl;

  send_notify(AsyncCompletePayload(request, r),
    new FunctionContext(boost::bind(&ImageWatcher<I>::handle_async_complete,
                        this, request, r, _1)));
}

template <typename I>
void ImageWatcher<I>::handle_async_complete(const AsyncRequestId &request,
                                            int r, int ret_val) {
  ldout(m_image_ctx.cct, 20) << this << " " << __func__ << ": "
                             << "request=" << request << ", r=" << ret_val
                             << dendl;
  if (ret_val < 0) {
    lderr(m_image_ctx.cct) << this << " failed to notify async complete: "
			   << cpp_strerror(ret_val) << dendl;
    if (ret_val == -ETIMEDOUT) {
      schedule_async_complete(request, r);
    }
<<<<<<< HEAD
  } else {
    RWLock::WLocker async_request_locker(m_async_request_lock);
    m_async_pending.erase(request);
=======
  }
  m_image_ctx.owner_lock.get_read();

  if (r < 0) {
    ldout(m_image_ctx.cct, 5) << this << " failed to acquire exclusive lock:"
			      << cpp_strerror(r) << dendl;
    return false;
  }

  if (is_lock_owner()) {
    ldout(m_image_ctx.cct, 15) << this << " successfully acquired exclusive lock"
			       << dendl;
  } else {
    ldout(m_image_ctx.cct, 15) << this
                               << " unable to acquire exclusive lock, retrying"
                               << dendl;
>>>>>>> upstream/hammer
  }
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::notify_flatten(uint64_t request_id,
                                     ProgressContext &prog_ctx,
                                     Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

  AsyncRequestId async_request_id(get_client_id(), request_id);

  notify_async_request(async_request_id, FlattenPayload(async_request_id),
                       prog_ctx, on_finish);
}

template <typename I>
void ImageWatcher<I>::notify_resize(uint64_t request_id, uint64_t size,
			            bool allow_shrink,
                                    ProgressContext &prog_ctx,
                                    Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());
=======
int ImageWatcher::get_lock_owner_info(entity_name_t *locker, std::string *cookie,
				      std::string *address, uint64_t *handle) {
  std::map<rados::cls::lock::locker_id_t,
	   rados::cls::lock::locker_info_t> lockers;
  ClsLockType lock_type;
  std::string lock_tag;
  int r = rados::cls::lock::get_lock_info(&m_image_ctx.md_ctx,
					  m_image_ctx.header_oid,
					  RBD_LOCK_NAME, &lockers, &lock_type,
					  &lock_tag);
  if (r < 0) {
    return r;
  }

  if (lockers.empty()) {
    ldout(m_image_ctx.cct, 20) << this << " no lockers detected" << dendl;
    return 0;
  }

  if (lock_tag != WATCHER_LOCK_TAG) {
    ldout(m_image_ctx.cct, 5) << this << " locked by external mechanism: tag="
			      << lock_tag << dendl;
    return -EBUSY;
  }

  if (lock_type == LOCK_SHARED) {
    ldout(m_image_ctx.cct, 5) << this << " shared lock type detected" << dendl;
    return -EBUSY;
  }

  std::map<rados::cls::lock::locker_id_t,
           rados::cls::lock::locker_info_t>::iterator iter = lockers.begin();
  if (!decode_lock_cookie(iter->first.cookie, handle)) {
    ldout(m_image_ctx.cct, 5) << this << " locked by external mechanism: "
                              << "cookie=" << iter->first.cookie << dendl;
    return -EBUSY;
  }

  *locker = iter->first.locker;
  *cookie = iter->first.cookie;
  *address = stringify(iter->second.addr);
  ldout(m_image_ctx.cct, 10) << this << " retrieved exclusive locker: "
                             << *locker << "@" << *address << dendl;
  return 0;
}

int ImageWatcher::lock() {
  int r = rados::cls::lock::lock(&m_image_ctx.md_ctx, m_image_ctx.header_oid,
				 RBD_LOCK_NAME, LOCK_EXCLUSIVE,
				 encode_lock_cookie(), WATCHER_LOCK_TAG, "",
				 utime_t(), 0);
  if (r < 0) {
    return r;
  }

  ldout(m_image_ctx.cct, 10) << this << " acquired exclusive lock" << dendl;
  m_lock_owner_state = LOCK_OWNER_STATE_LOCKED;

  ClientId owner_client_id = get_client_id();
  {
    Mutex::Locker l(m_owner_client_id_lock);
    set_owner_client_id(owner_client_id);
  }

  if (m_image_ctx.object_map.enabled()) {
    r = m_image_ctx.object_map.lock();
    if (r < 0 && r != -ENOENT) {
      unlock();
      return r;
    }
    RWLock::WLocker l2(m_image_ctx.snap_lock);
    m_image_ctx.object_map.refresh(CEPH_NOSNAP);
  }

  bufferlist bl;
  ::encode(NotifyMessage(AcquiredLockPayload(get_client_id())), bl);

  // send the notification when we aren't holding locks
  FunctionContext *ctx = new FunctionContext(
    boost::bind(&IoCtx::notify2, &m_image_ctx.md_ctx, m_image_ctx.header_oid,
		bl, NOTIFY_TIMEOUT, reinterpret_cast<bufferlist *>(NULL)));
  m_task_finisher->queue(TASK_CODE_ACQUIRED_LOCK, ctx);
  return 0;
}
>>>>>>> upstream/hammer

  AsyncRequestId async_request_id(get_client_id(), request_id);

  notify_async_request(async_request_id,
                       ResizePayload(size, allow_shrink, async_request_id),
                       prog_ctx, on_finish);
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::notify_snap_create(const std::string &snap_name,
					 const cls::rbd::SnapshotNamespace &snap_namespace,
                                         Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

  notify_lock_owner(SnapCreatePayload(snap_name, snap_namespace), on_finish);
}

template <typename I>
void ImageWatcher<I>::notify_snap_rename(const snapid_t &src_snap_id,
				         const std::string &dst_snap_name,
                                      Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

  notify_lock_owner(SnapRenamePayload(src_snap_id, dst_snap_name), on_finish);
=======
int ImageWatcher::unlock()
{
  assert(m_image_ctx.owner_lock.is_wlocked());
  if (m_lock_owner_state == LOCK_OWNER_STATE_NOT_LOCKED) {
    return 0;
  }

  ldout(m_image_ctx.cct, 10) << this << " releasing exclusive lock" << dendl;
  m_lock_owner_state = LOCK_OWNER_STATE_NOT_LOCKED;
  int r = rados::cls::lock::unlock(&m_image_ctx.md_ctx, m_image_ctx.header_oid,
				   RBD_LOCK_NAME, encode_lock_cookie());
  if (r < 0 && r != -ENOENT) {
    lderr(m_image_ctx.cct) << this << " failed to release exclusive lock: "
			   << cpp_strerror(r) << dendl;
    return r;
  }

  if (m_image_ctx.object_map.enabled()) {
    m_image_ctx.object_map.unlock();
  }

  Mutex::Locker l(m_owner_client_id_lock);
  set_owner_client_id(ClientId());

  FunctionContext *ctx = new FunctionContext(
    boost::bind(&ImageWatcher::notify_released_lock, this));
  m_task_finisher->queue(TASK_CODE_RELEASED_LOCK, ctx);
  return 0;
}

bool ImageWatcher::release_lock()
{
  assert(m_image_ctx.owner_lock.is_wlocked());
  ldout(m_image_ctx.cct, 10) << this << " releasing exclusive lock by request"
                             << dendl;
  if (!is_lock_owner()) {
    return false;
  }
  prepare_unlock();
  m_image_ctx.owner_lock.put_write();
  m_image_ctx.cancel_async_requests();
  m_image_ctx.flush_async_operations();

  {
    RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
    RWLock::WLocker md_locker(m_image_ctx.md_lock);
    m_image_ctx.flush();
  }

  m_image_ctx.owner_lock.get_write();
  if (!is_lock_owner()) {
    return false;
  }

  unlock();
  return true;
>>>>>>> upstream/hammer
}

template <typename I>
void ImageWatcher<I>::notify_snap_remove(const std::string &snap_name,
                                         Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

  notify_lock_owner(SnapRemovePayload(snap_name), on_finish);
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::notify_snap_protect(const std::string &snap_name,
                                          Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());
=======
int ImageWatcher::notify_async_progress(const AsyncRequestId &request,
					uint64_t offset, uint64_t total) {
  ldout(m_image_ctx.cct, 20) << this << " remote async request progress: "
			     << request << " @ " << offset
			     << "/" << total << dendl;

  bufferlist bl;
  ::encode(NotifyMessage(AsyncProgressPayload(request, offset, total)), bl);

  m_image_ctx.md_ctx.notify2(m_image_ctx.header_oid, bl, NOTIFY_TIMEOUT, NULL);
  return 0;
}
>>>>>>> upstream/hammer

  notify_lock_owner(SnapProtectPayload(snap_name), on_finish);
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::notify_snap_unprotect(const std::string &snap_name,
                                            Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

  notify_lock_owner(SnapUnprotectPayload(snap_name), on_finish);
=======
int ImageWatcher::notify_async_complete(const AsyncRequestId &request,
					int r) {
  ldout(m_image_ctx.cct, 20) << this << " remote async request finished: "
			     << request << " = " << r << dendl;

  bufferlist bl;
  ::encode(NotifyMessage(AsyncCompletePayload(request, r)), bl);

  librbd::notify_change(m_image_ctx.md_ctx, m_image_ctx.header_oid,
			&m_image_ctx);
  int ret = m_image_ctx.md_ctx.notify2(m_image_ctx.header_oid, bl,
				       NOTIFY_TIMEOUT, NULL);
  if (ret < 0) {
    lderr(m_image_ctx.cct) << this << " failed to notify async complete: "
			   << cpp_strerror(ret) << dendl;
    if (ret == -ETIMEDOUT) {
      schedule_async_complete(request, r);
    }
  } else {
    RWLock::WLocker l(m_async_request_lock);
    m_async_pending.erase(request);
  }
  return 0;
>>>>>>> upstream/hammer
}

template <typename I>
void ImageWatcher<I>::notify_rebuild_object_map(uint64_t request_id,
                                                ProgressContext &prog_ctx,
                                                Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

  AsyncRequestId async_request_id(get_client_id(), request_id);

  notify_async_request(async_request_id,
                       RebuildObjectMapPayload(async_request_id),
                       prog_ctx, on_finish);
}

template <typename I>
void ImageWatcher<I>::notify_rename(const std::string &image_name,
                                    Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

  notify_lock_owner(RenamePayload(image_name), on_finish);
}

template <typename I>
void ImageWatcher<I>::notify_update_features(uint64_t features, bool enabled,
                                             Context *on_finish) {
  assert(m_image_ctx.owner_lock.is_locked());
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

  notify_lock_owner(UpdateFeaturesPayload(features, enabled), on_finish);
}

template <typename I>
void ImageWatcher<I>::notify_header_update(Context *on_finish) {
  ldout(m_image_ctx.cct, 10) << this << ": " << __func__ << dendl;

  // supports legacy (empty buffer) clients
  send_notify(HeaderUpdatePayload(), on_finish);
}

template <typename I>
void ImageWatcher<I>::notify_header_update(librados::IoCtx &io_ctx,
				           const std::string &oid) {
  // supports legacy (empty buffer) clients
  bufferlist bl;
  ::encode(NotifyMessage(HeaderUpdatePayload()), bl);
<<<<<<< HEAD
  io_ctx.notify2(oid, bl, watcher::Notifier::NOTIFY_TIMEOUT, nullptr);
=======

  io_ctx.notify2(oid, bl, NOTIFY_TIMEOUT, NULL);
}

std::string ImageWatcher::encode_lock_cookie() const {
  RWLock::RLocker l(m_watch_lock);
  std::ostringstream ss;
  ss << WATCHER_LOCK_COOKIE_PREFIX << " " << m_watch_handle;
  return ss.str();
}

bool ImageWatcher::decode_lock_cookie(const std::string &tag,
				      uint64_t *handle) {
  std::string prefix;
  std::istringstream ss(tag);
  if (!(ss >> prefix >> *handle) || prefix != WATCHER_LOCK_COOKIE_PREFIX) {
    return false;
  }
  return true;
}

void ImageWatcher::schedule_retry_aio_requests(bool use_timer) {
  m_task_finisher->cancel(TASK_CODE_REQUEST_LOCK);
  Context *ctx = new FunctionContext(boost::bind(
    &ImageWatcher::retry_aio_requests, this));
  if (use_timer) {
    m_task_finisher->add_event_after(TASK_CODE_RETRY_AIO_REQUESTS,
                                     RETRY_DELAY_SECONDS, ctx);
  } else {
    m_task_finisher->queue(TASK_CODE_RETRY_AIO_REQUESTS, ctx);
  }
}

void ImageWatcher::retry_aio_requests() {
  m_task_finisher->cancel(TASK_CODE_RETRY_AIO_REQUESTS);
  std::vector<AioRequest> lock_request_restarts;
  {
    Mutex::Locker l(m_aio_request_lock);
    lock_request_restarts.swap(m_aio_requests);
  }

  ldout(m_image_ctx.cct, 15) << this << " retrying pending aio requests"
                             << dendl;
  for (std::vector<AioRequest>::iterator iter = lock_request_restarts.begin();
       iter != lock_request_restarts.end(); ++iter) {
    ldout(m_image_ctx.cct, 20) << this << " retrying aio request: "
                               << iter->second << dendl;
    iter->first(iter->second);
    iter->second->put();
  }
>>>>>>> upstream/hammer
}

template <typename I>
void ImageWatcher<I>::schedule_cancel_async_requests() {
  FunctionContext *ctx = new FunctionContext(
    boost::bind(&ImageWatcher<I>::cancel_async_requests, this));
  m_task_finisher->queue(TASK_CODE_CANCEL_ASYNC_REQUESTS, ctx);
}

template <typename I>
void ImageWatcher<I>::cancel_async_requests() {
  RWLock::WLocker l(m_async_request_lock);
  for (std::map<AsyncRequestId, AsyncRequest>::iterator iter =
	 m_async_requests.begin();
       iter != m_async_requests.end(); ++iter) {
    iter->second.first->complete(-ERESTART);
  }
  m_async_requests.clear();
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::set_owner_client_id(const ClientId& client_id) {
=======
void ImageWatcher::set_owner_client_id(const WatchNotify::ClientId& client_id) {
>>>>>>> upstream/hammer
  assert(m_owner_client_id_lock.is_locked());
  m_owner_client_id = client_id;
  ldout(m_image_ctx.cct, 10) << this << " current lock owner: "
                             << m_owner_client_id << dendl;
<<<<<<< HEAD
}

template <typename I>
ClientId ImageWatcher<I>::get_client_id() {
  RWLock::RLocker l(this->m_watch_lock);
  return ClientId(m_image_ctx.md_ctx.get_instance_id(), this->m_watch_handle);
}

template <typename I>
void ImageWatcher<I>::notify_acquired_lock() {
  ldout(m_image_ctx.cct, 10) << this << " notify acquired lock" << dendl;
=======
}

ClientId ImageWatcher::get_client_id() {
  RWLock::RLocker l(m_watch_lock);
  return ClientId(m_image_ctx.md_ctx.get_instance_id(), m_watch_handle);
}

void ImageWatcher::notify_release_lock() {
  RWLock::WLocker owner_locker(m_image_ctx.owner_lock);
  release_lock();
}

void ImageWatcher::notify_released_lock() {
  ldout(m_image_ctx.cct, 10) << this << " notify released lock" << dendl;
  bufferlist bl;
  ::encode(NotifyMessage(ReleasedLockPayload(get_client_id())), bl);
  m_image_ctx.md_ctx.notify2(m_image_ctx.header_oid, bl, NOTIFY_TIMEOUT, NULL);
}

void ImageWatcher::notify_request_lock() {
  ldout(m_image_ctx.cct, 10) << this << " notify request lock" << dendl;
  m_task_finisher->cancel(TASK_CODE_RETRY_AIO_REQUESTS);
>>>>>>> upstream/hammer

  ClientId client_id = get_client_id();
  {
    Mutex::Locker owner_client_id_locker(m_owner_client_id_lock);
    set_owner_client_id(client_id);
  }

  send_notify(AcquiredLockPayload(client_id));
}

template <typename I>
void ImageWatcher<I>::notify_released_lock() {
  ldout(m_image_ctx.cct, 10) << this << " notify released lock" << dendl;

<<<<<<< HEAD
  {
    Mutex::Locker owner_client_id_locker(m_owner_client_id_lock);
    set_owner_client_id(ClientId());
=======
  if (r == -ETIMEDOUT) {
    ldout(m_image_ctx.cct, 5) << this << "timed out requesting lock: retrying"
                              << dendl;
    retry_aio_requests();
  } else if (r < 0) {
    lderr(m_image_ctx.cct) << this << " error requesting lock: "
                           << cpp_strerror(r) << dendl;
    schedule_retry_aio_requests(true);
  } else {
    // lock owner acked -- but resend if we don't see them release the lock
    int retry_timeout = m_image_ctx.cct->_conf->client_notify_timeout;
    FunctionContext *ctx = new FunctionContext(
      boost::bind(&ImageWatcher::notify_request_lock, this));
    m_task_finisher->add_event_after(TASK_CODE_REQUEST_LOCK,
                                     retry_timeout, ctx);
>>>>>>> upstream/hammer
  }

  send_notify(ReleasedLockPayload(get_client_id()));
}

template <typename I>
void ImageWatcher<I>::schedule_request_lock(bool use_timer, int timer_delay) {
  assert(m_image_ctx.owner_lock.is_locked());

<<<<<<< HEAD
  if (m_image_ctx.exclusive_lock == nullptr) {
    // exclusive lock dynamically disabled via image refresh
    return;
=======
  // since we need to ack our own notifications, release the owner lock just in
  // case another notification occurs before this one and it requires the lock
  bufferlist response_bl;
  m_image_ctx.owner_lock.put_read();
  int r = m_image_ctx.md_ctx.notify2(m_image_ctx.header_oid, bl, NOTIFY_TIMEOUT,
				     &response_bl);
  m_image_ctx.owner_lock.get_read();
  if (r < 0 && r != -ETIMEDOUT) {
    lderr(m_image_ctx.cct) << this << " lock owner notification failed: "
			   << cpp_strerror(r) << dendl;
    return r;
>>>>>>> upstream/hammer
  }
  assert(m_image_ctx.exclusive_lock &&
         !m_image_ctx.exclusive_lock->is_lock_owner());

<<<<<<< HEAD
  RWLock::RLocker watch_locker(this->m_watch_lock);
  if (this->m_watch_state == Watcher::WATCH_STATE_REGISTERED) {
    ldout(m_image_ctx.cct, 15) << this << " requesting exclusive lock" << dendl;

    FunctionContext *ctx = new FunctionContext(
      boost::bind(&ImageWatcher<I>::notify_request_lock, this));
    if (use_timer) {
      if (timer_delay < 0) {
        timer_delay = RETRY_DELAY_SECONDS;
=======
  typedef std::map<std::pair<uint64_t, uint64_t>, bufferlist> responses_t;
  responses_t responses;
  if (response_bl.length() > 0) {
    try {
      bufferlist::iterator iter = response_bl.begin();
      ::decode(responses, iter);
    } catch (const buffer::error &err) {
      lderr(m_image_ctx.cct) << this << " failed to decode response" << dendl;
      return -EINVAL;
    }
  }

  bufferlist response;
  bool lock_owner_responded = false;
  for (responses_t::iterator i = responses.begin(); i != responses.end(); ++i) {
    if (i->second.length() > 0) {
      if (lock_owner_responded) {
	lderr(m_image_ctx.cct) << this << " duplicate lock owners detected"
                               << dendl;
	return -EIO;
>>>>>>> upstream/hammer
      }
      m_task_finisher->add_event_after(TASK_CODE_REQUEST_LOCK,
                                       timer_delay, ctx);
    } else {
      m_task_finisher->queue(TASK_CODE_REQUEST_LOCK, ctx);
    }
  }
}

template <typename I>
void ImageWatcher<I>::notify_request_lock() {
  RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
  RWLock::RLocker snap_locker(m_image_ctx.snap_lock);

<<<<<<< HEAD
  // ExclusiveLock state machine can be dynamically disabled or
  // race with task cancel
  if (m_image_ctx.exclusive_lock == nullptr ||
      m_image_ctx.exclusive_lock->is_lock_owner()) {
    return;
=======
  if (!lock_owner_responded) {
    lderr(m_image_ctx.cct) << this << " no lock owners detected" << dendl;
    return -ETIMEDOUT;
>>>>>>> upstream/hammer
  }

  ldout(m_image_ctx.cct, 10) << this << " notify request lock" << dendl;

  notify_lock_owner(RequestLockPayload(get_client_id(), false),
      create_context_callback<
        ImageWatcher, &ImageWatcher<I>::handle_request_lock>(this));
}

template <typename I>
void ImageWatcher<I>::handle_request_lock(int r) {
  RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
  RWLock::RLocker snap_locker(m_image_ctx.snap_lock);

  // ExclusiveLock state machine cannot transition -- but can be
  // dynamically disabled
  if (m_image_ctx.exclusive_lock == nullptr) {
    return;
  }

  if (r == -ETIMEDOUT) {
    ldout(m_image_ctx.cct, 5) << this << " timed out requesting lock: retrying"
                              << dendl;

    // treat this is a dead client -- so retest acquiring the lock
    m_image_ctx.exclusive_lock->handle_peer_notification(0);
  } else if (r == -EROFS) {
    ldout(m_image_ctx.cct, 5) << this << " peer will not release lock" << dendl;
    m_image_ctx.exclusive_lock->handle_peer_notification(r);
  } else if (r < 0) {
    lderr(m_image_ctx.cct) << this << " error requesting lock: "
                           << cpp_strerror(r) << dendl;
    schedule_request_lock(true);
  } else {
    // lock owner acked -- but resend if we don't see them release the lock
    int retry_timeout = m_image_ctx.cct->_conf->client_notify_timeout;
    ldout(m_image_ctx.cct, 15) << this << " will retry in " << retry_timeout
                               << " seconds" << dendl;
    schedule_request_lock(true, retry_timeout);
  }
}

template <typename I>
void ImageWatcher<I>::notify_lock_owner(const Payload& payload,
                                        Context *on_finish) {
  assert(on_finish != nullptr);
  assert(m_image_ctx.owner_lock.is_locked());

  bufferlist bl;
  ::encode(NotifyMessage(payload), bl);

  NotifyLockOwner *notify_lock_owner = NotifyLockOwner::create(
    m_image_ctx, this->m_notifier, std::move(bl), on_finish);
  notify_lock_owner->send();
}

template <typename I>
Context *ImageWatcher<I>::remove_async_request(const AsyncRequestId &id) {
  RWLock::WLocker async_request_locker(m_async_request_lock);
  auto it = m_async_requests.find(id);
  if (it != m_async_requests.end()) {
    Context *on_complete = it->second.first;
    m_async_requests.erase(it);
    return on_complete;
  }
  return nullptr;
}

template <typename I>
void ImageWatcher<I>::schedule_async_request_timed_out(const AsyncRequestId &id) {
  ldout(m_image_ctx.cct, 20) << "scheduling async request time out: " << id
                             << dendl;

  Context *ctx = new FunctionContext(boost::bind(
    &ImageWatcher<I>::async_request_timed_out, this, id));

  Task task(TASK_CODE_ASYNC_REQUEST, id);
  m_task_finisher->cancel(task);

  m_task_finisher->add_event_after(task, m_image_ctx.request_timed_out_seconds,
                                   ctx);
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::async_request_timed_out(const AsyncRequestId &id) {
  Context *on_complete = remove_async_request(id);
  if (on_complete != nullptr) {
    ldout(m_image_ctx.cct, 5) << "async request timed out: " << id << dendl;
    m_image_ctx.op_work_queue->queue(on_complete, -ETIMEDOUT);
=======
void ImageWatcher::async_request_timed_out(const AsyncRequestId &id) {
  RWLock::RLocker l(m_async_request_lock);
  std::map<AsyncRequestId, AsyncRequest>::iterator it =
    m_async_requests.find(id);
  if (it != m_async_requests.end()) {
    ldout(m_image_ctx.cct, 10) << this << " request timed-out: " << id << dendl;
    it->second.first->complete(-ERESTART);
>>>>>>> upstream/hammer
  }
}

template <typename I>
void ImageWatcher<I>::notify_async_request(const AsyncRequestId &async_request_id,
                                           const Payload& payload,
                                           ProgressContext& prog_ctx,
                                           Context *on_finish) {
  assert(on_finish != nullptr);
  assert(m_image_ctx.owner_lock.is_locked());

  ldout(m_image_ctx.cct, 10) << this << " async request: " << async_request_id
                             << dendl;

  Context *on_notify = new FunctionContext([this, async_request_id](int r) {
    if (r < 0) {
      // notification failed -- don't expect updates
      Context *on_complete = remove_async_request(async_request_id);
      if (on_complete != nullptr) {
        on_complete->complete(r);
      }
    }
  });

  Context *on_complete = new FunctionContext(
    [this, async_request_id, on_finish](int r) {
      m_task_finisher->cancel(Task(TASK_CODE_ASYNC_REQUEST, async_request_id));
      on_finish->complete(r);
    });

  {
    RWLock::WLocker async_request_locker(m_async_request_lock);
    m_async_requests[async_request_id] = AsyncRequest(on_complete, &prog_ctx);
  }

  schedule_async_request_timed_out(async_request_id);
  notify_lock_owner(payload, on_notify);
}

template <typename I>
int ImageWatcher<I>::prepare_async_request(const AsyncRequestId& async_request_id,
                                           bool* new_request, Context** ctx,
                                           ProgressContext** prog_ctx) {
  if (async_request_id.client_id == get_client_id()) {
    return -ERESTART;
  } else {
    RWLock::WLocker l(m_async_request_lock);
    if (m_async_pending.count(async_request_id) == 0) {
      m_async_pending.insert(async_request_id);
      *new_request = true;
      *prog_ctx = new RemoteProgressContext(*this, async_request_id);
      *ctx = new RemoteContext(*this, async_request_id, *prog_ctx);
    } else {
      *new_request = false;
    }
  }
  return 0;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const HeaderUpdatePayload &payload,
			             C_NotifyAck *ack_ctx) {
=======
bool ImageWatcher::handle_payload(const HeaderUpdatePayload &payload,
				  C_NotifyAck *ack_ctx) {
>>>>>>> upstream/hammer
  ldout(m_image_ctx.cct, 10) << this << " image header updated" << dendl;

  m_image_ctx.state->handle_update_notification();
  m_image_ctx.perfcounter->inc(l_librbd_notify);
<<<<<<< HEAD
  if (ack_ctx != nullptr) {
    m_image_ctx.state->flush_update_watchers(new C_ResponseMessage(ack_ctx));
    return false;
  }
  return true;
}

template <typename I>
bool ImageWatcher<I>::handle_payload(const AcquiredLockPayload &payload,
                                     C_NotifyAck *ack_ctx) {
  ldout(m_image_ctx.cct, 10) << this << " image exclusively locked announcement"
                             << dendl;

  bool cancel_async_requests = true;
=======
  return true;
}

bool ImageWatcher::handle_payload(const AcquiredLockPayload &payload,
                                  C_NotifyAck *ack_ctx) {
  ldout(m_image_ctx.cct, 10) << this << " image exclusively locked announcement"
                             << dendl;
>>>>>>> upstream/hammer
  if (payload.client_id.is_valid()) {
    Mutex::Locker owner_client_id_locker(m_owner_client_id_lock);
    if (payload.client_id == m_owner_client_id) {
<<<<<<< HEAD
      cancel_async_requests = false;
=======
      // we already know that the remote client is the owner
      return true;
>>>>>>> upstream/hammer
    }
    set_owner_client_id(payload.client_id);
  }

  RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    // potentially wake up the exclusive lock state machine now that
    // a lock owner has advertised itself
    m_image_ctx.exclusive_lock->handle_peer_notification(0);
  }
  if (cancel_async_requests &&
      (m_image_ctx.exclusive_lock == nullptr ||
       !m_image_ctx.exclusive_lock->is_lock_owner())) {
    schedule_cancel_async_requests();
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const ReleasedLockPayload &payload,
                                     C_NotifyAck *ack_ctx) {
  ldout(m_image_ctx.cct, 10) << this << " exclusive lock released" << dendl;

  bool cancel_async_requests = true;
=======
bool ImageWatcher::handle_payload(const ReleasedLockPayload &payload,
                                  C_NotifyAck *ack_ctx) {
  ldout(m_image_ctx.cct, 10) << this << " exclusive lock released" << dendl;
>>>>>>> upstream/hammer
  if (payload.client_id.is_valid()) {
    Mutex::Locker l(m_owner_client_id_lock);
    if (payload.client_id != m_owner_client_id) {
      ldout(m_image_ctx.cct, 10) << this << " unexpected owner: "
                                 << payload.client_id << " != "
                                 << m_owner_client_id << dendl;
<<<<<<< HEAD
      cancel_async_requests = false;
    } else {
      set_owner_client_id(ClientId());
    }
=======
      return true;
    }
    set_owner_client_id(ClientId());
>>>>>>> upstream/hammer
  }

  RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
  if (cancel_async_requests &&
      (m_image_ctx.exclusive_lock == nullptr ||
       !m_image_ctx.exclusive_lock->is_lock_owner())) {
    schedule_cancel_async_requests();
  }
<<<<<<< HEAD

  // alert the exclusive lock state machine that the lock is available
  if (m_image_ctx.exclusive_lock != nullptr &&
      !m_image_ctx.exclusive_lock->is_lock_owner()) {
    m_task_finisher->cancel(TASK_CODE_REQUEST_LOCK);
    m_image_ctx.exclusive_lock->handle_peer_notification(0);
  }
  return true;
}

template <typename I>
bool ImageWatcher<I>::handle_payload(const RequestLockPayload &payload,
                                     C_NotifyAck *ack_ctx) {
=======
  return true;
}

bool ImageWatcher::handle_payload(const RequestLockPayload &payload,
                                  C_NotifyAck *ack_ctx) {
>>>>>>> upstream/hammer
  ldout(m_image_ctx.cct, 10) << this << " exclusive lock requested" << dendl;
  if (payload.client_id == get_client_id()) {
    return true;
  }

  RWLock::RLocker l(m_image_ctx.owner_lock);
<<<<<<< HEAD
  if (m_image_ctx.exclusive_lock != nullptr &&
      m_image_ctx.exclusive_lock->is_lock_owner()) {
    int r = 0;
    bool accept_request = m_image_ctx.exclusive_lock->accept_requests(&r);
=======
  if (m_lock_owner_state == LOCK_OWNER_STATE_LOCKED) {
    // need to send something back so the client can detect a missing leader
    ::encode(ResponseMessage(0), ack_ctx->out);
>>>>>>> upstream/hammer

    if (accept_request) {
      assert(r == 0);
      Mutex::Locker owner_client_id_locker(m_owner_client_id_lock);
      if (!m_owner_client_id.is_valid()) {
<<<<<<< HEAD
        return true;
      }

      ldout(m_image_ctx.cct, 10) << this << " queuing release of exclusive lock"
                                 << dendl;
      r = m_image_ctx.get_exclusive_lock_policy()->lock_requested(
        payload.force);
    }
    ::encode(ResponseMessage(r), ack_ctx->out);
=======
	return true;
      }
    }

    ldout(m_image_ctx.cct, 10) << this << " queuing release of exclusive lock"
                               << dendl;
    FunctionContext *ctx = new FunctionContext(
      boost::bind(&ImageWatcher::notify_release_lock, this));
    m_task_finisher->queue(TASK_CODE_RELEASING_LOCK, ctx);
>>>>>>> upstream/hammer
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const AsyncProgressPayload &payload,
                                     C_NotifyAck *ack_ctx) {
=======
bool ImageWatcher::handle_payload(const AsyncProgressPayload &payload,
                                  C_NotifyAck *ack_ctx) {
>>>>>>> upstream/hammer
  RWLock::RLocker l(m_async_request_lock);
  std::map<AsyncRequestId, AsyncRequest>::iterator req_it =
    m_async_requests.find(payload.async_request_id);
  if (req_it != m_async_requests.end()) {
    ldout(m_image_ctx.cct, 20) << this << " request progress: "
			       << payload.async_request_id << " @ "
			       << payload.offset << "/" << payload.total
			       << dendl;
    schedule_async_request_timed_out(payload.async_request_id);
    req_it->second.second->update_progress(payload.offset, payload.total);
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const AsyncCompletePayload &payload,
                                     C_NotifyAck *ack_ctx) {
  Context *on_complete = remove_async_request(payload.async_request_id);
  if (on_complete != nullptr) {
=======
bool ImageWatcher::handle_payload(const AsyncCompletePayload &payload,
                                  C_NotifyAck *ack_ctx) {
  RWLock::RLocker l(m_async_request_lock);
  std::map<AsyncRequestId, AsyncRequest>::iterator req_it =
    m_async_requests.find(payload.async_request_id);
  if (req_it != m_async_requests.end()) {
>>>>>>> upstream/hammer
    ldout(m_image_ctx.cct, 10) << this << " request finished: "
                               << payload.async_request_id << "="
			       << payload.result << dendl;
    on_complete->complete(payload.result);
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const FlattenPayload &payload,
				     C_NotifyAck *ack_ctx) {
=======
bool ImageWatcher::handle_payload(const FlattenPayload &payload,
				  C_NotifyAck *ack_ctx) {
>>>>>>> upstream/hammer

  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      bool new_request;
      Context *ctx;
      ProgressContext *prog_ctx;
      r = prepare_async_request(payload.async_request_id, &new_request,
                                &ctx, &prog_ctx);
      if (new_request) {
        ldout(m_image_ctx.cct, 10) << this << " remote flatten request: "
				   << payload.async_request_id << dendl;
        m_image_ctx.operations->execute_flatten(*prog_ctx, ctx);
      }

      ::encode(ResponseMessage(r), ack_ctx->out);
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const ResizePayload &payload,
				     C_NotifyAck *ack_ctx) {
  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      bool new_request;
      Context *ctx;
      ProgressContext *prog_ctx;
      r = prepare_async_request(payload.async_request_id, &new_request,
                                &ctx, &prog_ctx);
      if (new_request) {
        ldout(m_image_ctx.cct, 10) << this << " remote resize request: "
				   << payload.async_request_id << " "
				   << payload.size << " "
				   << payload.allow_shrink << dendl;
        m_image_ctx.operations->execute_resize(payload.size, payload.allow_shrink, *prog_ctx, ctx, 0);
=======
    if (new_request) {
      RemoteProgressContext *prog_ctx =
	new RemoteProgressContext(*this, payload.async_request_id);
      RemoteContext *ctx = new RemoteContext(*this, payload.async_request_id,
					     prog_ctx);

      ldout(m_image_ctx.cct, 10) << this << " remote flatten request: "
				 << payload.async_request_id << dendl;
      r = librbd::async_flatten(&m_image_ctx, ctx, *prog_ctx);
      if (r < 0) {
	delete ctx;
	lderr(m_image_ctx.cct) << this << " remove flatten request failed: "
			       << cpp_strerror(r) << dendl;

	RWLock::WLocker l(m_async_request_lock);
	m_async_pending.erase(payload.async_request_id);
>>>>>>> upstream/hammer
      }

      ::encode(ResponseMessage(r), ack_ctx->out);
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const SnapCreatePayload &payload,
			             C_NotifyAck *ack_ctx) {
  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      ldout(m_image_ctx.cct, 10) << this << " remote snap_create request: "
			         << payload.snap_name << dendl;

      m_image_ctx.operations->execute_snap_create(payload.snap_name,
						  payload.snap_namespace,
                                                  new C_ResponseMessage(ack_ctx),
                                                  0, false);
      return false;
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
=======
    ::encode(ResponseMessage(r), ack_ctx->out);
>>>>>>> upstream/hammer
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const SnapRenamePayload &payload,
			             C_NotifyAck *ack_ctx) {
=======
bool ImageWatcher::handle_payload(const ResizePayload &payload,
				  C_NotifyAck *ack_ctx) {
>>>>>>> upstream/hammer
  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      ldout(m_image_ctx.cct, 10) << this << " remote snap_rename request: "
			         << payload.snap_id << " to "
			         << payload.snap_name << dendl;

      m_image_ctx.operations->execute_snap_rename(payload.snap_id,
                                                  payload.snap_name,
                                                  new C_ResponseMessage(ack_ctx));
      return false;
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const SnapRemovePayload &payload,
			             C_NotifyAck *ack_ctx) {
  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      ldout(m_image_ctx.cct, 10) << this << " remote snap_remove request: "
			         << payload.snap_name << dendl;

      m_image_ctx.operations->execute_snap_remove(payload.snap_name,
                                                  new C_ResponseMessage(ack_ctx));
      return false;
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
  }
  return true;
}

template <typename I>
bool ImageWatcher<I>::handle_payload(const SnapProtectPayload& payload,
                                     C_NotifyAck *ack_ctx) {
  RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      ldout(m_image_ctx.cct, 10) << this << " remote snap_protect request: "
                                 << payload.snap_name << dendl;

      m_image_ctx.operations->execute_snap_protect(payload.snap_name,
                                                   new C_ResponseMessage(ack_ctx));
      return false;
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
=======
    if (new_request) {
      RemoteProgressContext *prog_ctx =
	new RemoteProgressContext(*this, payload.async_request_id);
      RemoteContext *ctx = new RemoteContext(*this, payload.async_request_id,
					     prog_ctx);

      ldout(m_image_ctx.cct, 10) << this << " remote resize request: "
				 << payload.async_request_id << " "
				 << payload.size << dendl;
      r = librbd::async_resize(&m_image_ctx, ctx, payload.size, *prog_ctx);
      if (r < 0) {
	lderr(m_image_ctx.cct) << this << " remove resize request failed: "
			       << cpp_strerror(r) << dendl;
	delete ctx;

	RWLock::WLocker l(m_async_request_lock);
	m_async_pending.erase(payload.async_request_id);
      }
>>>>>>> upstream/hammer
    }
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const SnapUnprotectPayload& payload,
                                     C_NotifyAck *ack_ctx) {
  RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      ldout(m_image_ctx.cct, 10) << this << " remote snap_unprotect request: "
                                 << payload.snap_name << dendl;

      m_image_ctx.operations->execute_snap_unprotect(payload.snap_name,
                                                     new C_ResponseMessage(ack_ctx));
      return false;
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
=======
    ::encode(ResponseMessage(r), ack_ctx->out);
>>>>>>> upstream/hammer
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const RebuildObjectMapPayload& payload,
                                     C_NotifyAck *ack_ctx) {
  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      bool new_request;
      Context *ctx;
      ProgressContext *prog_ctx;
      r = prepare_async_request(payload.async_request_id, &new_request,
                                &ctx, &prog_ctx);
      if (new_request) {
        ldout(m_image_ctx.cct, 10) << this
                                   << " remote rebuild object map request: "
                                   << payload.async_request_id << dendl;
        m_image_ctx.operations->execute_rebuild_object_map(*prog_ctx, ctx);
      }

      ::encode(ResponseMessage(r), ack_ctx->out);
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
  }
  return true;
}

template <typename I>
bool ImageWatcher<I>::handle_payload(const RenamePayload& payload,
                                     C_NotifyAck *ack_ctx) {
  RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      ldout(m_image_ctx.cct, 10) << this << " remote rename request: "
                                 << payload.image_name << dendl;

      m_image_ctx.operations->execute_rename(payload.image_name,
                                             new C_ResponseMessage(ack_ctx));
      return false;
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
  }
  return true;
}

template <typename I>
bool ImageWatcher<I>::handle_payload(const UpdateFeaturesPayload& payload,
                                     C_NotifyAck *ack_ctx) {
  RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r)) {
      ldout(m_image_ctx.cct, 10) << this << " remote update_features request: "
                                 << payload.features << " "
                                 << (payload.enabled ? "enabled" : "disabled")
                                 << dendl;

      m_image_ctx.operations->execute_update_features(
        payload.features, payload.enabled, new C_ResponseMessage(ack_ctx), 0);
      return false;
    } else if (r < 0) {
      ::encode(ResponseMessage(r), ack_ctx->out);
    }
=======
bool ImageWatcher::handle_payload(const SnapCreatePayload &payload,
				  C_NotifyAck *ack_ctx) {
  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (m_lock_owner_state == LOCK_OWNER_STATE_LOCKED) {
    ldout(m_image_ctx.cct, 10) << this << " remote snap_create request: "
			       << payload.snap_name << dendl;

    // execute outside of librados AIO thread
    m_image_ctx.op_work_queue->queue(new C_SnapCreateResponseMessage(
      this, ack_ctx, payload.snap_name), 0);
    return false;
>>>>>>> upstream/hammer
  }
  return true;
}

<<<<<<< HEAD
template <typename I>
bool ImageWatcher<I>::handle_payload(const UnknownPayload &payload,
			             C_NotifyAck *ack_ctx) {
  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (m_image_ctx.exclusive_lock != nullptr) {
    int r;
    if (m_image_ctx.exclusive_lock->accept_requests(&r) || r < 0) {
      ::encode(ResponseMessage(-EOPNOTSUPP), ack_ctx->out);
    }
  }
  return true;
}

template <typename I>
void ImageWatcher<I>::process_payload(uint64_t notify_id, uint64_t handle,
                                      const Payload &payload, int r) {
  if (r < 0) {
    bufferlist out_bl;
    this->acknowledge_notify(notify_id, handle, out_bl);
  } else {
    apply_visitor(HandlePayloadVisitor<ImageWatcher<I>>(this, notify_id,
                                                       handle),
                  payload);
=======
bool ImageWatcher::handle_payload(const UnknownPayload &payload,
				  C_NotifyAck *ack_ctx) {
  RWLock::RLocker l(m_image_ctx.owner_lock);
  if (is_lock_owner()) {
    ::encode(ResponseMessage(-EOPNOTSUPP), ack_ctx->out);
>>>>>>> upstream/hammer
  }
  return true;
}

template <typename I>
void ImageWatcher<I>::handle_notify(uint64_t notify_id, uint64_t handle,
			            uint64_t notifier_id, bufferlist &bl) {
  NotifyMessage notify_message;
  if (bl.length() == 0) {
    // legacy notification for header updates
    notify_message = NotifyMessage(HeaderUpdatePayload());
  } else {
    try {
      bufferlist::iterator iter = bl.begin();
      ::decode(notify_message, iter);
    } catch (const buffer::error &err) {
      lderr(m_image_ctx.cct) << this << " error decoding image notification: "
			     << err.what() << dendl;
      return;
    }
  }

<<<<<<< HEAD
  // if an image refresh is required, refresh before processing the request
  if (notify_message.check_for_refresh() &&
      m_image_ctx.state->is_refresh_required()) {
    m_image_ctx.state->refresh(new C_ProcessPayload(this, notify_id, handle,
                                                    notify_message.payload));
  } else {
    process_payload(notify_id, handle, notify_message.payload, 0);
  }
}

template <typename I>
void ImageWatcher<I>::handle_error(uint64_t handle, int err) {
=======
  apply_visitor(HandlePayloadVisitor(this, notify_id, handle),
		notify_message.payload);
}

void ImageWatcher::handle_error(uint64_t handle, int err) {
>>>>>>> upstream/hammer
  lderr(m_image_ctx.cct) << this << " image watch failed: " << handle << ", "
                         << cpp_strerror(err) << dendl;

  {
    Mutex::Locker l(m_owner_client_id_lock);
    set_owner_client_id(ClientId());
  }

  Watcher::handle_error(handle, err);
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::handle_rewatch_complete(int r) {
  CephContext *cct = m_image_ctx.cct;
  ldout(cct, 10) << this << " " << __func__ << ": r=" << r << dendl;

  {
    RWLock::RLocker owner_locker(m_image_ctx.owner_lock);
    if (m_image_ctx.exclusive_lock != nullptr) {
      // update the lock cookie with the new watch handle
      m_image_ctx.exclusive_lock->reacquire_lock();
=======
void ImageWatcher::acknowledge_notify(uint64_t notify_id, uint64_t handle,
				      bufferlist &out) {
  m_image_ctx.md_ctx.notify_ack(m_image_ctx.header_oid, notify_id, handle, out);
}

void ImageWatcher::reregister_watch() {
  ldout(m_image_ctx.cct, 10) << this << " re-registering image watch" << dendl;

  {
    RWLock::WLocker l(m_image_ctx.owner_lock);
    bool was_lock_owner = false;
    if (m_lock_owner_state == LOCK_OWNER_STATE_LOCKED) {
      // ensure all async requests are canceled and IO is flushed
      was_lock_owner = release_lock();
    }

    int r;
    {
      RWLock::WLocker l(m_watch_lock);
      if (m_watch_state != WATCH_STATE_ERROR) {
	return;
      }

      r = m_image_ctx.md_ctx.watch2(m_image_ctx.header_oid,
                                    &m_watch_handle, &m_watch_ctx);
      if (r < 0) {
        lderr(m_image_ctx.cct) << this << " failed to re-register image watch: "
                               << cpp_strerror(r) << dendl;
	if (r != -ESHUTDOWN) {
	  FunctionContext *ctx = new FunctionContext(boost::bind(
	    &ImageWatcher::reregister_watch, this));
	  m_task_finisher->add_event_after(TASK_CODE_REREGISTER_WATCH,
                                           RETRY_DELAY_SECONDS, ctx);
	}
        return;
      }

      m_watch_state = WATCH_STATE_REGISTERED;
    }
    handle_payload(HeaderUpdatePayload(), NULL);

    if (was_lock_owner) {
      r = try_lock();
      if (r == -EBUSY) {
        ldout(m_image_ctx.cct, 5) << this << "lost image lock while "
                                  << "re-registering image watch" << dendl;
      } else if (r < 0) {
        lderr(m_image_ctx.cct) << this
                               << "failed to lock image while re-registering "
                               << "image watch" << cpp_strerror(r) << dendl;
      }
>>>>>>> upstream/hammer
    }
  }

  // image might have been updated while we didn't have active watch
  handle_payload(HeaderUpdatePayload(), nullptr);
}

template <typename I>
void ImageWatcher<I>::send_notify(const Payload &payload, Context *ctx) {
  bufferlist bl;

  ::encode(NotifyMessage(payload), bl);
  Watcher::send_notify(bl, nullptr, ctx);
}

template <typename I>
void ImageWatcher<I>::RemoteContext::finish(int r) {
  m_image_watcher.schedule_async_complete(m_async_request_id, r);
}

<<<<<<< HEAD
template <typename I>
void ImageWatcher<I>::C_ResponseMessage::finish(int r) {
  CephContext *cct = notify_ack->cct;
  ldout(cct, 10) << this << " C_ResponseMessage: r=" << r << dendl;

  ::encode(ResponseMessage(r), notify_ack->out);
  notify_ack->complete(0);
}

} // namespace librbd

template class librbd::ImageWatcher<librbd::ImageCtx>;
=======
ImageWatcher::C_NotifyAck::C_NotifyAck(ImageWatcher *image_watcher,
                                       uint64_t notify_id, uint64_t handle)
  : image_watcher(image_watcher), notify_id(notify_id), handle(handle) {
  CephContext *cct = image_watcher->m_image_ctx.cct;
  ldout(cct, 10) << this << " C_NotifyAck start: id=" << notify_id << ", "
                 << "handle=" << handle << dendl;
}

void ImageWatcher::C_NotifyAck::finish(int r) {
  assert(r == 0);
  CephContext *cct = image_watcher->m_image_ctx.cct;
  ldout(cct, 10) << this << " C_NotifyAck finish: id=" << notify_id << ", "
                 << "handle=" << handle << dendl;

  image_watcher->acknowledge_notify(notify_id, handle, out);
}

void ImageWatcher::C_SnapCreateResponseMessage::finish(int r) {
  CephContext *cct = notify_ack->image_watcher->m_image_ctx.cct;
  ldout(cct, 10) << this << " C_SnapCreateResponseMessage: r=" << r << dendl;

  RWLock::RLocker owner_locker(image_watcher->m_image_ctx.owner_lock);
  if (image_watcher->m_lock_owner_state == LOCK_OWNER_STATE_LOCKED) {
    r = librbd::snap_create_helper(&image_watcher->m_image_ctx, NULL,
                                   snap_name.c_str());
    ::encode(ResponseMessage(r), notify_ack->out);
  }
  notify_ack->complete(0);
}

} // namespace librbd
>>>>>>> upstream/hammer
