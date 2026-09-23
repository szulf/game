#include "items.h"
#include "assets.h"

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
    case ITEM_BALANCER:
      return TEXTURE_BALANCER_ITEM;

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
    case ITEM_BALANCER:
      return "Balancer";
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
    case ITEM_BALANCER:
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

void assign_slot(ItemSlot& to, const ItemSlot& from) {
  to.type   = from.type;
  to.count  = from.count;
  to.damage = from.damage;
}

void swap_slots(ItemSlot& a, ItemSlot& b) {
  ItemSlot temp = a;
  assign_slot(a, b);
  assign_slot(b, temp);
}

void swap_slot_flags(std::span<ItemSlot> inventory) {
  for (auto& slot : inventory) {
    slot.flags ^= ITEM_SLOT_FLAGS_ALL;
  }
}

static constexpr std::array<std::pair<ItemSlotFlag, ItemSlotFlag>, ITEM_TRANSFER_MODE_COUNT>
  TRANSFER_MODE_FLAGS = []() {
    std::array<std::pair<ItemSlotFlag, ItemSlotFlag>, ITEM_TRANSFER_MODE_COUNT> out{};
    out[ITEM_TRANSFER_HAND]    = {ITEM_SLOT_HAND_INPUT, ITEM_SLOT_HAND_OUTPUT};
    out[ITEM_TRANSFER_MACHINE] = {ITEM_SLOT_MACHINE_INPUT, ITEM_SLOT_MACHINE_OUTPUT};
    return out;
  }();

bool transfer_items(ItemSlot& to, ItemSlot& from, ItemTransferMode mode, std::optional<u32> count) {
  if (!count) {
    count = from.count;
  }

  ASSERT(*count < item_info(from.type).max_count, "count bigger than max_count");
  // TODO: should this really be an assert?
  ASSERT(from.count >= *count, "count bigger than from.count");

  if (!from) {
    return true;
  }

  auto [input_flag, output_flag] = TRANSFER_MODE_FLAGS[mode];
  if (!(from.flags & output_flag) || !(to.flags & input_flag)) {
    return false;
  }

  if (!to) {
    to.type   = from.type;
    to.count  = *count;
    to.damage = from.damage;
    from.count -= *count;
    if (from.count == 0) {
      from.damage = 0;
    }
    return true;
  }

  if (to && from && to.type == from.type) {
    auto info = item_info(to.type);
    if (to.count + *count > info.max_count) {
      to.count   = info.max_count;
      from.count = (to.count + from.count) - info.max_count;
      return false;
    } else {
      to.count += *count;
      from.count -= *count;
      return true;
    }
  }

  return false;
}

bool transfer_items(std::span<ItemSlot> inventory, ItemSlot& from, ItemTransferMode mode) {
  for (auto& to : inventory) {
    transfer_items(to, from, mode);
  }

  return !from;
}

bool transfer_items(std::span<ItemSlot> to, std::span<ItemSlot> from, ItemTransferMode mode) {
  for (auto& slot : from) {
    if (!transfer_items(to, slot, mode)) {
      return false;
    }
  }
  return true;
}

void swap_items(ItemSlot& a, ItemSlot& b, ItemTransferMode mode) {
  auto [input_flag, output_flag] = TRANSFER_MODE_FLAGS[mode];

  bool a_input  = a.flags & input_flag;
  bool a_output = a.flags & output_flag;
  bool b_input  = b.flags & input_flag;
  bool b_output = b.flags & output_flag;

  if (a && a_input && b && b_output && a.type == b.type) {
    auto max_count = item_info(a.type).max_count;
    if (a.count + b.count > max_count) {
      b.count = (a.count + b.count) - max_count;
      a.count = max_count;
    } else {
      a.count += b.count;
      b = {};
    }
  } else if (
    (a && a_input && a_output && b && b_input && b_output) || (a && a_output && !b && b_input) ||
    (!a && a_input && b && b_output)
  ) {
    swap_slots(a, b);
  }
}
