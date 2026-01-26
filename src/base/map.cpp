#include "map.h"

template <typename K, typename V>
Map<K, V> Map<K, V>::make(usize cap, Allocator& allocator)
{
  Map<K, V> out = {};
  out.cap = cap;
  out.entries = (MapEntry<K, V>*) allocator.alloc(cap * sizeof(MapEntry<K, V>));
  mem_set(out.entries, 0, cap * sizeof(MapEntry<K, V>));
  return out;
}

template <typename K, typename V>
V* Map<K, V>::operator[](const K& key)
{
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!entries[i].set)
    {
      break;
    }
    if (entries[i].key == key)
    {
      return &entries[i].value;
    }
    ++i;
    if (i == cap)
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
const V* Map<K, V>::operator[](const K& key) const
{
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!entries[i].set)
    {
      break;
    }
    if (entries[i].key == key)
    {
      return &entries[i].value;
    }
    ++i;
    if (i == cap)
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
const MapEntry<K, V>* Map<K, V>::entry(const K& key) const
{
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!entries[i].set)
    {
      break;
    }
    if (entries[i].key == key)
    {
      return &entries[i];
    }
    ++i;
    if (i == cap)
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
MapEntry<K, V>* Map<K, V>::entry(const K& key)
{
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!entries[i].set)
    {
      break;
    }
    if (entries[i].key == key)
    {
      return &entries[i];
    }
    ++i;
    if (i == cap)
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
void Map<K, V>::set(const K& key, const V& value)
{
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!entries[i].set)
    {
      entries[i].key = key;
      entries[i].value = value;
      entries[i].set = true;
      return;
    }
    ++i;
    if (i == cap)
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
bool Map<K, V>::contains(const K& key) const
{
  usize idx = hash(key) % cap;
  usize i = idx;
  bool looped = false;
  while (true)
  {
    if (!entries[i].set)
    {
      return false;
    }
    if (entries[i].key == key)
    {
      return true;
    }
    ++i;
    if (i == cap)
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

template <typename K, typename V>
void Map<K, V>::remove(const K& key)
{
  auto* e = entry(key);
  ASSERT(e != nullptr, "trying to remove a map entry with a non existent key");
  e->set = false;
}

template <typename K, typename V>
void Map<K, V>::clear()
{
  for (usize i = 0; i < cap; ++i)
  {
    entries[i].set = false;
  }
}
