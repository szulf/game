// NOTE: i dont think i will have more than U16_MAX(65'536) entities
struct EntityId {
  u16 idx{};
  u16 gen{};

  bool operator==(EntityId other) const {
    return idx == other.idx && gen == other.gen;
  }

  explicit inline operator bool() const {
    return idx != 0;
  }
};

static constexpr EntityId NULL_ENTITY = {0, 0};

static constexpr u32 PLAYER_INVENTORY_SIZE = 32;

struct Player {
  std::vector<ItemSlot> inventory = std::vector<ItemSlot>(PLAYER_INVENTORY_SIZE);
  i32 interaction_radius          = 4;
  EntityId open_inventory{};
  ItemSlot hand{};
};

struct Block {};

static constexpr u32 STORAGE_INVENTORY_SIZE = 32;

struct Storage {
  std::vector<ItemSlot> inventory = std::vector<ItemSlot>(STORAGE_INVENTORY_SIZE);
};

struct ConveyorItem {
  ItemSlot slot{};
  // NOTE: value in range [0; 1] that indicates how far along an item is
  f32 t{};
};

// NOTE: items per second
// NOTE: constant for now, might change in the future
// (for example have multiple types of conveyors that have different speeds
//  (they might be different entity types tho))
static constexpr u32 CONVEYOR_THROUGHPUT = 10;

struct Conveyor {
  Rotation rotation{};
  std::vector<ConveyorItem> items = std::vector<ConveyorItem>(CONVEYOR_THROUGHPUT);
};

Direction conveyor_from(const Conveyor& conveyor) {
  return opposite_direction(conveyor.rotation);
}

Direction conveyor_to(const Conveyor& conveyor) {
  return conveyor.rotation;
}

struct Item {
  ItemSlot slot{};
};

enum class World {
  OVERWORLD,
  OTHER,
};

struct WorldTunnel {
  World to{};
};

// NOTE: keep a type with no heap allocations as the first one,
// because std::variant by default initializes to the first type
// so i dont want "entity = {}" to do any heap allocations
using EntityData = std::variant<Block, Player, Storage, Conveyor, Item, WorldTunnel>;

struct Entity {
  EntityId id{};
  ivec2 pos{};
  World world{};
  EntityData data{};
};

template <typename T>
concept HasInventory = requires(T& t) { t.inventory; };

template <typename T>
concept Rotatable = requires(T& t) { t.rotation; };

struct AddCommand {
  Entity entity{};
};

struct RemoveCommand {
  EntityId id{};
};

using Command = std::variant<AddCommand, RemoveCommand>;

enum class EventType {
  PLAYER_COLLIDED,
};

// TODO: can i somehow use Events as a std::variant?
// i would still need to keep EventType for listen()
struct Event {
  EventType type{};
  // NOTE: PLAYER_COLLIDED
  EntityId entity{};
};

struct EntityStore {
  // NOTE: stores which idx is free and what generation it previously had
  std::vector<EntityId> free_slots{};
  u16 next_entity_idx{};
  std::vector<Entity> entities{};

  std::vector<Command> command_buffer{};
  std::vector<Event> event_bus{};
};

struct EntityIterator {
  Entity* curr{};
  Entity* end{};

  EntityIterator& operator++() {
    do {
      ++curr;
    } while (curr < end && !curr->id);
    return *this;
  }

  Entity& operator*() {
    return *curr;
  }

  bool operator!=(const EntityIterator& other) const {
    return curr != other.curr;
  }
};

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
  store.command_buffer.push_back(AddCommand{
    .entity = entity,
  });
  auto* cmd = std::get_if<AddCommand>(&store.command_buffer.back());
  ASSERT_NO_MSG(cmd);
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

// NOTE: DO NOT save the pointer for longer than a single system!
// It will break things when the entities vector reallocates
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

Entity* get_entity_at_pos(EntityStore& store, const ivec2& pos, World world) {
  for (auto& entity : store) {
    if (entity.world == world && entity.pos == pos) {
      return &entity;
    }
  }
  return nullptr;
}

std::vector<Entity*> get_entities_at_pos(EntityStore& store, const ivec2& pos, World world) {
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

struct EventView {
  EventType type{};
  std::vector<Event>& event_bus;

  struct Iterator {
    EventType type{};
    Event* curr{};
    Event* end{};

    Iterator& operator++() {
      do {
        ++curr;
      } while (curr < end && curr->type != type);
      return *this;
    }

    Event& operator*() {
      return *curr;
    }

    bool operator!=(const Iterator& other) const {
      return curr != other.curr;
    }
  };

  Iterator begin() {
    Iterator iter{
      .type = type,
      .curr = event_bus.data(),
      .end  = event_bus.data() + event_bus.size(),
    };
    while (iter.curr < iter.end && iter.curr->type != type) {
      ++iter.curr;
    }
    return iter;
  }

  Iterator end() {
    Iterator iter{};
    iter.curr = iter.end = event_bus.data() + event_bus.size();
    return iter;
  }
};

// NOTE: usage -> for (auto& event : listen(store, type)) { ... }
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

// NOTE: std::visit helpers

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
  ASSERT(false, "invalid item type: %d\n", i32(type));
}

bool solid(const Entity& entity) {
  return std::visit(
    [](auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, Block> || std::is_same_v<T, Storage>) {
        return true;
      } else if constexpr (std::is_same_v<T, Player> || std::is_same_v<T, Conveyor> ||
                           std::is_same_v<T, Item> || std::is_same_v<T, WorldTunnel>) {
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
      } else if constexpr (std::is_same_v<T, Player> || std::is_same_v<T, Item> ||
                           std::is_same_v<T, WorldTunnel>) {
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
      [](const Player&) -> std::optional<ItemType> {
        return std::nullopt;
      },
      [](const Block&) -> std::optional<ItemType> {
        return {ItemType::BLOCK};
      },
      [](const Storage&) -> std::optional<ItemType> {
        return {ItemType::STORAGE};
      },
      [](const Conveyor&) -> std::optional<ItemType> {
        return {ItemType::CONVEYOR};
      },
      [](const Item&) -> std::optional<ItemType> {
        return std::nullopt;
      },
      [](const WorldTunnel&) -> std::optional<ItemType> {
        return std::nullopt;
      }
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
  ASSERT(false, "invalid item type: %d\n", i32(item));
}

struct RenderRect {
  Color color{};
  ivec2 dims{};
};

RenderRect get_render_rect(const Entity& entity) {
  return std::visit(
    overloaded{
      [](const Player&) -> RenderRect {
        return {.color = MAROON, .dims = GRID_DIMS};
      },
      [](const Block&) -> RenderRect {
        return {.color = GRAY, .dims = GRID_DIMS};
      },
      [](const Storage&) -> RenderRect {
        static constexpr f32 SCALE = 0.75f;
        return {.color = DARKBROWN, .dims = GRID_DIMS * SCALE};
      },
      [](const Conveyor&) -> RenderRect {
        return {.color = YELLOW, .dims = {i32(f32(GRID_DIMS.x) * 0.625f), GRID_DIMS.y}};
      },
      [](const Item&) -> RenderRect {
        ASSERT(
          false,
          "the item entity does not have a render rect, use its items render rect instead"
        );
      },
      [](const WorldTunnel&) -> RenderRect {
        return {.color = ORANGE, .dims = GRID_DIMS};
      },
    },
    entity.data
  );
}

// NOTE: currently the same as entity one,
// but in the future this will have different values, so might as well just write it now
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
  ASSERT(false, "invalid item type: %d\n", i32(item_type));
}

// TODO: better name?
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
      [](Block&) {},
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
      [](Item&) {},
      [](WorldTunnel&) {},
    },
    entity.data
  );
}
