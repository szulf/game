#include "gui.h"
#include "entity.h"
#include "items.h"
#include "ui.h"
#include <raylib.h>

namespace gui {

// TODO: i dont really like the display_count argument
static bool item_slot_icon(
  const AssetManager& assets,
  ui::Layout& layout,
  const ItemSlot& item_slot,
  bool display_count = true
) {
  bool hovered = false;
  if (item_slot || item_slot.locked) {
    auto& texture = assets.textures[get_texture_type(item_slot.type)];

    UI(
      layout,
      {
        .layout_direction = ui::LAYOUT_DIRECTION_STACK,
        .sizing           = {ui::sizing_fill(), ui::sizing_fill()},
      },
      {.hovered = &hovered}
    ) {
      UI(
        layout,
        {
          .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
          .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
        }
      ) {
        ui::texture(layout, texture);
      }

      if (display_count) {
        UI(
          layout,
          {
            .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
            .padding         = {.right = 2},
            .child_alignment = {ui::CHILD_ALIGNMENT_END, ui::CHILD_ALIGNMENT_END},
          }
        ) {
          auto item_slot_info = item_info(item_slot.type);
          if (item_slot_info.has_durability) {
            ASSERT(item_slot_info.max_count == 1, "items with durability cannot stack");
            auto usage_percent =
              (f32(item_slot_info.max_damage - item_slot.damage) / f32(item_slot_info.max_damage)) *
              100.0f;
            ui::text(layout, std::format("{:2.0f}", usage_percent), 15);
          } else {
            ui::text(layout, std::format("{}", item_slot.count), 15);
          }
        }
      }

      if (item_slot.locked) {
        UI(
          layout,
          {
            .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
            .padding         = {.left = 2},
            .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_END},
          }
        ) {
          ui::text(layout, "L", 15);
        }
      }
    }
  }
  return hovered;
}

// TODO: render if the slot is input/output only
bool item_slot(
  const AssetManager& assets,
  ui::Layout& layout,
  const ItemSlot& item_slot,
  bool display_count
) {
  bool hovered = false;

  UI(
    layout,
    {
      .sizing          = {ui::sizing_fixed(GRID_DIMS.x), ui::sizing_fixed(GRID_DIMS.y)},
      .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
      .bg_color        = LIGHTGRAY,
    },
    {.hovered = &hovered}
  ) {
    item_slot_icon(assets, layout, item_slot, display_count);
  }

  return hovered;
}

ItemSlotIdx inventory(
  ui::Layout& layout,
  const AssetManager& assets,
  EntityId entity_id,
  std::span<const ItemSlot> inv
) {
  static constexpr i32 ROW_SIZE = 4;
  u32 row_count = inv.size() % 4 == 0 ? inv.size() / ROW_SIZE : (inv.size() / ROW_SIZE) + 1;
  ItemSlotIdx hovered_slot{};

  UI(layout, {.layout_direction = ui::LAYOUT_DIRECTION_VERTICAL, .child_gap = 2}) {
    for (u32 i = 0; i < row_count; ++i) {
      UI(layout, {.child_gap = 2}) {
        for (u32 j = 0; j < ROW_SIZE; ++j) {
          auto slot_idx = i * ROW_SIZE + j;
          if (slot_idx >= inv.size()) {
            break;
          }
          bool hovered = item_slot(assets, layout, inv[slot_idx]);
          if (hovered) {
            hovered_slot.entity   = entity_id;
            hovered_slot.slot_idx = slot_idx;
          }
        }
      }
    }
  }

  return hovered_slot;
}

ItemSlotIdx player_inventory(
  ui::Layout& layout,
  const AssetManager& assets,
  EntityStore& store,
  EntityId player_id
) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);

  ItemSlotIdx hovered_slot{};
  UI(
    layout,
    {
      .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
      .padding         = {.down = 8, .left = 8},
      .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_END},
    }
  ) {
    hovered_slot = inventory(layout, assets, player_id, player->inventory);
  }
  return hovered_slot;
}

ItemSlotIdx open_inventory(
  ui::Layout& layout,
  const AssetManager& assets,
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

  ItemSlotIdx hovered_slot{};
  UI(
    layout,
    {
      .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
      .padding         = {.left = 8},
      .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
    }
  ) {
    hovered_slot = inventory(layout, assets, player->open_gui, *open_inv);
  }
  return hovered_slot;
}

void player_hand(
  ui::System& ui_system,
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
    ui::layout_begin("player hand", ui_system, input, input.mouse_pos, GRID_DIMS);
  item_slot_icon(assets, player_hand_layout, player->hand);
  ui::layout_end(player_hand_layout);
}

static void maintenance_ui(
  ui::Layout& layout,
  const RenderTexture& render_texture,
  AssetManager& assets,
  Player& player,
  Maintenance& maintenance
) {
  auto fix_item       = maintenance_fix_item(maintenance);
  auto* minigame_open = maintenance_is_minigame_open(maintenance);

  if (minigame_open && *minigame_open) {
    ASSERT(IsRenderTextureValid(render_texture), "minigame render texture needs to be valid");
    vec2 window_offset = ui::element_get_pos(layout, "maintenance minigame window");
    maintenance_render_minigame(maintenance, assets, render_texture, window_offset);
    ui::texture(layout, render_texture.texture, true, "maintenance minigame window");
  } else {
    bool fix_clicked{};
    UI(layout, {.layout_direction = ui::LAYOUT_DIRECTION_VERTICAL, .child_gap = 4}) {
      ui::text(layout, std::format("Needs {}!", maintenance_name(maintenance)), 20, RED);

      UI(layout, {.child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER}}) {
        ui::text(layout, "Fix item: ", 15, WHITE);
        item_slot(assets, layout, {.type = fix_item, .count = 1}, false);
      }

      UI(
        layout,
        {.sizing          = {ui::sizing_fill(), ui::sizing_fit()},
         .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER}}
      ) {
        UI(
          layout,
          {.padding = ui::padding_all(4), .bg_color = LIGHTGRAY},
          {.clicked = &fix_clicked}
        ) {
          ui::text(layout, "FIX", 15, BLACK);
        }
      }
    }

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
  ui::Layout& layout,
  std::string_view title,
  std::string_view switch_page_text = ""
) {
  bool switch_page_clicked{};

  UI(layout) {
    ui::text(layout, title, 30, GREEN);
    UI(
      layout,
      {.sizing          = {ui::sizing_fill(), ui::sizing_fit()},
       .child_alignment = {ui::CHILD_ALIGNMENT_END, ui::CHILD_ALIGNMENT_START}}
    ) {
      if (!switch_page_text.empty()) {
        // TODO: should this depend on grid dims?
        UI(
          layout,
          {.sizing          = {ui::sizing_fixed(GRID_DIMS.x), ui::sizing_fixed(GRID_DIMS.y)},
           .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
           .bg_color        = LIGHTGRAY},
          {.clicked = &switch_page_clicked}
        ) {
          ui::text(layout, switch_page_text, 50, BLACK);
        }
      }
    }
  }

  return switch_page_clicked;
}

static bool
message_ui(ui::Layout& layout, AssetManager& assets, const ResourceMessage& msg, u32 idx) {
  static constexpr f32 FONT_SIZE = 15;
  bool cancel_clicked{};

  UI(
    layout,
    {
      .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
      .sizing           = {ui::sizing_fill(), ui::sizing_fit()},
    }
  ) {
    // NOTE: header
    UI(layout, {.sizing = {ui::sizing_fill(), ui::sizing_fit()}}) {
      UI(
        layout,
        {
          .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
          .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
        }
      ) {
        ui::text(layout, std::format("{}.", idx), FONT_SIZE, WHITE);
      }

      UI(
        layout,
        {
          .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
          .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
        }
      ) {
        u64 eta_hour = (msg.arrival_time / 60) % 24;
        u64 eta_min  = msg.arrival_time % 60;
        ui::text(layout, std::format("ETA: {}:{}", eta_hour, eta_min), FONT_SIZE, WHITE);
      }

      UI(
        layout,
        {
          .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
          .child_alignment = {ui::CHILD_ALIGNMENT_END, ui::CHILD_ALIGNMENT_CENTER},
        }
      ) {
        UI(layout, {.bg_color = LIGHTGRAY}, {.clicked = &cancel_clicked}) {
          ui::text(layout, "cancel", FONT_SIZE, RED);
        }
      }
    }

    UI(
      layout,
      {
        .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
        .sizing           = {ui::sizing_fill(), ui::sizing_fit()},
      }
    ) {
      ui::text(layout, "requested items:", FONT_SIZE, WHITE);

      for (u32 i = 0; i < msg.requested_items.size(); ++i) {
        ItemType requestable_item = REQUESTABLE_ITEMS[i];
        const auto& item_count    = msg.requested_items[i];
        auto& texture             = assets.textures[get_texture_type(requestable_item)];

        if (item_count == 0) {
          continue;
        }

        UI(layout, {.sizing = {ui::sizing_fill(), ui::sizing_fit()}}) {
          UI(
            layout,
            {
              .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
              .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
            }
          ) {
            UI(
              layout,
              {
                .sizing  = {ui::sizing_fixed(texture.width), ui::sizing_fixed(texture.height)},
                .texture = &texture,
              }
            ) {}
            ui::text(layout, get_item_name(requestable_item), FONT_SIZE, WHITE);
          }

          UI(
            layout,
            {
              .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
              .child_alignment = {ui::CHILD_ALIGNMENT_END, ui::CHILD_ALIGNMENT_CENTER},
            }
          ) {
            ui::text(layout, std::format("amount: {}", item_count), FONT_SIZE, WHITE);
          }
        }
      }
    }
  }

  return cancel_clicked;
}

void message_sender(
  ui::Layout& layout,
  const RenderTexture& render_texture,
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

  UI(
    layout,
    {
      .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
      .padding         = {.left = 8},
      .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
    }
  ) {
    UI(
      layout,
      {
        .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
        .padding          = ui::padding_all(4),
        .child_alignment  = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
        .bg_color         = BLACK,
      }
    ) {
      if (msg_sender->maintenance.index() != 0) {
        maintenance_ui(layout, render_texture, assets, *player, msg_sender->maintenance);
      } else {
        switch (msg_sender->page) {
          case SENDER_PAGE_DISPLAY: {
            bool switch_page_clicked = message_header_ui(layout, "Message Sender", "+");

            UI(
              layout,
              {
                .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
                .sizing           = {ui::sizing_fill(), ui::sizing_fit()},
                .child_gap        = 8,
              }
            ) {
              for (u32 i = 0; i < msg_queue.msgs.size();) {
                bool cancel_clicked = message_ui(layout, assets, msg_queue.msgs[i], i + 1);

                if (cancel_clicked) {
                  remove_resource_message(msg_queue, i);
                } else {
                  ++i;
                }
              }
            }

            if (switch_page_clicked) {
              msg_sender->page = SENDER_PAGE_CREATE;
            }
          } break;

          case SENDER_PAGE_CREATE: {
            auto& msg = msg_sender->msg_in_create;
            bool create_clicked{};

            bool switch_page_clicked = message_header_ui(layout, "Message Sender", "<");

            UI(
              layout,
              {
                .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
                .sizing           = {ui::sizing_fill(), ui::sizing_fit()},
                .child_gap        = 2,
              }
            ) {
              for (u32 i = 0; i < msg.requested_items.size(); ++i) {
                // NOTE: realistically REQUESTABLE_ITEMS[i] should always yield ItemType(i)
                ItemType requestable_item = REQUESTABLE_ITEMS[i];
                bool add_clicked{};
                bool remove_clicked{};

                UI(layout, {.sizing = {ui::sizing_fill(), ui::sizing_fit()}}) {
                  UI(
                    layout,
                    {
                      .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
                      .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
                    }
                  ) {
                    auto& texture = assets.textures[get_texture_type(ItemType(i))];
                    ui::texture(layout, texture);
                    ui::text(layout, get_item_name(requestable_item), 15, WHITE);
                  }

                  UI(
                    layout,
                    {
                      .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
                      .child_gap       = 4,
                      .child_alignment = {ui::CHILD_ALIGNMENT_END, ui::CHILD_ALIGNMENT_CENTER},
                    }
                  ) {
                    UI(
                      layout,
                      {
                        .sizing =
                          {ui::sizing_fixed(GRID_DIMS.x * 0.5f),
                           ui::sizing_fixed(GRID_DIMS.y * 0.5f)},
                        .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
                        .bg_color        = LIGHTGRAY,
                      },
                      {.clicked = &add_clicked}
                    ) {
                      ui::text(layout, "+", 20, BLACK);
                    }

                    ui::text(layout, std::format("amount: {}", msg.requested_items[i]), 15, WHITE);

                    UI(
                      layout,
                      {
                        .sizing =
                          {ui::sizing_fixed(GRID_DIMS.x * 0.5f),
                           ui::sizing_fixed(GRID_DIMS.y * 0.5f)},
                        .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
                        .bg_color        = LIGHTGRAY,
                      },
                      {.clicked = &remove_clicked}
                    ) {
                      ui::text(layout, "-", 20, BLACK);
                    }
                  }
                }

                if (add_clicked && msg.requested_items[i] < item_info(requestable_item).max_count) {
                  msg.requested_items[i] += REQUESTED_ITEMS_MULTIPLE;
                }

                if (remove_clicked && msg.requested_items[i] > 0) {
                  msg.requested_items[i] -= REQUESTED_ITEMS_MULTIPLE;
                }
              }

              UI(
                layout,
                {
                  .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
                  .child_alignment = {ui::CHILD_ALIGNMENT_END, ui::CHILD_ALIGNMENT_CENTER},
                }
              ) {
                UI(layout, {.padding = ui::padding_all(2), .bg_color = LIGHTGRAY}) {
                  ui::text(layout, "CREATE", 20, BLACK);
                }
              }
            }

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
    }
  }
}

ItemSlotIdx message_receiver(
  ui::Layout& layout,
  const RenderTexture& render_texture,
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

  UI(
    layout,
    {
      .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
      .padding         = {.left = 8},
      .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
    }
  ) {
    UI(
      layout,
      {
        .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
        .padding          = ui::padding_all(4),
        .child_alignment  = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
        .bg_color         = BLACK,
      }
    ) {
      if (msg_receiver->maintenance.index() != 0) {
        maintenance_ui(layout, render_texture, assets, *player, msg_receiver->maintenance);
      } else {
        message_header_ui(layout, "Message Receiver");

        UI(
          layout,
          {
            .layout_direction = ui::LAYOUT_DIRECTION_VERTICAL,
            .sizing           = {ui::sizing_fill(), ui::sizing_fit()},
            .child_gap        = 8,
          }
        ) {
          if (resource_message_receiver_empty(*msg_receiver)) {
            if (msg_queue.msgs.empty()) {
              UI(layout, {.layout_direction = ui::LAYOUT_DIRECTION_VERTICAL}) {
                ui::text(layout, "no arriving messages", 20, WHITE);
                ui::text(layout, "create messages in the Message Sender", 10, WHITE);
              }
            } else {
              ui::text(layout, "next arriving messages:", 20, WHITE);
              auto first_batch = get_first_resource_message_batch(msg_queue);
              for (u32 i = 0; i < first_batch.size(); ++i) {
                auto& msg = first_batch[i];
                message_ui(layout, assets, msg, i + 1);
              }
            }
          } else {
            ui::text(layout, "currently available items:", 20, WHITE);
            UI(layout, {.child_gap = 2}) {
              for (u32 i = 0; i < msg_receiver->inventory.size(); ++i) {
                auto& slot   = msg_receiver->inventory[i];
                bool hovered = item_slot(assets, layout, slot);
                if (hovered) {
                  hovered_slot.entity   = player->open_gui;
                  hovered_slot.slot_idx = i;
                }
              }
            }
          }
        }
      }
    }
  }

  return hovered_slot;
}

static bool recipe_button_ui(ui::Layout& layout, std::string_view recipe_name, bool selected) {
  bool clicked{};
  UI(
    layout,
    {.padding = ui::padding_all(4), .bg_color = selected ? GRAY : LIGHTGRAY},
    {.clicked = &clicked}
  ) {
    ui::text(layout, recipe_name, 15, BLACK);
  }
  return clicked;
}

ItemSlotIdx assembler(
  ui::Layout& layout,
  const RenderTexture& render_texture,
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
  UI(
    layout,
    {
      .sizing          = {ui::sizing_fill(), ui::sizing_fill()},
      .padding         = {.left = 8},
      .child_alignment = {ui::CHILD_ALIGNMENT_START, ui::CHILD_ALIGNMENT_CENTER},
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
      if (assembler->maintenance.index() != 0) {
        maintenance_ui(layout, render_texture, assets, *player, assembler->maintenance);
      } else {
        UI(layout, {.layout_direction = ui::LAYOUT_DIRECTION_VERTICAL, .child_gap = 4}) {
          static constexpr u32 ROW_SIZE  = 3;
          static constexpr u32 ROW_COUNT = Assembler::RECIPES.size() % 4 == 0
                                             ? Assembler::RECIPES.size() / ROW_SIZE
                                             : (Assembler::RECIPES.size() / ROW_SIZE) + 1;

          for (u32 i = 0; i < ROW_COUNT; ++i) {
            UI(
              layout,
              {
                .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
                .child_gap       = 4,
                .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
              }
            ) {
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
            }
          }
        }

        const auto& selected_recipe = Assembler::RECIPES[assembler->selected_recipe_idx];
        ui::text(layout, "Recipe:", 15, WHITE);
        UI(
          layout,
          {
            .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
            .child_gap       = 4,
            .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
          }
        ) {
          for (u32 i = 0; i < Recipe::MAX_INPUT_SLOTS; ++i) {
            if (selected_recipe.input_slots[i]) {
              item_slot(assets, layout, selected_recipe.input_slots[i]);
            }
          }
          ui::text(layout, "->", 20, WHITE);
          for (u32 i = 0; i < Recipe::MAX_OUTPUT_SLOTS; ++i) {
            if (selected_recipe.output_slots[i]) {
              item_slot(assets, layout, selected_recipe.output_slots[i]);
            }
          }
        }

        ui::text(layout, "Inventory:", 15, WHITE);
        UI(
          layout,
          {
            .sizing          = {ui::sizing_fill(), ui::sizing_fit()},
            .child_gap       = 4,
            .child_alignment = {ui::CHILD_ALIGNMENT_CENTER, ui::CHILD_ALIGNMENT_CENTER},
          }
        ) {
          for (u32 i = 0; i < Recipe::MAX_INPUT_SLOTS; ++i) {
            if (selected_recipe.input_slots[i]) {
              bool slot_hovered = item_slot(assets, layout, assembler_input_slot(*assembler, i));
              if (slot_hovered) {
                hovered.entity   = player->open_gui;
                hovered.slot_idx = i;
              }
            }
          }
          ui::text(layout, "->", 20, WHITE);
          for (u32 i = 0; i < Recipe::MAX_OUTPUT_SLOTS; ++i) {
            if (selected_recipe.output_slots[i]) {
              bool slot_hovered = item_slot(assets, layout, assembler_output_slot(*assembler, i));
              if (slot_hovered) {
                hovered.entity = player->open_gui;
                // TODO: dont like this addition here (it was supposed to be an implementation
                // detail)
                hovered.slot_idx = i + Recipe::MAX_INPUT_SLOTS;
              }
            }
          }
        }

        ui::text(
          layout,
          std::format("{:.1f}/{}", assembler->t, selected_recipe.recipe_time),
          20,
          WHITE
        );
      }
    }
  }

  return hovered;
}

}
