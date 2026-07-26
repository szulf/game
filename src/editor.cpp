namespace editor {

void update(EditorData& editor, EntityStore& store, const Input& input) {
  auto* entity_at_mouse_pos =
    get_entity_at_pos(store, grid_pos(input.mouse_pos), editor.current_world);

  if (input.lmb.pressed() && entity_at_mouse_pos) {
    remove_entity(store, entity_at_mouse_pos->id);
  }

  if (input.rmb.pressed() && !entity_at_mouse_pos) {
    const auto& placeable = PLACEABLE[editor.selected_placeable_idx];
    Entity entity         = placeable;
    entity.pos            = grid_pos(input.mouse_pos);
    entity.world          = editor.current_world;
    add_entity(store, entity);
  }

  if (input.rmb.pressed() && entity_at_mouse_pos) {
    editor.selected_entity_id = entity_at_mouse_pos->id;
  }
}

void entity_data_edit_ui(UI_Layout&, Block&) {}

void entity_data_edit_ui(UI_Layout&, Player&) {}

void entity_data_edit_ui(UI_Layout&, Storage&) {}

void entity_data_edit_ui(UI_Layout& layout, Conveyor& conveyor) {
  bool rotation_clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "rotation: ", 20, WHITE);
    ui_element_begin(layout, UI_AUTO_ID, {.clicked = &rotation_clicked});
    {
      ui_text(layout, direction_to_string(conveyor.rotation), 20, BLACK);
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
  }
  ui_element_end(layout, {});

  if (rotation_clicked) {
    conveyor.rotation = Rotation((i32(conveyor.rotation) + 1) % i32(Rotation::COUNT));
  }
}

void entity_data_edit_ui(UI_Layout&, Item&) {}

void entity_data_edit_ui(UI_Layout& layout, WorldTunnel& tunnel) {
  bool destination_clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "destination: ", 20, WHITE);
    ui_element_begin(layout, UI_AUTO_ID, {.clicked = &destination_clicked});
    {
      ui_text(layout, world_to_string(tunnel.to), 20, BLACK);
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
  }
  ui_element_end(layout, {});

  if (destination_clicked) {
    tunnel.to = World((i32(tunnel.to) + 1) % i32(World::COUNT));
  }
}

template <typename T>
void rotate_maintenace(Maintenance& maintenance) {
  if (std::holds_alternative<std::monostate>(maintenance)) {
    maintenance = T::POSSIBLE_MAINTENANCE[0];
  } else {
    for (u32 i = 0; i < T::POSSIBLE_MAINTENANCE.size(); ++i) {
      const auto& possible_maintenance = T::POSSIBLE_MAINTENANCE[i];
      if (i == T::POSSIBLE_MAINTENANCE.size() - 1) {
        maintenance = std::monostate{};
        break;
      } else if (maintenance.index() == possible_maintenance.index()) {
        maintenance = T::POSSIBLE_MAINTENANCE[i + 1];
        break;
      }
    }
  }
}

void entity_data_edit_ui(UI_Layout& layout, ResourceMessageSender& sender) {
  bool maintenance_clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "maintenance: ", 20, WHITE);
    ui_element_begin(layout, UI_AUTO_ID, {.clicked = &maintenance_clicked});
    {
      ui_text(layout, maintenance_name(sender.maintenance), 20, BLACK);
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
  }
  ui_element_end(layout, {});

  if (maintenance_clicked) {
    rotate_maintenace<ResourceMessageSender>(sender.maintenance);
  }
}

void entity_data_edit_ui(UI_Layout& layout, ResourceMessageReceiver& receiver) {
  bool maintenance_clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "maintenance: ", 20, WHITE);
    ui_element_begin(layout, UI_AUTO_ID, {.clicked = &maintenance_clicked});
    {
      ui_text(layout, maintenance_name(receiver.maintenance), 20, BLACK);
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
  }
  ui_element_end(layout, {});

  if (maintenance_clicked) {
    rotate_maintenace<ResourceMessageReceiver>(receiver.maintenance);
  }
}

void entity_data_edit_ui(UI_Layout& layout, Assembler& assembler) {
  bool maintenance_clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, "maintenance: ", 20, WHITE);
    ui_element_begin(layout, UI_AUTO_ID, {.clicked = &maintenance_clicked});
    {
      ui_text(layout, maintenance_name(assembler.maintenance), 20, BLACK);
    }
    ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
  }
  ui_element_end(layout, {});

  if (maintenance_clicked) {
    rotate_maintenace<Assembler>(assembler.maintenance);
  }
}

void entity_data_edit_ui(UI_Layout& layout, Entity& entity) {
  std::visit(
    [&](auto& data) {
      entity_data_edit_ui(layout, data);
    },
    entity.data
  );
}

void ui(
  EditorData& editor,
  EntityStore& store,
  UI_System& ui_system,
  const Input& input,
  const AssetManager& assets,
  // TODO: i dont like passing the whole state here, but i do need it for serialization,
  // and also passing references to objects inside of the state along side the state itself is icky
  const State& state
) {
  bool save_clicked{};

  auto layout = ui_layout_begin("editor ui", ui_system, input, {900, 100}, WINDOW_DIMS);
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

    if (editor.selected_entity_id) {
      auto* selected = get_entity(store, editor.selected_entity_id);
      ASSERT_NO_MSG(selected);
      ui_text(layout, "selected entity data:", 25, WHITE);
      ui_element_begin(layout, UI_AUTO_ID);
      {
        entity_data_edit_ui(layout, *selected);
      }
      ui_element_end(layout, {});
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

  if (save_clicked) {
    save_state_to_file(state, DEFAULT_MAP_FILEPATH);
    std::println("saved world file to '{}'", DEFAULT_MAP_FILEPATH);
  }
}

}
