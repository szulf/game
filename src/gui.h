#pragma once

#include <span>

#include "core.h"
#include "ui.h"
#include "assets.h"
#include "entity.h"

namespace gui {

bool item_slot(
  const AssetManager& assets,
  ui::Layout& layout,
  const ItemSlot& item_slot,
  bool display_count = true
);
ItemSlotIdx inventory(
  ui::Layout& layout,
  const AssetManager& assets,
  EntityId entity_id,
  std::span<const ItemSlot> inv
);

ItemSlotIdx player_inventory(
  ui::Layout& layout,
  const AssetManager& assets,
  EntityStore& store,
  EntityId player_id
);
ItemSlotIdx open_inventory(
  ui::Layout& layout,
  const AssetManager& assets,
  EntityStore& store,
  EntityId player_id
);
void player_hand(
  ui::System& ui_system,
  const AssetManager& assets,
  const Input& input,
  EntityStore& store,
  EntityId player_id
);
void message_sender(
  ui::Layout& layout,
  const RenderTexture& render_texture,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id,
  ResourceMessageQueue& msg_queue,
  u64 game_time
);
ItemSlotIdx message_receiver(
  ui::Layout& layout,
  const RenderTexture& render_texture,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id,
  ResourceMessageQueue& msg_queue
);
ItemSlotIdx assembler(
  ui::Layout& layout,
  const RenderTexture& render_texture,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id
);

}
