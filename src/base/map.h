#ifndef MAP_H
#define MAP_H

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
};

template <typename K, typename V>
Map<K, V> map_make(usize cap, Allocator& allocator);
template <typename K, typename V>
V* map_get(const Map<K, V>& map, const K& key);
template <typename K, typename V>
const MapEntry<K, V>* map_entry(const Map<K, V>& map, const K& key);
template <typename K, typename V>
void map_set(Map<K, V>& map, const K& key, const V& value);
template <typename K, typename V>
bool map_contains(const Map<K, V>& map, const K& key);

#endif
