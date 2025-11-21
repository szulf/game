#ifndef BADTL_MAP_HPP
#define BADTL_MAP_HPP

#include "allocator.hpp"
#include "utils.hpp"
#include "memory.hpp"
#include "types.hpp"

namespace btl {

template <typename K, typename V>
struct MapEntry {
  bool set;
  K key;
  V value;
};

template <typename K, typename V>
struct Map {
  usize cap;
  MapEntry<K, V>* entries;

  static Map make(usize cap, Allocator& allocator);
  const V& operator[](const K& key) const;
  const MapEntry<K, V>& get_entry(const K& key) const;
  void set(const K& key, const V& value);
  bool contains(const K& key) const;
};

}

namespace btl {

template <typename K, typename V>
Map<K, V> Map<K, V>::make(usize cap, Allocator& allocator) {
  Map<K, V> out = {};
  out.cap = cap;
  out.entries = static_cast<MapEntry<K, V>*>(allocator.alloc(cap * sizeof(MapEntry<K, V>)));
  mem::set(out.entries, 0, cap * sizeof(MapEntry<K, V>));
  return out;
}

template <typename K, typename V>
const V& Map<K, V>::operator[](const K& key) const {
  return get_entry(key).value;
}

template <typename K, typename V>
const MapEntry<K, V>& Map<K, V>::get_entry(const K& key) const {
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = true;
  while (true) {
    ASSERT(entries[i].set, "map element not found");
    if (entries[i].key == key) {
      return entries[i];
    }
    ++i;
    if (i == cap) {
      i = 0;
      looped = true;
    }
    ASSERT(!(looped && i == idx), "map element not found");
  }
  ASSERT(false, "map element not found");
}

template <typename K, typename V>
void Map<K, V>::set(const K& key, const V& value) {
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = false;
  while (true) {
    if (!entries[i].set) {
      entries[i].key = key;
      entries[i].value = value;
      entries[i].set = true;
      return;
    }
    ++i;
    if (i == cap) {
      i = 0;
      looped = true;
    }
    ASSERT(!(looped && i == idx), "out of memory in map");
  }
}

template <typename K, typename V>
bool Map<K, V>::contains(const K& key) const {
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = true;
  while (true) {
    if (!entries[i].set) {
      return false;
    }
    if (entries[i].key == key) {
      return true;
    }
    ++i;
    if (i == cap) {
      i = 0;
      looped = true;
    }
    if (looped && i == idx) {
      return false;
    }
  }
  return false;
}

}

#endif
