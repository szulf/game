static ivec2 grid_pos(const ivec2& pos) {
  return pos / GRID_DIMS;
}

static bool pos_in_radius(const ivec2& pos, const ivec2& start_pos, i32 radius) {
  auto diff2 = length2(pos - start_pos);
  return diff2 < radius * radius;
}

static void item_slot_icon_ui(
  const ItemSlot& item_slot,
  const ivec2& pos,
  std::vector<ui::Command>& ui_cmds,
  AssetManager& assets
) {
  // TODO: somehow combine this constant with inventory_ui?
  static constexpr ivec2 CELL_DIMS = GRID_DIMS;

  if (item_slot) {
    auto& texture   = assets.textures[get_texture_type(item_slot.type)];
    ivec2 icon_dims = ivec2{texture.width, texture.height};
    ivec2 icon_pos  = pos + ((CELL_DIMS - icon_dims) / 2);

    ui_cmds.push_back(ui::TextureCommand{
      .pos     = icon_pos,
      .texture = &texture,
    });

    ui_cmds.push_back(ui::TextCommand{
      .pos   = pos,
      .color = BLACK,
      .text  = std::format("{}", item_slot.count),
      .size  = 15,
    });
  }
}

static ItemSlotIdx inventory_ui(
  EntityId entity_id,
  const std::vector<ItemSlot>& inv,
  const ivec2& pos,
  const ivec2& mouse_pos,
  std::vector<ui::Command>& ui_cmds,
  AssetManager& assets
) {
  // TODO: somehow combine this constant with item_slot_icon_ui?
  static constexpr ivec2 CELL_DIMS = GRID_DIMS;
  static constexpr i32 ROW_SIZE    = 4;

  ItemSlotIdx hovered{};
  for (u32 i = 0; i < inv.size(); ++i) {
    ivec2 cell     = {i32(i % ROW_SIZE), i32(i / ROW_SIZE)};
    ivec2 cell_pos = pos + (cell * CELL_DIMS) + (cell * 2);

    ui_cmds.push_back(ui::RectCommand{
      .pos   = cell_pos,
      .color = LIGHTGRAY,
      .dims  = CELL_DIMS,
    });

    bool in_only = inv[i].flags == ITEM_SLOT_INPUT;
    if (in_only) {
      ui_cmds.push_back(ui::TextCommand{
        .pos   = cell_pos,
        .color = BLUE,
        .text  = "IN",
        .size  = 10,
      });
    }
    bool out_only = inv[i].flags == ITEM_SLOT_OUTPUT;
    if (out_only) {
      ui_cmds.push_back(ui::TextCommand{
        .pos   = cell_pos,
        .color = ORANGE,
        .text  = "OUT",
        .size  = 10,
      });
    }

    bool hovered_x = mouse_pos.x > cell_pos.x && mouse_pos.x < (cell_pos.x + CELL_DIMS.x);
    bool hovered_y = mouse_pos.y > cell_pos.y && mouse_pos.y < (cell_pos.y + CELL_DIMS.y);
    if (hovered_x && hovered_y) {
      hovered.entity   = entity_id;
      hovered.slot_idx = i;
    }

    item_slot_icon_ui(inv[i], cell_pos, ui_cmds, assets);
  }
  return hovered;
}

void system_move_player(EntityStore& store, EntityId player_id, Input& input) {
  if (input.move == ivec2{0, 0}) {
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

void system_player_inventory_interactions(
  EntityStore& store,
  EntityId player_id,
  ItemSlotIdx& hovered_slot,
  Input& input
) {
  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);
  auto* player = get_data<Player>(*player_entity);
  ASSERT_NO_MSG(player);

  // NOTE: opening
  // TODO: forbid opening the player inventory as another inventory
  if (input.interact &&
      pos_in_radius(grid_pos(input.mouse_pos), player_entity->pos, player->interaction_radius)) {
    auto hovered = get_entity_at_pos(store, grid_pos(input.mouse_pos), player_entity->world);
    if (hovered && has_inventory(*hovered)) {
      player->open_inventory = hovered->id;
    }
  }

  // NOTE: closing
  if (player->open_inventory) {
    auto* open_inv_entity = get_entity(store, player->open_inventory);
    if (input.close_inv ||
        (open_inv_entity &&
         !pos_in_radius(open_inv_entity->pos, player_entity->pos, player->interaction_radius)) ||
        (open_inv_entity && open_inv_entity->world != player_entity->world) || !open_inv_entity) {
      player->open_inventory = NULL_ENTITY;
    }
  }

  // NOTE: hand slot interactions
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

  // NOTE: hand dropping items on the ground (not sure if it belongs in this system)
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

ItemSlotIdx system_generate_inventory_uis(
  EntityStore& store,
  EntityId player_id,
  const ivec2& mouse_pos,
  std::vector<ui::Command>& ui_cmds,
  AssetManager& assets
) {
  auto* player = get_data<Player>(store, player_id);
  ASSERT_NO_MSG(player);
  auto hovered_slot =
    inventory_ui(player_id, player->inventory, {10, 580}, mouse_pos, ui_cmds, assets);

  if (player->open_inventory) {
    auto* open_inv = get_inventory(store, player->open_inventory);
    if (open_inv) {
      auto open_inv_hovered_slot =
        inventory_ui(player->open_inventory, *open_inv, {10, 300}, mouse_pos, ui_cmds, assets);
      if (open_inv_hovered_slot.entity) {
        hovered_slot = open_inv_hovered_slot;
      }
    }
  }

  // TODO: not sure whether this belongs here
  if (player->hand) {
    item_slot_icon_ui(player->hand, mouse_pos, ui_cmds, assets);
  }

  return hovered_slot;
}

void system_place_entity(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  Rotation place_rotation
) {
  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);
  auto* player = get_data<Player>(*player_entity);
  ASSERT_NO_MSG(player);

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
  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);
  auto* player = get_data<Player>(*player_entity);
  ASSERT_NO_MSG(player);

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

static bool transfer_items(std::vector<ItemSlot>& to, std::vector<ItemSlot>& from) {
  for (auto& slot : from) {
    if (!transfer_items(to, slot)) {
      return false;
    }
  }
  return true;
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
        ivec2 from_pos    = entity.pos + direction_to_ivec2(conveyor_from(*conveyor));
        auto* from_entity = get_entity_at_pos(store, from_pos, entity.world);
        if (from_entity && has_inventory(*from_entity) && !is<Player>(*from_entity)) {
          auto* from_inv = get_inventory(*from_entity);
          ASSERT(
            from_inv,
            "entity that satisifies HasInventory has to return an inventory from get_inventory()"
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
        ivec2 to_pos    = entity.pos + direction_to_ivec2(conveyor_to(*conveyor));
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

static void swap_slot_flags(std::vector<ItemSlot>& inventory) {
  for (auto& slot : inventory) {
    slot.flags ^= ITEM_SLOT_FLAGS_MASK;
  }
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

    Rectangle source_rect = {
      .x      = 0,
      .y      = 0,
      .width  = f32(texture->width),
      .height = f32(texture->height),
    };

    Rectangle dest_rect = {
      .x = f32((entity.pos.x * GRID_DIMS.x) + ((GRID_DIMS.x - texture->width) / 2)) +
           (f32(texture->width) * 0.5f),
      .y = f32((entity.pos.y * GRID_DIMS.y) + ((GRID_DIMS.y - texture->height) / 2)) +
           (f32(texture->height) * 0.5f),
      .width  = f32(texture->width),
      .height = f32(texture->height),
    };

    Vector2 origin = Vector2{f32(texture->width), f32(texture->height)} * 0.5f;

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
          ivec2 dims        = {on_texture.width, on_texture.height};
          Vector2 on_origin = vector2_from_ivec2(dims) * 0.5f * ON_CONVEYOR_SCALE;

          Rectangle on_source_rect = {0, 0, f32(dims.x), f32(dims.y)};

          Rectangle on_dest_rect = {
            .x = f32((entity.pos.x * GRID_DIMS.x) + ((GRID_DIMS.x - dims.x) / 2)) +
                 (f32(dims.x) * 0.5f),
            .y = f32((entity.pos.y * GRID_DIMS.y) + ((GRID_DIMS.y - dims.y) / 2)) +
                 (f32(dims.y) * 0.5f),
            .width  = f32(dims.x) * ON_CONVEYOR_SCALE,
            .height = f32(dims.y) * ON_CONVEYOR_SCALE,
          };

          on_dest_rect.x +=
            (f32(direction_to_ivec2(conveyor_from(*conveyor)).x) * 0.5f) * GRID_DIMS.x;
          on_dest_rect.y +=
            (f32(direction_to_ivec2(conveyor_from(*conveyor)).y) * 0.5f) * GRID_DIMS.y;

          on_dest_rect.x -=
            ((f32(direction_to_ivec2(conveyor_from(*conveyor)).x) * 0.5f) * item.t) * GRID_DIMS.x;
          on_dest_rect.y -=
            ((f32(direction_to_ivec2(conveyor_from(*conveyor)).y) * 0.5f) * item.t) * GRID_DIMS.y;

          on_dest_rect.x +=
            ((f32(direction_to_ivec2(conveyor_to(*conveyor)).x) * 0.5f) * item.t) * GRID_DIMS.x;
          on_dest_rect.y +=
            ((f32(direction_to_ivec2(conveyor_to(*conveyor)).y) * 0.5f) * item.t) * GRID_DIMS.y;

          DrawTexturePro(on_texture, on_source_rect, on_dest_rect, on_origin, 0, WHITE);
        }
      }
    }
  }
}
