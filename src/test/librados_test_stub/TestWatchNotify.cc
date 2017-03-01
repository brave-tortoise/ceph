// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include "test/librados_test_stub/TestWatchNotify.h"
#include "include/Context.h"
#include "common/Finisher.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>

#define dout_subsys ceph_subsys_rados
#undef dout_prefix
#define dout_prefix *_dout << "TestWatchNotify::" << __func__ << ": "

namespace librados {

<<<<<<< HEAD
std::ostream& operator<<(std::ostream& out,
			 const TestWatchNotify::WatcherID &watcher_id) {
  out << "(" << watcher_id.first << "," << watcher_id.second << ")";
  return out;
}

TestWatchNotify::TestWatchNotify(CephContext *cct, Finisher *finisher)
  : m_cct(cct), m_finisher(finisher), m_handle(), m_notify_id(),
    m_lock("librados::TestWatchNotify::m_lock"),
    m_pending_notifies(0) {
  m_cct->get();
}

TestWatchNotify::~TestWatchNotify() {
  m_cct->put();
=======
TestWatchNotify::TestWatchNotify(CephContext *cct, Finisher *finisher)
  : m_cct(cct), m_finisher(finisher), m_handle(), m_notify_id(),
    m_file_watcher_lock("librados::TestWatchNotify::m_file_watcher_lock"),
    m_pending_notifies(0) {
  m_cct->get();
}

TestWatchNotify::~TestWatchNotify() {
  m_cct->put();
}

TestWatchNotify::NotifyHandle::NotifyHandle()
  : lock("TestWatchNotify::NotifyHandle::lock") {
>>>>>>> upstream/hammer
}

void TestWatchNotify::flush() {
  ldout(m_cct, 20) << "enter" << dendl;
  // block until we know no additional async notify callbacks will occur
  Mutex::Locker locker(m_lock);
  while (m_pending_notifies > 0) {
    m_file_watcher_cond.Wait(m_lock);
  }
}

void TestWatchNotify::flush() {
  Mutex::Locker file_watcher_locker(m_file_watcher_lock);
  while (m_pending_notifies > 0) {
    m_file_watcher_cond.Wait(m_file_watcher_lock);
  }
}

int TestWatchNotify::list_watchers(const std::string& o,
                                   std::list<obj_watch_t> *out_watchers) {
  Mutex::Locker lock(m_lock);
  SharedWatcher watcher = get_watcher(o);

  out_watchers->clear();
  for (TestWatchNotify::WatchHandles::iterator it =
         watcher->watch_handles.begin();
       it != watcher->watch_handles.end(); ++it) {
    obj_watch_t obj;
<<<<<<< HEAD
    strcpy(obj.addr, "-");
=======
    strcpy(obj.addr, ":/0");
>>>>>>> upstream/hammer
    obj.watcher_id = static_cast<int64_t>(it->second.gid);
    obj.cookie = it->second.handle;
    obj.timeout_seconds = 30;
    out_watchers->push_back(obj);
  }
  return 0;
}

<<<<<<< HEAD
void TestWatchNotify::aio_flush(Context *on_finish) {
  m_finisher->queue(on_finish);
}

void TestWatchNotify::aio_watch(const std::string& o, uint64_t gid,
                                uint64_t *handle,
                                librados::WatchCtx2 *watch_ctx,
                                Context *on_finish) {
  int r = watch(o, gid, handle, nullptr, watch_ctx);
  m_finisher->queue(on_finish, r);
=======
int TestWatchNotify::notify(const std::string& oid, bufferlist& bl,
                            uint64_t timeout_ms, bufferlist *pbl) {
  uint64_t notify_id;
  {
    Mutex::Locker file_watcher_locker(m_file_watcher_lock);
    ++m_pending_notifies;
    notify_id = ++m_notify_id;
  }

  SharedNotifyHandle notify_handle(new NotifyHandle());
  {
    SharedWatcher watcher = get_watcher(oid);
    RWLock::WLocker watcher_locker(watcher->lock);

    WatchHandles watch_handles = watcher->watch_handles;
    for (WatchHandles::iterator w_it = watch_handles.begin();
         w_it != watch_handles.end(); ++w_it) {
      WatchHandle &watch_handle = w_it->second;

      Mutex::Locker notify_handle_locker(notify_handle->lock);
      notify_handle->pending_watcher_ids.insert(std::make_pair(
        watch_handle.gid, watch_handle.handle));
    }

    watcher->notify_handles[notify_id] = notify_handle;

    FunctionContext *ctx = new FunctionContext(
      boost::bind(&TestWatchNotify::execute_notify, this, oid, bl, notify_id));
    m_finisher->queue(ctx);
  }

  {
    utime_t timeout;
    timeout.set_from_double(ceph_clock_now(m_cct) + (timeout_ms / 1000.0));

    Mutex::Locker notify_locker(notify_handle->lock);
    while (!notify_handle->pending_watcher_ids.empty()) {
      notify_handle->cond.WaitUntil(notify_handle->lock, timeout);
    }

    if (pbl != NULL) {
      ::encode(notify_handle->notify_responses, *pbl);
      ::encode(notify_handle->pending_watcher_ids, *pbl);
    }
  }

  SharedWatcher watcher = get_watcher(oid);
  Mutex::Locker file_watcher_locker(m_file_watcher_lock);
  {
    RWLock::WLocker watcher_locker(watcher->lock);

    watcher->notify_handles.erase(notify_id);
    if (watcher->watch_handles.empty() && watcher->notify_handles.empty()) {
      m_file_watchers.erase(oid);
    }
  }

  if (--m_pending_notifies == 0) {
    m_file_watcher_cond.Signal();
  }
  return 0;
>>>>>>> upstream/hammer
}

void TestWatchNotify::aio_unwatch(uint64_t handle, Context *on_finish) {
  unwatch(handle);
  m_finisher->queue(on_finish);
}

void TestWatchNotify::aio_notify(const std::string& oid, bufferlist& bl,
                                 uint64_t timeout_ms, bufferlist *pbl,
                                 Context *on_notify) {
  Mutex::Locker lock(m_lock);
  ++m_pending_notifies;
  uint64_t notify_id = ++m_notify_id;

  ldout(m_cct, 20) << "oid=" << oid << ": notify_id=" << notify_id << dendl;

  SharedWatcher watcher = get_watcher(oid);

  SharedNotifyHandle notify_handle(new NotifyHandle());
  notify_handle->pbl = pbl;
  notify_handle->on_notify = on_notify;
  for (auto &watch_handle_pair : watcher->watch_handles) {
    WatchHandle &watch_handle = watch_handle_pair.second;
    notify_handle->pending_watcher_ids.insert(std::make_pair(
      watch_handle.gid, watch_handle.handle));
  }
  watcher->notify_handles[notify_id] = notify_handle;

  FunctionContext *ctx = new FunctionContext(
    boost::bind(&TestWatchNotify::execute_notify, this, oid, bl, notify_id));
  m_finisher->queue(ctx);
}

<<<<<<< HEAD
int TestWatchNotify::notify(const std::string& oid, bufferlist& bl,
                            uint64_t timeout_ms, bufferlist *pbl) {
  C_SaferCond cond;
  aio_notify(oid, bl, timeout_ms, pbl, &cond);
  return cond.wait();
}

void TestWatchNotify::notify_ack(const std::string& o, uint64_t notify_id,
                                 uint64_t handle, uint64_t gid,
                                 bufferlist& bl) {
  ldout(m_cct, 20) << "notify_id=" << notify_id << ", handle=" << handle
		   << ", gid=" << gid << dendl;
  Mutex::Locker lock(m_lock);
  WatcherID watcher_id = std::make_pair(gid, handle);
  ack_notify(o, notify_id, watcher_id, bl);
  finish_notify(o, notify_id);
=======
  SharedNotifyHandle notify_handle = it->second;
  Mutex::Locker notify_handle_locker(notify_handle->lock);

  WatcherID watcher_id = std::make_pair(gid, handle);
  notify_handle->notify_responses[watcher_id] = response;
  notify_handle->pending_watcher_ids.erase(watcher_id);
  if (notify_handle->pending_watcher_ids.empty()) {
    notify_handle->cond.Signal();
  }
>>>>>>> upstream/hammer
}

int TestWatchNotify::watch(const std::string& o, uint64_t gid,
                           uint64_t *handle, librados::WatchCtx *ctx,
                           librados::WatchCtx2 *ctx2) {
<<<<<<< HEAD
  Mutex::Locker lock(m_lock);
=======
>>>>>>> upstream/hammer
  SharedWatcher watcher = get_watcher(o);

  WatchHandle watch_handle;
  watch_handle.gid = gid;
  watch_handle.handle = ++m_handle;
  watch_handle.watch_ctx = ctx;
  watch_handle.watch_ctx2 = ctx2;
  watcher->watch_handles[watch_handle.handle] = watch_handle;

  *handle = watch_handle.handle;

  ldout(m_cct, 20) << "oid=" << o << ", gid=" << gid << ": handle=" << *handle
		   << dendl;
  return 0;
}

int TestWatchNotify::unwatch(uint64_t handle) {
<<<<<<< HEAD
  ldout(m_cct, 20) << "handle=" << handle << dendl;
  Mutex::Locker locker(m_lock);
  for (FileWatchers::iterator it = m_file_watchers.begin();
       it != m_file_watchers.end(); ++it) {
    SharedWatcher watcher = it->second;
=======
  Mutex::Locker l(m_file_watcher_lock);
  for (FileWatchers::iterator it = m_file_watchers.begin();
       it != m_file_watchers.end(); ++it) {
    SharedWatcher watcher = it->second;
    RWLock::WLocker watcher_locker(watcher->lock);
>>>>>>> upstream/hammer

    WatchHandles::iterator w_it = watcher->watch_handles.find(handle);
    if (w_it != watcher->watch_handles.end()) {
      watcher->watch_handles.erase(w_it);
      if (watcher->watch_handles.empty() && watcher->notify_handles.empty()) {
        m_file_watchers.erase(it);
      }
      break;
    }
  }
  return 0;
}

TestWatchNotify::SharedWatcher TestWatchNotify::get_watcher(
    const std::string& oid) {
  assert(m_lock.is_locked());
  SharedWatcher &watcher = m_file_watchers[oid];
  if (!watcher) {
    watcher.reset(new Watcher());
  }
  return watcher;
}

void TestWatchNotify::execute_notify(const std::string &oid,
                                     bufferlist &bl, uint64_t notify_id) {
<<<<<<< HEAD
  ldout(m_cct, 20) << "oid=" << oid << ", notify_id=" << notify_id << dendl;

  Mutex::Locker lock(m_lock);
  SharedWatcher watcher = get_watcher(oid);
=======
  SharedWatcher watcher = get_watcher(oid);
  RWLock::RLocker watcher_locker(watcher->lock);
>>>>>>> upstream/hammer
  WatchHandles &watch_handles = watcher->watch_handles;

  NotifyHandles::iterator n_it = watcher->notify_handles.find(notify_id);
  if (n_it == watcher->notify_handles.end()) {
    ldout(m_cct, 1) << "oid=" << oid << ", notify_id=" << notify_id
		    << ": not found" << dendl;
    return;
  }

  SharedNotifyHandle notify_handle = n_it->second;
<<<<<<< HEAD
=======
  Mutex::Locker notify_handle_locker(notify_handle->lock);

>>>>>>> upstream/hammer
  WatcherIDs watcher_ids(notify_handle->pending_watcher_ids);
  for (WatcherIDs::iterator w_id_it = watcher_ids.begin();
       w_id_it != watcher_ids.end(); ++w_id_it) {
    WatcherID watcher_id = *w_id_it;
    WatchHandles::iterator w_it = watch_handles.find(watcher_id.second);
    if (w_it == watch_handles.end()) {
<<<<<<< HEAD
      // client disconnected before notification processed
      notify_handle->pending_watcher_ids.erase(watcher_id);
    } else {
      WatchHandle watch_handle = w_it->second;
=======
      notify_handle->pending_watcher_ids.erase(watcher_id);
    } else {
      WatchHandle &watch_handle = w_it->second;
>>>>>>> upstream/hammer
      assert(watch_handle.gid == watcher_id.first);
      assert(watch_handle.handle == watcher_id.second);

      bufferlist notify_bl;
      notify_bl.append(bl);

<<<<<<< HEAD
      m_lock.Unlock();
=======
      notify_handle->lock.Unlock();
      watcher->lock.put_read();
>>>>>>> upstream/hammer
      if (watch_handle.watch_ctx2 != NULL) {
        watch_handle.watch_ctx2->handle_notify(notify_id, w_it->first, 0,
                                               notify_bl);
      } else if (watch_handle.watch_ctx != NULL) {
        watch_handle.watch_ctx->notify(0, 0, notify_bl);
      }
<<<<<<< HEAD
      m_lock.Lock();

      if (watch_handle.watch_ctx2 == NULL) {
        // auto ack old-style watch/notify clients
        ack_notify(oid, notify_id, watcher_id, bufferlist());
=======
      watcher->lock.get_read();
      notify_handle->lock.Lock();

      if (watch_handle.watch_ctx2 == NULL) {
        // auto ack old-style watch/notify clients
        notify_handle->notify_responses[watcher_id] = bufferlist();
        notify_handle->pending_watcher_ids.erase(watcher_id);
>>>>>>> upstream/hammer
      }
    }
  }

<<<<<<< HEAD
  finish_notify(oid, notify_id);

  if (--m_pending_notifies == 0) {
    m_file_watcher_cond.Signal();
  }
}

void TestWatchNotify::ack_notify(const std::string &oid,
                                 uint64_t notify_id,
                                 const WatcherID &watcher_id,
                                 const bufferlist &bl) {
  ldout(m_cct, 20) << "oid=" << oid << ", notify_id=" << notify_id
		   << ", WatcherID=" << watcher_id << dendl;

  assert(m_lock.is_locked());
  SharedWatcher watcher = get_watcher(oid);

  NotifyHandles::iterator it = watcher->notify_handles.find(notify_id);
  if (it == watcher->notify_handles.end()) {
    ldout(m_cct, 1) << "oid=" << oid << ", notify_id=" << notify_id
		    << ", WatcherID=" << watcher_id << ": not found" << dendl;
    return;
  }

  bufferlist response;
  response.append(bl);

  SharedNotifyHandle notify_handle = it->second;
  notify_handle->notify_responses[watcher_id] = response;
  notify_handle->pending_watcher_ids.erase(watcher_id);
}

void TestWatchNotify::finish_notify(const std::string &oid,
                                    uint64_t notify_id) {
  ldout(m_cct, 20) << "oid=" << oid << ", notify_id=" << notify_id << dendl;

  assert(m_lock.is_locked());
  SharedWatcher watcher = get_watcher(oid);

  NotifyHandles::iterator it = watcher->notify_handles.find(notify_id);
  if (it == watcher->notify_handles.end()) {
    ldout(m_cct, 1) << "oid=" << oid << ", notify_id=" << notify_id
		    << ": not found" << dendl;
    return;
  }

  SharedNotifyHandle notify_handle = it->second;
  if (!notify_handle->pending_watcher_ids.empty()) {
    ldout(m_cct, 10) << "oid=" << oid << ", notify_id=" << notify_id
		     << ": pending watchers, returning" << dendl;
    return;
  }

  ldout(m_cct, 20) << "oid=" << oid << ", notify_id=" << notify_id
		   << ": completing" << dendl;

  if (notify_handle->pbl != NULL) {
    ::encode(notify_handle->notify_responses, *notify_handle->pbl);
    ::encode(notify_handle->pending_watcher_ids, *notify_handle->pbl);
  }

  m_lock.Unlock();
  notify_handle->on_notify->complete(0);
  m_lock.Lock();

  watcher->notify_handles.erase(notify_id);
  if (watcher->watch_handles.empty() && watcher->notify_handles.empty()) {
    m_file_watchers.erase(oid);
  }
=======
  if (notify_handle->pending_watcher_ids.empty()) {
    notify_handle->cond.Signal();
  }
>>>>>>> upstream/hammer
}

} // namespace librados
