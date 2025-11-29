#include "map.h"

template <typename K, typename V>
Map<K, V> map_make(usize cap, Allocator& allocator)
{
  Map<K, V> out = {};
  out.cap = cap;
  out.entries = (MapEntry<K, V>*) alloc(allocator, cap * sizeof(MapEntry<K, V>));
  mem_set(out.entries, 0, cap * sizeof(MapEntry<K, V>));
  return out;
}

template <typename K, typename V>
V* map_get(Map<K, V>& map, const K& key)
{
  usize idx = hash(key) % map.cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!map.entries[i].set)
    {
      break;
    }
    if (map.entries[i].key == key)
    {
      return &map.entries[i].value;
    }
    ++i;
    if (i == map.cap)
    {
      i = 0;
      looped = true;
    }
    if (looped && i == idx)
    {
      break;
    }
  }
  ASSERT(false, "map element not found");
  return nullptr;
}

template <typename K, typename V>
MapEntry<K, V>* map_entry(Map<K, V>& map, const K& key)
{
  usize idx = hash(key) % map.cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!map.entries[i].set)
    {
      break;
    }
    if (equal(&map.entries[i].key, key))
    {
      return &map.entries[i];
    }
    ++i;
    if (i == map.cap)
    {
      i = 0;
      looped = true;
    }
    if (looped && i == idx)
    {
      break;
    }
  }
  ASSERT(false, "map entry not found");
  return nullptr;
}

template <typename K, typename V>
void map_set(Map<K, V>& map, const K& key, const V& value)
{
  usize idx = hash(key) % map.cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!map.entries[i].set)
    {
      map.entries[i].key = key;
      map.entries[i].value = value;
      map.entries[i].set = true;
      return;
    }
    ++i;
    if (i == map.cap)
    {
      i = 0;
      looped = true;
    }
    if (looped && i == idx)
    {
      ASSERT(false, "out of memory in map");
      return;
    }
  }
}

template <typename K, typename V>
bool map_contains(const Map<K, V>& map, const K& key)
{
  usize idx = hash(key) % map.cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!map.entries[i].set)
    {
      return false;
    }
    if (map.entries[i].key == key)
    {
      return true;
    }
    ++i;
    if (i == map.cap)
    {
      i = 0;
      looped = true;
    }
    if (looped && i == idx)
    {
      return false;
    }
  }
  return false;
}
