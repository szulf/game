#pragma once

#include "core.h"
#include "assets.h"
#include "input.h"
#include "ui.h"
#include "entity.h"

struct Editor {
  World current_world{};
  u32 selected_placeable_idx{};
  EntityId selected_entity_id{};

  ItemSlotIdx selected_inventory_edit_slot{};
};

struct EditorUpdateResult {
  EntityId player_id{};
  EntityId resource_message_receiver_id{};
};

EditorUpdateResult editor_update(Editor& editor, EntityStore& store, const Input& input);

struct EditorGUIResult {
  bool save_requested{};
};

EditorGUIResult editor_gui(
  Editor& editor,
  UI_Layout& layout,
  const Input& input,
  EntityStore& store,
  const AssetManager& assets
);
void editor_render(Editor& editor, EntityStore& store, const AssetManager& assets);
