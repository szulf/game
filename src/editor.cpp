#include "editor.h"

#include "gui.h"

EditorUpdateResult editor_update(Editor& editor, EntityStore& store, const Input& input) {
  EditorUpdateResult result{};
  auto* entity_at_mouse_pos =
    get_entity_at_pos(store, grid_pos(input.mouse_pos), editor.current_world);

  if (input.lmb.down && entity_at_mouse_pos) {
    if (entity_at_mouse_pos->id == editor.selected_entity_id) {
      editor.selected_entity_id = {};
    }
    remove_entity(store, entity_at_mouse_pos->id);
  }

  if (input.rmb.down && !entity_at_mouse_pos) {
    const auto& placeable = PLACEABLE[editor.selected_placeable_idx];
    Entity entity         = placeable;
    entity.pos            = grid_pos(input.mouse_pos);
    entity.world          = editor.current_world;
    auto id               = add_entity(store, entity);
    if (is<Player>(placeable)) {
      result.player_id = id;
    }
    if (is<ResourceMessageReceiver>(placeable)) {
      result.resource_message_receiver_id = id;
    }
  }

  if (input.rmb.pressed() && entity_at_mouse_pos) {
    editor.selected_entity_id = entity_at_mouse_pos->id;
  }
  return result;
}

static void rotation_data_edit_gui(UI_Layout& layout, Entity& entity) {
  auto* rotation = get_rotation(entity);
  ASSERT(rotation, "entity has no rotation to edit");
  bool clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "rotation: ", 20, WHITE);
    ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked});
    {
      ui_text(layout, direction_to_string(*rotation), 20, BLACK);
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
  }
  ui_element_end(layout, {});

  if (clicked) {
    *rotation = Direction((*rotation + 1) % DIR_COUNT);
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

static void maintenance_data_edit_gui(UI_Layout& layout, Entity& entity) {
  auto [maintenance, possible_maintenance] = get_maintenance(entity);
  ASSERT(maintenance, "entity has no maintenance to edit");
  bool clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "maintenance: ", 20, WHITE);
    ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked});
    {
      ui_text(layout, maintenance_name(*maintenance), 20, BLACK);
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
  }
  ui_element_end(layout, {});

  if (clicked) {
    rotate_maintenace(*maintenance, possible_maintenance);
  }
}

static void inventory_data_edit_gui(
  Editor& editor,
  UI_Layout& layout,
  const AssetManager& assets,
  const Input& input,
  Entity& entity
) {
  auto* inventory = get_inventory(entity);
  ASSERT(inventory, "entity has no inventory to edit");

  auto hovered_slot = gui_inventory(layout, assets, entity.id, *inventory);
  // TODO: this is kind of bad, i should get the information about whether
  // it was clicked or not from the inventory_ui function
  // (and this is not the only place im doing it this way)
  if (input.lmb.pressed() && hovered_slot) {
    editor.selected_inventory_edit_slot = hovered_slot;
  }

  if (editor.selected_inventory_edit_slot.entity == entity.id) {
    auto& selected_slot = (*inventory)[editor.selected_inventory_edit_slot.slot_idx];

    ui_element_begin(layout, UI_AUTO_ID);
    {
      ui_text(layout, "selected inventory slot:", 20, WHITE);

      ui_text(layout, "item type:", 15, WHITE);
      ui_element_begin(layout, UI_AUTO_ID);
      {
        static constexpr u32 ROW_SIZE = 8;
        static constexpr u32 ROW_COUNT =
          ITEM_COUNT % ROW_SIZE == 0 ? ITEM_COUNT / ROW_SIZE : (ITEM_COUNT / ROW_SIZE) + 1;

        for (u32 i = 0; i < ROW_COUNT; ++i) {
          ui_element_begin(layout, UI_AUTO_ID);
          for (u32 j = 0; j < ROW_SIZE; ++j) {
            auto idx = i * ROW_SIZE + j;
            if (idx >= ITEM_COUNT) {
              break;
            }

            auto item_type = ItemType(idx);
            bool clicked{};
            Color color = LIGHTGRAY;
            if (selected_slot.type == item_type) {
              color = GRAY;
            }

            ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked});
            {
              const auto& texture = assets.textures[get_texture_type(item_type)];
              ui_element_begin(layout, UI_AUTO_ID);
              ui_element_end(
                layout,
                {.sizing  = {ui_sizing_fixed(texture.width), ui_sizing_fixed(texture.height)},
                 .texture = &texture}
              );
            }
            ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = color});

            if (clicked) {
              selected_slot.type = item_type;
              if (selected_slot.count > item_info(selected_slot.type).max_count) {
                selected_slot.count = item_info(selected_slot.type).max_count;
              }
            }
          }
          ui_element_end(layout, {.child_gap = 2});
        }
      }
      ui_element_end(
        layout,
        {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
         .padding          = ui_padding_all(2),
         .child_gap        = 2}
      );

      ui_text(layout, "count:", 15, WHITE);
      ui_element_begin(layout, UI_AUTO_ID);
      {
        bool dec_clicked{};
        bool inc_clicked{};

        ui_element_begin(layout, UI_AUTO_ID, {.clicked = &dec_clicked});
        ui_text(layout, "-", 10, BLACK);
        ui_element_end(
          layout,
          {.sizing          = {ui_sizing_fixed(16), ui_sizing_fixed(16)},
           .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
           .bg_color        = LIGHTGRAY}
        );

        ui_text(layout, std::format("{}", selected_slot.count), 15, WHITE);

        ui_element_begin(layout, UI_AUTO_ID, {.clicked = &inc_clicked});
        ui_text(layout, "+", 10, BLACK);
        ui_element_end(
          layout,
          {.sizing          = {ui_sizing_fixed(16), ui_sizing_fixed(16)},
           .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
           .bg_color        = LIGHTGRAY}
        );

        if (dec_clicked && selected_slot.count > 0) {
          --selected_slot.count;
        }
        if (inc_clicked && selected_slot.count < item_info(selected_slot.type).max_count) {
          ++selected_slot.count;
        }
      }
      ui_element_end(layout, {.child_gap = 4});

      if (item_info(selected_slot.type).has_durability) {
        ui_text(layout, "damage:", 15, WHITE);
        ui_element_begin(layout, UI_AUTO_ID);
        {
          bool dec_clicked{};
          bool inc_clicked{};

          ui_element_begin(layout, UI_AUTO_ID, {.clicked = &dec_clicked});
          ui_text(layout, "-", 10, BLACK);
          ui_element_end(
            layout,
            {.sizing          = {ui_sizing_fixed(16), ui_sizing_fixed(16)},
             .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
             .bg_color        = LIGHTGRAY}
          );

          ui_text(layout, std::format("{}", selected_slot.damage), 15, WHITE);

          ui_element_begin(layout, UI_AUTO_ID, {.clicked = &inc_clicked});
          ui_text(layout, "+", 10, BLACK);
          ui_element_end(
            layout,
            {.sizing          = {ui_sizing_fixed(16), ui_sizing_fixed(16)},
             .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
             .bg_color        = LIGHTGRAY}
          );

          if (dec_clicked && selected_slot.damage > 0) {
            --selected_slot.damage;
          }
          if (inc_clicked && selected_slot.damage < item_info(selected_slot.type).max_damage) {
            ++selected_slot.damage;
          }
        }
        ui_element_end(layout, {.child_gap = 4});
      }

      ui_text(layout, "flags:", 15, WHITE);
      ui_element_begin(layout, UI_AUTO_ID);
      {
        struct FlagsUiData {
          std::string_view text{};
          ItemSlotFlags flags{};
        };
        static constexpr std::array FLAGS_UI_DATA = std::to_array<FlagsUiData>({
          {.text = "none", .flags = 0},
          {.text = "in", .flags = ITEM_SLOT_INPUT},
          {.text = "out", .flags = ITEM_SLOT_OUTPUT},
          {.text = "in & out", .flags = ITEM_SLOT_INPUT | ITEM_SLOT_OUTPUT},
        });

        for (const auto& data : FLAGS_UI_DATA) {
          bool clicked{};
          Color color = selected_slot.flags == data.flags ? GRAY : LIGHTGRAY;

          ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked});
          ui_text(layout, data.text, 15, BLACK);
          ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = color});

          if (clicked) {
            selected_slot.flags = data.flags;
          }
        }
      }
      ui_element_end(layout, {.child_gap = 4});
    }
    ui_element_end(layout, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL});
  }
}

static void world_tunnel_destination_data_edit_gui(UI_Layout& layout, Entity& entity) {
  auto* tunnel = get_data<WorldTunnel>(entity);
  ASSERT(tunnel, "entity is not of type WorldTunnel");
  bool clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "destination: ", 20, WHITE);
    ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked});
    {
      ui_text(layout, world_to_string(tunnel->to), 20, BLACK);
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
  }
  ui_element_end(layout, {});

  if (clicked) {
    tunnel->to = World((tunnel->to + 1) % WORLD_COUNT);
  }
}

// TODO: move the ifs into the functions?
static void entity_data_edit_gui(
  Editor& editor,
  UI_Layout& layout,
  const AssetManager& assets,
  const Input& input,
  Entity& entity
) {
  if (rotatable(entity)) {
    rotation_data_edit_gui(layout, entity);
  }
  if (has_inventory(entity)) {
    inventory_data_edit_gui(editor, layout, assets, input, entity);
  }
  if (has_maintenance(entity)) {
    maintenance_data_edit_gui(layout, entity);
  }
  if (is<WorldTunnel>(entity)) {
    world_tunnel_destination_data_edit_gui(layout, entity);
  }
}

EditorGUIResult editor_gui(
  Editor& editor,
  EntityStore& store,
  UI_System& ui_system,
  const Input& input,
  const AssetManager& assets
) {
  EditorGUIResult result{};
  bool save_clicked{};

  auto layout = ui_layout_begin("data ui", ui_system, input, {900, 100}, WINDOW_DIMS);
  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "placeables:", 25, WHITE);
    ui_element_begin(layout, UI_AUTO_ID);
    {
      for (u32 i = 0; i < PLACEABLE.size(); ++i) {
        const auto& placeable = PLACEABLE[i];
        bool clicked{};
        Color color = LIGHTGRAY;
        if (i == editor.selected_placeable_idx) {
          color = GRAY;
        }
        ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked});
        {
          const auto& texture = assets.textures[get_texture_type(placeable)];
          ui_element_begin(layout, UI_AUTO_ID);
          ui_element_end(
            layout,
            {.sizing  = {ui_sizing_fixed(texture.width), ui_sizing_fixed(texture.height)},
             .texture = &texture}
          );
        }
        ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = color});

        if (clicked) {
          editor.selected_placeable_idx = i;
        }
      }
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .child_gap = 2});

    bool current_world_clicked{};
    ui_element_begin(layout, UI_AUTO_ID);
    {
      ui_text(layout, "current world: ", 20, WHITE);
      ui_element_begin(layout, UI_AUTO_ID, {.clicked = &current_world_clicked});
      {
        ui_text(layout, world_to_string(editor.current_world), 20, BLACK);
      }
      ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
    }
    ui_element_end(layout, {});
    if (current_world_clicked) {
      editor.current_world = World((editor.current_world + 1) % WORLD_COUNT);
    }

    if (editor.selected_entity_id) {
      auto* selected = get_entity(store, editor.selected_entity_id);
      ASSERT_NO_MSG(selected);
      ui_text(layout, "selected entity data:", 25, WHITE);
      ui_element_begin(layout, UI_AUTO_ID);
      {
        entity_data_edit_gui(editor, layout, assets, input, *selected);
      }
      ui_element_end(layout, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL});
    }

    ui_element_begin(layout, UI_AUTO_ID);
    {
      ui_element_begin(layout, UI_AUTO_ID, {.clicked = &save_clicked});
      ui_text(layout, "SAVE", 25, BLACK);
      ui_element_end(layout, {.bg_color = LIGHTGRAY});
    }
    ui_element_end(
      layout,
      {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
       .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER}}
    );
  }
  ui_element_end(
    layout,
    {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
     .padding          = ui_padding_all(4),
     .child_gap        = 4,
     .bg_color         = BLACK}
  );
  ui_layout_end(layout);

  result.save_requested = save_clicked;
  return result;
}

void editor_render(Editor& editor, EntityStore& store, const AssetManager& assets) {
  render_entities(store, editor.current_world, assets);
}
