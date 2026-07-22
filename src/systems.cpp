static vec2 grid_pos(const vec2& pos) {
  return {std::floor(pos.x / GRID_DIMS.x), std::floor(pos.y / GRID_DIMS.y)};
}

static bool pos_in_radius(const vec2& pos, const vec2& start_pos, f32 radius) {
  auto diff2 = length2(pos - start_pos);
  return diff2 < radius * radius;
}

// TODO: this is bad now, the text is rendered on top of the texture, starting at the textures top
// left corner, it should start at the cells top left corner instead, idk if its a limitation of the
// ui library, or i just dont know how to do it, but yeah
static bool item_slot_icon_ui(AssetManager& assets, UI_Layout& layout, const ItemSlot& item_slot) {
  bool hovered = false;
  if (item_slot) {
    auto& texture = assets.textures[get_texture_type(item_slot.type)];

    ui_element_begin(layout, UI_AUTO_ID, {.hovered = &hovered});
    {
      ui_text(layout, std::format("{}", item_slot.count), 15);
    }
    ui_element_end(
      layout,
      {.sizing  = {ui_sizing_fixed(texture.width), ui_sizing_fixed(texture.height)},
       .texture = &texture}
    );
  }
  return hovered;
}

static bool item_slot_ui(AssetManager& assets, UI_Layout& layout, const ItemSlot& item_slot) {
  bool hovered = false;

  ui_element_begin(layout, UI_AUTO_ID, {.hovered = &hovered});
  {
    // TODO: this is also bad, same reasoning as in item_slot_icon_ui()
#if 0
        bool in = inv[slot_idx].flags == ITEM_SLOT_INPUT;
        if (in) {
          ui_text(layout, "IN ", 10, BLUE);
        }
        bool out = inv[slot_idx].flags == ITEM_SLOT_OUTPUT;
        if (out) {
          ui_text(layout, "OUT ", 10, ORANGE);
        }
#endif

    item_slot_icon_ui(assets, layout, item_slot);
  }
  ui_element_end(
    layout,
    {.sizing          = {ui_sizing_fixed(GRID_DIMS.x), ui_sizing_fixed(GRID_DIMS.y)},
     .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
     .bg_color        = LIGHTGRAY}
  );

  return hovered;
}

static ItemSlotIdx inventory_ui(
  AssetManager& assets,
  UI_Id layout_id,
  UI_System& ui_system,
  const Input& input,
  const vec2& pos,
  EntityId entity_id,
  const std::vector<ItemSlot>& inv
) {
  static constexpr i32 ROW_SIZE = 4;

  ItemSlotIdx hovered_slot{};
  auto layout = ui_layout_begin(layout_id, ui_system, input, pos, WINDOW_DIMS);

  ui_element_begin(layout, UI_AUTO_ID);
  for (u32 i = 0; i < inv.size() / ROW_SIZE; ++i) {
    ui_element_begin(layout, UI_AUTO_ID);
    for (u32 j = 0; j < ROW_SIZE; ++j) {
      auto slot_idx = i * ROW_SIZE + j;
      bool hovered  = item_slot_ui(assets, layout, inv[slot_idx]);
      if (hovered) {
        hovered_slot.entity   = entity_id;
        hovered_slot.slot_idx = slot_idx;
      }
    }
    ui_element_end(layout, {.child_gap = 2});
  }
  ui_element_end(layout, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL, .child_gap = 2});

  ui_layout_end(layout);
  return hovered_slot;
}

void system_update_time(u64& min, f32& min_accumulator, f32 dt) {
  min_accumulator += dt;
  if (min_accumulator > 1.0f) {
    ++min;
    min_accumulator -= 1.0f;
  }
}

void system_move_player(EntityStore& store, EntityId player_id, Input& input) {
  if (input.move == vec2{0, 0}) {
    return;
  }

  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);
  auto collided = get_entities_at_pos(store, player_entity->pos + input.move, player_entity->world);
  bool can_move = true;

  for (auto& collision : collided) {
    emit(store, {.type = EventType::PLAYER_COLLIDED, .entity = collision->id});
    if (solid(*collision)) {
      can_move = false;
    }
  }
  if (can_move) {
    player_entity->pos += input.move;
  }
}

void system_open_gui(EntityStore& store, EntityId player_id, const Input& input) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);

  if (input.interact &&
      pos_in_radius(grid_pos(input.mouse_pos), player_entity->pos, player->interaction_radius)) {
    auto hovered = get_entity_at_pos(store, grid_pos(input.mouse_pos), player_entity->world);
    if (hovered && has_gui(*hovered)) {
      player->open_gui = hovered->id;
    }
  }
}

void system_close_gui(EntityStore& store, EntityId player_id, const Input& input) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);

  if (player->open_gui) {
    auto* gui_entity = get_entity(store, player->open_gui);
    if (input.close_inv ||
        (gui_entity &&
         !pos_in_radius(gui_entity->pos, player_entity->pos, player->interaction_radius)) ||
        (gui_entity && gui_entity->world != player_entity->world) || !gui_entity) {
      player->open_gui = NULL_ENTITY;
    }
  }
}

void system_hand_slot_interactions(
  EntityStore& store,
  EntityId player_id,
  const ItemSlotIdx& hovered_slot,
  const Input& input
) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);

  if (hovered_slot.entity && input.lmb_pressed) {
    auto* hovered_inv = get_inventory(store, hovered_slot.entity);
    if (hovered_inv) {
      auto& slot = (*hovered_inv)[hovered_slot.slot_idx];
      // NOTE: im not checking the flags for the hand slot,
      // i dont see myself ever changing the flags on the player hand
      auto& hand = player->hand;

      if (slot && (slot.flags & ITEM_SLOT_INPUT) && hand && slot.type == hand.type) {
        if (slot.count + hand.count > ITEM_MAX_COUNT) {
          hand.count = (slot.count + hand.count) - ITEM_MAX_COUNT;
          slot.count = ITEM_MAX_COUNT;
        } else {
          slot.count += hand.count;
          hand = {};
        }
      } else if (slot && (slot.flags & ITEM_SLOT_INPUT) && (slot.flags & ITEM_SLOT_OUTPUT) &&
                 hand) {
        swap_slots(slot, hand);
      } else if (slot && (slot.flags & ITEM_SLOT_OUTPUT) && !hand) {
        swap_slots(slot, hand);
      } else if (!slot && (slot.flags & ITEM_SLOT_INPUT) && hand) {
        swap_slots(slot, hand);
      }
    }
  }
}

void system_drop_items(EntityStore& store, EntityId player_id, const Input& input) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);

  // TODO: not sure if lmb_pressed is the right keybind
  if (input.lmb_pressed && player->hand &&
      pos_in_radius(grid_pos(input.mouse_pos), player_entity->pos, player->interaction_radius)) {
    auto hovered = get_entity_at_pos(store, grid_pos(input.mouse_pos), player_entity->world);
    if (!hovered || is<Item>(*hovered)) {
      Entity entity = {
        .pos   = grid_pos(input.mouse_pos),
        .world = player_entity->world,
        .data  = Item{.slot = player->hand},
      };
      add_entity(store, entity);
      player->hand = {};
    }
  }
}

// TODO: split into multiple systems
ItemSlotIdx system_inventory_uis(
  UI_System& ui_system,
  AssetManager& assets,
  const Input& input,
  EntityStore& store,
  EntityId player_id
) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);
  auto hovered_slot = inventory_ui(
    assets,
    "player inventory",
    ui_system,
    input,
    {10, 580},
    player_id,
    player->inventory
  );

  // TODO: will want a different gui for all gui types
  // different systems will handle them too
  // currently just have a different guis for
  // message receiver, storage/world_tunnel, assembler
  if (player->open_gui && !is<ResourceMessageReceiver>(store, player->open_gui) &&
      !is<Assembler>(store, player->open_gui)) {
    auto* open_inv = get_inventory(store, player->open_gui);
    if (open_inv) {
      auto open_inv_hovered_slot = inventory_ui(
        assets,
        "open inventory",
        ui_system,
        input,
        {10, 300},
        player->open_gui,
        *open_inv
      );
      if (open_inv_hovered_slot.entity) {
        hovered_slot = open_inv_hovered_slot;
      }
    }
  }

  // TODO: not sure whether this belongs here
  if (player->hand) {
    auto player_hand_layout =
      ui_layout_begin("player hand", ui_system, input, input.mouse_pos, GRID_DIMS);
    item_slot_icon_ui(assets, player_hand_layout, player->hand);
    ui_layout_end(player_hand_layout);
  }

  return hovered_slot;
}

static bool
message_ui(UI_Layout& layout, AssetManager& assets, const ResourceMessage& msg, u32 idx) {
  static constexpr f32 FONT_SIZE = 15;
  bool cancel_clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    // NOTE: header
    ui_element_begin(layout, UI_AUTO_ID);
    {
      ui_element_begin(layout, UI_AUTO_ID);
      {
        ui_text(layout, std::format("{}.", idx), FONT_SIZE, WHITE);
      }
      ui_element_end(
        layout,
        {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
         .child_alignment = {UI_CHILD_ALIGNMENT_START, UI_CHILD_ALIGNMENT_CENTER}}
      );

      ui_element_begin(layout, UI_AUTO_ID);
      {
        u64 eta_hour = (msg.arrival_time / 60) % 24;
        u64 eta_min  = msg.arrival_time % 60;
        ui_text(layout, std::format("ETA: {}:{}", eta_hour, eta_min), FONT_SIZE, WHITE);
      }
      ui_element_end(
        layout,
        {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
         .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER}}
      );

      ui_element_begin(layout, UI_AUTO_ID);
      {
        ui_element_begin(layout, UI_AUTO_ID, {.clicked = &cancel_clicked});
        {
          ui_text(layout, "cancel", FONT_SIZE, RED);
        }
        ui_element_end(layout, {.bg_color = LIGHTGRAY});
      }
      ui_element_end(
        layout,
        {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
         .child_alignment = {UI_CHILD_ALIGNMENT_END, UI_CHILD_ALIGNMENT_CENTER}}
      );
    }
    ui_element_end(layout, {.sizing = {ui_sizing_fill(), ui_sizing_fit()}});

    ui_element_begin(layout, UI_AUTO_ID);
    {
      ui_text(layout, "requested items:", FONT_SIZE, WHITE);

      for (u32 i = 0; i < ITEM_COUNT; ++i) {
        ItemType item_type     = ItemType(i);
        const auto& item_count = msg.requested_items[i];
        auto& texture          = assets.textures[get_texture_type(item_type)];

        if (item_count == 0) {
          continue;
        }

        ui_element_begin(layout, UI_AUTO_ID);
        {
          ui_element_begin(layout, UI_AUTO_ID);
          {
            ui_element_begin(layout, UI_AUTO_ID);
            ui_element_end(
              layout,
              {.sizing  = {ui_sizing_fixed(texture.width), ui_sizing_fixed(texture.height)},
               .texture = &texture}
            );
            ui_text(layout, get_item_name(item_type), FONT_SIZE, WHITE);
          }
          ui_element_end(
            layout,
            {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
             .child_alignment = {UI_CHILD_ALIGNMENT_START, UI_CHILD_ALIGNMENT_CENTER}}
          );

          ui_element_begin(layout, UI_AUTO_ID);
          {
            ui_text(layout, std::format("amount: {}", item_count), FONT_SIZE, WHITE);
          }
          ui_element_end(
            layout,
            {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
             .child_alignment = {UI_CHILD_ALIGNMENT_END, UI_CHILD_ALIGNMENT_CENTER}}
          );
        }
        ui_element_end(layout, {.sizing = {ui_sizing_fill(), ui_sizing_fit()}});
      }
    }
    ui_element_end(
      layout,
      {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
       .sizing           = {ui_sizing_fill(), ui_sizing_fit()}}
    );
  }
  ui_element_end(
    layout,
    {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL, .sizing = {ui_sizing_fill(), ui_sizing_fit()}
    }
  );

  return cancel_clicked;
}

template <typename T>
static bool message_header_ui(UI_Layout& layout, std::string_view switch_page_text = "") {
  bool switch_page_clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    if constexpr (std::is_same_v<T, ResourceMessageSender>) {
      ui_text(layout, "Message Sender", 30, GREEN);
    } else if constexpr (std::is_same_v<T, ResourceMessageReceiver>) {
      ui_text(layout, "Message Receiver", 30, GREEN);
    } else {
      static_assert(false);
    }

    ui_element_begin(layout, UI_AUTO_ID);
    if (!switch_page_text.empty()) {
      ui_element_begin(layout, UI_AUTO_ID, {.clicked = &switch_page_clicked});
      {
        ui_text(layout, switch_page_text, 50, BLACK);
      }
      // TODO: should this depend on grid dims?
      ui_element_end(
        layout,
        {.sizing          = {ui_sizing_fixed(GRID_DIMS.x), ui_sizing_fixed(GRID_DIMS.y)},
         .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
         .bg_color        = LIGHTGRAY}
      );
    }
    ui_element_end(
      layout,
      {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
       .child_alignment = {UI_CHILD_ALIGNMENT_END, UI_CHILD_ALIGNMENT_START}}
    );
  }
  ui_element_end(layout, {});

  return switch_page_clicked;
}

void maintenance_ui(
  UI_Layout& layout,
  AssetManager& assets,
  Player& player,
  Maintenance& maintenance
) {
  auto fix_item       = maintenance_fix_item(maintenance);
  auto* minigame_open = maintenance_is_minigame_open(maintenance);

  if (minigame_open && *minigame_open) {
    auto& render = player.maintenance_minigame_texture;
    ASSERT(IsRenderTextureValid(render), "minigame render texture needs to be valid");
    vec2 window_offset = ui_element_get_pos(layout, "maintenance minigame window");
    maintenance_render_minigame(maintenance, assets, render, window_offset);
    ui_element_begin(layout, "maintenance minigame window");
    ui_element_end(
      layout,
      {.sizing  = {ui_sizing_fixed(render.texture.width), ui_sizing_fixed(render.texture.height)},
       .texture = &render.texture,
       .flip_texture_vertically = true}
    );
  } else {
    bool fix_clicked{};
    ui_element_begin(layout, UI_AUTO_ID);
    {
      ui_text(layout, std::format("Needs {}!", maintenance_name(maintenance)), 20, RED);

      ui_element_begin(layout, UI_AUTO_ID);
      {
        ui_text(layout, "Fix item: ", 15, WHITE);
        item_slot_ui(assets, layout, fix_item);
      }
      ui_element_end(
        layout,
        {.child_alignment = {UI_CHILD_ALIGNMENT_START, UI_CHILD_ALIGNMENT_CENTER}}
      );

      ui_element_begin(layout, UI_AUTO_ID);
      {
        ui_element_begin(layout, UI_AUTO_ID, {.clicked = &fix_clicked});
        {
          ui_text(layout, "FIX", 15, BLACK);
        }
        ui_element_end(layout, {.padding = ui_padding_all(4), .bg_color = LIGHTGRAY});
      }
      ui_element_end(
        layout,
        {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
         .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER}}
      );
    }
    ui_element_end(layout, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL, .child_gap = 4});

    if (fix_clicked) {
      if (player.hand.type == fix_item.type && player.hand.count >= fix_item.count) {
        auto* minigame_open = maintenance_is_minigame_open(maintenance);
        if (minigame_open) {
          *minigame_open = true;
          // TODO: maybe check if inited in update and init there?
          maintenance_init_minigame(maintenance);
        } else {
          maintenance = std::monostate{};
        }
        player.hand.count -= fix_item.count;
      } else {
        // TODO: notify the user they dont have the item
      }
    }
  }
}

void system_message_sender_ui(
  UI_System& ui_system,
  const Input& input,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id,
  ResourceMessageQueue& msg_queue,
  u64 game_time
) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);
  auto* msg_sender = get_data<ResourceMessageSender>(store, player->open_gui);
  if (!msg_sender) {
    return;
  }

  auto layout = ui_layout_begin("message sender", ui_system, input, {10, 200}, WINDOW_DIMS);
  ui_element_begin(layout, UI_AUTO_ID);
  if (msg_sender->maintenance.index() != 0) {
    maintenance_ui(layout, assets, *player, msg_sender->maintenance);
  } else {
    switch (msg_sender->page) {
      case ResourceMessageSenderPage::DISPLAY: {
        bool switch_page_clicked = message_header_ui<ResourceMessageSender>(layout, "+");

        ui_element_begin(layout, UI_AUTO_ID);
        {
          for (u32 i = 0; i < msg_queue.msgs.size();) {
            bool cancel_clicked = message_ui(layout, assets, msg_queue.msgs[i], i + 1);

            if (cancel_clicked) {
              remove_resource_message(msg_queue, i);
            } else {
              ++i;
            }
          }
        }
        ui_element_end(
          layout,
          {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
           .sizing           = {ui_sizing_fill(), ui_sizing_fit()},
           .child_gap        = 8}
        );

        if (switch_page_clicked) {
          msg_sender->page = ResourceMessageSenderPage::CREATE;
        }
      } break;

      case ResourceMessageSenderPage::CREATE: {
        auto& msg = msg_sender->msg_in_create;
        bool create_clicked{};

        bool switch_page_clicked = message_header_ui<ResourceMessageSender>(layout, "<");

        ui_element_begin(layout, UI_AUTO_ID);
        {
          for (u32 i = 0; i < ITEM_COUNT; ++i) {
            bool add_clicked{};
            bool remove_clicked{};

            ui_element_begin(layout, UI_AUTO_ID);
            {

              ui_element_begin(layout, UI_AUTO_ID);
              {
                auto& texture = assets.textures[get_texture_type(ItemType(i))];
                ui_element_begin(layout, UI_AUTO_ID);
                ui_element_end(
                  layout,
                  {.sizing  = {ui_sizing_fixed(texture.width), ui_sizing_fixed(texture.height)},
                   .texture = &texture}
                );
                ui_text(layout, get_item_name(ItemType(i)), 15, WHITE);
              }
              ui_element_end(
                layout,
                {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
                 .child_alignment = {UI_CHILD_ALIGNMENT_START, UI_CHILD_ALIGNMENT_CENTER}}
              );

              ui_element_begin(layout, UI_AUTO_ID);
              {
                ui_element_begin(layout, UI_AUTO_ID, {.clicked = &add_clicked});
                {
                  ui_text(layout, "+", 20, BLACK);
                }
                ui_element_end(
                  layout,
                  {.sizing =
                     {ui_sizing_fixed(GRID_DIMS.x * 0.5f), ui_sizing_fixed(GRID_DIMS.y * 0.5f)},
                   .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
                   .bg_color        = LIGHTGRAY}
                );

                ui_text(layout, std::format("amount: {}", msg.requested_items[i]), 15, WHITE);

                ui_element_begin(layout, UI_AUTO_ID, {.clicked = &remove_clicked});
                {
                  ui_text(layout, "-", 20, BLACK);
                }
                ui_element_end(
                  layout,
                  {.sizing =
                     {ui_sizing_fixed(GRID_DIMS.x * 0.5f), ui_sizing_fixed(GRID_DIMS.y * 0.5f)},
                   .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
                   .bg_color        = LIGHTGRAY}
                );
              }
              ui_element_end(
                layout,
                {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
                 .child_gap       = 4,
                 .child_alignment = {UI_CHILD_ALIGNMENT_END, UI_CHILD_ALIGNMENT_CENTER}}
              );
            }
            ui_element_end(layout, {.sizing = {ui_sizing_fill(), ui_sizing_fit()}});

            if (add_clicked && msg.requested_items[i] < MAX_REQUESTED_ITEMS) {
              msg.requested_items[i] += REQUESTED_ITEMS_MULTIPLE;
            }

            if (remove_clicked && msg.requested_items[i] > 0) {
              msg.requested_items[i] -= REQUESTED_ITEMS_MULTIPLE;
            }
          }

          ui_element_begin(layout, UI_AUTO_ID);
          {
            ui_element_begin(layout, UI_AUTO_ID, {.clicked = &create_clicked});
            {
              ui_text(layout, "CREATE", 20, BLACK);
            }
            ui_element_end(layout, {.padding = ui_padding_all(2), .bg_color = LIGHTGRAY});
          }
          ui_element_end(
            layout,
            {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
             .child_alignment = {UI_CHILD_ALIGNMENT_END, UI_CHILD_ALIGNMENT_CENTER}}
          );
        }
        ui_element_end(
          layout,
          {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
           .sizing           = {ui_sizing_fill(), ui_sizing_fit()},
           .child_gap        = 2}
        );

        if (switch_page_clicked) {
          msg_sender->page = ResourceMessageSenderPage::DISPLAY;
        }

        if (create_clicked) {
          bool all_zero = true;
          for (u32 i = 0; i < ITEM_COUNT; ++i) {
            if (msg.requested_items[i] > 0) {
              all_zero = false;
              break;
            }
          }
          if (!all_zero) {
            add_resource_message(msg_queue, msg, game_time);
            msg = {};
          }
        }
      } break;
    }
  }
  ui_element_end(
    layout,
    {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
     .padding          = ui_padding_all(4),
     .child_alignment  = {UI_CHILD_ALIGNMENT_START, UI_CHILD_ALIGNMENT_CENTER},
     .bg_color         = BLACK}
  );
  ui_layout_end(layout);
}

ItemSlotIdx system_message_receiver_ui(
  UI_System& ui_system,
  const Input& input,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id,
  ResourceMessageQueue& msg_queue
) {
  auto player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);
  auto* msg_receiver = get_data<ResourceMessageReceiver>(store, player->open_gui);
  if (!msg_receiver) {
    return {};
  }
  ItemSlotIdx hovered_slot{};

  auto layout = ui_layout_begin("message receiver", ui_system, input, {10, 200}, WINDOW_DIMS);
  ui_element_begin(layout, UI_AUTO_ID);
  if (msg_receiver->maintenance.index() != 0) {
    maintenance_ui(layout, assets, *player, msg_receiver->maintenance);
  } else {
    message_header_ui<ResourceMessageReceiver>(layout);

    ui_element_begin(layout, UI_AUTO_ID);
    if (resource_message_receiver_empty(*msg_receiver)) {
      if (msg_queue.msgs.empty()) {
        ui_element_begin(layout, UI_AUTO_ID);
        {
          ui_text(layout, "no arriving messages", 20, WHITE);
          ui_text(layout, "create messages in the Message Sender", 10, WHITE);
        }
        ui_element_end(layout, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL});
      } else {
        ui_text(layout, "next arriving messages:", 20, WHITE);
        auto first_batch = get_first_resource_message_batch(msg_queue);
        for (u32 i = 0; i < first_batch.size(); ++i) {
          auto& msg = first_batch[i];
          message_ui(layout, assets, msg, i + 1);
        }
      }
    } else {
      ui_text(layout, "currently available items:", 20, WHITE);
      ui_element_begin(layout, UI_AUTO_ID);
      for (u32 i = 0; i < msg_receiver->inventory.size(); ++i) {
        auto& slot   = msg_receiver->inventory[i];
        bool hovered = item_slot_ui(assets, layout, slot);
        if (hovered) {
          hovered_slot.entity   = player->open_gui;
          hovered_slot.slot_idx = i;
        }
      }
      ui_element_end(layout, {.child_gap = 2});
    }
    ui_element_end(
      layout,
      {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
       .sizing           = {ui_sizing_fill(), ui_sizing_fit()},
       .child_gap        = 8}
    );
  }
  ui_element_end(
    layout,
    {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
     .padding          = ui_padding_all(4),
     .child_alignment  = {UI_CHILD_ALIGNMENT_START, UI_CHILD_ALIGNMENT_CENTER},
     .bg_color         = BLACK}
  );
  ui_layout_end(layout);

  return hovered_slot;
}

// NOTE: returns whether it succeeded in transfering all items from the slot into the inventory
// also modified the slot to contain the left amount of items after the transfer
// so if it succeeded slot.count == 0
static bool transfer_items(std::vector<ItemSlot>& inventory, ItemSlot& slot) {
  if (!(slot.flags & ITEM_SLOT_OUTPUT)) {
    return false;
  }

  for (u32 i = 0; i < inventory.size(); ++i) {
    if (!(inventory[i].flags & ITEM_SLOT_INPUT)) {
      continue;
    }

    if (inventory[i].type == slot.type) {
      if (inventory[i].count + slot.count > ITEM_MAX_COUNT) {
        slot.count         = (inventory[i].count + slot.count) - ITEM_MAX_COUNT;
        inventory[i].count = ITEM_MAX_COUNT;
      } else {
        inventory[i].count += slot.count;
        slot.count = 0;
        return true;
      }
    } else if (!inventory[i]) {
      swap_slots(inventory[i], slot);
      return true;
    }
  }
  return false;
}

static bool transfer_items(std::vector<ItemSlot>& to, std::span<ItemSlot> from) {
  for (auto& slot : from) {
    if (!transfer_items(to, slot)) {
      return false;
    }
  }
  return true;
}

// NOTE: currently voiding items that cannot fit into the message receivers inventory
// is that really the behaviour i want?
void system_transfer_resource_messages(
  EntityStore& store,
  EntityId message_receiver_id,
  ResourceMessageQueue& msg_queue,
  u64 game_time
) {
  auto* msg_receiver = get_data<ResourceMessageReceiver>(store, message_receiver_id);
  ASSERT_NO_MSG(msg_receiver);

  // NOTE: dont need to care about batches here,
  // the max requested item count is already guaranteed here
  // and all of the messages in a single batch have the same arrival_time
  // so i can just treat them as separate units
  for (u32 i = 0; i < msg_queue.msgs.size();) {
    auto& msg = msg_queue.msgs[i];
    if (msg.arrival_time == game_time) {
      std::array<ItemSlot, ITEM_COUNT> msg_items{};
      for (u32 item_type = 0; item_type < ITEM_COUNT; ++item_type) {
        msg_items[item_type] = {
          .type  = ItemType(item_type),
          .count = msg.requested_items[item_type],
        };
      }
      swap_slot_flags(msg_receiver->inventory);
      transfer_items(msg_receiver->inventory, msg_items);
      swap_slot_flags(msg_receiver->inventory);
      remove_resource_message(msg_queue, i);
    } else {
      ++i;
    }
  }
}

bool recipe_button_ui(UI_Layout& layout, std::string_view recipe_name, bool selected) {
  bool clicked{};
  ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked});
  {
    ui_text(layout, recipe_name, 15, BLACK);
  }
  ui_element_end(layout, {.padding = ui_padding_all(4), .bg_color = selected ? GRAY : LIGHTGRAY});
  return clicked;
}

ItemSlotIdx system_assembler_ui(
  UI_System& ui_system,
  const Input& input,
  AssetManager& assets,
  EntityStore& store,
  EntityId player_id
) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);
  if (!player->open_gui) {
    return {};
  }

  auto* assembler = get_data<Assembler>(store, player->open_gui);
  if (!assembler) {
    return {};
  }

  ItemSlotIdx hovered{};
  auto layout = ui_layout_begin("assembler", ui_system, input, {10, 200}, WINDOW_DIMS);
  ui_element_begin(layout, UI_AUTO_ID);
  if (assembler->maintenance.index() != 0) {
    maintenance_ui(layout, assets, *player, assembler->maintenance);
  } else {
    ui_element_begin(layout, UI_AUTO_ID);
    for (u32 i = 0; i < Assembler::RECIPES.size(); ++i) {
      const auto& recipe = Assembler::RECIPES[i];
      bool clicked = recipe_button_ui(layout, recipe.name, assembler->selected_recipe_idx == i);
      if (clicked && assembler->selected_recipe_idx != i) {
        assembler->selected_recipe_idx = i;
        // TODO: pull out to a clear or something function
        assembler->t = 0;
      }
    }
    ui_element_end(layout, {.child_gap = 4});

    const auto& selected_recipe = Assembler::RECIPES[assembler->selected_recipe_idx];
    ui_text(layout, "Recipe:", 15, WHITE);
    ui_element_begin(layout, UI_AUTO_ID);
    {
      for (u32 i = 0; i < Recipe::MAX_INPUT_SLOTS; ++i) {
        if (selected_recipe.input_slots[i]) {
          item_slot_ui(assets, layout, selected_recipe.input_slots[i]);
        }
      }
      ui_text(layout, "->", 20, WHITE);
      for (u32 i = 0; i < Recipe::MAX_OUTPUT_SLOTS; ++i) {
        if (selected_recipe.output_slots[i]) {
          item_slot_ui(assets, layout, selected_recipe.output_slots[i]);
        }
      }
    }
    ui_element_end(
      layout,
      {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
       .child_gap       = 4,
       .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER}}
    );

    ui_text(layout, "Inventory:", 15, WHITE);
    ui_element_begin(layout, UI_AUTO_ID);
    {
      for (u32 i = 0; i < Recipe::MAX_INPUT_SLOTS; ++i) {
        if (selected_recipe.input_slots[i]) {
          bool slot_hovered = item_slot_ui(assets, layout, assembler_input_slot(*assembler, i));
          if (slot_hovered) {
            hovered.entity   = player->open_gui;
            hovered.slot_idx = i;
          }
        }
      }
      ui_text(layout, "->", 20, WHITE);
      for (u32 i = 0; i < Recipe::MAX_OUTPUT_SLOTS; ++i) {
        if (selected_recipe.output_slots[i]) {
          bool slot_hovered = item_slot_ui(assets, layout, assembler_output_slot(*assembler, i));
          if (slot_hovered) {
            hovered.entity = player->open_gui;
            // TODO: dont like this addition here (it was supposed to be an implementation detail)
            hovered.slot_idx = i + Recipe::MAX_INPUT_SLOTS;
          }
        }
      }
    }
    ui_element_end(
      layout,
      {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
       .child_gap       = 4,
       .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER}}
    );

    ui_text(layout, std::format("{:.1f}/{}", assembler->t, selected_recipe.recipe_time), 20, WHITE);
  }
  ui_element_end(
    layout,
    {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
     .padding          = ui_padding_all(4),
     .child_gap        = 4,
     .bg_color         = BLACK}
  );
  ui_layout_end(layout);

  return hovered;
}

void system_progress_recipes(EntityStore& store, f32 dt) {
  for (auto& entity : store) {
    auto* assembler = get_data<Assembler>(entity);
    if (!assembler) {
      continue;
    }

    if (assembler->maintenance.index() != 0) {
      continue;
    }

    auto& selected_recipe = Assembler::RECIPES[assembler->selected_recipe_idx];
    bool inputs_ok        = true;
    // TODO: this shouldnt really care about the ordering of the items
    // 5. if you have two conveyors on the side pushing into a conveyor in the middle,
    //    they wont do a nice split between them, just one will push all the items then the next one
    // or maybe it should, but i could add a way to lock item slots to only a specific kind
    for (u32 i = 0; i < Recipe::MAX_INPUT_SLOTS; ++i) {
      auto& recipe_input = selected_recipe.input_slots[i];
      if (!recipe_input) {
        continue;
      }
      auto& assembler_input = assembler_input_slot(*assembler, i);
      if (assembler_input.type != recipe_input.type || assembler_input.count < recipe_input.count) {
        inputs_ok = false;
        break;
      }
    }

    bool output_ok = true;
    for (u32 i = 0; i < Recipe::MAX_OUTPUT_SLOTS; ++i) {
      auto& recipe_output = selected_recipe.output_slots[i];
      if (!recipe_output) {
        continue;
      }
      auto& assembler_output = assembler_output_slot(*assembler, i);
      if (assembler_output && assembler_output.type != recipe_output.type) {
        output_ok = false;
        break;
      }
      if (assembler_output.count + recipe_output.count > ITEM_MAX_COUNT) {
        output_ok = false;
        break;
      }
    }

    if (inputs_ok) {
      if (output_ok) {
        assembler->t += dt;
      }
    } else {
      assembler->t = 0;
    }

    if (assembler->t >= selected_recipe.recipe_time) {
      for (u32 i = 0; i < Recipe::MAX_INPUT_SLOTS; ++i) {
        auto& recipe_input = selected_recipe.input_slots[i];
        if (!recipe_input) {
          continue;
        }
        auto& assembler_input = assembler_input_slot(*assembler, i);
        assembler_input.count -= recipe_input.count;
      }
      for (u32 i = 0; i < Recipe::MAX_OUTPUT_SLOTS; ++i) {
        auto& recipe_output = selected_recipe.output_slots[i];
        if (!recipe_output) {
          continue;
        }
        auto& assembler_output = assembler_output_slot(*assembler, i);
        assembler_output.type  = recipe_output.type;
        assembler_output.count += recipe_output.count;
      }
      assembler->t -= selected_recipe.recipe_time;
    }
  }
}

void system_place_entity(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  Rotation place_rotation
) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);

  // TODO: should check if im not hovering over an item slot
  if (input.rmb_pressed && player->hand &&
      pos_in_radius(grid_pos(input.mouse_pos), player_entity->pos, player->interaction_radius) &&
      !get_entity_at_pos(store, grid_pos(input.mouse_pos), player_entity->world)) {
    auto entity  = entity_from_item(player->hand.type);
    entity.pos   = grid_pos(input.mouse_pos);
    entity.world = player_entity->world;
    if (auto* rotation = get_rotation(entity)) {
      *rotation = place_rotation;
    }
    add_entity(store, entity);
    --player->hand.count;
  }
}

void system_remove_entity(EntityStore& store, EntityId player_id, const Input& input) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);

  if (input.lmb_pressed &&
      pos_in_radius(grid_pos(input.mouse_pos), player_entity->pos, player->interaction_radius)) {
    auto hovered = get_entity_at_pos(store, grid_pos(input.mouse_pos), player_entity->world);
    if (hovered && breakable(*hovered)) {
      auto item_type = entity_to_item(*hovered);
      ASSERT(item_type, "broken breakable item doesnt have an item_type");

      Entity entity = {
        .pos   = hovered->pos,
        .world = player_entity->world,
        .data  = Item{.slot = {.type = *item_type, .count = 1}},
      };
      add_entity(store, entity);

      for_each_active_slot(*hovered, [&](const ItemSlot& slot) {
        Entity item_entity = {
          .pos   = hovered->pos,
          .world = player_entity->world,
          .data  = Item{.slot = slot},
        };
        add_entity(store, item_entity);
      });

      remove_entity(store, hovered->id);
    }
  }
}

void system_pickup_item(EntityStore& store, EntityId player_id) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);

  for (auto& event : listen(store, EventType::PLAYER_COLLIDED)) {
    auto* item = get_data<Item>(store, event.entity);
    if (item) {
      if (transfer_items(player->inventory, item->slot)) {
        remove_entity(store, event.entity);
      }
    }
  }
}

static ItemSlot* find_first_extractable_slot(std::vector<ItemSlot>& inventory) {
  for (u32 i = 0; i < inventory.size(); ++i) {
    if (inventory[i] && (inventory[i].flags & ITEM_SLOT_OUTPUT)) {
      return &inventory[i];
    }
  }
  return nullptr;
}

// TODO: currently moving items, fuckin sucks actually
// 1. its too dependent on the ordering of conveyors in EntityStore
//    so it gets choppy if conveyors are not stored in the right order
//    possible solutions (i should implement this shit before finishing):
//    - double buffering (reminds me of my game-of-life)
//      update everything based on this frames step into a second buffer then swap them
//    - build graphs of conveyor chains, sort them, then update in order
//      (this fucking sucks actually (but sounds like the coolest thing ever))
//    - something like a update push system, not updating all conveyors in a frame,
//      not sure how this one works saw it briefly mentioned on youtube
// 2. items are moved faster if they are moving through containers all the time
//    so this
//    c-c-c-c-c-c
//    is faster than this
//    -----------
//    (where 'c' is a container, and '-' is a belt)
//    (by about 2x i think)
// 3. dont know if its actually bad
//    but if you have more than a single conveyor pulling items out of an inventory
//    and also a single conveyor pushing items into that inventory
//    they will not round robin
//    just one of the pulling conveyors will take all items as they are coming
// 4. i think i duped an item somehow, no clue how tho (potentially fixable by fixing 1.)
// 5. if you have two conveyors on the side pushing into a conveyor in the middle,
//    they wont do a nice split between them, just one will push all the items then the next one
void system_move_items(EntityStore& store, f32 dt) {
  for (auto& entity : store) {
    auto* conveyor = get_data<Conveyor>(entity);
    if (!conveyor) {
      continue;
    }
    f32 item_gap = 1.0f / CONVEYOR_THROUGHPUT;

    // NOTE: move items that are already on the conveyor
    for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
      auto& item = conveyor->items[i];
      if (item.slot) {
        if (item.t < 1 - (i * item_gap)) {
          item.t += dt;
        }
      } else {
        item.t = 0;
      }
    }

    // NOTE: pull in more items
    for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
      auto& item    = conveyor->items[i];
      bool can_pull = !item.slot;
      if (i > 0) {
        auto& previous_item = conveyor->items[i - 1];
        can_pull            = can_pull && previous_item.t >= item_gap;
      }
      if (can_pull) {
        vec2 from_pos     = entity.pos + direction_to_vec2(conveyor_from(*conveyor));
        auto* from_entity = get_entity_at_pos(store, from_pos, entity.world);
        if (from_entity && has_inventory(*from_entity) && !is<Player>(*from_entity)) {
          auto* from_inv = get_inventory(*from_entity);
          ASSERT(
            from_inv,
            "entity that satisifies HasInventory has to return an inventory from "
            "get_inventory()"
          );
          auto* first_extractable = find_first_extractable_slot(*from_inv);
          if (first_extractable) {
            item.slot.type  = first_extractable->type;
            item.slot.count = 1;
            --first_extractable->count;
          }
        }
        break;
      }
    }

    // NOTE: push items off
    {
      auto& item = conveyor->items[0];
      if (item.t >= 1) {
        vec2 to_pos     = entity.pos + direction_to_vec2(conveyor_to(*conveyor));
        auto* to_entity = get_entity_at_pos(store, to_pos, entity.world);
        if (to_entity && !is<Player>(*to_entity)) {
          bool success = false;

          if (auto* to_conveyor = get_data<Conveyor>(*to_entity)) {
            for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
              auto& to_item = to_conveyor->items[i];
              if (!to_item.slot) {
                swap_slots(item.slot, to_item.slot);
                to_item.t = 0;
                success   = true;
                break;
              }
            }
          } else if (auto* to_inv = get_inventory(*to_entity)) {
            success = transfer_items(*to_inv, item.slot);
          }

          if (success) {
            std::ranges::rotate(conveyor->items, conveyor->items.begin() + 1);
          }
        }
      }
    }
  }
}

Entity* find_corresponding_world_tunnel(EntityStore& store, Entity& tunnel_entity) {
  auto* tunnel = get_data<WorldTunnel>(tunnel_entity);
  ASSERT(tunnel, "cannot find corresponding world tunnel of none world tunnel entity");
  for (auto& entity : store) {
    if (entity.world == tunnel->to) {
      auto* tunnel = get_data<WorldTunnel>(entity);
      if (tunnel && tunnel->to == tunnel_entity.world) {
        return &entity;
      }
    }
  }
  return nullptr;
}

// NOTE: assumes there is always a 1-1 mapping of tunnels between worlds
// TODO: not sure whether i want to travel via interaction or via walk into
void system_tunnel_through_worlds(EntityStore& store, EntityId player_id) {
  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);

  // NOTE: player
  for (auto& event : listen(store, EventType::PLAYER_COLLIDED)) {
    auto* tunnel = get_data<WorldTunnel>(store, event.entity);
    if (tunnel) {
      player_entity->world = tunnel->to;
    }
  }

  // NOTE: items
  for (auto& entity : store) {
    if (auto* tunnel = get_data<WorldTunnel>(entity)) {
      auto* corresponding_tunnel_entity = find_corresponding_world_tunnel(store, entity);
      ASSERT(corresponding_tunnel_entity, "there should always be a corresponding tunnel");
      auto* corresponding_tunnel = get_data<WorldTunnel>(*corresponding_tunnel_entity);
      ASSERT_NO_MSG(corresponding_tunnel);

      swap_slot_flags(corresponding_tunnel->inventory);
      swap_slot_flags(tunnel->inventory);
      transfer_items(corresponding_tunnel->inventory, tunnel->inventory);
      swap_slot_flags(corresponding_tunnel->inventory);
      swap_slot_flags(tunnel->inventory);
    }
  }
}

void system_apply_maintenance(EntityStore& store) {
  for (auto& entity : store) {
    auto [maintenance, possible_maintenance] = get_maintenance(entity);
    if (!maintenance || maintenance->index() != 0) {
      continue;
    }

    // TODO: this needs a much much lower chance to happen
    auto value = random_get<u32>(1, 100);
    if (value != 1) {
      continue;
    }

    auto maintenance_idx = random_get<u32>(0, possible_maintenance.size() - 1);
    *maintenance         = possible_maintenance[maintenance_idx];
    std::println("Maintenance needs happened!");
    std::println("Current maintenance: {}", maintenance_name(*maintenance));
  }
}

void system_update_maintenance_minigames(EntityStore& store, const Input& input, f32 dt) {
  for (auto& entity : store) {
    auto [maintenance, _] = get_maintenance(entity);
    if (!maintenance) {
      continue;
    }

    auto* minigame_open = maintenance_is_minigame_open(*maintenance);
    if (!minigame_open || !*minigame_open) {
      continue;
    }

    // TODO: should only apply mouse inputs if the player hand is empty
    bool done = maintenance_update_minigame(*maintenance, input, dt);
    if (done) {
      *maintenance = std::monostate{};
    }
  }
}

// TODO: not sure if this is a system
void system_serialization(State& state, const std::filesystem::path& filepath) {
  auto& input = state.tick_input;
  if (input.serialize) {
    json j(state);
    std::ofstream file{filepath};
    file << std::setw(4) << j << '\n';
  }

  if (input.deserialize) {
    std::ifstream file{filepath};
    json j         = json::parse(file);
    auto new_state = j.get<State>();

    // TODO: pull this state assigning out to a separate function?
    state.frame_input            = {};
    state.tick_input             = {};
    state.frame                  = {};
    state.ui_system              = {};
    state.minutes_accumulator    = {};
    state.current_place_rotation = {};
    state.debug                  = {};

    state.minutes                      = new_state.minutes;
    state.resource_message_queue       = new_state.resource_message_queue;
    state.player_id                    = new_state.player_id;
    state.resource_message_receiver_id = new_state.resource_message_receiver_id;
    state.store                        = new_state.store;
  }
}

void system_render(EntityStore& store, EntityId player_id, const AssetManager& assets) {
  static constexpr f32 ON_CONVEYOR_SCALE = 0.375f;

  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);

  for (auto& entity : store) {
    if (entity.world != player_entity->world) {
      continue;
    }

    const Texture2D* texture{};
    if (auto* item = get_data<Item>(entity)) {
      texture = &assets.textures[get_texture_type(item->slot.type)];
    } else {
      texture = &assets.textures[get_texture_type(entity)];
    }
    vec2 dims = dims_from_texture(*texture);

    auto source_rect    = rect_from_vec2x2({}, dims);
    Rectangle dest_rect = {
      .x      = (entity.pos.x * GRID_DIMS.x) + ((GRID_DIMS.x - dims.x) / 2.0f) + (dims.x * 0.5f),
      .y      = (entity.pos.y * GRID_DIMS.y) + ((GRID_DIMS.y - dims.y) / 2.0f) + (dims.y * 0.5f),
      .width  = dims.x,
      .height = dims.y,
    };

    auto origin = vector2_from_vec2(dims * 0.5f);

    f32 rotation = 0;
    if (auto* rot = get_rotation(entity)) {
      rotation = rotation_degrees(*rot);
    }

    DrawTexturePro(*texture, source_rect, dest_rect, origin, rotation, WHITE);

    if (auto* conveyor = get_data<Conveyor>(entity)) {
      for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
        auto& item = conveyor->items[i];
        if (item.slot) {
          auto& on_texture  = assets.textures[get_texture_type(item.slot.type)];
          vec2 on_dims      = dims_from_texture(on_texture);
          Vector2 on_origin = vector2_from_vec2(on_dims) * 0.5f * ON_CONVEYOR_SCALE;

          auto on_source_rect = rect_from_vec2x2({}, on_dims);

          Rectangle on_dest_rect = {
            .x = (entity.pos.x * GRID_DIMS.x) + ((GRID_DIMS.x - on_dims.x) / 2.0f) +
                 (on_dims.x * 0.5f),
            .y = (entity.pos.y * GRID_DIMS.y) + ((GRID_DIMS.y - on_dims.y) / 2.0f) +
                 (on_dims.y * 0.5f),
            .width  = on_dims.x * ON_CONVEYOR_SCALE,
            .height = on_dims.y * ON_CONVEYOR_SCALE,
          };

          on_dest_rect.x += (direction_to_vec2(conveyor_from(*conveyor)).x * 0.5f) * GRID_DIMS.x;
          on_dest_rect.y += (direction_to_vec2(conveyor_from(*conveyor)).y * 0.5f) * GRID_DIMS.y;

          on_dest_rect.x -=
            (direction_to_vec2(conveyor_from(*conveyor)).x * 0.5f * item.t) * GRID_DIMS.x;
          on_dest_rect.y -=
            (direction_to_vec2(conveyor_from(*conveyor)).y * 0.5f * item.t) * GRID_DIMS.y;

          on_dest_rect.x +=
            (direction_to_vec2(conveyor_to(*conveyor)).x * 0.5f * item.t) * GRID_DIMS.x;
          on_dest_rect.y +=
            (direction_to_vec2(conveyor_to(*conveyor)).y * 0.5f * item.t) * GRID_DIMS.y;

          DrawTexturePro(on_texture, on_source_rect, on_dest_rect, on_origin, 0, WHITE);
        }
      }
    }
  }
}
