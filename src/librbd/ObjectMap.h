// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef CEPH_LIBRBD_OBJECT_MAP_H
#define CEPH_LIBRBD_OBJECT_MAP_H

#include "include/int_types.h"
<<<<<<< HEAD
#include "include/fs_types.h"
=======
#include "include/rados/librados.hpp"
>>>>>>> upstream/hammer
#include "include/rbd/object_map_types.h"
#include "common/bit_vector.hpp"
#include "librbd/Utils.h"
#include <boost/optional.hpp>

class Context;
class RWLock;
namespace librados {
  class IoCtx;
}

namespace librbd {

<<<<<<< HEAD
template <typename Op> class BlockGuard;
struct BlockGuardCell;
=======
>>>>>>> upstream/hammer
class ImageCtx;

template <typename ImageCtxT = ImageCtx>
class ObjectMap {
public:
  static ObjectMap *create(ImageCtxT &image_ctx, uint64_t snap_id) {
    return new ObjectMap(image_ctx, snap_id);
  }

  ObjectMap(ImageCtxT &image_ctx, uint64_t snap_id);
  ~ObjectMap();

  static int aio_remove(librados::IoCtx &io_ctx, const std::string &image_id, librados::AioCompletion *c);
  static std::string object_map_name(const std::string &image_id,
				     uint64_t snap_id);

<<<<<<< HEAD
  static bool is_compatible(const file_layout_t& layout, uint64_t size);

  ceph::BitVector<2u>::Reference operator[](uint64_t object_no);
  uint8_t operator[](uint64_t object_no) const;
  inline uint64_t size() const {
    return m_object_map.size();
  }

  void open(Context *on_finish);
  void close(Context *on_finish);
=======
  uint8_t operator[](uint64_t object_no) const;

  int lock();
  int unlock();
>>>>>>> upstream/hammer

  bool object_may_exist(uint64_t object_no) const;

  void aio_save(Context *on_finish);
  void aio_resize(uint64_t new_size, uint8_t default_object_state,
		  Context *on_finish);

<<<<<<< HEAD
  template <typename T, void(T::*MF)(int) = &T::complete>
  bool aio_update(uint64_t snap_id, uint64_t start_object_no, uint8_t new_state,
                  const boost::optional<uint8_t> &current_state,
                  T *callback_object) {
    return aio_update<T, MF>(snap_id, start_object_no, start_object_no + 1,
                             new_state, current_state, callback_object);
  }

  template <typename T, void(T::*MF)(int) = &T::complete>
  bool aio_update(uint64_t snap_id, uint64_t start_object_no,
                  uint64_t end_object_no, uint8_t new_state,
                  const boost::optional<uint8_t> &current_state,
                  T *callback_object) {
    assert(start_object_no < end_object_no);
    if (snap_id == CEPH_NOSNAP) {
      uint64_t object_no;
      for (object_no = start_object_no; object_no < end_object_no;
           ++object_no) {
        if (update_required(object_no, new_state)) {
          break;
        }
      }

      if (object_no == end_object_no) {
        return false;
      }

      UpdateOperation update_operation(start_object_no, end_object_no,
                                       new_state, current_state,
                                       util::create_context_callback<T, MF>(
                                         callback_object));
      detained_aio_update(std::move(update_operation));
    } else {
      aio_update(snap_id, start_object_no, end_object_no, new_state,
                 current_state,
                 util::create_context_callback<T, MF>(callback_object));
=======
  void refresh(uint64_t snap_id);
  void rollback(uint64_t snap_id);
  void snapshot(uint64_t snap_id);

  bool enabled() const;

private:

  class Request : public AsyncRequest {
  public:
    Request(ImageCtx &image_ctx, Context *on_finish)
      : AsyncRequest(image_ctx, on_finish), m_state(STATE_REQUEST)
    {
    }

  protected:

    virtual bool safely_cancel(int r) {
      return false;
    }
    virtual bool should_complete(int r);
    virtual int filter_return_code(int r) {
      // never propagate an error back to the caller
      return 0;
    }
    virtual void finish(ObjectMap *object_map) = 0;

  private:
    /**
     * <start> ---> STATE_REQUEST ---> <finish>
     *                   |                ^
     *                   v                |
     *            STATE_INVALIDATE -------/
     */
    enum State {
      STATE_REQUEST,
      STATE_INVALIDATE
    };

    State m_state;

    bool invalidate();
  };

  class ResizeRequest : public Request {
  public:
    ResizeRequest(ImageCtx &image_ctx, uint64_t new_size,
		  uint8_t default_object_state, Context *on_finish)
      : Request(image_ctx, on_finish), m_num_objs(0), m_new_size(new_size),
        m_default_object_state(default_object_state)
    {
>>>>>>> upstream/hammer
    }
    return true;
  }

  void rollback(uint64_t snap_id, Context *on_finish);
  void snapshot_add(uint64_t snap_id, Context *on_finish);
  void snapshot_remove(uint64_t snap_id, Context *on_finish);

private:
  struct UpdateOperation {
    uint64_t start_object_no;
    uint64_t end_object_no;
    uint8_t new_state;
    boost::optional<uint8_t> current_state;
    Context *on_finish;

    UpdateOperation(uint64_t start_object_no, uint64_t end_object_no,
                    uint8_t new_state,
                    const boost::optional<uint8_t> &current_state,
                    Context *on_finish)
      : start_object_no(start_object_no), end_object_no(end_object_no),
        new_state(new_state), current_state(current_state),
        on_finish(on_finish) {
    }
  };

  typedef BlockGuard<UpdateOperation> UpdateGuard;

  ImageCtxT &m_image_ctx;
  ceph::BitVector<2> m_object_map;
  uint64_t m_snap_id;

  UpdateGuard *m_update_guard = nullptr;

<<<<<<< HEAD
  void detained_aio_update(UpdateOperation &&update_operation);
  void handle_detained_aio_update(BlockGuardCell *cell, int r,
                                  Context *on_finish);

  void aio_update(uint64_t snap_id, uint64_t start_object_no,
                  uint64_t end_object_no, uint8_t new_state,
                  const boost::optional<uint8_t> &current_state,
                  Context *on_finish);
  bool update_required(uint64_t object_no, uint8_t new_state);
=======
  void invalidate(bool force);
>>>>>>> upstream/hammer

};

} // namespace librbd

extern template class librbd::ObjectMap<librbd::ImageCtx>;

#endif // CEPH_LIBRBD_OBJECT_MAP_H
