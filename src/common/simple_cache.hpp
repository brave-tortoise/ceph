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

using namespace std::tr1;

template <class K, class V>
class SimpleLRU {
  Mutex lock;
  size_t max_size;
  map<K, typename list<pair<K, V> >::iterator> contents;
  list<pair<K, V> > lru;
  map<K, V> pinned;

  void trim_cache() {
    while (lru.size() > max_size) {
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
  SimpleLRU(size_t max_size) : lock("SimpleLRU::lock"), max_size(max_size) {}

  void pin(K key, V val) {
    Mutex::Locker l(lock);
    pinned.insert(make_pair(key, val));
  }

  void clear_pinned(K e) {
    Mutex::Locker l(lock);
    for (typename map<K, V>::iterator i = pinned.begin();
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
    typename map<K, typename list<pair<K, V> >::iterator>::iterator i =
      contents.find(key);
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
    typename list<pair<K, V> >::iterator loc = contents.count(key) ?
      contents[key] : lru.end();
    if (loc != lru.end()) {
      *out = loc->second;
      lru.splice(lru.begin(), lru, loc);
      return true;
    }
    if (pinned.count(key)) {
      *out = pinned[key];
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
// lru cache for io access

template <class T>
class RWLRU {
  Mutex lock;
  size_t size, max_size;
  unordered_map<T, typename list<T>::iterator> contents;
  list<T> lru;

  void trim_cache() {
    while (size > max_size) {
      --size;
      contents.erase(lru.back());
      lru.pop_back();
    }
  }

  void _add(const T& entry) {
    ++size;
    lru.push_front(entry);
    contents[entry] = lru.begin();
    trim_cache();
  }

public:
  RWLRU(size_t max_size) : lock("RWLRU::lock"), size(0), max_size(max_size) {}

  void remove(const T& entry) {
    Mutex::Locker l(lock);
    typename unordered_map<T, typename list<T>::iterator>::iterator i =
      contents.find(entry);
    if (i == contents.end())
      return;
    --size;
    lru.erase(i->second);
    contents.erase(i);
  }

  bool lookup(const T& entry) {
    Mutex::Locker l(lock);
    typename list<T>::iterator loc = contents.count(entry) ?
      contents[entry] : lru.end();
    if (loc != lru.end()) {
      return true;
    }
    return false;
  }

  void adjust_or_add(const T& entry) {
    Mutex::Locker l(lock);
    typename list<T>::iterator loc = contents.count(entry) ?
      contents[entry] : lru.end();
    if (loc != lru.end()) {
      lru.splice(lru.begin(), lru, loc);
      return;
    }
    _add(entry);
    return;
  }
};


// ========================================================================
// mrfu cache for promotion

enum CacheType {
  CEPH_PROMOTE_L1_CACHE,
  CEPH_PROMOTE_L2_CACHE,
};

template <class K, class V>
class PromoteMRFU {
  Mutex lock;
  size_t mru_size, mfu_size;
  size_t mru_max_size, mfu_max_size;
  unordered_map<K, pair<CacheType, typename list<pair<K, V> >::iterator> > contents;
  list<pair<K, V> > mru, mfu;

  void trim_mru_cache() {
    while (mru_size > mru_max_size) {
      --mru_size;
      contents.erase(mru.back().first);
      mru.pop_back();
    }
  }

  void trim_mfu_cache() {
    while (mfu_size > mfu_max_size) {
      --mfu_size;
      ++mru_size;

      K& key = mfu.back().first;
      typename list<pair<K, V> >::iterator loc = (--mfu.end());

      mru.splice(mru.begin(), mfu, loc);
      mfu.pop_back();
      contents[key].first = CEPH_PROMOTE_L2_CACHE;
    }
    trim_mru_cache();
  }

  void _add(const K& key, const V& value) {
    ++mru_size;
    mru.push_front(make_pair(key, value));
    contents[key] = make_pair(CEPH_PROMOTE_L2_CACHE, mru.begin());
    trim_mru_cache();
  }

public:
  PromoteMRFU(size_t mru_max_size, size_t mfu_max_size) :
	lock("PromoteMRFU::lock"),
	mru_size(0), mfu_size(0),
	mru_max_size(mru_max_size),
	mfu_max_size(mfu_max_size) {}

  void remove(const K& key) {
    Mutex::Locker l(lock);
    typename unordered_map<K, pair<CacheType, typename list<pair<K, V> >::iterator> >::iterator i = contents.find(key);
    if(i != contents.end()) {
      if(i->second.first == CEPH_PROMOTE_L1_CACHE) {
      	mfu.erase(i->second.second);
      } else {
    	mru.erase(i->second.second);
      }
      contents.erase(i);
    }
  }

  void adjust_or_add(const K& key, const V& value) {
    Mutex::Locker l(lock);
    if(contents.count(key)) {
      typename list<pair<K, V> >::iterator loc = contents[key].second;
      if(contents[key].first == CEPH_PROMOTE_L1_CACHE) {  // mfu
        mfu.splice(mfu.begin(), mfu, loc);
      } else {	// mru
        --mru_size;
        ++mfu_size;
        mfu.splice(mfu.begin(), mru, loc);
	contents[key].first = CEPH_PROMOTE_L1_CACHE;
        trim_mfu_cache();
      }
    } else {
      _add(key, value);
    }
  }

  bool empty() {
    Mutex::Locker l(lock);
    return contents.empty();
  }

  void pop(K* const key, V* const value) {
    Mutex::Locker l(lock);
    if(!mfu.empty()) {
      *key = mfu.front().first;
      *value = mfu.front().second;
      contents.erase(*key);
      mfu.pop_front();
    } else if(!mru.empty()) {
      *key = mru.front().first;
      *value = mru.front().second;
      contents.erase(*key);
      mru.pop_front();
    }
  }
};

#endif
