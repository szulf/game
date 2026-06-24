#ifndef ECS_H
#define ECS_H

namespace ecs {

struct Entity {
  u32 idx{};

  explicit inline operator bool() {
    return idx != 0;
  }
};

static constexpr Entity NULL_ENTITY = {.idx = 0};

inline bool operator==(const Entity& a, const Entity& b) {
  return a.idx == b.idx;
}

}

template <>
struct std::hash<ecs::Entity> {
  inline std::size_t operator()(const ecs::Entity& h) const noexcept {
    return h.idx;
  }
};

namespace ecs {

struct DestroyCommand {
  Entity entity{};
};

struct AddCommand {
  Entity entity{};
  u32 index{};
  std::any component{};
};

struct RemoveCommand {
  Entity entity{};
  u32 index{};
};

using Command = std::variant<DestroyCommand, AddCommand, RemoveCommand>;

static constexpr u32 MAX_COMPONENT_COUNT = 64;

using ComponentMap = std::bitset<MAX_COMPONENT_COUNT>;

static constexpr u32 MAX_EVENT_COUNT = 64;

struct World {
  u32 next_handle_{};
  u32 next_component_idx_{};
  u32 next_event_idx_{};

  std::vector<Command> command_buffer{};
  // NOTE: event type -> queue of events
  // TODO: do i need to keep this for two frames to make ordering of systems easier?
  std::vector<std::vector<std::any>> event_bus =
    std::vector<std::vector<std::any>>(MAX_EVENT_COUNT);

  std::unordered_map<Entity, ComponentMap> component_maps{};
  std::vector<std::unordered_map<Entity, std::any>> components =
    std::vector<std::unordered_map<Entity, std::any>>(MAX_COMPONENT_COUNT);
};

Entity create(World& world);
void destroy(World& world, Entity& entity);
Entity duplicate(World& world, Entity entity);

template <typename T>
void add(World& world, Entity entity, const T& component);
// NOTE: currently silently ignores if a component does not exist on that entity,
// not sure if that is good
template <typename... Ts>
void remove(World& world, Entity entity);

void flush(World& world);
void clear_event_bus(World& world);

template <typename T>
T& get(World& world, Entity entity);
template <typename T>
bool has(World& world, Entity entity);

template <typename... Ts, typename Func>
void query(World& world, Func&& callback);
template <typename... Ts, typename Pred>
std::optional<Entity> find(World& world, Pred&& pred);
template <typename... Ts>
bool components_equal(World& world, Entity a, Entity b);

inline ComponentMap& get_component_map(World& world, Entity entity);
template <typename T>
inline bool component_map_has(World& world, const ComponentMap& component_map);
// NOTE: checks if a passed in component exists in the component map, if so adds it to the entity
template <typename... Ts>
void apply_from_component_map(
  World& world,
  Entity entity,
  const ComponentMap& component_map,
  const Ts&... components
);

template <typename T>
void emit(World& world, const T& event);
template <typename T, typename Func>
void listen(World& world, Func&& callback);

}

#endif
