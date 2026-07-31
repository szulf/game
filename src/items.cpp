enum ItemType {
  // NOTE: requestable items
  ITEM_COPPER,
  // TODO: make plastic not requestable, but instead made from oil?
  ITEM_PLASTIC,
  ITEM_ALUMINIUM,
  ITEM_OIL_CANISTER,

  // NOTE: not requestable items
  ITEM_COPPER_WIRE,
  ITEM_BASIC_CIRCUIT_BOARD,
  ITEM_ANTENNA,
  ITEM_COMMUNICATION_COMPONENT,
  ITEM_WIRE_BUNDLE,
  ITEM_COGWHEEL,
  ITEM_SPARE_PARTS,

  // NOTE: block items
  ITEM_BLOCK,
  ITEM_STORAGE,
  ITEM_CONVEYOR,
  ITEM_ASSEMBLER,

  // NOTE: tool items
  ITEM_BRUSH,
  ITEM_LUBRICANT_CAN,
  // TODO: make this rechargeable with power in the future
  ITEM_CALIBRATOR,

  ITEM_COUNT,
};

// TODO: i dont really like that i have to put the requestable items in two places
// but i dont know how to do it better for now
// (used only to get REQUESTABLE_ITEMS.size() which is needed for ResourceMessage)
static constexpr std::array REQUESTABLE_ITEMS = std::to_array<ItemType>({
  ITEM_COPPER,
  ITEM_PLASTIC,
  ITEM_ALUMINIUM,
  ITEM_OIL_CANISTER,
});

enum ItemSlotFlag {
  ITEM_SLOT_INPUT  = 1 << 0,
  ITEM_SLOT_OUTPUT = 1 << 1,
};

using ItemSlotFlags = u32;

static constexpr ItemSlotFlags ITEM_SLOT_FLAGS_MASK = ITEM_SLOT_INPUT | ITEM_SLOT_OUTPUT;

// TODO: do i want to destroy items when they reach max damage?

struct ItemSlot {
  ItemSlotFlags flags = ITEM_SLOT_INPUT | ITEM_SLOT_OUTPUT;
  ItemType type{};
  u32 count{};
  // NOTE: ignored for items with item_data(type).has_durability == false
  u32 damage{};

  explicit inline operator bool() const {
    return count > 0;
  }
};

void swap_slots(ItemSlot& a, ItemSlot& b) {
  ItemSlot temp = a;
  a.type        = b.type;
  a.count       = b.count;
  a.damage      = b.damage;

  b.type   = temp.type;
  b.count  = temp.count;
  b.damage = temp.damage;
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
    case ITEM_WIRE_BUNDLE:
      return TEXTURE_WIRE_BUNDLE_ITEM;
    case ITEM_COGWHEEL:
      return TEXTURE_COGWHEEL_ITEM;
    case ITEM_SPARE_PARTS:
      return TEXTURE_SPARE_PARTS_ITEM;
    case ITEM_BRUSH:
      return TEXTURE_BRUSH_ITEM;
    case ITEM_LUBRICANT_CAN:
      return TEXTURE_LUBRICANT_CAN_ITEM;
    case ITEM_OIL_CANISTER:
      return TEXTURE_OIL_CANISTER_ITEM;
    case ITEM_CALIBRATOR:
      return TEXTURE_CALIBRATOR_ITEM;
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
    case ITEM_WIRE_BUNDLE:
      return "Wire Bundle";
    case ITEM_COGWHEEL:
      return "Cogwheel";
    case ITEM_SPARE_PARTS:
      return "Spare Parts";
    case ITEM_BRUSH:
      return "Brush";
    case ITEM_LUBRICANT_CAN:
      return "Lubricant Can";
    case ITEM_OIL_CANISTER:
      return "Oil Canister";
    case ITEM_CALIBRATOR:
      return "Calibrator";
    case ITEM_COUNT:
      break;
  }
  ASSERT_NO_MSG(false);
}

struct ItemInfo {
  u32 max_count{};
  bool has_durability{};
  // NOTE: used only when has_durability == true
  u32 max_damage{};
};

constexpr ItemInfo item_info(ItemType item) {
  switch (item) {
    case ITEM_BLOCK:
    case ITEM_STORAGE:
    case ITEM_CONVEYOR:
    case ITEM_ASSEMBLER:
    case ITEM_COPPER:
    case ITEM_PLASTIC:
    case ITEM_ALUMINIUM:
    case ITEM_COPPER_WIRE:
    case ITEM_BASIC_CIRCUIT_BOARD:
    case ITEM_ANTENNA:
    case ITEM_COMMUNICATION_COMPONENT:
    case ITEM_WIRE_BUNDLE:
    case ITEM_COGWHEEL:
    case ITEM_SPARE_PARTS:
      return {.max_count = 100};
    case ITEM_OIL_CANISTER:
      return {.max_count = 20};
    case ITEM_BRUSH:
      return {.max_count = 1, .has_durability = true, .max_damage = 20};
    case ITEM_LUBRICANT_CAN:
      return {.max_count = 1, .has_durability = true, .max_damage = 100};
    case ITEM_CALIBRATOR:
      return {.max_count = 1, .has_durability = true, .max_damage = 50};
    case ITEM_COUNT:
      break;
  }
  ASSERT_NO_MSG(false);
}
