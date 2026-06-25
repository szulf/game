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

enum class EntityType {
  PLAYER,
  BLOCK,
  STORAGE,
  CONVEYOR,
  ITEM,
};

// NOTE: all of these could be arrays and would be "faster" that way, but functions look better
bool rotatable(EntityType type);
bool solid(EntityType type);
bool breakable(EntityType type);
bool has_inventory(EntityType type);
u32 inventory_size(EntityType type);
std::optional<ItemType> entity_to_item(EntityType entity);
EntityType item_to_entity(ItemType item);

struct RenderRect {
  Color color{};
  ivec2 dims{};
};

RenderRect render_rect(EntityType type);

enum class Direction {
  Up,
  Right,
  Down,
  Left,
  Count,
};

Direction opposite_direction(Direction direction) {
  switch (direction) {
    case Direction::Up:
      return Direction::Down;
    case Direction::Down:
      return Direction::Up;
    case Direction::Right:
      return Direction::Left;
    case Direction::Left:
      return Direction::Right;
    case Direction::Count:
      break;
  }
  ASSERT(false);
}

ivec2 direction_to_ivec2(Direction direction) {
  switch (direction) {
    case Direction::Up:
      return {0, -1};
    case Direction::Down:
      return {0, 1};
    case Direction::Right:
      return {1, 0};
    case Direction::Left:
      return {-1, 0};
    case Direction::Count:
      break;
  }
  ASSERT(false);
}

using Rotation = Direction;

// TODO: not sure if right and left degrees are correct
f32 rotation_degrees(Rotation rotation) {
  switch (rotation) {
    case Rotation::Up:
      return 0;
    case Rotation::Down:
      return 180;
    case Rotation::Right:
      return 90;
    case Rotation::Left:
      return 270;
    case Rotation::Count:
      break;
  }
  ASSERT(false);
}

struct ConveyorItem {
  ItemSlot slot{};
  // NOTE: value in range [0; 1] that indicates how far along an item is
  f32 t{};
};

struct Entity {
  // NOTE: common
  EntityId id{};
  EntityType type{};
  ivec2 pos{};
  Rotation rotation{};
  std::vector<ItemSlot> inventory{};

  // NOTE: player type
  i32 interaction_radius{};
  EntityId open_inventory{};
  ItemSlot hand{};

  // NOTE: conveyor type
  Direction from{};
  Direction to{};
  // NOTE: items per second
  // NOTE: constant for now, might change in the future
  // (for example have multiple types of conveyors that have different speeds)
  static constexpr u32 throughput = 10;
  std::vector<ConveyorItem> conveyor_items{};

  // NOTE: item type
  // TODO: combine with the players hand?
  ItemSlot slot{};
};

Entity init_entity(EntityType type, const ivec2& pos, Rotation rotation);

enum class CommandType {
  Add,
  Remove,
};

struct Command {
  CommandType type{};
  Entity add_entity{};
  EntityId remove_id{};
};

enum class EventType {
  PLAYER_COLLIDED,
};

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
