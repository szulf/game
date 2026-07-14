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

static constexpr u32 PLAYER_INVENTORY_SIZE = 16;

struct Player {
  std::vector<ItemSlot> inventory = std::vector<ItemSlot>(PLAYER_INVENTORY_SIZE);
  i32 interaction_radius          = 4;
  EntityId open_gui{};
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
  std::vector<ItemSlot> inventory = {
    {.flags = ITEM_SLOT_INPUT},
    {.flags = ITEM_SLOT_INPUT},
    {.flags = ITEM_SLOT_INPUT},
    {.flags = ITEM_SLOT_INPUT},
    {.flags = ITEM_SLOT_OUTPUT},
    {.flags = ITEM_SLOT_OUTPUT},
    {.flags = ITEM_SLOT_OUTPUT},
    {.flags = ITEM_SLOT_OUTPUT}
  };
};

// TODO: give them actual names
enum MaintenanceFlag {
  MAINTENANCE_ONE,
  MAINTENANCE_TWO,
  MAINTENANCE_THREE,
  MAINTENANCE_COUNT,
};

using Maintenance = std::bitset<MAINTENANCE_COUNT>;

static constexpr u32 MAX_REQUESTED_ITEMS      = ITEM_MAX_COUNT;
static constexpr u32 REQUESTED_ITEMS_MULTIPLE = 4;

struct ResourceMessage {
  // TODO: not all items should be requestable through the message systems,
  // only a group of base items should be
  // NOTE: map from ItemType to amount of item requested
  std::array<u32, ITEM_COUNT> requested_items{};
  u64 arrival_time{};
  u32 batch_number{};
};

struct ResourceMessageQueue {
  std::vector<ResourceMessage> msgs{};
};

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

bool fits_in_last_batch(ResourceMessageQueue& queue, const ResourceMessage& msg, u64 game_time) {
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
    for (u32 i = 0; i < ITEM_COUNT; ++i) {
      dummy_msg.requested_items[i] += batch_msg.requested_items[i];
    }
  }
  for (u32 i = 0; i < ITEM_COUNT; ++i) {
    dummy_msg.requested_items[i] += msg.requested_items[i];
  }
  for (u32 i = 0; i < ITEM_COUNT; ++i) {
    if (dummy_msg.requested_items[i] > MAX_REQUESTED_ITEMS) {
      return false;
    }
  }
  return true;
}

void add_resource_message(ResourceMessageQueue& queue, ResourceMessage& msg, u64 game_time) {
  // TODO: maybe calculate this from the amount of items requested?
  static constexpr u64 MINUTES_TO_ARRIVAL = 300;

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

enum class ResourceMessageSenderPage {
  DISPLAY,
  CREATE,
};

struct ResourceMessageSender {
  static constexpr Maintenance POSSIBLE_MAINTENANCE = (1 << MAINTENANCE_ONE) | (1 << MAINTENANCE_THREE);
  Maintenance maintenance{};
  ResourceMessageSenderPage page{};
  ResourceMessage msg_in_create{};
};

struct ResourceMessageReceiver {
  static constexpr Maintenance POSSIBLE_MAINTENANCE =
    (1 << MAINTENANCE_ONE) | (1 << MAINTENANCE_TWO);
  Maintenance maintenance{};
  // NOTE: this effectively means that max batch size is a max stack of each item type
  std::vector<ItemSlot> inventory = std::vector<ItemSlot>(ITEM_COUNT);

  ResourceMessageReceiver() {
    for (auto& slot : inventory) {
      slot.flags = ITEM_SLOT_OUTPUT;
    }
  }
};

bool resource_message_receiver_empty(const ResourceMessageReceiver& msg_receiver) {
  for (auto& slot : msg_receiver.inventory) {
    if (slot) {
      return false;
    }
  }
  return true;
}

// NOTE: could potentially have different recipe types for different machines in the future
// just to have different MAX_INPUT/OUTPUT_SLOTS values, dont think it would be too big of an issue
struct Recipe {
  static constexpr u32 MAX_INPUT_SLOTS  = 4;
  static constexpr u32 MAX_OUTPUT_SLOTS = 2;

  std::string_view name{};
  f32 recipe_time{};
  std::array<ItemSlot, MAX_INPUT_SLOTS> input_slots{};
  std::array<ItemSlot, MAX_OUTPUT_SLOTS> output_slots{};
};

struct Assembler {
  static constexpr Maintenance POSSIBLE_MAINTENANCE = (1 << MAINTENANCE_ONE) | (1 << MAINTENANCE_TWO) | (1 << MAINTENANCE_THREE);
  Maintenance maintenance{};
  u32 selected_recipe_idx{};
  std::vector<ItemSlot> inventory =
    std::vector<ItemSlot>(Recipe::MAX_INPUT_SLOTS + Recipe::MAX_OUTPUT_SLOTS);
  f32 t{};

  Assembler() {
    for (u32 i = Recipe::MAX_INPUT_SLOTS; i < inventory.size(); ++i) {
      auto& slot = inventory[i];
      slot.flags = ITEM_SLOT_OUTPUT;
    }
  }

  static constexpr std::array RECIPES = {
    Recipe{
      .name         = "conveyor",
      .recipe_time  = 3.0f,
      .input_slots  = {{{.type = ITEM_BLOCK, .count = 5}, {.type = ITEM_STORAGE, .count = 1}}},
      .output_slots = {{{.type = ITEM_CONVEYOR, .count = 10}}},
    },
    Recipe{
      .name        = "assembler",
      .recipe_time = 10.0f,
      .input_slots =
        {{{.type = ITEM_BLOCK, .count = 8},
          {.type = ITEM_STORAGE, .count = 1},
          {.type = ITEM_CONVEYOR, .count = 4}}},
      .output_slots = {{{.type = ITEM_ASSEMBLER, .count = 1}}},
    },
  };
};

ItemSlot& assembler_input_slot(Assembler& assembler, u32 idx) {
  ASSERT_NO_MSG(idx < Recipe::MAX_INPUT_SLOTS);
  return assembler.inventory[idx];
}

ItemSlot& assembler_output_slot(Assembler& assembler, u32 idx) {
  ASSERT_NO_MSG(idx < Recipe::MAX_OUTPUT_SLOTS);
  return assembler.inventory[idx + Recipe::MAX_INPUT_SLOTS];
}

// NOTE: keep a type with no heap allocations as the first one,
// because std::variant by default initializes to the first type
// so i dont want "entity = {}" to do any heap allocations
using EntityData = std::variant<
  Block,
  Player,
  Storage,
  Conveyor,
  Item,
  WorldTunnel,
  ResourceMessageSender,
  ResourceMessageReceiver,
  Assembler>;

struct Entity {
  EntityId id{};
  vec2 pos{};
  World world{};
  EntityData data{};
};

template <typename T>
concept HasInventory = requires(T& t) { t.inventory; };

template <typename T>
concept Rotatable = requires(T& t) { t.rotation; };

template <typename T>
concept HasMaintenance = requires(T& t) {
  t.maintenance;
  T::POSSIBLE_MAINTENANCE;
};

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
    case ITEM_BLOCK:
      return Rotatable<Block>;
    case ITEM_STORAGE:
      return Rotatable<Storage>;
    case ITEM_CONVEYOR:
      return Rotatable<Conveyor>;
    case ITEM_ASSEMBLER:
      return Rotatable<Assembler>;
    case ITEM_COUNT:
      break;
  }
  ASSERT(false, "invalid item type: %d\n", i32(type));
}

bool solid(const Entity& entity) {
#define t(type) std::is_same_v<T, type>
  return std::visit(
    [](auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (t(Block) || t(Storage) || t(ResourceMessageSender) ||
                    t(ResourceMessageReceiver) || t(Assembler)) {
        return true;
      } else if constexpr (t(Player) || t(Conveyor) || t(Item) || t(WorldTunnel)) {
        return false;
      } else {
        static_assert(false);
      }
    },
    entity.data
  );
#undef t
}

bool breakable(const Entity& entity) {
#define t(type) std::is_same_v<T, type>
  return std::visit(
    [](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (t(Block) || t(Storage) || t(Conveyor) || t(Assembler)) {
        return true;
      } else if constexpr (t(Player) || t(Item) || t(WorldTunnel) || t(ResourceMessageSender) ||
                           t(ResourceMessageReceiver)) {
        return false;
      } else {
        static_assert(false);
      }
    },
    entity.data
  );
#undef t
}

bool has_gui(const Entity& entity) {
#define t(type) std::is_same_v<T, type>
  return std::visit(
    [](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (t(Storage) || t(WorldTunnel) || t(ResourceMessageSender) ||
                    t(ResourceMessageReceiver) || t(Assembler)) {
        return true;
      } else if constexpr (t(Block) || t(Player) || t(Conveyor) || t(Item)) {
        return false;
      } else {
        static_assert(false);
      }
    },
    entity.data
  );
#undef t
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

Entity entity_from_item(ItemType item) {
  switch (item) {
    case ITEM_BLOCK:
      return {.data = Block{}};
    case ITEM_STORAGE:
      return {.data = Storage{}};
    case ITEM_CONVEYOR:
      return {.data = Conveyor{}};
    case ITEM_ASSEMBLER:
      return {.data = Assembler{}};
    case ITEM_COUNT:
      break;
  }
  ASSERT(false, "invalid item type: %d\n", i32(item));
}

struct RenderRect {
  Color color{};
  ivec2 dims{};
};

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
std::tuple<Entity*, T*> get_entity_and_data(EntityStore& store, EntityId id) {
  auto* entity = get_entity(store, id);
  if (entity) {
    return {entity, get_data<T>(*entity)};
  }
  return {nullptr, nullptr};
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

std::tuple<Maintenance*, const Maintenance*> get_maintenance(Entity& entity) {
  return std::visit(
    [](auto& value) -> std::tuple<Maintenance*, const Maintenance*> {
      using T = std::decay_t<decltype(value)>;
      if constexpr (HasMaintenance<T>) {
        return {&value.maintenance, &value.POSSIBLE_MAINTENANCE};
      } else {
        return {nullptr, nullptr};
      }
    },
    entity.data
  );
}

std::tuple<Maintenance*, const Maintenance*> get_maintenance(EntityStore& store, EntityId id) {
  auto* entity = get_entity(store, id);
  if (entity) {
    return get_maintenance(*entity);
  }
  return {nullptr, nullptr};
}

// TODO: think about what is the real purpose of this function
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
      [](Item&) {
        // TODO: should i put the item here?
      },
      [](WorldTunnel&) {
        // TODO: should i put the inventory here?
      },
      [](ResourceMessageSender&) {},
      [&](ResourceMessageReceiver& receiver) {
        for (auto& slot : receiver.inventory) {
          func(slot);
        }
      },
      [](Assembler&) {
        // TODO: put the thing here
        ASSERT(false, "TODO");
      }
    },
    entity.data
  );
}
