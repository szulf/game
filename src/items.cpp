#include "items.h"

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
    slot.flags ^= ITEM_SLOT_FLAGS_ALL;
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
    case ITEM_CIRCUIT_BOARD:
      return TEXTURE_CIRCUIT_BOARD_ITEM;
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
    case ITEM_SILICON_WAFER:
      return TEXTURE_SILICON_WAFER_ITEM;
    case ITEM_BLANK_BOARD:
      return TEXTURE_BLANK_BOARD_ITEM;
    case ITEM_TRANSISTOR:
      return TEXTURE_TRANSISTOR_ITEM;
    case ITEM_CAPACITOR:
      return TEXTURE_CAPACITOR_ITEM;

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
    case ITEM_CIRCUIT_BOARD:
      return "Circuit Board";
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
    case ITEM_SILICON_WAFER:
      return "Silicon Wafer";
    case ITEM_BLANK_BOARD:
      return "Blank Board";
    case ITEM_TRANSISTOR:
      return "Transistor";
    case ITEM_CAPACITOR:
      return "Capacitor";
    case ITEM_COUNT:
      break;
  }
  ASSERT_NO_MSG(false);
}

ItemInfo item_info(ItemType item) {
  switch (item) {
    case ITEM_BLOCK:
    case ITEM_STORAGE:
    case ITEM_CONVEYOR:
    case ITEM_ASSEMBLER:
    case ITEM_COPPER:
    case ITEM_PLASTIC:
    case ITEM_ALUMINIUM:
    case ITEM_COPPER_WIRE:
    case ITEM_CIRCUIT_BOARD:
    case ITEM_ANTENNA:
    case ITEM_COMMUNICATION_COMPONENT:
    case ITEM_WIRE_BUNDLE:
    case ITEM_COGWHEEL:
    case ITEM_SPARE_PARTS:
    case ITEM_SILICON_WAFER:
    case ITEM_BLANK_BOARD:
    case ITEM_TRANSISTOR:
    case ITEM_CAPACITOR:
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
