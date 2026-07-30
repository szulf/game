enum ItemType {
  // NOTE: requestable items
  ITEM_COPPER,
  ITEM_PLASTIC,
  ITEM_ALUMINIUM,

  // NOTE: not requestable items
  ITEM_COPPER_WIRE,
  ITEM_BASIC_CIRCUIT_BOARD,
  ITEM_ANTENNA,
  ITEM_COMMUNICATION_COMPONENT,

  // NOTE: block items
  ITEM_BLOCK,
  ITEM_STORAGE,
  ITEM_CONVEYOR,
  ITEM_ASSEMBLER,

  ITEM_COUNT,
};

// TODO: i dont really like that i have to put the requestable items in two places
// but i dont know how to do it better for now
// (used only to get REQUESTABLE_ITEMS.size() which is needed for ResourceMessage)
static constexpr std::array REQUESTABLE_ITEMS = std::to_array<ItemType>({
  ITEM_COPPER,
  ITEM_PLASTIC,
  ITEM_ALUMINIUM,
});

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
    case ITEM_COPPER:
      return TEXTURE_COPPER_ITEM;
    case ITEM_PLASTIC:
      return TEXTURE_PLASTIC_ITEM;
    case ITEM_ALUMINIUM:
      return TEXTURE_ALUMINIUM_ITEM;
    case ITEM_COPPER_WIRE:
      return TEXTURE_COPPER_WIRE_ITEM;
    case ITEM_BASIC_CIRCUIT_BOARD:
      return TEXTURE_BASIC_CIRCUIT_BOARD_ITEM;
    case ITEM_ANTENNA:
      return TEXTURE_ANTENNA_ITEM;
    case ITEM_COMMUNICATION_COMPONENT:
      return TEXTURE_COMMUNICATION_COMPONENT_ITEM;

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
    case ITEM_COPPER:
      return "Copper";
    case ITEM_PLASTIC:
      return "Plastic";
    case ITEM_ALUMINIUM:
      return "Aluminium";
    case ITEM_COPPER_WIRE:
      return "Copper Wire";
    case ITEM_BASIC_CIRCUIT_BOARD:
      return "Basic Circuit Board";
    case ITEM_ANTENNA:
      return "Antenna";
    case ITEM_COMMUNICATION_COMPONENT:
      return "Communication Component";
    case ITEM_COUNT:
      break;
  }
  ASSERT_NO_MSG(false);
}
