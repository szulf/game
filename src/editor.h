#pragma once

#include "core.h"
#include "assets.h"
#include "input.h"
#include "ui.h"
#include "systems.h"
#include "entity.h"

namespace editor {

struct Data {
  World current_world{};
  u32 selected_placeable_idx{};
  EntityId selected_entity_id{};

  ItemSlotIdx selected_inventory_edit_slot{};
};

struct UpdateResult {
  EntityId player_id{};
  EntityId resource_message_receiver_id{};
};

UpdateResult update(Data& editor, EntityStore& store, const Input& input);

struct GUIResult {
  bool save_requested{};
};

GUIResult gui(
  Data& editor,
  EntityStore& store,
  UI_System& ui_system,
  const Input& input,
  const AssetManager& assets
);
void render(Data& editor, EntityStore& store, const AssetManager& assets);

}
