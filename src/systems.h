#pragma once

#include "core.h"
#include "assets.h"
#include "input.h"
#include "entity.h"

void system_update_time(u64& min, f32& min_accumulator, f32 dt);
void system_move_player(EntityStore& store, EntityId player_id, Input& input);
void system_open_gui(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  const vec2& mouse_world_pos
);
void system_close_gui(EntityStore& store, EntityId player_id, const Input& input);
void system_hand_slot_interactions(
  EntityStore& store,
  EntityId player_id,
  const ItemSlotIdx& hovered_slot,
  const Input& input
);
void system_drop_items(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  const vec2& mouse_world_pos
);
void system_transfer_resource_messages(
  EntityStore& store,
  EntityId message_receiver_id,
  ResourceMessageQueue& msg_queue,
  u64 game_time
);
void system_progress_recipes(EntityStore& store, f32 dt);
void system_place_entity(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  const vec2& mouse_world_pos,
  Direction place_rotation
);
void system_remove_entity(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  const vec2& mouse_world_pos
);
void system_pickup_item(EntityStore& store, EntityId player_id);
void system_output_items(EntityStore& store, f32 dt);
void system_move_items(EntityStore& store, f32 dt);
void system_tunnel_through_worlds(EntityStore& store, EntityId player_id);
void system_apply_maintenance(EntityStore& store);
void system_update_maintenance_minigames(EntityStore& store, const Input& input, f32 dt);
void system_update_camera(
  Camera2D& camera,
  const Input& input,
  EntityStore& store,
  EntityId player_id,
  const vec2& window_dims
);
// TODO: remove this, its not really a system (?)
void system_render(EntityStore& store, EntityId player_id, const AssetManager& assets);
