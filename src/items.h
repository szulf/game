#pragma once

#include <array>
#include <string_view>
#include <span>

#include "core.h"
#include "assets.h"

enum ItemType {
  // NOTE: requestable items
  ITEM_COPPER,
  // TODO: make plastic not requestable, but instead made from oil?
  ITEM_PLASTIC,
  ITEM_ALUMINIUM,
  ITEM_OIL_CANISTER,
  ITEM_SILICON_WAFER,

  // NOTE: not requestable items
  ITEM_COPPER_WIRE,
  ITEM_BLANK_BOARD,
  ITEM_TRANSISTOR,
  ITEM_CAPACITOR,
  ITEM_CIRCUIT_BOARD,
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
  ITEM_SILICON_WAFER,
});

enum ItemSlotFlag {
  ITEM_SLOT_HAND_INPUT     = 1 << 0,
  ITEM_SLOT_HAND_OUTPUT    = 1 << 1,
  ITEM_SLOT_MACHINE_INPUT  = 1 << 2,
  ITEM_SLOT_MACHINE_OUTPUT = 1 << 3,
};

using ItemSlotFlags = u32;

static constexpr ItemSlotFlags ITEM_SLOT_FLAGS_INPUT =
  ITEM_SLOT_HAND_INPUT | ITEM_SLOT_MACHINE_INPUT;
static constexpr ItemSlotFlags ITEM_SLOT_FLAGS_OUTPUT =
  ITEM_SLOT_HAND_OUTPUT | ITEM_SLOT_MACHINE_OUTPUT;
static constexpr ItemSlotFlags ITEM_SLOT_FLAGS_ALL =
  ITEM_SLOT_HAND_INPUT | ITEM_SLOT_HAND_OUTPUT | ITEM_SLOT_MACHINE_INPUT | ITEM_SLOT_MACHINE_OUTPUT;

// TODO: do i want to destroy items when they reach max damage?

struct ItemSlot {
  ItemSlotFlags flags = ITEM_SLOT_FLAGS_ALL;
  ItemType type{};
  u32 count{};
  // NOTE: ignored for items with item_data(type).has_durability == false
  u32 damage{};

  explicit inline operator bool() const {
    return count > 0;
  }
};

void assign_slot(ItemSlot& to, const ItemSlot& from);
void swap_slots(ItemSlot& a, ItemSlot& b);
void swap_slot_flags(std::span<ItemSlot> inventory);
TextureType get_texture_type(ItemType item);
std::string_view get_item_name(ItemType item);

struct ItemInfo {
  u32 max_count{};
  bool has_durability{};
  // NOTE: used only when has_durability == true
  u32 max_damage{};
};

ItemInfo item_info(ItemType item);
