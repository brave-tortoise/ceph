// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef CEPH_SIMPLECACHE_H
#define CEPH_SIMPLECACHE_H

#include <map>
#include <tr1/unordered_map>
#include <list>
#include <memory>
#include "common/Mutex.h"
#include "common/Cond.h"
#include "include/encoding.h"
#include "common/hobject.h"

using namespace std::tr1;

template <class K, class V>
class SimpleLRU {
  Mutex lock;
  size_t max_size;
  typedef typename list<pair<K,V> >::iterator LRUIter;
  unordered_map<K, LRUIter> contents;
  list<pair<K, V> > lru;
  unordered_map<K, V> pinned;

  void trim_cache() {
    while (contents.size() > max_size) {
      contents.erase(lru.back().first);
      lru.pop_back();
    }
  }

  void _add(K key, V value) {
    lru.push_front(make_pair(key, value));
    contents[key] = lru.begin();
    trim_cache();
  }

public:
  SimpleLRU(size_t max_size) : lock("SimpleLRU::lock"), max_size(max_size) {
    contents.rehash(max_size);
    pinned.rehash(max_size);
  }

  void pin(K key, V val) {
    Mutex::Locker l(lock);
    pinned.insert(make_pair(key, val));
  }

  void clear_pinned(K e) {
    Mutex::Locker l(lock);
    for (typename unordered_map<K, V>::iterator i = pinned.begin();
	 i != pinned.end() && i->first <= e;
	 pinned.erase(i++)) {
      if (!contents.count(i->first))
	_add(i->first, i->second);
      else
	lru.splice(lru.begin(), lru, contents[i->first]);
    }
  }

  void clear(K key) {
    Mutex::Locker l(lock);
    typename unordered_map<K, LRUIter>::iterator i = contents.find(key);
    if (i == contents.end())
      return;
    lru.erase(i->second);
    contents.erase(i);
  }

  void set_size(size_t new_size) {
    Mutex::Locker l(lock);
    max_size = new_size;
    trim_cache();
  }

  bool lookup(K key, V *out) {
    Mutex::Locker l(lock);
    typename unordered_map<K, LRUIter>::iterator i = contents.find(key);
    if(i != contents.end()) {
      *out = i->second->second;
      lru.splice(lru.begin(), lru, i->second);
      return true;
    }
    typename unordered_map<K, V>::iterator it = pinned.find(key);
    if(it != pinned.end()) {
      *out = it->second;
      return true;
    }
    return false;
  }

  void add(K key, V value) {
    Mutex::Locker l(lock);
    _add(key, value);
  }
};


// ========================================================================
// fifo cache
template <class T>
class FIFOCache {
  Mutex lock;
  size_t max_size;
  list<T> fifo;
  typedef typename list<T>::iterator FIFOIter;
  unordered_map<T, FIFOIter> contents;

  void _add(const T& entry) {
    fifo.push_front(entry);
    contents[entry] = fifo.begin();
    if(contents.size() > max_size) {
      contents.erase(fifo.back());
      fifo.pop_back();
    }
  }

  void _remove(const T& entry) {
    typename unordered_map<T, FIFOIter>::iterator i = contents.find(entry);
    if(i != contents.end()) {
      fifo.erase(i->second);
      contents.erase(i);
    }
  }

public:
  FIFOCache(size_t max_size) : lock("FIFOCache::lock"), max_size(max_size) {
    contents.rehash(max_size);
  }

  bool lookup_or_add(const T& entry) {
    Mutex::Locker l(lock);
    if(contents.count(entry)) {
      return true;
    } else {
      _add(entry);
      return false;
    }
  }

  bool adjust_or_add(const T& entry) {
    Mutex::Locker l(lock);
    typename unordered_map<T, FIFOIter>::iterator i = contents.find(entry);
    if(i != contents.end()) {
      fifo.splice(fifo.begin(), fifo, i->second);
      return true;
    } else {
      _add(entry);
      return false;
    }
  }

  bool promote_or_add(const T& entry) {
    Mutex::Locker l(lock);
    if(contents.count(entry)) {
      _remove(entry);
      return true;
    } else {
      _add(entry);
      return false;
    }
  }

  void add(const T& entry) {
    Mutex::Locker l(lock);
    _add(entry);
  }

  void remove(const T& entry) {
    Mutex::Locker l(lock);
    _remove(entry);
  }

  bool empty() {
    Mutex::Locker l(lock);
    return contents.empty();
  }
};


// ========================================================================
// mru cache
template <class K, class V>
class MRUCache {
  Mutex lock;
  size_t max_size;
  list<pair<K, V> > mru;
  typedef typename list<pair<K, V> >::iterator MRUIter;
  unordered_map<K, MRUIter> contents;
  //typedef ceph::shared_ptr<V> VPtr;

  void _add(const K& key, const V& value) {
    mru.push_front(make_pair(key, value));
    contents[key] = mru.begin();
  }

public:
  MRUCache(size_t max_size) : lock("MRUCache::lock"), max_size(max_size) {
    contents.rehash(max_size);
  }

  /*
  V* adjust_or_add(const K& key, const V& value) {
    Mutex::Locker l(lock);
    V* val = NULL;
    typename unordered_map<K, MRUIter>::iterator i = contents.find(key);
    if(i != contents.end()) {
      mru.splice(mru.begin(), mru, i->second);
    } else {
      _add(key, value);
      if(contents.size() > max_size) {
        val = &(mru.back().second);
	contents.erase(mru.back().first);
	mru.pop_back();
      }
    }
    return val;
  }
  */

  bool adjust_or_add(const K& key, const V& value) {
    Mutex::Locker l(lock);
    typename unordered_map<K, MRUIter>::iterator i = contents.find(key);
    if(i != contents.end()) {
      mru.splice(mru.begin(), mru, i->second);
      return true;
    } else {
      _add(key, value);
      if(contents.size() > max_size) {
	contents.erase(mru.back().first);
	mru.pop_back();
      }
      return false;
    }
  }

  bool adjust_or_lookup(const K& key) {
    Mutex::Locker l(lock);
    typename unordered_map<K, MRUIter>::iterator i = contents.find(key);
    if(i != contents.end()) {
      mru.splice(mru.begin(), mru, i->second);
      return true;
    }
    return false;
  }

  bool lookup(const K& key) {
    Mutex::Locker l(lock);
    if(contents.count(key))
      return true;
    return false;
  }

  void pop(K* const key, V* const value) {
    Mutex::Locker l(lock);
    if(!mru.empty()) {
      *key = mru.front().first;
      *value = mru.front().second;
      contents.erase(*key);
      mru.pop_front();
    }
  }

  void remove(const K& key) {
    Mutex::Locker l(lock);
    typename unordered_map<K, MRUIter>::iterator i = contents.find(key);
    if(i != contents.end()) {
      mru.erase(i->second);
      contents.erase(i);
    }
  }

  bool empty() {
    Mutex::Locker l(lock);
    return contents.empty();
  }

  int get_size() {
    Mutex::Locker l(lock);
    return contents.size();
  }
};

// ========================================================================
// lru cache
template <class T>
class LRUCache {
  Mutex lock;
  int percentile;
  int update_count, persist_update_count;
  list<T> lru;
  typedef typename list<T>::iterator LRUIter;
  unordered_map<T, LRUIter> contents;

  void _add_to_front(const T& entry) {
    lru.push_front(entry);
    contents[entry] = lru.begin();
  }

  void _add_to_back(const T& entry) {
    lru.push_back(entry);
    contents[entry] = (--lru.end());
  }

  void _add_to_middle(const T& entry) {
    int pos = contents.size() * percentile / 100;
    LRUIter loc = lru.begin();
    advance(loc, pos);
    lru.insert(loc, entry);
    contents[entry] = (--loc);
  }

  void rebuild_contents(int lru_size) {
    contents.rehash(lru_size);
    for(LRUIter p = lru.begin(); p != lru.end(); ++p) {
      contents[*p] = p;
    }
  }

public:
  LRUCache(int percentile, int persist_update_count) :
	lock("LRUCache::lock"),
	percentile(percentile),
	update_count(0), persist_update_count(persist_update_count) {
    contents.rehash(1024);
  }

  void add(const T& entry) {
    Mutex::Locker l(lock);
    ++update_count;
    typename unordered_map<T, LRUIter>::iterator i = contents.find(entry);
    if(i != contents.end()) {
      int pos = contents.size() * percentile / 100;
      LRUIter loc = lru.begin();
      advance(loc, pos);
      lru.splice(loc, lru, i->second);
    } else {
      _add_to_middle(entry);
    }
  }

  void adjust_or_add(const T& entry) {
    Mutex::Locker l(lock);
    ++update_count;
    typename unordered_map<T, LRUIter>::iterator i = contents.find(entry);
    if(i != contents.end()) {
      lru.splice(lru.begin(), lru, i->second);
    } else {
      //_add_to_front(entry);
      _add_to_middle(entry);
    }
  }

  void adjust_to_cold(const T& entry) {
    Mutex::Locker l(lock);
    ++update_count;
    typename unordered_map<T, LRUIter>::iterator i = contents.find(entry);
    if(i != contents.end()) {
      lru.splice(lru.end(), lru, i->second);
    } else {
      _add_to_back(entry);
    }
  }

  void lookup_or_add(const T& entry) {
    Mutex::Locker l(lock);
    if(contents.count(entry)) {
      return;
    } else {
      ++update_count;
      _add_to_middle(entry);
    }
  }

  bool lookup(const T& entry) {
    Mutex::Locker l(lock);
    return contents.count(entry);
  }

  void remove(const T& entry) {
    Mutex::Locker l(lock);
    typename unordered_map<T, LRUIter>::iterator i = contents.find(entry);
    if(i != contents.end()) {
      ++update_count;
      lru.erase(i->second);
      contents.erase(i);
    }
  }

  bool pop(T* const entry) {
    Mutex::Locker l(lock);
    if(contents.size()) {
      ++update_count;
      *entry = lru.back();
      lru.pop_back();
      contents.erase(*entry);
      return true;
    }
    return false;
  }

  bool to_persist() {
    Mutex::Locker l(lock);
    if(update_count > persist_update_count) {
      update_count = 0;
      return true;
    }
    return false;
  }

  int get_size() {
    Mutex::Locker l(lock);
    return contents.size();
  }

  void encode(bufferlist &bl) const {
    ENCODE_START(1, 1, bl);
    ::encode(contents.size(), bl);
    ::encode(lru, bl);
    ENCODE_FINISH(bl);
  }

  void decode(bufferlist::iterator &bl) {
    int lru_size;
    DECODE_START(1, bl);
    ::decode(lru_size, bl);
    ::decode(lru, bl);
    DECODE_FINISH(bl);
    rebuild_contents(lru_size);
  }
};
WRITE_CLASS_ENCODER(LRUCache<hobject_t>)

/*
// ========================================================================
// lru-2q cache
enum CacheLevel {
  CEPH_RW_RESIDENT_CACHE,
  CEPH_RW_GHOST_CACHE,
  CEPH_RW_VICTIM_CACHE,
};

template <class T>
class LRU2QCache {
  Mutex lock;
  size_t resident_size, ghost_size, victim_size;
  size_t ghost_max_size;
  int resident_victim_ratio;
  //list<T> lru;
  list<T> resident, ghost, victim;
  typedef typename list<T>::iterator LRUIter;
  unordered_map<T, pair<CacheLevel, LRUIter> > contents;

  void trim_resident_cache() {
    if(resident_size > (victim_size+1) * 8) {
      T& entry = resident.back();
      contents[entry].first = CEPH_RW_GHOST_CACHE;
      LRUIter loc = (--resident.end());
      ghost.splice(ghost.begin(), resident, loc);
      --resident_size;
      ++ghost_size;
      trim_ghost_cache();
    }
  }

  void trim_ghost_cache() {
    if(ghost_size > ghost_max_size) {
      T& entry = ghost.back();
      contents[entry].first = CEPH_RW_VICTIM_CACHE;
      LRUIter loc = (--ghost.end());
      victim.splice(victim.begin(), ghost, loc);
      --ghost_size;
      ++victim_size;
    }
  }

  void _add_to_ghost(const T& entry) {
    ghost.push_front(entry);
    contents[entry] = make_pair(CEPH_RW_GHOST_CACHE, ghost.begin());
    ++ghost_size;
    trim_ghost_cache();
  }

  void _add_to_resident(const T& entry) {
    resident.push_front(entry);
    contents[entry] = make_pair(CEPH_RW_RESIDENT_CACHE, resident.begin());
    ++resident_size;
    trim_resident_cache();
  }

public:
  LRU2QCache(size_t ghost_max_size, int resident_victim_ratio) :
	lock("LRU2QCache::lock"),
	resident_size(0), ghost_size(0), victim_size(0),
	ghost_max_size(ghost_max_size),
	resident_victim_ratio(resident_victim_ratio) {}

  void add(const T& entry) {
    Mutex::Locker l(lock);
    typename unordered_map<K, pair<CacheLevel, LRUIter> >::iterator i = contents.find(entry);
    // if already exist in cache, splice
    if(i != contents.end()) {
      CacheLevel level = i->second->first;
      LRUIter loc = i->second->second;
      if(level == CEPH_RW_RESIDENT_CACHE) {
	ghost.splice(ghost.begin(), resident, loc);
	--resident_size;
	++ghost_size;
	trim_ghost_cache();
      } else if(level = CEPH_RW_GHOST_CACHE) {
	ghost.splice(ghost.begin(), ghost, loc);
      } else {
	ghost.splice(ghost.begin(), victim, loc);
	--victim_size;
	++ghost_size;
	trim_ghost_cache();
      }
    } else {
      // otherwise, add to ghost
      _add_to_ghost(entry);
    }
  }

  void adjust_or_add(const T& entry) {
    Mutex::Locker l(lock);
    typename unordered_map<K, pair<CacheLevel, LRUIter> >::iterator i = contents.find(entry);
    if(i != contents.end()) {
      CacheLevel level = i->second->first;
      LRUIter loc = i->second->second;
      if(level == CEPH_RW_RESIDENT_CACHE) {
	resident.splice(resident.begin(), resident, loc);
      } else if(level = CEPH_RW_GHOST_CACHE) {
	resident.splice(resident.begin(), ghost, loc);
	--ghost_size;
	++resident_size;
	trim_resident_cache();
      } else {
	ghost.splice(ghost.begin(), victim, loc);
	--victim_size;
	++ghost_size;
	trim_ghost_cache();
      }
    } else {
      _add_to_resident(entry);
    }
  }

  void remove(const T& entry) {
    Mutex::Locker l(lock);
    typename unordered_map<T, LRUIter>::iterator i = contents.find(entry);
    if(i != contents.end()) {
      lru.erase(i->second);
      contents.erase(i);
    }
  }

  bool lookup(const T& entry) {
    Mutex::Locker l(lock);
    if(contents.count(entry)) {
      return true;
    }
    return false;
  }

  void pop(T* const entry) {
    Mutex::Locker l(lock);
    if(contents.size()) {
      *entry = lru.back();
      lru.pop_back();
      contents.erase(*entry);
    }
  }

  int get_size() {
    Mutex::Locker l(lock);
    return contents.size();
  }
};
*/

#endif
