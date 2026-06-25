static ivec2 grid_pos(const ivec2& pos) {
  return pos / GRID_DIMS;
}

static bool pos_in_radius(const ivec2& pos, const ivec2& start_pos, i32 radius) {
  auto diff2 = length2(pos - start_pos);
  return diff2 < radius * radius;
}

static void
item_slot_icon_ui(const ItemSlot& item_slot, const ivec2& pos, std::vector<ui::Command>& ui_cmds) {
  // TODO: somehow combine this constant with inventory_ui?
  static constexpr ivec2 CELL_DIMS = GRID_DIMS;
  static constexpr f32 ICON_SCALE  = 0.75f;

  if (item_slot) {
    auto render     = render_rect(item_to_entity(item_slot.type));
    ivec2 icon_dims = render.dims * ICON_SCALE;
    ivec2 icon_pos  = pos + ((CELL_DIMS - icon_dims) / 2);

    ui_cmds.push_back(ui::Command{
      .type  = ui::CommandType::Rect,
      .pos   = icon_pos,
      .color = render.color,
      .dims  = icon_dims,
    });

    ui_cmds.push_back(ui::Command{
      .type  = ui::CommandType::Text,
      .pos   = pos,
      .color = BLACK,
      .text  = std::format("{}", item_slot.count),
      .size  = 15,
    });
  }
}

static ItemSlotIdx inventory_ui(
  EntityStore& store,
  EntityId entity_id,
  const ivec2& pos,
  const ivec2& mouse_pos,
  std::vector<ui::Command>& ui_cmds
) {
  // TODO: somehow combine this constant with item_slot_icon_ui?
  static constexpr ivec2 CELL_DIMS = GRID_DIMS;
  static constexpr i32 ROW_SIZE    = 4;

  auto* entity = get_entity(store, entity_id);
  if (!entity) {
    return {};
  }

  ASSERT(has_inventory(entity->type));
  auto& inv = entity->inventory;
  ItemSlotIdx hovered{};
  for (u32 i = 0; i < inv.size(); ++i) {
    ivec2 cell     = {i32(i % ROW_SIZE), i32(i / ROW_SIZE)};
    ivec2 cell_pos = pos + (cell * CELL_DIMS) + (cell * 2);

    ui_cmds.push_back(ui::Command{
      .type  = ui::CommandType::Rect,
      .pos   = cell_pos,
      .color = LIGHTGRAY,
      .dims  = CELL_DIMS,
    });

    bool hovered_x = mouse_pos.x > cell_pos.x && mouse_pos.x < (cell_pos.x + CELL_DIMS.x);
    bool hovered_y = mouse_pos.y > cell_pos.y && mouse_pos.y < (cell_pos.y + CELL_DIMS.y);
    if (hovered_x && hovered_y) {
      hovered.entity   = entity_id;
      hovered.slot_idx = i;
    }

    item_slot_icon_ui(inv[i], cell_pos, ui_cmds);
  }
  return hovered;
}

void system_move_player(EntityStore& store, EntityId player_entity, Input& input) {
  if (input.move == ivec2{0, 0}) {
    return;
  }

  auto* player = get_entity(store, player_entity);
  ASSERT(player);
  auto collided = find_all(store, [&](Entity& entity) {
    return entity.pos == player->pos + input.move;
  });
  bool can_move = true;

  for (auto& collision : collided) {
    emit(store, {.type = EventType::PLAYER_COLLIDED, .entity = collision->id});
    if (solid(collision->type)) {
      can_move = false;
    }
  }
  if (can_move) {
    player->pos += input.move;
  }
}

void system_player_inventory_interactions(
  EntityStore& store,
  EntityId player_entity,
  ItemSlotIdx& hovered_slot,
  Input& input
) {
  auto* player = get_entity(store, player_entity);
  ASSERT(player);

  // NOTE: opening
  if (input.interact &&
      pos_in_radius(grid_pos(input.mouse_pos), player->pos, player->interaction_radius)) {
    auto hovered = find(store, [&](Entity& entity) {
      return entity.pos == grid_pos(input.mouse_pos) && has_inventory(entity.type);
    });
    if (hovered) {
      player->open_inventory = hovered->id;
    }
  }

  // NOTE: closing
  if (player->open_inventory) {
    auto* open_inv_entity = get_entity(store, player->open_inventory);
    if (input.close_inv ||
        (open_inv_entity &&
         !pos_in_radius(open_inv_entity->pos, player->pos, player->interaction_radius)) ||
        !open_inv_entity) {
      player->open_inventory = NULL_ENTITY;
    }
  }

  // NOTE: hand slot interactions
  if (hovered_slot.entity && input.lmb_pressed) {
    auto* hovered_slot_entity = get_entity(store, hovered_slot.entity);
    if (hovered_slot_entity) {
      auto& slot = hovered_slot_entity->inventory[hovered_slot.slot_idx];
      auto& hand = player->hand;
      if (slot && hand && slot.type == hand.type) {
        if (slot.count + hand.count > ITEM_MAX_COUNT) {
          hand.count = (slot.count + hand.count) - ITEM_MAX_COUNT;
          slot.count = ITEM_MAX_COUNT;
        } else {
          slot.count += hand.count;
          hand = {};
        }
      } else {
        std::swap(slot, hand);
      }
    }
  }
}

ItemSlotIdx system_generate_inventory_uis(
  EntityStore& store,
  EntityId player_entity,
  const ivec2& mouse_pos,
  std::vector<ui::Command>& ui_cmds
) {
  auto hovered_slot = inventory_ui(store, player_entity, {10, 580}, mouse_pos, ui_cmds);

  auto* player = get_entity(store, player_entity);
  ASSERT(player);
  if (player->open_inventory) {
    auto open_inv_hovered_slot =
      inventory_ui(store, player->open_inventory, {10, 300}, mouse_pos, ui_cmds);
    if (open_inv_hovered_slot.entity) {
      hovered_slot = open_inv_hovered_slot;
    }
  }

  // TODO: not sure whether this belongs here
  if (player->hand) {
    item_slot_icon_ui(player->hand, mouse_pos, ui_cmds);
  }

  return hovered_slot;
}

void system_place_entity(
  EntityStore& store,
  EntityId player_entity,
  const Input& input,
  Rotation place_rotation
) {
  auto* player = get_entity(store, player_entity);
  ASSERT(player);
  // TODO: should check if im not hovering over an item slot
  if (input.rmb_pressed && player->hand &&
      pos_in_radius(grid_pos(input.mouse_pos), player->pos, player->interaction_radius) &&
      !find(store, [&](Entity& entity) {
        return entity.pos == grid_pos(input.mouse_pos);
      })) {
    add_entity(
      store,
      init_entity(item_to_entity(player->hand.type), grid_pos(input.mouse_pos), place_rotation)
    );
    --player->hand.count;
  }
}

void system_remove_entity(EntityStore& store, EntityId player_entity, const Input& input) {
  auto* player = get_entity(store, player_entity);
  ASSERT(player);
  if (input.lmb_pressed &&
      pos_in_radius(grid_pos(input.mouse_pos), player->pos, player->interaction_radius)) {
    auto hovered = find(store, [&](Entity& entity) {
      return breakable(entity.type) && entity.pos == grid_pos(input.mouse_pos);
    });
    if (hovered) {
      auto item_type = entity_to_item(hovered->type);
      // NOTE: if an entity is breakable it should have an item_type (at least for now)
      ASSERT(item_type);

      // TODO: use init_item()????
      add_entity(
        store,
        {.type = EntityType::ITEM, .pos = hovered->pos, .slot = {.type = *item_type, .count = 1}}
      );
      if (has_inventory(hovered->type)) {
        for (auto& slot : hovered->inventory) {
          if (slot) {
            add_entity(store, {.type = EntityType::ITEM, .pos = hovered->pos, .slot = slot});
          }
        }
      } else if (hovered->type == EntityType::CONVEYOR) {
        for (auto& conveyor_item : hovered->conveyor_items) {
          if (conveyor_item.slot) {
            add_entity(
              store,
              {.type = EntityType::ITEM, .pos = hovered->pos, .slot = conveyor_item.slot}
            );
          }
        }
      }

      remove_entity(store, hovered->id);
    }
  }
}

// NOTE: returns whether it succeeded in transfering all items from the slot into the inventory
// also modified the slot to contain the left amount of items after the transfer
// so if it succeeded slot.count == 0
static bool transfer_items(std::vector<ItemSlot>& inventory, ItemSlot& slot) {
  for (u32 i = 0; i < inventory.size(); ++i) {
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
      std::swap(inventory[i], slot);
      return true;
    }
  }
  return false;
}

void system_pickup_item(EntityStore& store, EntityId player_entity) {
  auto* player = get_entity(store, player_entity);
  ASSERT(player);
  for (auto& event : listen(store, EventType::PLAYER_COLLIDED)) {
    auto* entity = get_entity(store, event.entity);
    if (entity && entity->type == EntityType::ITEM) {
      if (transfer_items(player->inventory, entity->slot)) {
        remove_entity(store, entity->id);
      }
    }
  }
}

static ItemSlot* find_first_used_slot(std::vector<ItemSlot>& inventory) {
  for (u32 i = 0; i < inventory.size(); ++i) {
    if (inventory[i]) {
      return &inventory[i];
    }
  }
  return nullptr;
}

// TODO: currently moving items, fuckin sucks actually
// its too dependent on the ordering of conveyors in EntityStore
// so it gets choppy if conveyors are not stored in the right order
// possible solutions (i should implement this shit before finishing):
// - double buffering (reminds me of my game-of-life)
//   update everything based on this frames step into a second buffer then swap them
// - build graphs of conveyor chains, sort them, then update in order
//   (this fucking sucks actually)
void system_move_items(EntityStore& store, f32 dt) {
  for (auto& entity : store) {
    if (entity.type == EntityType::CONVEYOR) {
      f32 item_gap = 1.0f / entity.throughput;
      // NOTE: move items that are already on the conveyor
      for (u32 i = 0; i < entity.throughput; ++i) {
        auto& conveyor_item = entity.conveyor_items[i];
        if (conveyor_item.slot) {
          if (conveyor_item.t < 1 - (i * item_gap)) {
            conveyor_item.t += dt;
          }
        } else {
          conveyor_item.t = 0;
        }
      }

      // NOTE: pull in more items
      for (u32 i = 0; i < entity.throughput; ++i) {
        auto& conveyor_item = entity.conveyor_items[i];
        bool can_pull       = !conveyor_item.slot;
        if (i > 0) {
          auto& previous_item = entity.conveyor_items[i - 1];
          can_pull            = can_pull && previous_item.t >= item_gap;
        }
        if (can_pull) {
          ivec2 from_pos    = entity.pos + direction_to_ivec2(entity.from);
          auto* from_entity = find(store, [&](Entity& from_entity) {
            return has_inventory(from_entity.type) && from_entity.type != EntityType::PLAYER &&
                   from_entity.type != EntityType::CONVEYOR && from_entity.pos == from_pos;
          });
          if (from_entity) {
            auto* first_used = find_first_used_slot(from_entity->inventory);
            if (first_used) {
              conveyor_item.slot.type  = first_used->type;
              conveyor_item.slot.count = 1;
              --first_used->count;
            }
          }
          break;
        }
      }

      // NOTE: push items off
      {
        auto& conveyor_item = entity.conveyor_items[0];
        if (conveyor_item.t >= 1) {
          ivec2 to_pos    = entity.pos + direction_to_ivec2(entity.to);
          auto* to_entity = find(store, [&](Entity& to_entity) {
            return to_entity.type != EntityType::PLAYER && to_entity.pos == to_pos;
          });
          if (to_entity) {
            bool success = false;
            if (to_entity->type == EntityType::CONVEYOR) {
              for (u32 i = 0; i < to_entity->throughput; ++i) {
                auto& to_conveyor_item = to_entity->conveyor_items[i];
                if (!to_conveyor_item.slot) {
                  std::swap(conveyor_item.slot, to_conveyor_item.slot);
                  to_conveyor_item.t = 0;
                  success            = true;
                  break;
                }
              }
            } else if (has_inventory(to_entity->type)) {
              success = transfer_items(to_entity->inventory, conveyor_item.slot);
            }
            if (success) {
              std::ranges::rotate(entity.conveyor_items, entity.conveyor_items.begin() + 1);
            }
          }
        }
      }
    }
  }
}

void system_render(EntityStore& store) {
  static constexpr f32 ITEM_SCALE        = 0.625f;
  static constexpr f32 ON_CONVEYOR_SCALE = 0.375f;

  for (auto& entity : store) {
    RenderRect render{};
    if (entity.type == EntityType::ITEM) {
      render = render_rect(item_to_entity(entity.slot.type));
    } else {
      render = render_rect(entity.type);
    }

    Rectangle rect = {
      .x = f32((entity.pos.x * GRID_DIMS.x) + ((GRID_DIMS.x - render.dims.x) / 2)) +
           (f32(render.dims.x) * 0.5f),
      .y = f32((entity.pos.y * GRID_DIMS.y) + ((GRID_DIMS.y - render.dims.y) / 2)) +
           (f32(render.dims.y) * 0.5f),
      .width  = f32(render.dims.x),
      .height = f32(render.dims.y),
    };
    Vector2 origin = vector2_from_ivec2(render.dims) * 0.5f;

    f32 rotation = 0;
    if (rotatable(entity.type)) {
      rotation = rotation_degrees(entity.rotation);
    }

    if (entity.type == EntityType::ITEM) {
      rect.width *= ITEM_SCALE;
      rect.height *= ITEM_SCALE;
      origin *= ITEM_SCALE;
    }

    DrawRectanglePro(rect, origin, rotation, render.color);

    if (entity.type == EntityType::CONVEYOR) {
      for (u32 i = 0; i < entity.throughput; ++i) {
        auto& conveyor_item = entity.conveyor_items[i];
        if (conveyor_item.slot) {
          RenderRect on_render = render_rect(item_to_entity(conveyor_item.slot.type));
          Vector2 on_origin    = vector2_from_ivec2(on_render.dims) * 0.5f * ON_CONVEYOR_SCALE;
          Rectangle on_rect    = {
               .x = f32((entity.pos.x * GRID_DIMS.x) + ((GRID_DIMS.x - on_render.dims.x) / 2)) +
                 (f32(on_render.dims.x) * 0.5f),
               .y = f32((entity.pos.y * GRID_DIMS.y) + ((GRID_DIMS.y - on_render.dims.y) / 2)) +
                 (f32(on_render.dims.y) * 0.5f),
               .width  = f32(on_render.dims.x) * ON_CONVEYOR_SCALE,
               .height = f32(on_render.dims.y) * ON_CONVEYOR_SCALE,
          };

          on_rect.x += (f32(direction_to_ivec2(entity.from).x) * 0.5f) * GRID_DIMS.x;
          on_rect.y += (f32(direction_to_ivec2(entity.from).y) * 0.5f) * GRID_DIMS.y;

          on_rect.x -=
            ((f32(direction_to_ivec2(entity.from).x) * 0.5f) * conveyor_item.t) * GRID_DIMS.x;
          on_rect.y -=
            ((f32(direction_to_ivec2(entity.from).y) * 0.5f) * conveyor_item.t) * GRID_DIMS.y;

          on_rect.x +=
            ((f32(direction_to_ivec2(entity.to).x) * 0.5f) * conveyor_item.t) * GRID_DIMS.x;
          on_rect.y +=
            ((f32(direction_to_ivec2(entity.to).y) * 0.5f) * conveyor_item.t) * GRID_DIMS.y;

          DrawRectanglePro(on_rect, on_origin, 0, on_render.color);
        }
      }
    }
  }
}
