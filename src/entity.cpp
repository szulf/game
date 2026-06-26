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
  store.command_buffer.push_back(AddCommand{
    .entity = entity,
  });
  auto* cmd = std::get_if<AddCommand>(&store.command_buffer.back());
  ASSERT(cmd);
  cmd->entity.id = get_next_entity_id(store);
  return cmd->entity.id;
}

void remove_entity(EntityStore& store, EntityId id) {
  store.command_buffer.push_back(RemoveCommand{
    .id = id,
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

bool rotatable(ItemType type) {
  switch (type) {
    case ItemType::BLOCK:
      return Rotatable<Block>;
    case ItemType::STORAGE:
      return Rotatable<Storage>;
    case ItemType::CONVEYOR:
      return Rotatable<Conveyor>;
  }
  ASSERT(false);
}

bool solid(const Entity& entity) {
  return std::visit(
    [](auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, Block> || std::is_same_v<T, Storage>) {
        return true;
      } else if constexpr (std::is_same_v<T, Player> || std::is_same_v<T, Conveyor> ||
                           std::is_same_v<T, Item>) {
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
    [](auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, Block> || std::is_same_v<T, Storage> ||
                    std::is_same_v<T, Conveyor>) {
        return true;
      } else if constexpr (std::is_same_v<T, Player> || std::is_same_v<T, Item>) {
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

std::optional<ItemType> entity_to_item(const Entity& entity) {
  return std::visit(
    overloaded{
      []([[maybe_unused]] const Player& player) -> std::optional<ItemType> {
        return std::nullopt;
      },
      []([[maybe_unused]] const Block& block) -> std::optional<ItemType> {
        return {ItemType::BLOCK};
      },
      []([[maybe_unused]] const Storage& storage) -> std::optional<ItemType> {
        return {ItemType::STORAGE};
      },
      []([[maybe_unused]] const Conveyor& conveyor) -> std::optional<ItemType> {
        return {ItemType::CONVEYOR};
      },
      []([[maybe_unused]] const Item& item) -> std::optional<ItemType> {
        return std::nullopt;
      },
    },
    entity.data
  );
}

Entity entity_from_item(ItemType item) {
  switch (item) {
    case ItemType::BLOCK:
      return {.data = Block{}};
    case ItemType::STORAGE:
      return {.data = Storage{}};
    case ItemType::CONVEYOR:
      return {.data = Conveyor{}};
  }
  ASSERT(false);
}

RenderRect get_render_rect(const Entity& entity) {
  return std::visit(
    overloaded{
      [&]([[maybe_unused]] const Player& player) -> RenderRect {
        return {.color = MAROON, .dims = GRID_DIMS};
      },
      [&]([[maybe_unused]] const Block& block) -> RenderRect {
        return {.color = GRAY, .dims = GRID_DIMS};
      },
      [&]([[maybe_unused]] const Storage& storage) -> RenderRect {
        static constexpr f32 SCALE = 0.75f;
        return {.color = DARKBROWN, .dims = GRID_DIMS * SCALE};
      },
      [&]([[maybe_unused]] const Conveyor& conveyor) -> RenderRect {
        return {.color = YELLOW, .dims = {i32(f32(GRID_DIMS.x) * 0.625f), GRID_DIMS.y}};
      },
      [&]([[maybe_unused]] const Item& item) -> RenderRect {
        ASSERT(false);
      },
    },
    entity.data
  );
}

// NOTE: currently the same as entity one,
// but in the future this will have different values, so might as well just write it no
RenderRect get_render_rect(ItemType item_type) {
  switch (item_type) {
    case ItemType::BLOCK:
      return {.color = GRAY, .dims = GRID_DIMS};
    case ItemType::STORAGE:
      static constexpr f32 SCALE = 0.75f;
      return {.color = DARKBROWN, .dims = GRID_DIMS * SCALE};
    case ItemType::CONVEYOR:
      return {.color = YELLOW, .dims = {i32(f32(GRID_DIMS.x) * 0.625f), GRID_DIMS.y}};
  }
  ASSERT(false);
}

template <typename T>
T* get_data(Entity& entity) {
  return std::get_if<T>(&entity.data);
}

template <typename T>
T* get_data(EntityStore& store, EntityId id) {
  auto* entity = get_entity(store, id);
  if (entity) {
    return get_data<T>(*entity);
  }
  return nullptr;
}

template <typename T>
bool is(Entity& entity) {
  return get_data<T>(entity) != nullptr;
}

template <typename T>
bool is(EntityStore& store, EntityId id) {
  return get_data<T>(store, id) != nullptr;
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

template <typename Func>
void for_each_active_slot(Entity& entity, Func&& func) {
  std::visit(
    overloaded{
      [&](Player& player) {
        for (auto& slot : player.inventory) {
          if (slot) {
            func(slot);
          }
        }
      },
      [&]([[maybe_unused]] Block& block) {},
      [&](Storage& storage) {
        for (auto& slot : storage.inventory) {
          if (slot) {
            func(slot);
          }
        }
      },
      [&](Conveyor& conveyor) {
        for (auto& item : conveyor.items) {
          if (item.slot) {
            func(item.slot);
          }
        }
      },
      [&]([[maybe_unused]] Item& item) {},
    },
    entity.data
  );
}
