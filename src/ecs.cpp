#include "ecs.h"

namespace ecs {

u32 next_component_idx(World& world) {
  ++world.next_component_idx_;
  ASSERT(world.next_component_idx_ < MAX_COMPONENT_COUNT);
  return world.next_component_idx_ - 1;
}

// TODO: i dont think this really works with multiple different worlds
template <typename T>
u32 get_component_idx(World& world) {
  static u32 component_idx = next_component_idx(world);
  return component_idx;
}

u32 next_event_idx(World& world) {
  ++world.next_event_idx_;
  ASSERT(world.next_event_idx_ < MAX_EVENT_COUNT);
  return world.next_event_idx_ - 1;
}

template <typename T>
u32 get_event_idx(World& world) {
  static u32 event_idx = next_event_idx(world);
  return event_idx;
}

Entity create(World& world) {
  ++world.next_handle_;
  return Entity{world.next_handle_};
}

void destroy(World& world, Entity& entity) {
  world.command_buffer.push_back(DestroyCommand{.entity = entity});
  entity = NULL_ENTITY;
}

Entity duplicate(World& world, Entity entity) {
  auto dup = create(world);

  for (u32 index = 0; auto& component : world.components) {
    if (component.contains(entity)) {
      world.command_buffer.push_back(
        AddCommand{.entity = dup, .index = index, .component = component[entity]}
      );
    }
    ++index;
  }

  return dup;
}

template <typename T>
void add(World& world, Entity entity, const T& component) {
  auto index = get_component_idx<T>(world);
  world.command_buffer.push_back(
    AddCommand{.entity = entity, .index = index, .component = component}
  );
}

template <typename... Ts>
void remove(World& world, Entity entity) {
  (
    [&]() {
      auto index = get_component_idx<Ts>(world);
      world.command_buffer.push_back(RemoveCommand{.entity = entity, .index = index});
    }(),
    ...
  );
}

void flush(World& world) {
  for (auto& command : world.command_buffer) {
    std::visit(
      [&](auto&& cmd) {
        using T = std::decay_t<decltype(cmd)>;
        if constexpr (std::is_same_v<T, DestroyCommand>) {
          for (auto& component : world.components) {
            if (component.contains(cmd.entity)) {
              component.erase(cmd.entity);
            }
          }
          world.component_maps[cmd.entity].reset();
        } else if constexpr (std::is_same_v<T, AddCommand>) {
          world.components[cmd.index][cmd.entity] = cmd.component;
          get_component_map(world, cmd.entity).set(cmd.index);
        } else if constexpr (std::is_same_v<T, RemoveCommand>) {
          world.components[cmd.index].erase(cmd.entity);
          get_component_map(world, cmd.entity).reset(cmd.index);
        } else {
          static_assert(false);
        }
      },
      command
    );
  }
  world.command_buffer.clear();
}

void clear_event_bus(World& world) {
  for (u32 i = 0; i < world.next_event_idx_; ++i) {
    world.event_bus[i].clear();
  }
}

template <typename T>
T& get(World& world, Entity entity) {
  ASSERT(has<T>(world, entity));
  auto index = get_component_idx<T>(world);
  return std::any_cast<T&>(world.components[index][entity]);
}

template <typename T>
bool has(World& world, Entity entity) {
  return component_map_has<T>(world, get_component_map(world, entity));
}

template <typename... Ts, typename Func>
void query(World& world, Func&& callback) {
  static_assert(sizeof...(Ts) > 0);

  std::unordered_map<Entity, std::any>* components[] = {
    &world.components[get_component_idx<Ts>(world)]...
  };
  auto* smallest_map = components[0];
  for (auto* map : components) {
    if (map->size() < smallest_map->size()) {
      smallest_map = map;
    }
  }

  for (auto& [entity, any_first] : *smallest_map) {
    if ((has<Ts>(world, entity) && ...)) {
      callback(entity, get<Ts>(world, entity)...);
    }
  }
}

template <typename... Ts, typename Pred>
std::optional<Entity> find(World& world, Pred&& pred) {
  static_assert(sizeof...(Ts) > 0);

  std::unordered_map<Entity, std::any>* components[] = {
    &world.components[get_component_idx<Ts>(world)]...
  };
  auto* smallest_map = components[0];
  for (auto* map : components) {
    if (map->size() < smallest_map->size()) {
      smallest_map = map;
    }
  }

  for (auto& [entity, any_first] : *smallest_map) {
    if ((has<Ts>(world, entity) && ...)) {
      if (pred(entity, get<Ts>(world, entity)...)) {
        return {entity};
      }
    }
  }

  return std::nullopt;
}

template <typename... Ts>
bool components_equal(World& world, Entity a, Entity b) {
  return (
    ((has<Ts>(world, a) == has<Ts>(world, b)) &&
     (!has<Ts>(world, a) || get<Ts>(world, a) == get<Ts>(world, b))) &&
    ...
  );
}

inline ComponentMap& get_component_map(World& world, Entity entity) {
  return world.component_maps[entity];
}

template <typename T>
inline bool component_map_has(World& world, const ComponentMap& component_map) {
  return component_map.test(get_component_idx<T>(world));
}

template <typename... Ts>
void apply_from_component_map(
  World& world,
  Entity entity,
  const ComponentMap& component_map,
  const Ts&... components
) {
  (
    [&]() {
      if (component_map_has<Ts>(world, component_map)) {
        add(world, entity, components);
      }
    }(),
    ...
  );
}

template <typename T>
void emit(World& world, const T& event) {
  u32 index = get_event_idx<T>(world);
  world.event_bus[index].push_back(event);
}

template <typename T, typename Func>
void listen(World& world, Func&& callback) {
  u32 index = get_event_idx<T>(world);
  for (auto& event : world.event_bus[index]) {
    callback(std::any_cast<T&>(event));
  }
}

}
