#pragma once

#include <string_view>
#include <vector>
#include <variant>

#include "core.h"
#include "math.h"
#include "utils.h"
#include "input.h"
#include "items.h"

// NOTE: i dont think i will have more than U16_MAX(65'536) entities
struct EntityId {
  u16 idx{};
  u16 gen{};

  inline bool operator==(EntityId other) const {
    return idx == other.idx && gen == other.gen;
  }

  explicit inline operator bool() const {
    return idx != 0;
  }
};

static constexpr EntityId NULL_ENTITY = {0, 0};

struct ItemSlotIdx {
  EntityId entity{};
  u32 slot_idx{};

  explicit inline operator bool() const {
    return bool(entity);
  }
};

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
  Direction rotation{};
  std::vector<ConveyorItem> items = std::vector<ConveyorItem>(CONVEYOR_THROUGHPUT);
};

inline Direction conveyor_from(const Conveyor& conveyor) {
  return opposite_direction(conveyor.rotation);
}

inline Direction conveyor_to(const Conveyor& conveyor) {
  return conveyor.rotation;
}

struct Item {
  ItemSlot slot{};
};

enum World {
  WORLD_MAIN,
  WORLD_MESSAGING,
  WORLD_STORAGE,

  WORLD_COUNT,
};

std::string_view world_to_string(World world);

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

static constexpr vec2 MAINTENANCE_MINIGAME_DIMS = {256, 256};

struct Cogwheel {
  vec2 pos{};
  f32 radius{};
  Color color{};
};

struct LubricationPoint {
  vec2 dims{};
  vec2 pos{};
  Color color{};
  static constexpr f32 TIME_TO_LUBRICATE = 0.15f;
  // NOTE: value in range [0; 1]
  // 0 -> no lubrication applied
  // 1 -> fully lubricated
  f32 progress{};
};

struct MaintenanceLubrication {
  static constexpr std::string_view NAME = "lubrication";
  static constexpr ItemType FIX_ITEM     = ITEM_LUBRICANT_CAN;
  vec2 window_offset{};
  bool open{};

  std::vector<Cogwheel> cogwheels{};
  std::vector<LubricationPoint> points{};
};

void maintenance_init_minigame(MaintenanceLubrication& state);
bool maintenance_update_minigame(MaintenanceLubrication& state, const Input& input, f32 dt);
void maintenance_render_minigame(
  MaintenanceLubrication& state,
  const AssetManager&,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
);

struct DirtyRect {
  Rectangle area{};
  // NOTE: value in range [0; 1] that indicates how clean it is
  // 0 -> dirty
  // 1 -> clean
  f32 progress{};
};

struct MaintenanceCleaning {
  static constexpr std::string_view NAME = "cleaning";
  static constexpr ItemType FIX_ITEM     = ITEM_BRUSH;
  vec2 window_offset{};
  bool open{};

  static constexpr vec2 DIMS            = {224, 224};
  static constexpr Rectangle OUTER_RECT = rect_from_vec2x2(vec2{128, 128} - (DIMS * 0.5f), DIMS);
  std::vector<DirtyRect> dirty_rects{};
  vec2 last_mouse_pos{};
};

void maintenance_init_minigame(MaintenanceCleaning& state);
bool maintenance_update_minigame(MaintenanceCleaning& state, const Input& input, f32);
void maintenance_render_minigame(
  MaintenanceCleaning& state,
  const AssetManager&,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
);

enum ComponentSlotType {
  COMPONENT_SLOT_BROKEN,
  COMPONENT_SLOT_FIXED,
  COMPONENT_SLOT_MACHINE,
  COMPONENT_SLOT_COUNT,
};

struct Component {
  static constexpr vec2 DIMS = {64, 64};

  ComponentSlotType slot{};
  vec2 pos{};
  bool dragging{};
};

struct ComponentSlot {
  static constexpr vec2 DIMS = Component::DIMS + vec2{8, 8};
  vec2 pos{};
};

struct MaintenanceComponentReplacement {
  static constexpr std::string_view NAME = "component_replacement";
  static constexpr ItemType FIX_ITEM     = ITEM_SPARE_PARTS;
  vec2 window_offset{};
  bool open{};

  std::array<ComponentSlot, COMPONENT_SLOT_COUNT> slots{};
  Component broken{};
  Component fixed{};
};

void maintenance_init_minigame(MaintenanceComponentReplacement& state);
bool maintenance_update_minigame(MaintenanceComponentReplacement& state, const Input& input, f32);
void maintenance_render_minigame(
  MaintenanceComponentReplacement& state,
  const AssetManager& assets,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
);

struct MaintenanceCalibration {
  static constexpr std::string_view NAME = "calibration";
  static constexpr ItemType FIX_ITEM     = ITEM_CALIBRATOR;
  vec2 window_offset{};
  bool open{};

  static constexpr Rectangle add_rect    = rect_from_vec2x2({192, 128}, {64, 64});
  static constexpr Rectangle remove_rect = rect_from_vec2x2({64, 128}, {64, 64});
  f32 range_low{};
  f32 range_high{};
  f32 value{};
  // NOTE: once you get the value into the range you have to wait a little bit, that is this t value
  f32 t{};
};

void maintenance_init_minigame(MaintenanceCalibration& state);
bool maintenance_update_minigame(MaintenanceCalibration& state, const Input& input, f32 dt);
void maintenance_render_minigame(
  MaintenanceCalibration& state,
  const AssetManager&,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
);

struct MaintenanceMessagingSystem {
  static constexpr std::string_view NAME = "messaging_system";
  static constexpr ItemType FIX_ITEM     = ITEM_COMMUNICATION_COMPONENT;
};

static constexpr u32 MAINTENANCE_FIX_ITEM_COUNT  = 1;
static constexpr u32 MAINTENANCE_FIX_ITEM_DAMAGE = 1;

using Maintenance = std::variant<
  std::monostate,
  MaintenanceLubrication,
  MaintenanceCleaning,
  MaintenanceComponentReplacement,
  MaintenanceCalibration,
  MaintenanceMessagingSystem>;

std::string_view maintenance_name(const Maintenance& maintenance);
ItemType maintenance_fix_item(const Maintenance& maintenance);

template <typename T>
concept MaintenanceHasMiniGame = requires(
  T& t,
  const Input& input,
  f32 dt,
  const AssetManager& assets,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
) {
  t.open;
  maintenance_init_minigame(t);
  maintenance_update_minigame(t, input, dt);
  maintenance_render_minigame(t, assets, render_texture, window_offset);
};

bool* maintenance_is_minigame_open(Maintenance& maintenance);
void maintenance_init_minigame(Maintenance& maintenance);
bool maintenance_update_minigame(Maintenance& maintenance, const Input& input, f32 dt);
void maintenance_render_minigame(
  Maintenance& maintenance,
  const AssetManager& assets,
  const RenderTexture2D& render_texture,
  const vec2& window_offset
);

static constexpr u32 REQUESTED_ITEMS_MULTIPLE = 4;

struct ResourceMessage {
  // NOTE: map from ItemType to amount of item requested
  std::array<u32, REQUESTABLE_ITEMS.size()> requested_items{};
  u64 arrival_time{};
  u32 batch_number{};
};

struct ResourceMessageQueue {
  std::vector<ResourceMessage> msgs{};
};

std::span<ResourceMessage> get_first_resource_message_batch(ResourceMessageQueue& queue);
std::span<ResourceMessage> get_last_resource_message_batch(ResourceMessageQueue& queue);
void add_resource_message(ResourceMessageQueue& queue, ResourceMessage& msg, u64 game_time);
void remove_resource_message(ResourceMessageQueue& queue, u32 idx);

enum ResourceMessageSenderPage {
  SENDER_PAGE_DISPLAY,
  SENDER_PAGE_CREATE,
};

struct ResourceMessageSender {
  static constexpr std::array POSSIBLE_MAINTENANCE = std::to_array<Maintenance>({
    MaintenanceLubrication{},
    MaintenanceCleaning{},
    MaintenanceComponentReplacement{},
    MaintenanceCalibration{},
    MaintenanceMessagingSystem{},
  });
  Maintenance maintenance{};
  ResourceMessageSenderPage page{};
  ResourceMessage msg_in_create{};
};

struct ResourceMessageReceiver {
  static constexpr std::array POSSIBLE_MAINTENANCE = std::to_array<Maintenance>({
    MaintenanceLubrication{},
    MaintenanceCleaning{},
    MaintenanceComponentReplacement{},
    MaintenanceCalibration{},
    MaintenanceMessagingSystem{},
  });
  Maintenance maintenance{};
  std::vector<ItemSlot> inventory = std::vector<ItemSlot>(REQUESTABLE_ITEMS.size());

  // TODO: this is not really needed
  constexpr ResourceMessageReceiver() {
    for (auto& slot : inventory) {
      slot.flags = ITEM_SLOT_OUTPUT;
    }
  }
};

bool resource_message_receiver_empty(const ResourceMessageReceiver& msg_receiver);

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
  static constexpr std::array POSSIBLE_MAINTENANCE = std::to_array<Maintenance>({
    MaintenanceLubrication{},
    MaintenanceCleaning{},
    MaintenanceComponentReplacement{},
    MaintenanceCalibration{},
  });
  Maintenance maintenance{};
  u32 selected_recipe_idx{};
  std::vector<ItemSlot> inventory =
    std::vector<ItemSlot>(Recipe::MAX_INPUT_SLOTS + Recipe::MAX_OUTPUT_SLOTS);
  f32 t{};

  constexpr Assembler() {
    for (u32 i = Recipe::MAX_INPUT_SLOTS; i < inventory.size(); ++i) {
      auto& slot = inventory[i];
      slot.flags = ITEM_SLOT_OUTPUT;
    }
  }

  static constexpr std::array RECIPES = std::to_array<Recipe>({
    {
      .name         = "storage",
      .recipe_time  = 2.0f,
      .input_slots  = {{
        {.type = ITEM_ALUMINIUM, .count = 4},
      }},
      .output_slots = {{{.type = ITEM_STORAGE, .count = 1}}},
    },
    {
      .name         = "conveyor",
      .recipe_time  = 0.5f,
      .input_slots  = {{
        {.type = ITEM_ALUMINIUM, .count = 1},
        {.type = ITEM_COGWHEEL, .count = 1},
      }},
      .output_slots = {{{.type = ITEM_CONVEYOR, .count = 2}}},
    },
    {
      .name         = "assembler",
      .recipe_time  = 5.0f,
      .input_slots  = {{
        {.type = ITEM_CIRCUIT_BOARD, .count = 1},
        {.type = ITEM_COGWHEEL, .count = 2},
        {.type = ITEM_WIRE_BUNDLE, .count = 2},
      }},
      .output_slots = {{{.type = ITEM_ASSEMBLER, .count = 1}}},
    },
    {
      .name         = "copper wire",
      .recipe_time  = 1.0f,
      .input_slots  = {{
        {.type = ITEM_COPPER, .count = 2},
      }},
      .output_slots = {{{.type = ITEM_COPPER_WIRE, .count = 3}}},
    },
    {
      .name         = "blank board",
      .recipe_time  = 2.5f,
      .input_slots  = {{
        {.type = ITEM_PLASTIC, .count = 2},
        {.type = ITEM_COPPER_WIRE, .count = 4},
      }},
      .output_slots = {{{.type = ITEM_BLANK_BOARD, .count = 1}}},
    },
    {
      .name         = "transistor",
      .recipe_time  = 1.0f,
      .input_slots  = {{
        {.type = ITEM_SILICON_WAFER, .count = 1},
        {.type = ITEM_COPPER_WIRE, .count = 2},
        {.type = ITEM_PLASTIC, .count = 1},
      }},
      .output_slots = {{{.type = ITEM_TRANSISTOR, .count = 4}}},
    },
    {
      .name         = "capacitor",
      .recipe_time  = 1.0f,
      .input_slots  = {{
        {.type = ITEM_ALUMINIUM, .count = 1},
        {.type = ITEM_PLASTIC, .count = 1},
      }},
      .output_slots = {{{.type = ITEM_CAPACITOR, .count = 2}}},
    },
    {
      .name         = "circuit board",
      .recipe_time  = 5.0f,
      .input_slots  = {{
        {.type = ITEM_BLANK_BOARD, .count = 1},
        {.type = ITEM_TRANSISTOR, .count = 2},
        {.type = ITEM_CAPACITOR, .count = 2},
      }},
      .output_slots = {{{.type = ITEM_CIRCUIT_BOARD, .count = 1}}},
    },
    {
      .name         = "antenna",
      .recipe_time  = 5.0f,
      .input_slots  = {{
        {.type = ITEM_ALUMINIUM, .count = 4},
        {.type = ITEM_COPPER_WIRE, .count = 2},
      }},
      .output_slots = {{{.type = ITEM_ANTENNA, .count = 1}}},
    },
    {
      .name         = "communication component",
      .recipe_time  = 25.0f,
      .input_slots  = {{
        {.type = ITEM_CIRCUIT_BOARD, .count = 1},
        {.type = ITEM_ANTENNA, .count = 1},
      }},
      .output_slots = {{{.type = ITEM_COMMUNICATION_COMPONENT, .count = 1}}},
    },
    {
      .name         = "wire bundle",
      .recipe_time  = 2.0f,
      .input_slots  = {{
        {.type = ITEM_COPPER_WIRE, .count = 2},
        {.type = ITEM_PLASTIC, .count = 1},
      }},
      .output_slots = {{{.type = ITEM_WIRE_BUNDLE, .count = 1}}},
    },
    {
      .name         = "cogwheel",
      .recipe_time  = 4.0f,
      .input_slots  = {{
        {.type = ITEM_ALUMINIUM, .count = 4},
      }},
      .output_slots = {{{.type = ITEM_COGWHEEL, .count = 3}}},
    },
    {
      .name         = "spare parts",
      .recipe_time  = 8.0f,
      .input_slots  = {{
        {.type = ITEM_CIRCUIT_BOARD, .count = 1},
        {.type = ITEM_COGWHEEL, .count = 2},
        {.type = ITEM_WIRE_BUNDLE, .count = 2},
      }},
      .output_slots = {{{.type = ITEM_SPARE_PARTS, .count = 1}}},
    },
    {
      .name         = "lubricant cans",
      .recipe_time  = 15.0f,
      .input_slots  = {{
        {.type = ITEM_ALUMINIUM, .count = 2},
        {.type = ITEM_OIL_CANISTER, .count = 1},
      }},
      .output_slots = {{{.type = ITEM_LUBRICANT_CAN, .count = 1}}},
    },
  });
};

ItemSlot& assembler_input_slot(Assembler& assembler, u32 idx);
ItemSlot& assembler_output_slot(Assembler& assembler, u32 idx);

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

static const std::array PLACEABLE = std::to_array<Entity>({
  {.data = Block{}},
  {.data = Player{}},
  {.data = Storage{}},
  {.data = Conveyor{}},
  {.data = WorldTunnel{}},
  {.data = ResourceMessageSender{}},
  {.data = ResourceMessageReceiver{}},
  {.data = Assembler{}},
});

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

enum EventType {
  EVENT_PLAYER_COLLIDED,
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

EntityIterator begin(EntityStore& store);
EntityIterator end(EntityStore& store);

EntityId add_entity(EntityStore& store, const Entity& entity);
void remove_entity(EntityStore& store, EntityId id);
bool contains_entity(EntityStore& store, EntityId id);
// NOTE: DO NOT save the pointer for longer than a single system!
// It will break things when the entities vector reallocates
Entity* get_entity(EntityStore& store, EntityId id);
Entity* get_entity_at_pos(EntityStore& store, const vec2& pos, World world);
std::vector<Entity*> get_entities_at_pos(EntityStore& store, const vec2& pos, World world);
void emit(EntityStore& store, const Event& event);

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
inline EventView listen(EntityStore& store, EventType type) {
  return {.type = type, .event_bus = store.event_bus};
}

void flush(EntityStore& store);
void clear_event_bus(EntityStore& store);

bool rotatable(const Entity& entity);
std::optional<bool> rotatable(ItemType type);
bool solid(const Entity& entity);
bool breakable(const Entity& entity);
bool has_gui(const Entity& entity);
bool has_inventory(const Entity& entity);
bool has_maintenance(const Entity& entity);
std::optional<ItemType> entity_to_item(const Entity& entity);
std::optional<Entity> entity_from_item(ItemType item);
TextureType get_texture_type(const Entity& entity);

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
bool is(const Entity& entity) {
  return std::holds_alternative<T>(entity.data);
}

template <typename T>
bool is(EntityStore& store, EntityId id) {
  auto* entity = get_entity(store, id);
  if (entity) {
    return std::holds_alternative<T>(entity->data);
  }
  return false;
}

Direction* get_rotation(Entity& entity);
Direction* get_rotation(EntityStore& store, EntityId id);
std::vector<ItemSlot>* get_inventory(Entity& entity);
std::vector<ItemSlot>* get_inventory(EntityStore& store, EntityId id);
// NOTE: both return a (maintenance, possible_maintenances) tuple
std::tuple<Maintenance*, std::span<const Maintenance>> get_maintenance(Entity& entity);
std::tuple<Maintenance*, std::span<const Maintenance>>
get_maintenance(EntityStore& store, EntityId id);

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

void render_entities(EntityStore& store, World world, const AssetManager& assets);
