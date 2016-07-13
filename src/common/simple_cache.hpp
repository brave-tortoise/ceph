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
  size_t max_size;
  unordered_map<T, typename list<T>::iterator> contents;
  list<T> lru;

  void trim_cache() {
    while (lru.size() > max_size) {
      contents.erase(lru.back());
      lru.pop_back();
    }
  }

  void _add(const T& entry) {
    lru.push_front(entry);
    contents[entry] = lru.begin();
    trim_cache();
  }

public:
  RWLRU(size_t max_size) : lock("RWLRU::lock"), max_size(max_size) {}

  void clear(const T& entry) {
    Mutex::Locker l(lock);
    typename unordered_map<T, typename list<T>::iterator>::iterator i =
      contents.find(entry);
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
// mru cache for promotion

template <class K, class V>
class PromoteMRU {
  Mutex lock;
  size_t max_size;
  unordered_map<K, typename list<pair<K, V> >::iterator> contents;
  list<pair<K, V> > lru;

  void trim_cache() {
    while (lru.size() > max_size) {
      contents.erase(lru.back().first);
      lru.pop_back();
    }
  }

  void _add(const K& key, const V& value) {
    lru.push_front(make_pair(key, value));
    contents[key] = lru.begin();
    trim_cache();
  }

public:
  PromoteMRU(size_t max_size) : lock("PromoteMRU::lock"), max_size(max_size) {}

  void clear(const K& key) {
    Mutex::Locker l(lock);
    typename unordered_map<K, typename list<pair<K, V> >::iterator>::iterator i =
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

  void adjust_or_add(const K& key, const V& value) {
    Mutex::Locker l(lock);
    typename list<pair<K, V> >::iterator loc = contents.count(key) ?
      contents[key] : lru.end();
    if (loc != lru.end()) {
      lru.splice(lru.begin(), lru, loc);
      return;
    }
    _add(key, value);
    return;
  }

  bool empty() {
    Mutex::Locker l(lock);
    return lru.empty();
  }

  void pop(K* const key, V* const value) {
    Mutex::Locker l(lock);
    if(!lru.empty()) {
      *key = lru.begin().first;
      *value = lru.begin().second;
      contents.erase(lru.front().first);
      lru.pop_front();
    }
  }
};

#endif
