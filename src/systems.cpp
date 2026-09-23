#include "systems.h"

#include <array>
#include <ranges>
#include <algorithm>
#include <variant>
#include <cmath>

#include "core.h"
#include "utils.h"
#include "entity.h"
#include "input.h"
#include "items.h"

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

void system_move_player(EntityStore& store, EntityId player_id, const Input& input, f32 dt) {
  auto [player_entity, player] = get_entity_and_data<Player>(store, player_id);
  ASSERT_NO_MSG(player_entity && player);
  auto& curr_move = player->current_movement;

  // TODO: do i somehow prioritize the newest input?
  static constexpr std::array<std::pair<Action, Direction>, 4> MOVEMENT_DIRECTIONS = {{
    {ACTION_MOVE_UP, DIR_UP},
    {ACTION_MOVE_DOWN, DIR_DOWN},
    {ACTION_MOVE_RIGHT, DIR_RIGHT},
    {ACTION_MOVE_LEFT, DIR_LEFT},
  }};
  std::optional<MovementAction> movement{};
  for (auto& [action, direction] : MOVEMENT_DIRECTIONS) {
    if (action_state(input, action).down) {
      movement = {.direction = direction};
      break;
    }
  }

  if (movement) {
    bool can_move = true;
    auto collided = get_entities_at_pos(
      store,
      player_entity->world,
      player_entity->pos + direction_to_vec2(movement->direction),
      Player::DIMS
    );

    for (auto& collision : collided) {
      movement->collision_events.push_back({
        .type   = EVENT_PLAYER_COLLIDED,
        .entity = collision->id,
      });
      if (solid(*collision)) {
        can_move = false;
      }
    }

    if (can_move) {
      if (curr_move && movement->direction == opposite_direction(curr_move->direction)) {
        curr_move->collision_events = movement->collision_events;
        curr_move->direction        = movement->direction;
        curr_move->t                = -curr_move->t;
      } else if (!curr_move) {
        curr_move = movement;
      }
    }
  }

  if (curr_move) {
    curr_move->t += dt;
    if (curr_move->t >= PLAYER_MOVE_ACTION_DURATION) {
      player_entity->pos += direction_to_vec2(curr_move->direction);
      for (auto& event : curr_move->collision_events) {
        emit(store, event);
      }
      curr_move = std::nullopt;
    }
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

  if (
    action_state(input, ACTION_INTERACT).pressed() &&
    pos_in_radius(mouse_grid_pos, player_entity->pos, player->interaction_radius)
  ) {
    auto hovered = get_entity_at_pos(store, player_entity->world, mouse_grid_pos, CURSOR_DIMS);
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
    if (
      action_state(input, ACTION_CLOSE_INV).pressed() ||
      (gui_entity &&
       !pos_in_radius(gui_entity->pos, player_entity->pos, player->interaction_radius)) ||
      (gui_entity && gui_entity->world != player_entity->world) || !gui_entity
    ) {
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
      if (input.keys[GKEY_LCTRL].down) {
        slot.locked = !slot.locked;
      } else {
        auto& hand = player->hand;
        ASSERT(hand.flags == ITEM_SLOT_FLAGS_ALL, "player hand has to be input and output");

        swap_items(slot, hand, ITEM_TRANSFER_HAND);
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
  if (
    input.lmb.pressed() && player->hand &&
    pos_in_radius(mouse_grid_pos, player_entity->pos, player->interaction_radius)
  ) {
    auto hovered = get_entity_at_pos(store, player_entity->world, mouse_grid_pos, CURSOR_DIMS);
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

// NOTE: currently voiding items that cannot fit into the message receivers inventory
// is that really the behaviour i want? (its not)
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
  if (
    !input.rmb.pressed() || !player->hand ||
    !pos_in_radius(mouse_grid_pos, player_entity->pos, player->interaction_radius)
  ) {
    return;
  }

  auto entity = entity_from_item(player->hand.type);
  if (!entity) {
    return;
  }
  auto dims = get_dims(*entity);
  if (get_entity_at_pos(store, player_entity->world, mouse_grid_pos, dims, place_rotation)) {
    return;
  }

  entity->pos   = mouse_grid_pos;
  entity->world = player_entity->world;
  if (auto* rotation = get_rotation(*entity)) {
    *rotation = place_rotation;
  }
  // TODO: setup before/after place hooks, instead of this shit
  // or maybe apply these through events?
  if (auto* conveyor = get_data<Conveyor>(*entity)) {
    // TODO: this is kind of weird, but i dont know what else to do
    conveyor->to = conveyor->rotation;
    set_conveyor_from_direction(store, *entity);
  }
  if (is<Balancer>(*entity)) {
    set_balancer_moves_from_position(*entity);
  }
  add_entity(store, *entity);
  --player->hand.count;
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

  if (
    input.lmb.pressed() &&
    pos_in_radius(mouse_grid_pos, player_entity->pos, player->interaction_radius)
  ) {
    auto hovered = get_entity_at_pos(store, player_entity->world, mouse_grid_pos, CURSOR_DIMS);
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
  static constexpr std::array<Direction, 4> SIDES = {{DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT}};

  // TODO: only do anything if a conveyor is attached?
  for (auto& entity : store) {
    auto output_properties = get_outputs_items_properties(entity);
    if (!output_properties.item_output_accumulator) {
      continue;
    }

    if (has_maintenance(entity)) {
      auto [maintenance, _] = get_maintenance(entity);
      if (!std::holds_alternative<std::monostate>(*maintenance)) {
        continue;
      }
    }

    *output_properties.item_output_accumulator += dt;

    if (*output_properties.item_output_accumulator >= (1.0f / output_properties.output_rate)) {
      auto* from_inv = get_inventory(entity);
      ASSERT(from_inv, "entities with OutputsItems must satisfy HasInventory");
      // TODO: is this enough to take rotation into account?
      auto rect = get_rect(entity);

      for (u32 y = 0; y < u32(rect.height); ++y) {
        for (u32 x = 0; x < u32(rect.width); ++x) {
          auto pos = entity.pos + vec2{f32(x), f32(y)};
          for (auto side : SIDES) {
            if (!(output_properties.output_sides[(rect.width * y) + x] & side)) {
              continue;
            }
            auto output_pos     = pos + direction_to_vec2(side);
            auto* output_entity = get_entity_at_pos(store, entity.world, output_pos, {1, 1});
            if (!output_entity) {
              continue;
            }
            auto props = get_moves_items_properties(*output_entity);
            if (!props) {
              continue;
            }

            for (u32 cell_idx = 0; cell_idx < props.from.size(); ++cell_idx) {
              auto from_pos = props.from[cell_idx];
              if (from_pos != pos) {
                continue;
              }

              for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
                auto lane_idx = mover_choose_lane_idx(*output_entity, cell_idx);
                auto& item    = props.items[lane_idx, i];
                bool can_pull = !item.slot;
                if (can_pull) {
                  auto* first_extractable = find_first_extractable_slot(*from_inv);
                  if (first_extractable) {
                    transfer_items(item.slot, *first_extractable, ITEM_TRANSFER_MACHINE, 1);
                    mover_update_lane_idx(*output_entity, cell_idx);
                  }
                  break;
                }
              }
            }
          }
        }
      }

      *output_properties.item_output_accumulator = 0;
    }
  }
}

// NOTE: cannot use transfer_items() in this function,
// because taking on an item and then clearing the from slot happen at two different points in time,
// and transfer_items() does both at once
void system_move_items(EntityStore& store, f32 dt) {
  // NOTE: move items that are already on the conveyor
  for (auto& entity : store) {
    auto props = get_moves_items_properties(entity);
    if (!props) {
      continue;
    }

    for (u32 lane_idx = 0; lane_idx < props.from.size(); ++lane_idx) {
      for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
        auto& item = props.items[lane_idx, i];
        if (item.slot) {
          if (item.t < conveyor_item_max_t(i)) {
            item.t += dt;
          }
        } else {
          item.t = 0;
        }
      }
    }
  }

  struct ConsumedSlot {
    EntityId entity_id{};
    u32 cell_idx{};

    bool operator==(const ConsumedSlot& other) const {
      return entity_id == other.entity_id && cell_idx == other.cell_idx;
    }
  };
  std::vector<ConsumedSlot> consumed_slots{};

  // NOTE: take items on
  for (auto& entity : store) {
    auto props = get_moves_items_properties(entity);
    if (!props) {
      continue;
    }

    for (u32 cell_idx = 0; cell_idx < props.from.size(); ++cell_idx) {
      auto from_pos     = props.from[cell_idx];
      auto* from_entity = get_entity_at_pos(store, entity.world, from_pos, {1, 1});

      if (from_entity && moves_items(*from_entity)) {
        const auto& from_props = get_moves_items_properties(*from_entity);
        ASSERT_NO_MSG(from_props);

        for (u32 from_cell_idx = 0; from_cell_idx < from_props.from.size(); ++from_cell_idx) {
          const auto& from_item = from_props.items[from_cell_idx, 0];

          if (
            mover_to_pos(*from_entity, from_cell_idx) == mover_cell_pos(entity, cell_idx) &&
            from_item.t >= 1.0f
          ) {
            u32 lane_idx = mover_choose_lane_idx(entity, cell_idx);

            for (u32 i = 0; i < CONVEYOR_THROUGHPUT; ++i) {
              auto& item = props.items[lane_idx, i];

              if (!item.slot) {
                assign_slot(item.slot, from_item.slot);
                item.t = 0;
                mover_update_lane_idx(entity, cell_idx);
                consumed_slots.push_back({
                  .entity_id = from_entity->id,
                  .cell_idx  = from_cell_idx,
                });
                break;
              }
            }
          }
        }
      }
    }
  }

  // NOTE: push items off
  for (auto& entity : store) {
    auto props = get_moves_items_properties(entity);
    if (!props) {
      continue;
    }

    for (u32 cell_idx = 0; cell_idx < props.from.size(); ++cell_idx) {
      auto& item = props.items[cell_idx, 0];
      if (item.t < 1.0f) {
        continue;
      }

      bool success = false;
      if (std::ranges::contains(consumed_slots, ConsumedSlot{entity.id, cell_idx})) {
        clear_slot(item.slot);
        success = true;
      } else {
        auto to_pos    = mover_to_pos(entity, cell_idx);
        auto to_entity = get_entity_at_pos(store, entity.world, to_pos, {1, 1});
        if (to_entity && !is<Player>(*to_entity) && !moves_items(*to_entity)) {
          if (auto* inv = get_inventory(*to_entity)) {
            success = transfer_items(*inv, item.slot, ITEM_TRANSFER_MACHINE);
          }
        }
      }

      if (success) {
        auto row = std::span<ConveyorItem>(
          props.items.data_handle() + (cell_idx * props.items.extent(1)),
          props.items.extent(1)
        );
        std::ranges::rotate(row, row.begin() + 1);
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

  camera.offset = vec2_to_raylib(window_dims / 2.0f);
  camera.target =
    vec2_to_raylib((player_actual_pos(*player_entity) * GRID_DIMS) + (GRID_DIMS / 2.0f));
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
