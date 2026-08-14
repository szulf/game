#include "gui.h"
#include "entity.h"

// TODO: this is bad now, the text is rendered on top of the texture, starting at the textures top
// left corner, it should start at the cells top left corner instead, idk if its a limitation of the
// ui library, or i just dont know how to do it, but yeah
// TODO: i dont really like the display_count argument
static bool item_slot_icon_ui(
  const AssetManager& assets,
  UI_Layout& layout,
  const ItemSlot& item_slot,
  bool display_count = true
) {
  bool hovered = false;
  if (item_slot) {
    auto& texture = assets.textures[get_texture_type(item_slot.type)];

    ui_element_begin(layout, UI_AUTO_ID, {.hovered = &hovered});
    {
      if (display_count) {
        auto item_slot_info = item_info(item_slot.type);
        if (item_slot_info.has_durability) {
          ASSERT(item_slot_info.max_count == 1, "items with durability cannot stack");
          auto usage_percent =
            (f32(item_slot_info.max_damage - item_slot.damage) / f32(item_slot_info.max_damage)) *
            100.0f;
          ui_text(layout, std::format("{:2}", usage_percent), 15);
        } else {
          ui_text(layout, std::format("{}", item_slot.count), 15);
        }
      }
    }
    ui_element_end(
      layout,
      {.sizing  = {ui_sizing_fixed(texture.width), ui_sizing_fixed(texture.height)},
       .texture = &texture}
    );
  }
  return hovered;
}

// TODO: render if the slot is input/output only
static bool item_slot_ui(
  const AssetManager& assets,
  UI_Layout& layout,
  const ItemSlot& item_slot,
  bool display_count = true
) {
  bool hovered = false;

  ui_element_begin(layout, UI_AUTO_ID, {.hovered = &hovered});
  {
    item_slot_icon_ui(assets, layout, item_slot, display_count);
  }
  ui_element_end(
    layout,
    {.sizing          = {ui_sizing_fixed(GRID_DIMS.x), ui_sizing_fixed(GRID_DIMS.y)},
     .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER},
     .bg_color        = LIGHTGRAY}
  );

  return hovered;
}

ItemSlotIdx gui_inventory(
  UI_Layout& layout,
  const AssetManager& assets,
  EntityId entity_id,
  std::span<const ItemSlot> inv
) {
  static constexpr i32 ROW_SIZE = 4;
  u32 row_count = inv.size() % 4 == 0 ? inv.size() / ROW_SIZE : (inv.size() / ROW_SIZE) + 1;
  ItemSlotIdx hovered_slot{};

  ui_element_begin(layout, UI_AUTO_ID);
  for (u32 i = 0; i < row_count; ++i) {
    ui_element_begin(layout, UI_AUTO_ID);
    for (u32 j = 0; j < ROW_SIZE; ++j) {
      auto slot_idx = i * ROW_SIZE + j;
      if (slot_idx >= inv.size()) {
        break;
      }
      bool hovered = item_slot_ui(assets, layout, inv[slot_idx]);
      if (hovered) {
        hovered_slot.entity   = entity_id;
        hovered_slot.slot_idx = slot_idx;
      }
    }
    ui_element_end(layout, {.child_gap = 2});
  }
  ui_element_end(layout, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL, .child_gap = 2});

  return hovered_slot;
}

ItemSlotIdx gui_player_inventory(
  UI_System& ui_system,
  const AssetManager& assets,
  const Input& input,
  EntityStore& store,
  EntityId player_id
) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);

  auto player_inv_layout =
    ui_layout_begin("player inventory", ui_system, input, {10, 580}, WINDOW_DIMS);
  auto hovered_slot = gui_inventory(player_inv_layout, assets, player_id, player->inventory);
  ui_layout_end(player_inv_layout);

  return hovered_slot;
}

ItemSlotIdx gui_open_inventory(
  UI_System& ui_system,
  const AssetManager& assets,
  const Input& input,
  EntityStore& store,
  EntityId player_id
) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);
  // TODO: will want a different gui for all gui types
  // different functions will handle them too
  // currently just have a different guis for
  // message receiver, assembler
  if (
    !player->open_gui || is<ResourceMessageReceiver>(store, player->open_gui) ||
    is<Assembler>(store, player->open_gui)
  ) {
    return {};
  }

  auto* open_inv = get_inventory(store, player->open_gui);
  if (!open_inv) {
    return {};
  }

  auto open_inv_layout =
    ui_layout_begin("open inventory", ui_system, input, {10, 300}, WINDOW_DIMS);
  auto open_inv_hovered_slot = gui_inventory(open_inv_layout, assets, player->open_gui, *open_inv);
  ui_layout_end(open_inv_layout);
  return open_inv_hovered_slot;

  return {};
}

void gui_player_hand(
  UI_System& ui_system,
  const AssetManager& assets,
  const Input& input,
  EntityStore& store,
  EntityId player_id
) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);
  if (!player->hand) {
    return;
  }

  auto player_hand_layout =
    ui_layout_begin("player hand", ui_system, input, input.mouse_pos, GRID_DIMS);
  item_slot_icon_ui(assets, player_hand_layout, player->hand);
  ui_layout_end(player_hand_layout);
}

static void maintenance_ui(
  UI_Layout& layout,
  const RenderTexture& render_texture,
  AssetManager& assets,
  Player& player,
  Maintenance& maintenance
) {
  auto fix_item       = maintenance_fix_item(maintenance);
  auto* minigame_open = maintenance_is_minigame_open(maintenance);

  if (minigame_open && *minigame_open) {
    ASSERT(IsRenderTextureValid(render_texture), "minigame render texture needs to be valid");
    vec2 window_offset = ui_element_get_pos(layout, "maintenance minigame window");
    maintenance_render_minigame(maintenance, assets, render_texture, window_offset);
    ui_element_begin(layout, "maintenance minigame window");
    ui_element_end(
      layout,
      {.sizing =
         {ui_sizing_fixed(render_texture.texture.width),
          ui_sizing_fixed(render_texture.texture.height)},
       .texture                 = &render_texture.texture,
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
        item_slot_ui(assets, layout, {.type = fix_item, .count = 1}, false);
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
      bool succeeded{};
      if (player.hand.type == fix_item && player.hand.count >= MAINTENANCE_FIX_ITEM_COUNT) {
        auto hand_item_info = item_info(player.hand.type);
        if (hand_item_info.has_durability) {
          if (hand_item_info.max_damage - player.hand.damage >= MAINTENANCE_FIX_ITEM_DAMAGE) {
            player.hand.damage += MAINTENANCE_FIX_ITEM_DAMAGE;
            succeeded = true;
          }
        } else {
          player.hand.count -= MAINTENANCE_FIX_ITEM_COUNT;
          succeeded = true;
        }
      }

      if (succeeded) {
        auto* minigame_open = maintenance_is_minigame_open(maintenance);
        if (minigame_open) {
          *minigame_open = true;
          // TODO: maybe check if inited in update and init there?
          maintenance_init_minigame(maintenance);
        } else {
          maintenance = std::monostate{};
        }
      } else {
        // TODO: notify the user they dont have the item
      }
    }
  }
}

static bool message_header_ui(
  UI_Layout& layout,
  std::string_view title,
  std::string_view switch_page_text = ""
) {
  bool switch_page_clicked{};

  ui_element_begin(layout, UI_AUTO_ID);
  {
    ui_text(layout, title, 30, GREEN);
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

      for (u32 i = 0; i < msg.requested_items.size(); ++i) {
        ItemType requestable_item = REQUESTABLE_ITEMS[i];
        const auto& item_count    = msg.requested_items[i];
        auto& texture             = assets.textures[get_texture_type(requestable_item)];

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
            ui_text(layout, get_item_name(requestable_item), FONT_SIZE, WHITE);
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
    {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL,
     .sizing           = {ui_sizing_fill(), ui_sizing_fit()}}
  );

  return cancel_clicked;
}

void gui_message_sender(
  UI_System& ui_system,
  const RenderTexture& render_texture,
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
    maintenance_ui(layout, render_texture, assets, *player, msg_sender->maintenance);
  } else {
    switch (msg_sender->page) {
      case SENDER_PAGE_DISPLAY: {
        bool switch_page_clicked = message_header_ui(layout, "Message Sender", "+");

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
          msg_sender->page = SENDER_PAGE_CREATE;
        }
      } break;

      case SENDER_PAGE_CREATE: {
        auto& msg = msg_sender->msg_in_create;
        bool create_clicked{};

        bool switch_page_clicked = message_header_ui(layout, "Message Sender", "<");

        ui_element_begin(layout, UI_AUTO_ID);
        {
          for (u32 i = 0; i < msg.requested_items.size(); ++i) {
            // NOTE: realistically REQUESTABLE_ITEMS[i] should always yield ItemType(i)
            ItemType requestable_item = REQUESTABLE_ITEMS[i];
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
                ui_text(layout, get_item_name(requestable_item), 15, WHITE);
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

            if (add_clicked && msg.requested_items[i] < item_info(requestable_item).max_count) {
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
          msg_sender->page = SENDER_PAGE_DISPLAY;
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

ItemSlotIdx gui_message_receiver(
  UI_System& ui_system,
  const RenderTexture& render_texture,
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
    maintenance_ui(layout, render_texture, assets, *player, msg_receiver->maintenance);
  } else {
    message_header_ui(layout, "Message Receiver");

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

static bool recipe_button_ui(UI_Layout& layout, std::string_view recipe_name, bool selected) {
  bool clicked{};
  ui_element_begin(layout, UI_AUTO_ID, {.clicked = &clicked});
  {
    ui_text(layout, recipe_name, 15, BLACK);
  }
  ui_element_end(layout, {.padding = ui_padding_all(4), .bg_color = selected ? GRAY : LIGHTGRAY});
  return clicked;
}

ItemSlotIdx gui_assembler(
  UI_System& ui_system,
  const RenderTexture& render_texture,
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
    maintenance_ui(layout, render_texture, assets, *player, assembler->maintenance);
  } else {
    ui_element_begin(layout, UI_AUTO_ID);
    {
      static constexpr u32 ROW_SIZE  = 3;
      static constexpr u32 ROW_COUNT = Assembler::RECIPES.size() % 4 == 0
                                         ? Assembler::RECIPES.size() / ROW_SIZE
                                         : (Assembler::RECIPES.size() / ROW_SIZE) + 1;

      for (u32 i = 0; i < ROW_COUNT; ++i) {
        ui_element_begin(layout, UI_AUTO_ID);
        for (u32 j = 0; j < ROW_SIZE; ++j) {
          auto idx = i * ROW_SIZE + j;
          if (idx >= Assembler::RECIPES.size()) {
            break;
          }
          const auto& recipe = Assembler::RECIPES[idx];
          bool clicked =
            recipe_button_ui(layout, recipe.name, assembler->selected_recipe_idx == idx);
          if (clicked && assembler->selected_recipe_idx != idx) {
            assembler->selected_recipe_idx = idx;
            // TODO: pull out to a clear or something function
            assembler->t = 0;
          }
        }
        ui_element_end(
          layout,
          {.sizing          = {ui_sizing_fill(), ui_sizing_fit()},
           .child_gap       = 4,
           .child_alignment = {UI_CHILD_ALIGNMENT_CENTER, UI_CHILD_ALIGNMENT_CENTER}}
        );
      }
    }
    ui_element_end(layout, {.layout_direction = UI_LAYOUT_DIRECTION_VERTICAL, .child_gap = 4});

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
