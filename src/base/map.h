#ifndef MAP_H
#define MAP_H

#include "memory.h"

template <typename T>
usize hash(const T& value);

template <typename K, typename V>
struct MapEntry
{
  bool set;
  K key;
  V value;
};

template <typename K, typename V>
struct Map
{
  usize cap;
  MapEntry<K, V>* entries;

  static Map<K, V> make(usize cap, Allocator& allocator);

  const V* operator[](const K& key) const;
  V* operator[](const K& key);

  const MapEntry<K, V>* entry(const K& key) const;
  MapEntry<K, V>* entry(const K& key);
  void set(const K& key, const V& value);
  bool contains(const K& key) const;

  void remove(const K& key);
  void clear();
};

#include "map_impl.h"

#endif
