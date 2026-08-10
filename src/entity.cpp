#include "entity.h"

std::string_view world_to_string(World world) {
  switch (world) {
    case World::MAIN:
      return "main";
    case World::MESSAGING:
      return "messaging";
    case World::STORAGE:
      return "storage";
    case World::COUNT:
      break;
  }
  ASSERT(false, "invalid world: %d", i32(world));
}

// NOTE: find the y-intercept of a tangent line to a cog_a that is closer to cog_b
// takes an equation like this
// y * y_multipler = a * x + _
// the y_multiplier exists to account for equations in the form of x = ...
// (you just pass y_multiplier = 0, a = -1)
static f32 closer_y_intercept_of_tangent_line(const Cogwheel& cog_a, const Cogwheel& cog_b, f32 a) {
  f32 b_1 = (cog_a.radius * std::sqrt((a * a) + 1)) - (a * cog_a.pos.x) + cog_a.pos.y;
  f32 b_2 = -(cog_a.radius * std::sqrt((a * a) + 1)) - (a * cog_a.pos.x) + cog_a.pos.y;

  f32 d_1 = std::abs((a * cog_b.pos.x) - cog_b.pos.y + (b_1)) / std::sqrt((a * a) + 1);
  f32 d_2 = std::abs((a * cog_b.pos.x) - cog_b.pos.y + (b_2)) / std::sqrt((a * a) + 1);

  if (d_1 < d_2) {
    return b_1;
  }
  return b_2;
}

void maintenance_init_minigame(MaintenanceLubrication& state) {
  // TODO: randomize these in the future
  state.cogwheels.push_back({{64, 64}, 64.0f, WHITE});
  state.cogwheels.push_back({{156, 128}, 48.0f, GRAY});
  state.cogwheels.push_back({{64, 192}, 64.0f, LIGHTGRAY});
  state.cogwheels.push_back({{208, 208}, 48.0f, DARKGRAY});
  state.cogwheels.push_back({{145, 192}, 16.0f, {245, 244, 227, 255}});
  state.cogwheels.push_back({{164, 40}, 40.0f, {106, 86, 92, 255}});

  // NOTE: find the intersection point between every two cogs (treated as circles)
  // only exists if two circles are close enough to each other
  for (u32 i = 0; i < state.cogwheels.size(); ++i) {
    auto& cog_a = state.cogwheels[i];
    for (u32 j = i + 1; j < state.cogwheels.size(); ++j) {
      auto& cog_b = state.cogwheels[j];

      f32 dist_between_circles = length(cog_b.pos - cog_a.pos) - (cog_b.radius + cog_a.radius);
      if (dist_between_circles > 3.0f) {
        continue;
      }

      vec2 tangency_a{};
      vec2 tangency_b{};
      if (cog_a.pos.y == cog_b.pos.y) {
        ASSERT(cog_a.pos.x != cog_b.pos.x, "two circles cannot have the same position");
        tangency_a = cog_a.pos;
        tangency_b = cog_b.pos;
        if (cog_a.pos.x > cog_b.pos.x) {
          tangency_a.x -= cog_a.radius;
          tangency_b.x += cog_b.radius;
        } else {
          tangency_a.x += cog_a.radius;
          tangency_b.x -= cog_b.radius;
        }
      } else if (cog_a.pos.x == cog_b.pos.x) {
        tangency_a = cog_a.pos;
        tangency_b = cog_b.pos;
        if (cog_a.pos.y > cog_b.pos.y) {
          tangency_a.y -= cog_a.radius;
          tangency_b.y += cog_b.radius;
        } else {
          tangency_a.y += cog_a.radius;
          tangency_b.y -= cog_b.radius;
        }
      } else {
        // NOTE: line connecting the centers of the two circles
        // f(x) = a * x + b
        f32 a = (cog_b.pos.y - cog_a.pos.y) / (cog_b.pos.x - cog_a.pos.x);
        f32 b = cog_a.pos.y - a * cog_a.pos.x;

        // NOTE: line equations for tangent lines to the circles
        // g_a(x) = a_parallel * x + b_a
        // g_b(x) = a_parallel * x + b_b
        f32 a_parallel = -1.0f / a;
        f32 b_a        = closer_y_intercept_of_tangent_line(cog_a, cog_b, a_parallel);
        f32 b_b        = closer_y_intercept_of_tangent_line(cog_b, cog_a, a_parallel);

        // NOTE: tangency points
        // calculated as the intersection points of the line connecting the cirle centers
        // and the tangent line
        tangency_a.x = (a * (b_a - b)) / ((a * a) + 1);
        tangency_a.y = a * tangency_a.x + b;
        tangency_b.x = (a * (b_b - b)) / ((a * a) + 1);
        tangency_b.y = a * tangency_b.x + b;
      }

      vec2 intersection{(tangency_a + tangency_b) / 2.0f};
      state.points.push_back({{16, 16}, intersection, BROWN});
    }
  }
}

bool maintenance_update_minigame(MaintenanceLubrication& state, const Input& input, f32 dt) {
  bool done = true;
  for (auto& point : state.points) {
    vec2 origin = point.dims / 2.0f;
    if (
      input.lmb.down && CheckCollisionRecs(
                          rect_from_vec2x2(point.pos + state.window_offset - origin, point.dims),
                          rect_from_vec2x2(input.mouse_pos, {1, 1})
                        )
    ) {
      if (point.progress < 1.0f) {
        point.progress += dt * (1.0f / LubricationPoint::TIME_TO_LUBRICATE);
      }
    }

    if (point.progress < 1.0f) {
      done = false;
    }
  }
  return done;
}

void maintenance_render_minigame(
  MaintenanceLubrication& state,
  const AssetManager&,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
) {
  // TODO: i really really dont like this, dont know how else to do it tho
  state.window_offset = window_offset;

  BeginTextureMode(render_texture);
  ClearBackground(BLACK);
  for (const auto& cog : state.cogwheels) {
    DrawCircleV(cog.pos.to_raylib(), cog.radius, cog.color);
  }
  for (const auto& point : state.points) {
    Color color = point.color;
    if (point.progress >= 1.0f) {
      color = YELLOW;
    }
    DrawRectanglePro(
      rect_from_vec2x2(point.pos, point.dims),
      (point.dims / 2.0f).to_raylib(),
      0,
      color
    );
  }
  EndTextureMode();
}

// TODO: sometimes it seems like the dirty rects get initialized with progress > 0
void maintenance_init_minigame(MaintenanceCleaning& state) {
  static constexpr vec2 MAX_DIMS = {96, 96};
  for (i32 i = 0; i < 3; ++i) {
    Rectangle area{};
    area.x =
      random_get<f32>(state.OUTER_RECT.x, state.OUTER_RECT.x + state.OUTER_RECT.width - MAX_DIMS.x);
    area.y = random_get<f32>(
      state.OUTER_RECT.y,
      state.OUTER_RECT.y + state.OUTER_RECT.height - MAX_DIMS.y
    );
    area.width  = random_get<f32>(32, MAX_DIMS.x);
    area.height = random_get<f32>(32, MAX_DIMS.y);

    state.dirty_rects.push_back({area});
  }
}

bool maintenance_update_minigame(MaintenanceCleaning& state, const Input& input, f32) {
  bool done = true;

  for (auto& dirty_rect : state.dirty_rects) {
    if (dirty_rect.progress >= 1.0f) {
      continue;
    }
    done = false;

    auto dirty_rect_check = dirty_rect.area;
    dirty_rect_check.x += state.window_offset.x;
    dirty_rect_check.y += state.window_offset.y;

    if (
      input.lmb.down &&
      CheckCollisionRecs(dirty_rect_check, rect_from_vec2x2(input.mouse_pos, {1, 1}))
    ) {
      auto moved_dist = length(state.last_mouse_pos - input.mouse_pos);
      dirty_rect.progress += moved_dist * 0.003f;
      // NOTE: clamping to 1.0f to avoid weird rendering glitches with the opacity
      dirty_rect.progress = std::clamp(dirty_rect.progress, 0.0f, 1.0f);
      break;
    }
  }

  state.last_mouse_pos = input.mouse_pos;
  return done;
}

void maintenance_render_minigame(
  MaintenanceCleaning& state,
  const AssetManager&,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
) {
  state.window_offset = window_offset;

  BeginTextureMode(render_texture);
  ClearBackground(BLACK);
  DrawRectangleLinesEx(state.OUTER_RECT, 5, GRAY);
  DrawCircleV({128, 128}, 20, GRAY);
  DrawCircleLinesV({128, 128}, 112, GRAY);
  for (const auto& dirty_rect : state.dirty_rects) {
    Color color = BROWN;
    color.a *= (1.0f - dirty_rect.progress);
    DrawRectanglePro(dirty_rect.area, {}, 0, color);
  }
  EndTextureMode();
}

void maintenance_init_minigame(MaintenanceComponentReplacement& state) {
  state.slots[COMPONENT_SLOT_BROKEN]  = {.pos = {64, 192}};
  state.slots[COMPONENT_SLOT_FIXED]   = {.pos = {192, 192}};
  state.slots[COMPONENT_SLOT_MACHINE] = {.pos = {128, 64}};

  state.broken = {
    .slot = COMPONENT_SLOT_MACHINE,
    .pos  = state.slots[COMPONENT_SLOT_MACHINE].pos,
  };
  state.fixed = {
    .slot = COMPONENT_SLOT_FIXED,
    .pos  = state.slots[COMPONENT_SLOT_FIXED].pos,
  };
}

static void update_component(
  Component& comp,
  const Component& other,
  std::span<ComponentSlot> slots,
  const Input& input,
  const vec2& window_offset
) {
  auto origin = comp.DIMS * 0.5f;
  if (
    input.lmb.pressed() && CheckCollisionRecs(
                             rect_from_vec2x2(comp.pos + window_offset - origin, comp.DIMS),
                             rect_from_vec2x2(input.mouse_pos, {1, 1})
                           )
  ) {
    comp.dragging = true;
  }
  if (comp.dragging) {
    comp.pos = input.mouse_pos - window_offset;

    if (!input.lmb.down) {
      comp.dragging = false;
      for (u32 slot_idx = 0; slot_idx < slots.size(); ++slot_idx) {
        auto& slot       = slots[slot_idx];
        auto slot_origin = slot.DIMS * 0.5f;
        if (
          other.slot != slot_idx &&
          CheckCollisionRecs(
            rect_from_vec2x2(slot.pos + window_offset - slot_origin, slot.DIMS),
            rect_from_vec2x2(input.mouse_pos, {1, 1})
          )
        ) {
          comp.slot = ComponentSlotType(slot_idx);
        }
      }
      comp.pos = slots[comp.slot].pos;
    }
  }
}

bool maintenance_update_minigame(MaintenanceComponentReplacement& state, const Input& input, f32) {
  update_component(state.broken, state.fixed, state.slots, input, state.window_offset);
  update_component(state.fixed, state.broken, state.slots, input, state.window_offset);

  return state.fixed.slot == COMPONENT_SLOT_MACHINE;
}

static void
render_component(Component& comp, const AssetManager& assets, TextureType texture_type) {
  auto& texture     = assets.textures[texture_type];
  auto texture_dims = dims_from_texture(texture);
  auto source_rect  = rect_from_vec2x2({}, texture_dims);
  auto dest_rect    = rect_from_vec2x2(comp.pos, comp.DIMS);
  auto origin       = comp.DIMS * 0.5f;
  DrawTexturePro(texture, source_rect, dest_rect, origin.to_raylib(), 0, WHITE);
}

void maintenance_render_minigame(
  MaintenanceComponentReplacement& state,
  const AssetManager& assets,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
) {
  state.window_offset = window_offset;

  BeginTextureMode(render_texture);
  ClearBackground(BLACK);
  for (const auto& slot : state.slots) {
    auto rect   = rect_from_vec2x2(slot.pos, slot.DIMS);
    auto origin = slot.DIMS * 0.5f;
    DrawRectanglePro(rect, origin.to_raylib(), 0, LIGHTGRAY);
  }
  render_component(state.broken, assets, get_texture_type(state.FIX_ITEM));
  render_component(state.fixed, assets, get_texture_type(state.FIX_ITEM));
  EndTextureMode();
}

void maintenance_init_minigame(MaintenanceCalibration& state) {
  state.range_low  = random_get<f32>(30.0f, 50.0f);
  state.range_low  = std::round(state.range_low * 10.0f) / 10.0f;
  state.range_high = state.range_low + random_get<f32>(1.0f, 50.0f);
  state.range_high = std::round(state.range_high * 10.0f) / 10.0f;
  // TODO: not sure if i want for it to be possible for the value to start in the range
  state.value = random_get<f32>(0.0f, 100.0f);
  state.value = std::round(state.value * 10.0f) / 10.0f;
}

bool maintenance_update_minigame(MaintenanceCalibration& state, const Input& input, f32 dt) {
  ASSERT_NO_MSG(
    state.add_rect.width == state.remove_rect.width &&
    state.add_rect.height == state.remove_rect.height
  );
  vec2 origin = vec2{state.add_rect.width, state.add_rect.height} / 2.0f;

  auto check_add_rect = rect_from_vec2x2(
    vec2{state.add_rect.x, state.add_rect.y} + state.window_offset - origin,
    {state.add_rect.width, state.add_rect.height}
  );
  if (
    input.lmb.down && CheckCollisionRecs(check_add_rect, rect_from_vec2x2(input.mouse_pos, {1, 1}))
  ) {
    state.value += 0.1f;
  }
  auto check_remove_rect = rect_from_vec2x2(
    vec2{state.remove_rect.x, state.remove_rect.y} + state.window_offset - origin,
    {state.remove_rect.width, state.remove_rect.height}
  );
  if (
    input.lmb.down &&
    CheckCollisionRecs(check_remove_rect, rect_from_vec2x2(input.mouse_pos, {1, 1}))
  ) {
    state.value -= 0.1f;
  }
  state.value = std::round(state.value * 10.0f) / 10.0f;

  if (state.range_low <= state.value && state.value <= state.range_high) {
    state.t += dt;
  } else {
    state.t = 0;
  }

  return state.t >= 0.5f;
}

// TODO: display the range and the value as a wave
// the middle of the range would be one displayed wavelength
// and the value would be the other displayed wavelength
// amplitude would be constant???
// and you have to get the value wavelength near the range wavelength
void maintenance_render_minigame(
  MaintenanceCalibration& state,
  const AssetManager&,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
) {
  state.window_offset = window_offset;

  ASSERT_NO_MSG(
    state.add_rect.width == state.remove_rect.width &&
    state.add_rect.height == state.remove_rect.height
  );
  vec2 origin     = vec2{state.add_rect.width, state.add_rect.height} / 2.0f;
  auto value_text = std::format("{}", state.value);
  auto range_text = std::format("Expected range:\n[{};{}]", state.range_low, state.range_high);

  BeginTextureMode(render_texture);
  ClearBackground(BLACK);
  DrawTextPro(
    GetFontDefault(),
    range_text.c_str(),
    {128, 64},
    MeasureTextEx(GetFontDefault(), range_text.c_str(), 20, 2) / 2.0f,
    0,
    20,
    2,
    WHITE
  );
  DrawTextPro(
    GetFontDefault(),
    value_text.c_str(),
    {128, 128},
    MeasureTextEx(GetFontDefault(), value_text.c_str(), 20, 2) / 2.0f,
    0,
    20,
    2,
    WHITE
  );
  DrawRectanglePro(state.add_rect, origin.to_raylib(), 0, GREEN);
  DrawRectanglePro(state.remove_rect, origin.to_raylib(), 0, RED);
  EndTextureMode();
}

std::string_view maintenance_name(const Maintenance& maintenance) {
  return std::visit(
    [](const auto& value) -> std::string_view {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, std::monostate>) {
        return "null";
      } else {
        return value.NAME;
      }
    },
    maintenance
  );
}

ItemType maintenance_fix_item(const Maintenance& maintenance) {
  return std::visit(
    [](const auto& value) -> ItemType {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, std::monostate>) {
        ASSERT(false, "null maintenance has no fix item");
      } else {
        return value.FIX_ITEM;
      }
    },
    maintenance
  );
}

bool* maintenance_is_minigame_open(Maintenance& maintenance) {
  return std::visit(
    [](auto& value) -> bool* {
      using T = std::decay_t<decltype(value)>;
      if constexpr (MaintenanceHasMiniGame<T>) {
        return &value.open;
      }
      return nullptr;
    },
    maintenance
  );
}

void maintenance_init_minigame(Maintenance& maintenance) {
  std::visit(
    [](auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (MaintenanceHasMiniGame<T>) {
        maintenance_init_minigame(value);
      }
    },
    maintenance
  );
}

bool maintenance_update_minigame(Maintenance& maintenance, const Input& input, f32 dt) {
  return std::visit(
    [&](auto& value) -> bool {
      using T = std::decay_t<decltype(value)>;
      if constexpr (MaintenanceHasMiniGame<T>) {
        return maintenance_update_minigame(value, input, dt);
      }
      return false;
    },
    maintenance
  );
}

void maintenance_render_minigame(
  Maintenance& maintenance,
  const AssetManager& assets,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
) {
  std::visit(
    [&](auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (MaintenanceHasMiniGame<T>) {
        maintenance_render_minigame(value, assets, render_texture, window_offset);
      }
    },
    maintenance
  );
}

std::span<ResourceMessage> get_first_resource_message_batch(ResourceMessageQueue& queue) {
  if (queue.msgs.empty()) {
    return {};
  }
  u32 first_batch_number = queue.msgs[0].batch_number;
  u32 batch_end{};
  for (auto& msg : queue.msgs) {
    if (msg.batch_number != first_batch_number) {
      break;
    }
    ++batch_end;
  }
  return {queue.msgs.begin(), queue.msgs.begin() + batch_end};
}

std::span<ResourceMessage> get_last_resource_message_batch(ResourceMessageQueue& queue) {
  if (queue.msgs.empty()) {
    return {};
  }
  u32 last_batch_number = queue.msgs.back().batch_number;
  u32 batch_start{};
  for (i32 i = i32(queue.msgs.size()) - 1; i >= 0; --i) {
    auto& msg = queue.msgs[i];
    if (msg.batch_number != last_batch_number) {
      break;
    }
    batch_start = i;
  }
  return {queue.msgs.begin() + batch_start, queue.msgs.end()};
}

static bool
fits_in_last_batch(ResourceMessageQueue& queue, const ResourceMessage& msg, u64 game_time) {
  ResourceMessage dummy_msg{};
  auto last_batch = get_last_resource_message_batch(queue);
  if (last_batch.empty()) {
    return true;
  }

  // NOTE: time check
  if (last_batch[0].arrival_time - game_time <= 3 * 60) {
    return false;
  }

  // NOTE: item count check
  for (const auto& batch_msg : last_batch) {
    for (u32 i = 0; i < dummy_msg.requested_items.size(); ++i) {
      dummy_msg.requested_items[i] += batch_msg.requested_items[i];
    }
  }
  for (u32 i = 0; i < dummy_msg.requested_items.size(); ++i) {
    dummy_msg.requested_items[i] += msg.requested_items[i];
  }
  for (u32 i = 0; i < dummy_msg.requested_items.size(); ++i) {
    if (dummy_msg.requested_items[i] > item_info(REQUESTABLE_ITEMS[i]).max_count) {
      return false;
    }
  }
  return true;
}

void add_resource_message(ResourceMessageQueue& queue, ResourceMessage& msg, u64 game_time) {
  // TODO: maybe calculate this from the amount of items requested?
  static constexpr u64 MINUTES_TO_ARRIVAL = 10;

  if (queue.msgs.empty()) {
    msg.batch_number = 0;
    msg.arrival_time = game_time + MINUTES_TO_ARRIVAL;
  } else {
    if (fits_in_last_batch(queue, msg, game_time)) {
      msg.batch_number = queue.msgs.back().batch_number;
      msg.arrival_time = queue.msgs.back().arrival_time;
    } else {
      msg.batch_number = queue.msgs.back().batch_number + 1;
      msg.arrival_time = game_time + MINUTES_TO_ARRIVAL;
    }
  }
  queue.msgs.push_back(msg);
}

void remove_resource_message(ResourceMessageQueue& queue, u32 idx) {
  queue.msgs.erase(queue.msgs.begin() + idx);
}

bool resource_message_receiver_empty(const ResourceMessageReceiver& msg_receiver) {
  for (auto& slot : msg_receiver.inventory) {
    if (slot) {
      return false;
    }
  }
  return true;
}

ItemSlot& assembler_input_slot(Assembler& assembler, u32 idx) {
  ASSERT_NO_MSG(idx < Recipe::MAX_INPUT_SLOTS);
  return assembler.inventory[idx];
}

ItemSlot& assembler_output_slot(Assembler& assembler, u32 idx) {
  ASSERT_NO_MSG(idx < Recipe::MAX_OUTPUT_SLOTS);
  return assembler.inventory[idx + Recipe::MAX_INPUT_SLOTS];
}

EntityIterator begin(EntityStore& store) {
  EntityIterator iter{};
  iter.curr = store.entities.data();
  iter.end  = iter.curr + store.entities.size();
  while (iter.curr < iter.end && !iter.curr->id) {
    ++iter.curr;
  }
  return iter;
}

EntityIterator end(EntityStore& store) {
  EntityIterator iter{};
  iter.curr = iter.end = store.entities.data() + store.entities.size();
  return iter;
}

static EntityId get_next_entity_id(EntityStore& store) {
  if (!store.free_slots.empty()) {
    auto id = store.free_slots.back();
    store.free_slots.pop_back();
    ++id.gen;
    return id;
  }
  ASSERT(store.next_entity_idx < U16_MAX, "exceeded max entity index");
  ++store.next_entity_idx;
  return {.idx = store.next_entity_idx, .gen = 0};
}

EntityId add_entity(EntityStore& store, const Entity& entity) {
  store.command_buffer.push_back(
    AddCommand{
      .entity = entity,
    }
  );
  auto* cmd = std::get_if<AddCommand>(&store.command_buffer.back());
  ASSERT_NO_MSG(cmd);
  cmd->entity.id = get_next_entity_id(store);
  return cmd->entity.id;
}

void remove_entity(EntityStore& store, EntityId id) {
  store.command_buffer.push_back(
    RemoveCommand{
      .id = id,
    }
  );
}

bool contains_entity(EntityStore& store, EntityId id) {
  if (!id || id.idx > store.entities.size()) {
    return false;
  }
  auto& entity = store.entities[id.idx - 1];
  return entity.id == id;
}

Entity* get_entity(EntityStore& store, EntityId id) {
  if (!id || id.idx > store.entities.size()) {
    return nullptr;
  }
  auto& entity = store.entities[id.idx - 1];
  if (entity.id != id) {
    return nullptr;
  }
  return &entity;
}

Entity* get_entity_at_pos(EntityStore& store, const vec2& pos, World world) {
  for (auto& entity : store) {
    if (entity.world == world && entity.pos == pos) {
      return &entity;
    }
  }
  return nullptr;
}

std::vector<Entity*> get_entities_at_pos(EntityStore& store, const vec2& pos, World world) {
  std::vector<Entity*> entities{};
  for (auto& entity : store) {
    if (entity.world == world && entity.pos == pos) {
      entities.push_back(&entity);
    }
  }
  return entities;
}

void emit(EntityStore& store, const Event& event) {
  store.event_bus.push_back(event);
}

void flush(EntityStore& store) {
  for (auto& cmd : store.command_buffer) {
    std::visit(
      overloaded{
        [&](const AddCommand& cmd) {
          if (store.entities.size() < store.next_entity_idx) {
            store.entities.resize(store.next_entity_idx);
          }
          store.entities[cmd.entity.id.idx - 1] = cmd.entity;
        },
        [&](const RemoveCommand& cmd) {
          store.entities[cmd.id.idx - 1] = {};
          store.free_slots.push_back(cmd.id);
        },
      },
      cmd
    );
  }
  store.command_buffer.clear();
}

void clear_event_bus(EntityStore& store) {
  store.event_bus.clear();
}

bool rotatable(const Entity& entity) {
  return std::visit(
    [](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      return Rotatable<T>;
    },
    entity.data
  );
}

std::optional<bool> rotatable(ItemType type) {
  switch (type) {
    case ITEM_BLOCK:
      return {Rotatable<Block>};
    case ITEM_STORAGE:
      return {Rotatable<Storage>};
    case ITEM_CONVEYOR:
      return {Rotatable<Conveyor>};
    case ITEM_ASSEMBLER:
      return {Rotatable<Assembler>};
    case ITEM_COPPER:
    case ITEM_PLASTIC:
    case ITEM_ALUMINIUM:
    case ITEM_COPPER_WIRE:
    case ITEM_CIRCUIT_BOARD:
    case ITEM_ANTENNA:
    case ITEM_COMMUNICATION_COMPONENT:
    case ITEM_WIRE_BUNDLE:
    case ITEM_COGWHEEL:
    case ITEM_SPARE_PARTS:
    case ITEM_BRUSH:
    case ITEM_LUBRICANT_CAN:
    case ITEM_OIL_CANISTER:
    case ITEM_CALIBRATOR:
    case ITEM_SILICON_WAFER:
    case ITEM_BLANK_BOARD:
    case ITEM_TRANSISTOR:
    case ITEM_CAPACITOR:
      return std::nullopt;
    case ITEM_COUNT:
      break;
  }
  ASSERT(false, "invalid item type: %d\n", i32(type));
}

bool solid(const Entity& entity) {
  return std::visit(
    [](auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (
        is_any_of<T, Block, Storage, ResourceMessageSender, ResourceMessageReceiver, Assembler>
      ) {
        return true;
      } else if constexpr (is_any_of<T, Player, Conveyor, Item, WorldTunnel>) {
        return false;
      } else {
        static_assert(false);
      }
    },
    entity.data
  );
}

bool breakable(const Entity& entity) {
  return std::visit(
    [](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (is_any_of<T, Storage, Conveyor, Assembler>) {
        return true;
      } else if constexpr (
        is_any_of<
          T,
          Block,
          Player,
          Item,
          WorldTunnel,
          ResourceMessageSender,
          ResourceMessageReceiver>
      ) {
        return false;
      } else {
        static_assert(false);
      }
    },
    entity.data
  );
}

bool has_gui(const Entity& entity) {
  return std::visit(
    [](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (
        is_any_of<
          T,
          Storage,
          WorldTunnel,
          ResourceMessageSender,
          ResourceMessageReceiver,
          Assembler>
      ) {
        return true;
      } else if constexpr (is_any_of<T, Block, Player, Conveyor, Item>) {
        return false;
      } else {
        static_assert(false);
      }
    },
    entity.data
  );
}

bool has_inventory(const Entity& entity) {
  return std::visit(
    [](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      return HasInventory<T>;
    },
    entity.data
  );
}

bool has_maintenance(const Entity& entity) {
  return std::visit(
    [](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      return HasMaintenance<T>;
    },
    entity.data
  );
};

std::optional<ItemType> entity_to_item(const Entity& entity) {
  return std::visit(
    overloaded{
      [](const Player&) -> std::optional<ItemType> {
        return std::nullopt;
      },
      [](const Block&) -> std::optional<ItemType> {
        return {ITEM_BLOCK};
      },
      [](const Storage&) -> std::optional<ItemType> {
        return {ITEM_STORAGE};
      },
      [](const Conveyor&) -> std::optional<ItemType> {
        return {ITEM_CONVEYOR};
      },
      [](const Item&) -> std::optional<ItemType> {
        return std::nullopt;
      },
      [](const WorldTunnel&) -> std::optional<ItemType> {
        return std::nullopt;
      },
      [](const ResourceMessageSender&) -> std::optional<ItemType> {
        return std::nullopt;
      },
      [](const ResourceMessageReceiver&) -> std::optional<ItemType> {
        return std::nullopt;
      },
      [](const Assembler&) -> std::optional<ItemType> {
        return {ITEM_ASSEMBLER};
      }
    },
    entity.data
  );
}

std::optional<Entity> entity_from_item(ItemType item) {
  switch (item) {
    case ITEM_BLOCK:
      return {{.data = Block{}}};
    case ITEM_STORAGE:
      return {{.data = Storage{}}};
    case ITEM_CONVEYOR:
      return {{.data = Conveyor{}}};
    case ITEM_ASSEMBLER:
      return {{.data = Assembler{}}};
    case ITEM_COPPER:
    case ITEM_PLASTIC:
    case ITEM_ALUMINIUM:
    case ITEM_COPPER_WIRE:
    case ITEM_CIRCUIT_BOARD:
    case ITEM_ANTENNA:
    case ITEM_COMMUNICATION_COMPONENT:
    case ITEM_WIRE_BUNDLE:
    case ITEM_COGWHEEL:
    case ITEM_SPARE_PARTS:
    case ITEM_BRUSH:
    case ITEM_LUBRICANT_CAN:
    case ITEM_OIL_CANISTER:
    case ITEM_CALIBRATOR:
    case ITEM_SILICON_WAFER:
    case ITEM_BLANK_BOARD:
    case ITEM_TRANSISTOR:
    case ITEM_CAPACITOR:
      return std::nullopt;
    case ITEM_COUNT:
      break;
  }
  ASSERT(false, "invalid item type: %d\n", i32(item));
}

TextureType get_texture_type(const Entity& entity) {
  return std::visit(
    overloaded{
      [](const Player&) {
        return TEXTURE_PLAYER;
      },
      [](const Block&) {
        return TEXTURE_BLOCK;
      },
      [](const Storage&) {
        return TEXTURE_STORAGE;
      },
      [](const Conveyor&) {
        return TEXTURE_CONVEYOR;
      },
      [](const Item&) -> TextureType {
        ASSERT(
          false,
          "the item entity does not have a render rect, use its items render rect instead"
        );
      },
      [](const WorldTunnel&) {
        return TEXTURE_WORLD_TUNNEL;
      },
      [](const ResourceMessageSender&) {
        return TEXTURE_MESSAGE_SENDER;
      },
      [](const ResourceMessageReceiver&) {
        return TEXTURE_MESSAGE_RECEIVER;
      },
      [](const Assembler&) {
        return TEXTURE_ASSEMBLER;
      }
    },
    entity.data
  );
}

Rotation* get_rotation(Entity& entity) {
  return std::visit(
    [](auto& value) -> Rotation* {
      using T = std::decay_t<decltype(value)>;
      if constexpr (Rotatable<T>) {
        return &value.rotation;
      } else {
        return nullptr;
      }
    },
    entity.data
  );
}

Rotation* get_rotation(EntityStore& store, EntityId id) {
  auto* entity = get_entity(store, id);
  if (entity) {
    return get_rotation(*entity);
  }
  return nullptr;
}

std::vector<ItemSlot>* get_inventory(Entity& entity) {
  return std::visit(
    [](auto& value) -> std::vector<ItemSlot>* {
      using T = std::decay_t<decltype(value)>;
      if constexpr (HasInventory<T>) {
        return &value.inventory;
      } else {
        return nullptr;
      }
    },
    entity.data
  );
}

std::vector<ItemSlot>* get_inventory(EntityStore& store, EntityId id) {
  auto* entity = get_entity(store, id);
  if (entity) {
    return get_inventory(*entity);
  }
  return nullptr;
}

std::tuple<Maintenance*, std::span<const Maintenance>> get_maintenance(Entity& entity) {
  return std::visit(
    [](auto& value) -> std::tuple<Maintenance*, std::span<const Maintenance>> {
      using T = std::decay_t<decltype(value)>;
      if constexpr (HasMaintenance<T>) {
        return {&value.maintenance, value.POSSIBLE_MAINTENANCE};
      } else {
        return {nullptr, {}};
      }
    },
    entity.data
  );
}

std::tuple<Maintenance*, std::span<const Maintenance>>
get_maintenance(EntityStore& store, EntityId id) {
  auto* entity = get_entity(store, id);
  if (entity) {
    return get_maintenance(*entity);
  }
  return {nullptr, {}};
}

void render_entities(EntityStore& store, World world, const AssetManager& assets) {
  static constexpr f32 ON_CONVEYOR_SCALE = 0.375f;

  for (auto& entity : store) {
    if (entity.world != world) {
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

    auto origin = (dims * 0.5f).to_raylib();

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
          Vector2 on_origin = on_dims.to_raylib() * 0.5f * ON_CONVEYOR_SCALE;

          auto on_source_rect = rect_from_vec2x2({}, on_dims);

          Rectangle on_dest_rect = {
            .x      = (entity.pos.x * GRID_DIMS.x) + ((GRID_DIMS.x - on_dims.x) / 2.0f) +
                      (on_dims.x * 0.5f),
            .y      = (entity.pos.y * GRID_DIMS.y) + ((GRID_DIMS.y - on_dims.y) / 2.0f) +
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
