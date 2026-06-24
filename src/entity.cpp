bool rotatable(EntityType type) {
  switch (type) {
    case EntityType::PLAYER:
      return false;
    case EntityType::BLOCK:
      return false;
    case EntityType::STORAGE:
      return false;
    case EntityType::CONVEYOR:
      return true;
    case EntityType::ITEM:
      return false;
  }
  ASSERT(false);
}

bool solid(EntityType type) {
  switch (type) {
    case EntityType::PLAYER:
      return false;
    case EntityType::BLOCK:
      return true;
    case EntityType::STORAGE:
      return true;
    case EntityType::CONVEYOR:
      return true;
    case EntityType::ITEM:
      return false;
  }
  ASSERT(false);
}

bool breakable(EntityType type) {
  switch (type) {
    case EntityType::PLAYER:
      return false;
    case EntityType::BLOCK:
      return true;
    case EntityType::STORAGE:
      return true;
    case EntityType::CONVEYOR:
      return true;
    case EntityType::ITEM:
      return false;
  }
  ASSERT(false);
}

bool has_inventory(EntityType type) {
  return inventory_size(type) != 0;
}

u32 inventory_size(EntityType type) {
  switch (type) {
    case EntityType::PLAYER:
      return 16;
    case EntityType::STORAGE:
      return 32;
    case EntityType::BLOCK:
    case EntityType::CONVEYOR:
    case EntityType::ITEM:
      return 0;
  }
  ASSERT(false);
}

std::optional<ItemType> entity_to_item(EntityType entity) {
  switch (entity) {
    case EntityType::PLAYER:
      return std::nullopt;
    case EntityType::BLOCK:
      return {ItemType::BLOCK};
    case EntityType::STORAGE:
      return {ItemType::STORAGE};
    case EntityType::CONVEYOR:
      return {ItemType::CONVEYOR};
    case EntityType::ITEM:
      return std::nullopt;
  }
  ASSERT(false);
}

EntityType item_to_entity(ItemType item) {
  switch (item) {
    case ItemType::BLOCK:
      return EntityType::BLOCK;
    case ItemType::STORAGE:
      return EntityType::STORAGE;
    case ItemType::CONVEYOR:
      return EntityType::CONVEYOR;
  }
  ASSERT(false);
}

RenderRect render_rect(EntityType type) {
  switch (type) {
    case EntityType::PLAYER:
      return {.color = MAROON, .dims = GRID_DIMS};
    case EntityType::BLOCK:
      return {.color = GRAY, .dims = GRID_DIMS};
    case EntityType::STORAGE:
      static constexpr f32 SCALE = 0.75f;
      return {.color = DARKBROWN, .dims = GRID_DIMS * SCALE};
    case EntityType::CONVEYOR:
      return {.color = YELLOW, .dims = {i32(f32(GRID_DIMS.x) * 0.625f), GRID_DIMS.y}};
    case EntityType::ITEM:
      // NOTE: get it from the slot property instead
      ASSERT(false);
  }
  ASSERT(false);
}

Entity init_entity(EntityType type, const ivec2& pos, Rotation rotation = Rotation::Up) {
  Entity entity{
    .type = type,
    .pos  = pos,
  };

  if (rotatable(type)) {
    entity.rotation = rotation;
  }
  if (has_inventory(type)) {
    entity.inventory = std::vector<ItemSlot>(inventory_size(type));
  }

  if (type == EntityType::PLAYER) {
    entity.interaction_radius = 4;
  }

  return entity;
}

static EntityId get_next_entity_id(EntityStore& store) {
  if (!store.free_slots.empty()) {
    auto id = store.free_slots.back();
    store.free_slots.pop_back();
    ++id.gen;
    return id;
  }
  ASSERT(store.next_entity_idx < U16_MAX);
  ++store.next_entity_idx;
  return {.idx = store.next_entity_idx, .gen = 0};
}

EntityId add_entity(EntityStore& store, const Entity& entity) {
  store.command_buffer.push_back(Command{
    .type       = CommandType::Add,
    .add_entity = entity,
  });
  auto& ent = store.command_buffer.back().add_entity;
  ent.id    = get_next_entity_id(store);
  return ent.id;
}

void remove_entity(EntityStore& store, EntityId id) {
  store.command_buffer.push_back(Command{
    .type      = CommandType::Remove,
    .remove_id = id,
  });
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

template <typename Pred>
Entity* find(EntityStore& store, Pred&& pred) {
  for (auto& entity : store) {
    if (pred(entity)) {
      return &entity;
    }
  }
  return nullptr;
}

template <typename Pred>
std::vector<Entity*> find_all(EntityStore& store, Pred&& pred) {
  std::vector<Entity*> out{};
  for (auto& entity : store) {
    if (pred(entity)) {
      out.push_back(&entity);
    }
  }
  return out;
}

void emit(EntityStore& store, const Event& event) {
  store.event_bus.push_back(event);
}

EventView listen(EntityStore& store, EventType type) {
  return {.type = type, .event_bus = store.event_bus};
}

void flush(EntityStore& store) {
  for (auto& cmd : store.command_buffer) {
    switch (cmd.type) {
      case CommandType::Add:
        if (store.entities.size() < store.next_entity_idx) {
          store.entities.resize(store.next_entity_idx);
        }
        store.entities[cmd.add_entity.id.idx - 1] = cmd.add_entity;
        break;
      case CommandType::Remove:
        store.entities[cmd.remove_id.idx - 1] = {};
        store.free_slots.push_back(cmd.remove_id);
        break;
    }
  }
  store.command_buffer.clear();
}

void clear_event_bus(EntityStore& store) {
  store.event_bus.clear();
}
