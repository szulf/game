#include "editor.h"

#include "entity.h"
#include "gui.h"
#include "input.h"
#include "items.h"
#include "ui.h"

EditorUpdateResult
editor_update(Editor& editor, EntityStore& store, const Input& input, const vec2& mouse_world_pos) {
  EditorUpdateResult result{};
  auto mouse_grid_pos = grid_pos(mouse_world_pos);
  auto* entity_at_mouse_pos =
    get_entity_at_pos(store, editor.current_world, mouse_grid_pos, CURSOR_DIMS);

  if (input.lmb.down && entity_at_mouse_pos) {
    if (entity_at_mouse_pos->id == editor.selected_entity_id) {
      editor.selected_entity_id = {};
    }
    remove_entity(store, entity_at_mouse_pos->id);
  }

  // TODO: implement place rotation
  if (input.rmb.down) {
    const auto& placeable = PLACEABLE[editor.selected_placeable_idx];
    auto dims             = get_dims(placeable);
    if (!get_entity_at_pos(store, editor.current_world, mouse_grid_pos, dims)) {
      Entity entity = placeable;
      entity.pos    = mouse_grid_pos;
      entity.world  = editor.current_world;
      auto id       = add_entity(store, entity);
      if (is<Player>(placeable)) {
        result.player_id = id;
      }
      if (is<ResourceMessageReceiver>(placeable)) {
        result.resource_message_receiver_id = id;
      }
    }
  }

  if (input.rmb.pressed() && entity_at_mouse_pos) {
    editor.selected_entity_id = entity_at_mouse_pos->id;
  }

  return result;
}

static void conveyor_data_edit_gui(ui::Layout& layout, Entity& entity) {
  auto* conveyor = get_data<Conveyor>(entity);
  ASSERT(conveyor, "entity is not a conveyor");
  bool clicked{};

  UI(layout, {}) {
    ui::text(layout, "to: ", 20, WHITE);
    UI(layout, {.padding = ui::padding_all(2), .bg_color = LIGHTGRAY}, {.clicked = &clicked}) {
      ui::text(layout, direction_to_string(conveyor->to), 20, BLACK);
    }
  }

  if (clicked) {
    conveyor->to = next_direction(conveyor->to);
  }
}

static void rotation_data_edit_gui(ui::Layout& layout, Entity& entity) {
  auto* rotation = get_rotation(entity);
  ASSERT(rotation, "entity has no rotation to edit");
  bool clicked{};

  UI(layout, {}) {
    ui::text(layout, "rotation: ", 20, WHITE);
    UI(layout, {.padding = ui::padding_all(2), .bg_color = LIGHTGRAY}, {.clicked = &clicked}) {
      ui::text(layout, direction_to_string(*rotation), 20, BLACK);
    }
  }

  if (clicked) {
    *rotation = next_direction(*rotation);
  }
}

static void
rotate_maintenace(Maintenance& maintenance, std::span<const Maintenance> possible_maintenances) {
  if (std::holds_alternative<std::monostate>(maintenance)) {
    maintenance = possible_maintenances[0];
  } else {
    for (u32 i = 0; i < possible_maintenances.size(); ++i) {
      const auto& possible_maintenance = possible_maintenances[i];
      if (i == possible_maintenances.size() - 1) {
        maintenance = std::monostate{};
        break;
      } else if (maintenance.index() == possible_maintenance.index()) {
        maintenance = possible_maintenances[i + 1];
        break;
      }
    }
  }
}

static void maintenance_data_edit_gui(ui::Layout& layout, Entity& entity) {
  auto [maintenance, possible_maintenance] = get_maintenance(entity);
  ASSERT(maintenance, "entity has no maintenance to edit");
  bool clicked{};

  UI(layout, {}) {
    ui::text(layout, "maintenance: ", 20, WHITE);
    UI(layout, {.padding = ui::padding_all(2), .bg_color = LIGHTGRAY}, {.clicked = &clicked}) {
      ui::text(layout, maintenance_name(*maintenance), 20, BLACK);
    }
  }

  if (clicked) {
    rotate_maintenace(*maintenance, possible_maintenance);
  }
}

static u32 value_button_gui(ui::Layout& layout, const Input& input, std::string_view text) {
  bool clicked{};

  UI(
    layout,
    {
      .sizing          = {ui::sizing_fixed(16), ui::sizing_fixed(16)},
      .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
      .bg_color        = LIGHTGRAY,
    },
    {.clicked = &clicked}
  ) {
    ui::text(layout, text, 10, BLACK);
  }

  if (clicked) {
    if (input.keys[GKEY_LSHIFT].down) {
      return 10;
    }
    return 1;
  }
  return 0;
}

static void
item_edit_gui(ui::Layout& layout, const Input& input, const AssetManager& assets, ItemSlot& slot) {
  UI(layout, {.layout_direction = ui::LAYOUT_DIRECTION_VERTICAL}) {
    ui::text(layout, "selected inventory slot:", 20, WHITE);

    ui::text(layout, "item type:", 15, WHITE);
    UI(
      layout,
      {
        .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
        .padding          = ui::padding_all(2),
        .child_gap        = 2,
      }
    ) {
      static constexpr u32 ROW_SIZE = 8;
      static constexpr u32 ROW_COUNT =
        ITEM_COUNT % ROW_SIZE == 0 ? ITEM_COUNT / ROW_SIZE : (ITEM_COUNT / ROW_SIZE) + 1;

      for (u32 i = 0; i < ROW_COUNT; ++i) {
        UI(layout, {.child_gap = 2}) {
          for (u32 j = 0; j < ROW_SIZE; ++j) {
            auto idx = i * ROW_SIZE + j;
            if (idx >= ITEM_COUNT) {
              break;
            }

            auto item_type = ItemType(idx);
            bool clicked{};
            Color color = LIGHTGRAY;
            if (slot.type == item_type) {
              color = GRAY;
            }

            UI(layout, {.padding = ui::padding_all(2), .bg_color = color}, {.clicked = &clicked}) {
              const auto& texture = assets.textures[get_texture_type(item_type)];
              ui::texture(layout, texture);
            }

            if (clicked) {
              slot.type = item_type;
              if (slot.count > item_info(slot.type).max_count) {
                slot.count = item_info(slot.type).max_count;
              }
            }
          }
        }
      }
    }

    ui::text(layout, "count:", 15, WHITE);
    UI(layout, {.child_gap = 4}) {
      u32 decrement = value_button_gui(layout, input, "-");
      if (i32(slot.count) - i32(decrement) >= 0) {
        slot.count -= decrement;
      }

      ui::text(layout, std::format("{}", slot.count), 15, WHITE);

      u32 increment = value_button_gui(layout, input, "+");
      if (slot.count + increment <= item_info(slot.type).max_count) {
        slot.count += increment;
      }
    }

    if (item_info(slot.type).has_durability) {
      ui::text(layout, "damage:", 15, WHITE);
      UI(layout, {.child_gap = 4}) {
        u32 decrement = value_button_gui(layout, input, "-");
        if (i32(slot.damage) - i32(decrement) >= 0) {
          slot.damage -= decrement;
        }

        ui::text(layout, std::format("{}", slot.damage), 15, WHITE);

        u32 increment = value_button_gui(layout, input, "+");
        if (slot.damage + increment <= item_info(slot.type).max_damage) {
          slot.damage += increment;
        }
      }
    }

    ui::text(layout, "flags:", 15, WHITE);
    UI(layout, {.child_gap = 4}) {
      struct FlagsUiData {
        std::string_view text{};
        ItemSlotFlag flag{};
      };
      static constexpr std::array FLAGS_UI_DATA = std::to_array<FlagsUiData>({
        {.text = "hand in", .flag = ITEM_SLOT_HAND_INPUT},
        {.text = "hand out", .flag = ITEM_SLOT_HAND_OUTPUT},
        {.text = "mach in", .flag = ITEM_SLOT_MACHINE_INPUT},
        {.text = "mach out", .flag = ITEM_SLOT_MACHINE_OUTPUT},
      });

      for (const auto& data : FLAGS_UI_DATA) {
        bool clicked{};
        Color color = slot.flags & data.flag ? GRAY : LIGHTGRAY;

        UI(layout, {.padding = ui::padding_all(2), .bg_color = color}, {.clicked = &clicked}) {
          ui::text(layout, data.text, 15, BLACK);
        }

        if (clicked) {
          slot.flags ^= data.flag;
        }
      }
    }
  }
}

static void mover_data_edit_gui(
  Editor& editor,
  ui::Layout& layout,
  const Input& input,
  const AssetManager& assets,
  Entity& entity
) {
  auto props = get_moves_items_properties(entity);
  ASSERT(props, "entity is not a mover");

  UI(layout, {.layout_direction = ui::LAYOUT_DIRECTION_HORIZONTAL}) {
    for (u32 lane_idx = 0; lane_idx < props.from.size(); ++lane_idx) {
      UI(
        layout,
        {
          .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
          .sizing           = {ui::sizing_fill(), ui::sizing_fit()},
        }
      ) {
        for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
          auto& item = props.items[lane_idx, i];

          UI(layout, {.child_gap = 4}) {
            ui::text(layout, std::format("{}.", i), 15, WHITE);

            ui::text(layout, "t: ", 10, WHITE);

            f32 decrement = f32(value_button_gui(layout, input, "-")) * 0.01f;
            if (item.t - decrement >= 0.0f) {
              item.t -= decrement;
            }

            ui::text(layout, std::format("{:.2f}", item.t), 10, WHITE);

            f32 increment = f32(value_button_gui(layout, input, "+")) * 0.01f;
            if (item.t + increment <= conveyor_item_max_t(i)) {
              item.t += increment;
            }

            // TODO: should get whether it was clicked or not from gui::item_slot
            bool hovered = gui::item_slot(assets, layout, item.slot);
            if (hovered && input.lmb.pressed()) {
              // TODO: i dont really like how im encoding the lane_idx here, but ig it works for now
              editor.selected_inventory_edit_slot = {
                .entity   = entity.id,
                .slot_idx = i + (lane_idx * 100),
              };
            }
          }
        }
      }
    }
  }

  if (editor.selected_inventory_edit_slot.entity == entity.id) {
    u32 lane_idx        = editor.selected_inventory_edit_slot.slot_idx / 100;
    u32 slot_idx        = editor.selected_inventory_edit_slot.slot_idx % 100;
    auto& selected_slot = props.items[lane_idx, slot_idx];
    item_edit_gui(layout, input, assets, selected_slot.slot);
  }
}

static void inventory_data_edit_gui(
  Editor& editor,
  ui::Layout& layout,
  const AssetManager& assets,
  const Input& input,
  Entity& entity
) {
  auto* inventory = get_inventory(entity);
  ASSERT(inventory, "entity has no inventory to edit");

  auto hovered_slot = gui::inventory(layout, assets, entity.id, *inventory);
  // TODO: this is kind of bad, i should get the information about whether
  // it was clicked or not from the inventory_ui function
  // (and this is not the only place im doing it this way)
  if (input.lmb.pressed() && hovered_slot) {
    editor.selected_inventory_edit_slot = hovered_slot;
  }

  if (editor.selected_inventory_edit_slot.entity == entity.id) {
    auto& selected_slot = (*inventory)[editor.selected_inventory_edit_slot.slot_idx];
    item_edit_gui(layout, input, assets, selected_slot);
  }
}

static void world_tunnel_destination_data_edit_gui(ui::Layout& layout, Entity& entity) {
  auto* tunnel = get_data<WorldTunnel>(entity);
  ASSERT(tunnel, "entity is not of type WorldTunnel");
  bool clicked{};

  UI(layout, {}) {
    ui::text(layout, "destination: ", 20, WHITE);
    UI(layout, {.padding = ui::padding_all(2), .bg_color = LIGHTGRAY}, {.clicked = &clicked}) {
      ui::text(layout, world_to_string(tunnel->to), 20, BLACK);
    }
  }

  if (clicked) {
    tunnel->to = World((tunnel->to + 1) % WORLD_COUNT);
  }
}

// TODO: move the ifs into the functions?
static void entity_data_edit_gui(
  Editor& editor,
  ui::Layout& layout,
  const AssetManager& assets,
  const Input& input,
  Entity& entity
) {
  if (rotatable(entity)) {
    rotation_data_edit_gui(layout, entity);
  }
  if (is<Conveyor>(entity)) {
    conveyor_data_edit_gui(layout, entity);
  }
  if (has_inventory(entity)) {
    inventory_data_edit_gui(editor, layout, assets, input, entity);
  }
  if (has_maintenance(entity)) {
    maintenance_data_edit_gui(layout, entity);
  }
  if (moves_items(entity)) {
    mover_data_edit_gui(editor, layout, input, assets, entity);
  }
  if (is<WorldTunnel>(entity)) {
    world_tunnel_destination_data_edit_gui(layout, entity);
  }
}

EditorGUIResult editor_gui(
  Editor& editor,
  ui::Layout& layout,
  const Input& input,
  EntityStore& store,
  const AssetManager& assets,
  u64& time
) {
  EditorGUIResult result{};
  bool save_clicked{};

  UI(
    layout,
    {
      .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
      .padding         = {.top = 8, .right = 8},
      .child_alignment = {ui::CHILD_ALIGNMENT_END, ui::CHILD_ALIGNMENT_START},
    }
  ) {
    UI(
      layout,
      {
        .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
        .padding          = ui::padding_all(4),
        .child_gap        = 4,
        .bg_color         = BLACK,
      }
    ) {
      UI(
        layout,
        {
          .padding         = ui::padding_all(2),
          .child_gap       = 2,
          .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
        }
      ) {
        ui::text(layout, "time: ", 25, WHITE);

        u32 decrement = value_button_gui(layout, input, "-");
        if (i32(time) - i32(decrement) >= 0) {
          time -= decrement;
        }

        ui::text(layout, get_time_string(time), 15, WHITE);

        u32 increment = value_button_gui(layout, input, "+");
        time += increment;
      }

      ui::text(layout, "placeables:", 25, WHITE);
      UI(layout, {.padding = ui::padding_all(2), .child_gap = 2}) {
        for (u32 i = 0; i < PLACEABLE.size(); ++i) {
          const auto& placeable = PLACEABLE[i];
          bool clicked{};
          Color color = LIGHTGRAY;
          if (i == editor.selected_placeable_idx) {
            color = GRAY;
          }
          UI(layout, {.padding = ui::padding_all(2), .bg_color = color}, {.clicked = &clicked}) {
            const auto& texture = assets.textures[get_texture_type(placeable)];
            ui::texture(layout, texture);
          }

          if (clicked) {
            editor.selected_placeable_idx = i;
          }
        }
      }

      bool current_world_clicked{};
      UI(layout, {}) {
        ui::text(layout, "current world: ", 20, WHITE);
        UI(
          layout,
          {.padding = ui::padding_all(2), .bg_color = LIGHTGRAY},
          {.clicked = &current_world_clicked}
        ) {
          ui::text(layout, world_to_string(editor.current_world), 20, BLACK);
        }
      }
      if (current_world_clicked) {
        editor.current_world = World((editor.current_world + 1) % WORLD_COUNT);
      }

      if (editor.selected_entity_id) {
        auto* selected = get_entity(store, editor.selected_entity_id);
        if (selected) {
          ui::text(layout, "selected entity data:", 25, WHITE);
          UI(layout, {.layout_direction = ui::LAYOUT_DIRECTION_VERTICAL}) {
            entity_data_edit_gui(editor, layout, assets, input, *selected);
          }
        }
      }

      UI(
        layout,
        {
          .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
          .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
        }
      ) {
        UI(layout, {.bg_color = LIGHTGRAY}, {.clicked = &save_clicked}) {
          ui::text(layout, "SAVE", 25, BLACK);
        }
      }
    }
  }

  result.save_requested = save_clicked;
  return result;
}

void editor_render(Editor& editor, EntityStore& store, const AssetManager& assets) {
  render_entities(store, editor.current_world, assets);
}
