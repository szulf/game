#include "systems.h"
#include "core.h"
#include "entity.h"
#include "items.h"

#include <algorithm>

static bool pos_in_radius(const vec2& pos, const vec2& start_pos, f32 radius) {
  auto diff2 = length2(pos - start_pos);
  return diff2 < radius * radius;
}

void system_update_time(u64& min, f32& min_accumulator, f32 dt) {
  min_accumulator += dt;
  if (min_accumulator > 1.0f) {
    ++min;
    min_accumulator -= 1.0f;
  }
}

void system_move_player(EntityStore& store, EntityId player_id, Input& input) {
  auto move_vector = get_move_vector(input);
  if (move_vector == vec2{0, 0}) {
    return;
  }

  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);
  auto collided =
    get_entities_at_pos(store, player_entity->pos + move_vector, player_entity->world);
  bool can_move = true;

  for (auto& collision : collided) {
    emit(store, {.type = EVENT_PLAYER_COLLIDED, .entity = collision->id});
    if (solid(*collision)) {
      can_move = false;
    }
  }
  if (can_move) {
    player_entity->pos += move_vector;
  }
}

void system_open_gui(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  const vec2& mouse_world_pos
) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);
  auto mouse_grid_pos = grid_pos(mouse_world_pos);

  if (action_state(input, ACTION_INTERACT).pressed() &&
      pos_in_radius(mouse_grid_pos, player_entity->pos, player->interaction_radius)) {
    auto hovered = get_entity_at_pos(store, mouse_grid_pos, player_entity->world);
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
    if (action_state(input, ACTION_CLOSE_INV).pressed() ||
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

  if (hovered_slot && input.lmb.pressed()) {
    auto* hovered_inv = get_inventory(store, hovered_slot.entity);
    if (hovered_inv) {
      auto& slot = (*hovered_inv)[hovered_slot.slot_idx];
      auto& hand = player->hand;
      ASSERT(
        hand.flags == (ITEM_SLOT_HAND_INPUT | ITEM_SLOT_HAND_OUTPUT),
        "player hand has to be input and output"
      );

      if (slot && (slot.flags & ITEM_SLOT_HAND_INPUT) && hand && slot.type == hand.type) {
        auto max_count = item_info(slot.type).max_count;
        if (slot.count + hand.count > max_count) {
          hand.count = (slot.count + hand.count) - max_count;
          slot.count = max_count;
        } else {
          slot.count += hand.count;
          hand = {};
        }
      } else if (slot && (slot.flags & ITEM_SLOT_HAND_INPUT) &&
                 (slot.flags & ITEM_SLOT_HAND_OUTPUT) && hand) {
        swap_slots(slot, hand);
      } else if (slot && (slot.flags & ITEM_SLOT_HAND_OUTPUT) && !hand) {
        swap_slots(slot, hand);
      } else if (!slot && (slot.flags & ITEM_SLOT_HAND_INPUT) && hand) {
        swap_slots(slot, hand);
      }
    }
  }
}

void system_drop_items(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  const vec2& mouse_world_pos
) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);
  auto mouse_grid_pos = grid_pos(mouse_world_pos);

  // TODO: not sure if lmb_pressed is the right keybind
  if (input.lmb.pressed() && player->hand &&
      pos_in_radius(mouse_grid_pos, player_entity->pos, player->interaction_radius)) {
    auto hovered = get_entity_at_pos(store, mouse_grid_pos, player_entity->world);
    if (!hovered || is<Item>(*hovered)) {
      Entity entity = {
        .pos   = mouse_grid_pos,
        .world = player_entity->world,
        .data  = Item{.slot = player->hand},
      };
      add_entity(store, entity);
      player->hand = {};
    }
  }
}

enum ItemTransferMode {
  ITEM_TRANSFER_HAND,
  ITEM_TRANSFER_MACHINE,
};

// NOTE: returns whether it succeeded in transfering all items from the slot into the inventory
// also modified the slot to contain the left amount of items after the transfer
// so if it succeeded slot.count == 0
static bool
transfer_items(std::vector<ItemSlot>& inventory, ItemSlot& slot, ItemTransferMode mode) {
  ItemSlotFlag input_flag;
  ItemSlotFlag output_flag;
  switch (mode) {
    case ITEM_TRANSFER_HAND:
      input_flag  = ITEM_SLOT_HAND_INPUT;
      output_flag = ITEM_SLOT_HAND_OUTPUT;
      break;
    case ITEM_TRANSFER_MACHINE:
      input_flag  = ITEM_SLOT_MACHINE_INPUT;
      output_flag = ITEM_SLOT_MACHINE_OUTPUT;
      break;
    default:
      ASSERT_NO_MSG(false);
  }

  if (!(slot.flags & output_flag)) {
    return false;
  }

  for (u32 i = 0; i < inventory.size(); ++i) {
    if (!(inventory[i].flags & input_flag)) {
      continue;
    }

    // TODO: do i need to check (inventory[i] && slot) here?
    if (inventory[i].type == slot.type) {
      auto max_count = item_info(slot.type).max_count;
      if (inventory[i].count + slot.count > max_count) {
        slot.count         = (inventory[i].count + slot.count) - max_count;
        inventory[i].count = max_count;
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

static bool
transfer_items(std::vector<ItemSlot>& to, std::span<ItemSlot> from, ItemTransferMode mode) {
  for (auto& slot : from) {
    if (!transfer_items(to, slot, mode)) {
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
      std::array<ItemSlot, REQUESTABLE_ITEMS.size()> msg_items{};
      for (auto requestable_item : REQUESTABLE_ITEMS) {
        msg_items[requestable_item] = {
          .type  = requestable_item,
          .count = msg.requested_items[requestable_item],
        };
      }
      swap_slot_flags(msg_receiver->inventory);
      transfer_items(msg_receiver->inventory, msg_items, ITEM_TRANSFER_MACHINE);
      swap_slot_flags(msg_receiver->inventory);
      remove_resource_message(msg_queue, i);
    } else {
      ++i;
    }
  }
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
      if (assembler_output.count + recipe_output.count > item_info(recipe_output.type).max_count) {
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
  const vec2& mouse_world_pos,
  Direction place_rotation
) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);
  auto mouse_grid_pos = grid_pos(mouse_world_pos);

  // TODO: should check if im not hovering over an item slot
  if (input.rmb.pressed() && player->hand &&
      pos_in_radius(mouse_grid_pos, player_entity->pos, player->interaction_radius) &&
      !get_entity_at_pos(store, mouse_grid_pos, player_entity->world)) {
    auto entity = entity_from_item(player->hand.type);
    if (entity) {
      entity->pos   = mouse_grid_pos;
      entity->world = player_entity->world;
      if (auto* rotation = get_rotation(*entity)) {
        *rotation = place_rotation;
      }
      add_entity(store, *entity);
      --player->hand.count;
    }
  }
}

void system_remove_entity(
  EntityStore& store,
  EntityId player_id,
  const Input& input,
  const vec2& mouse_world_pos
) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);
  auto mouse_grid_pos = grid_pos(mouse_world_pos);

  if (input.lmb.pressed() &&
      pos_in_radius(mouse_grid_pos, player_entity->pos, player->interaction_radius)) {
    auto hovered = get_entity_at_pos(store, mouse_grid_pos, player_entity->world);
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

  for (auto& event : listen(store, EVENT_PLAYER_COLLIDED)) {
    auto* item = get_data<Item>(store, event.entity);
    if (item) {
      if (transfer_items(player->inventory, item->slot, ITEM_TRANSFER_HAND)) {
        remove_entity(store, event.entity);
      }
    }
  }
}

static ItemSlot* find_first_extractable_slot(std::vector<ItemSlot>& inventory) {
  for (u32 i = 0; i < inventory.size(); ++i) {
    if (inventory[i] && (inventory[i].flags & ITEM_SLOT_MACHINE_OUTPUT)) {
      return &inventory[i];
    }
  }
  return nullptr;
}

void system_output_items(EntityStore& store, f32 dt) {
  for (auto& entity : store) {
    auto output_properties = get_outputs_item_properties(entity);
    if (!output_properties.item_output_accumulator) {
      continue;
    }

    *output_properties.item_output_accumulator += dt;

    if (*output_properties.item_output_accumulator >= (1.0f / output_properties.output_rate)) {
      auto* from_inv = get_inventory(entity);
      ASSERT_NO_MSG(from_inv);

      for (auto dir : output_properties.output_sides) {
        auto output_pos     = entity.pos + direction_to_vec2(dir);
        auto* output_entity = get_entity_at_pos(store, output_pos, entity.world);
        if (!output_entity) {
          continue;
        }
        auto* conveyor = get_data<Conveyor>(*output_entity);
        if (!conveyor) {
          continue;
        }
        if (output_pos + direction_to_vec2(conveyor_from(*conveyor)) != entity.pos) {
          continue;
        }

        for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
          auto& item    = conveyor->items[i];
          bool can_pull = !item.slot;
          if (can_pull) {
            auto* first_extractable = find_first_extractable_slot(*from_inv);
            if (first_extractable) {
              // TODO: do i extract this into some function?
              // like somehow use transfer_items() here?
              item.slot.type  = first_extractable->type;
              item.slot.count = 1;
              if (item_info(first_extractable->type).has_durability) {
                item.slot.damage          = first_extractable->damage;
                first_extractable->damage = 0;
              }
              --first_extractable->count;
            }
            break;
          }
        }
      }
      *output_properties.item_output_accumulator = 0;
    }
  }
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
// 2. i think i duped an item somehow, no clue how tho (potentially fixable by fixing 1.)
// 3. if you have two conveyors on the side pushing into a conveyor in the middle,
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
            success = transfer_items(*to_inv, item.slot, ITEM_TRANSFER_MACHINE);
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

// TODO: not sure whether i want to travel via interaction or via walk into
void system_tunnel_through_worlds(EntityStore& store, EntityId player_id) {
  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);

  // NOTE: player
  for (auto& event : listen(store, EVENT_PLAYER_COLLIDED)) {
    auto [tunnel_entity, tunnel] = get_entity_and_data<WorldTunnel>(store, event.entity);
    ASSERT_NO_MSG(tunnel_entity);
    if (tunnel) {
      auto* corresponding_tunnel_entity = find_corresponding_world_tunnel(store, *tunnel_entity);
      ASSERT(corresponding_tunnel_entity, "there should always be a corresponding tunnel");
      player_entity->world = tunnel->to;
      player_entity->pos   = corresponding_tunnel_entity->pos;
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
      transfer_items(corresponding_tunnel->inventory, tunnel->inventory, ITEM_TRANSFER_MACHINE);
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

    // TODO: is this a good chance?
    auto value = random_get<u32>(1, 10000);
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

void system_update_camera(
  Camera2D& camera,
  const Input& input,
  EntityStore& store,
  EntityId player_id,
  const vec2& window_dims
) {
  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);

  camera.offset   = vec2_to_raylib(window_dims / 2.0f);
  camera.target   = vec2_to_raylib((player_entity->pos * GRID_DIMS) + (GRID_DIMS / 2.0f));
  camera.rotation = 0.0f;

  // TODO: copied from raylibs example, maybe something else feels better
  camera.zoom = std::exp(std::log(camera.zoom) + (input.mouse_scroll * 0.05f));
  camera.zoom = std::clamp(camera.zoom, 0.3f, 8.0f);
}

void system_render(EntityStore& store, EntityId player_id, const AssetManager& assets) {
  auto* player_entity = get_entity(store, player_id);
  ASSERT_NO_MSG(player_entity);

  render_entities(store, player_entity->world, assets);
}
