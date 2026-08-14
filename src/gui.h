#pragma once

#include <span>

#include "core.h"
#include "ui.h"
#include "assets.h"
#include "entity.h"

ItemSlotIdx gui_inventory(
  UI_Layout& layout,
  const AssetManager& assets,
  EntityId entity_id,
  std::span<const ItemSlot> inv
);

ItemSlotIdx gui_player_inventory(
  UI_Layout& layout,
  const AssetManager& assets,
  EntityStore& store,
  EntityId player_id
);
ItemSlotIdx gui_open_inventory(
  UI_Layout& layout,
  const AssetManager& assets,
  EntityStore& store,
  EntityId player_id
);
void gui_player_hand(
  UI_System& ui_system,
  const AssetManager& assets,
  const Input& input,
  EntityStore& store,
  EntityId player_id
);
void gui_message_sender(
  UI_Layout& layout,
  const RenderTexture& render_texture,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id,
  ResourceMessageQueue& msg_queue,
  u64 game_time
);
ItemSlotIdx gui_message_receiver(
  UI_Layout& layout,
  const RenderTexture& render_texture,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id,
  ResourceMessageQueue& msg_queue
);
ItemSlotIdx gui_assembler(
  UI_Layout& layout,
  const RenderTexture& render_texture,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id
);
