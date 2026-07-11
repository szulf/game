enum ItemType {
  ITEM_BLOCK,
  ITEM_STORAGE,
  ITEM_CONVEYOR,
  ITEM_ASSEMBLER,

  ITEM_COUNT,
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

void swap_slot_flags(std::span<ItemSlot> inventory) {
  for (auto& slot : inventory) {
    slot.flags ^= ITEM_SLOT_FLAGS_MASK;
  }
}

TextureType get_texture_type(ItemType item) {
  switch (item) {
    case ITEM_BLOCK:
      return TEXTURE_BLOCK_ITEM;
    case ITEM_STORAGE:
      return TEXTURE_STORAGE_ITEM;
    case ITEM_CONVEYOR:
      return TEXTURE_CONVEYOR_ITEM;
    case ITEM_ASSEMBLER:
      return TEXTURE_ASSEMBLER_ITEM;
    case ITEM_COUNT:
      break;
  }
  ASSERT_NO_MSG(false);
}

std::string_view get_item_name(ItemType item) {
  switch (item) {
    case ITEM_BLOCK:
      return "Block";
    case ITEM_STORAGE:
      return "Storage";
    case ITEM_CONVEYOR:
      return "Conveyor";
    case ITEM_ASSEMBLER:
      return "Assembler";
    case ITEM_COUNT:
      break;
  }
  ASSERT_NO_MSG(false);
}
