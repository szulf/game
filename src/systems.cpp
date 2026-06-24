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
      }

      remove_entity(store, hovered->id);
    }
  }
}

void system_pickup_item(EntityStore& store, EntityId player_entity) {
  auto* player = get_entity(store, player_entity);
  ASSERT(player);
  for (auto& event : listen(store, EventType::PLAYER_COLLIDED)) {
    auto* entity = get_entity(store, event.entity);
    if (entity && entity->type == EntityType::ITEM) {
      for (u32 i = 0; i < player->inventory.size(); ++i) {
        auto& inv_slot = player->inventory[i];
        if (!inv_slot) {
          inv_slot = entity->slot;
          remove_entity(store, entity->id);
          break;
        } else if (inv_slot.type == entity->slot.type) {
          if (inv_slot.count + entity->slot.count > ITEM_MAX_COUNT) {
            entity->slot.count -= ITEM_MAX_COUNT - inv_slot.count;
            inv_slot.count = ITEM_MAX_COUNT;
          } else {
            inv_slot.count += entity->slot.count;
            remove_entity(store, entity->id);
            break;
          }
        }
      }
    }
  }
}

void system_render(EntityStore& store) {
  static constexpr f32 ITEM_SCALE = 0.625f;

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
  }
}
