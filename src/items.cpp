enum class ItemType {
  BLOCK,
  STORAGE,
  CONVEYOR,
};

enum ItemSlotFlag {
  ITEM_SLOT_INPUT  = 1 << 0,
  ITEM_SLOT_OUTPUT = 1 << 1,
};

using ItemSlotFlags = u32;

static constexpr ItemSlotFlags ITEM_SLOT_FLAGS_MASK = ITEM_SLOT_INPUT | ITEM_SLOT_OUTPUT;

static constexpr u32 ITEM_MAX_COUNT = 100;

struct ItemSlot {
  ItemSlotFlags flags = ITEM_SLOT_INPUT | ITEM_SLOT_OUTPUT;
  ItemType type{};
  u32 count{};

  explicit inline operator bool() const {
    return count > 0;
  }
};

void swap_slots(ItemSlot& a, ItemSlot& b) {
  ItemSlot temp = a;
  a.type        = b.type;
  a.count       = b.count;

  b.type  = temp.type;
  b.count = temp.count;
}
