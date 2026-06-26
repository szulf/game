#ifndef ENTITY_H
#define ENTITY_H

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

// NOTE: keep a type with no heap allocations as the first one,
// because std::variant by default initializes to the first type
// so i dont want "entity = {}" to do any heap allocations
using EntityData = std::variant<Block, Player, Storage, Conveyor, Item>;

struct Entity {
  EntityId id{};
  ivec2 pos{};
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

EntityId add_entity(EntityStore& store, const Entity& entity);
void remove_entity(EntityStore& store, EntityId id);
bool contains_entity(EntityStore& store, EntityId id);
// NOTE: DO NOT save the pointer for longer than a single system!
// It will break things when the entities vector reallocates
Entity* get_entity(EntityStore& store, EntityId id);

template <typename Pred>
Entity* find(EntityStore& store, Pred&& pred);
template <typename Pred>
std::vector<Entity*> find_all(EntityStore& store, Pred&& pred);

void emit(EntityStore& store, const Event& event);
struct EventView;
// NOTE: usage -> for (auto& event : listen(store, type)) { ... }
EventView listen(EntityStore& store, EventType type);

void flush(EntityStore& store);
void clear_event_bus(EntityStore& store);

// NOTE: std visit helpers

bool rotatable(const Entity& entity);
bool rotatable(ItemType type);
bool solid(const Entity& entity);
bool breakable(const Entity& entity);
bool has_inventory(const Entity& entity);
std::optional<ItemType> entity_to_item(const Entity& entity);
Entity entity_from_item(ItemType item);

struct RenderRect {
  Color color{};
  ivec2 dims{};
};

RenderRect get_render_rect(const Entity& entity);
RenderRect get_render_rect(ItemType item_type);

// TODO: better name?
template <typename T>
T* get_data(Entity& entity);
template <typename T>
T* get_data(EntityStore& store, EntityId id);

template <typename T>
bool is(Entity& entity);
template <typename T>
bool is(EntityStore& store, EntityId id);

Rotation* get_rotation(Entity& entity);
Rotation* get_rotation(EntityStore& store, EntityId id);

std::vector<ItemSlot>* get_inventory(Entity& entity);
std::vector<ItemSlot>* get_inventory(EntityStore& store, EntityId id);

template <typename Func>
void for_each_active_slot(Entity& entity, Func&& func);

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

#endif
