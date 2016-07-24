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
  FIFOCache(size_t max_size) : lock("FIFOCache::lock"), max_size(max_size) {}

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
  MRUCache(size_t max_size) : lock("MRUCache::lock"), max_size(max_size) {}

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
  list<T> lru;
  typedef typename list<T>::iterator LRUIter;
  unordered_map<T, LRUIter> contents;

  void _add(const T& entry) {
    lru.push_front(entry);
    contents[entry] = lru.begin();
  }

public:
  LRUCache() : lock("LRUCache::lock") {}

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

  void adjust_or_add(const T& entry) {
    Mutex::Locker l(lock);
    if(contents.count(entry)) {
      LRUIter loc = contents[entry];
      lru.splice(lru.begin(), lru, loc);
      return;
    }
    _add(entry);
    return;
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

#endif
