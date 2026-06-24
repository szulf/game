#ifndef ITEMS_H
#define ITEMS_H

enum class ItemType {
  BLOCK,
  STORAGE,
  CONVEYOR,
};

static constexpr u32 ITEM_MAX_COUNT = 100;

struct ItemSlot {
  ItemType type{};
  u32 count{};

  explicit inline operator bool() const {
    return count > 0;
  }
};

#endif
