#pragma once

#include <span>

#include "core.h"
#include "math.h"
#include "assets.h"
#include "ui.h"
#include "input.h"
#include "entity.h"

// TODO: does this really belong here?
struct ItemSlotIdx {
  EntityId entity{};
  u32 slot_idx{};

  explicit inline operator bool() const {
    return bool(entity);
  }
};

ItemSlotIdx inventory_ui(
  UI_Layout& layout,
  const AssetManager& assets,
  EntityId entity_id,
  std::span<const ItemSlot> inv
);

void system_update_time(u64& min, f32& min_accumulator, f32 dt);
void system_move_player(EntityStore& store, EntityId player_id, Input& input);
void system_open_gui(EntityStore& store, EntityId player_id, const Input& input);
void system_close_gui(EntityStore& store, EntityId player_id, const Input& input);
void system_hand_slot_interactions(
  EntityStore& store,
  EntityId player_id,
  const ItemSlotIdx& hovered_slot,
  const Input& input
);
void system_drop_items(EntityStore& store, EntityId player_id, const Input& input);
ItemSlotIdx system_inventory_uis(
  UI_System& ui_system,
  const AssetManager& assets,
  const Input& input,
  EntityStore& store,
  EntityId player_id
);
void system_message_sender_ui(
  UI_System& ui_system,
  const RenderTexture& render_texture,
  const Input& input,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id,
  ResourceMessageQueue& msg_queue,
  u64 game_time
);
ItemSlotIdx system_message_receiver_ui(
  UI_System& ui_system,
  const RenderTexture& render_texture,
  const Input& input,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id,
  ResourceMessageQueue& msg_queue
);
void system_transfer_resource_messages(
  EntityStore& store,
  EntityId message_receiver_id,
  ResourceMessageQueue& msg_queue,
  u64 game_time
);
ItemSlotIdx system_assembler_ui(
  UI_System& ui_system,
  const RenderTexture& render_texture,
  const Input& input,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id
);
void system_progress_recipes(EntityStore& store, f32 dt);
void system_place_entity(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  Rotation place_rotation
);
void system_remove_entity(EntityStore& store, EntityId player_id, const Input& input);
void system_pickup_item(EntityStore& store, EntityId player_id);
void system_move_items(EntityStore& store, f32 dt);
void system_tunnel_through_worlds(EntityStore& store, EntityId player_id);
void system_apply_maintenance(EntityStore& store);
void system_update_maintenance_minigames(EntityStore& store, const Input& input, f32 dt);
void system_render(EntityStore& store, EntityId player_id, const AssetManager& assets);
