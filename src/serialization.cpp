#include "serialization.h"

#include <fstream>

#include "json.hpp"
using json = nlohmann::json;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec2, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rectangle, x, y, width, height);

void to_json(json& j, ItemType t) {
  j = get_item_name(t);
}

void from_json(const json& j, ItemType& t) {
  for (u32 i = 0; i < ITEM_COUNT; ++i) {
    if (j == get_item_name(ItemType(i))) {
      t = ItemType(i);
      return;
    }
  }
  ASSERT(false, "invalid json item type");
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ItemSlot, flags, type, count, damage);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EntityId, idx, gen);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConveyorItem, slot, t);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Cogwheel, pos, radius, color);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LubricationPoint, dims, pos, color, progress);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DirtyRect, area, progress);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Component, slot, pos);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ComponentSlot, pos);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceMessage, requested_items, arrival_time, batch_number);

void to_json(json& j, const Maintenance& m) {
  std::visit(
    overloaded{
      [&](const std::monostate&) {
        j = nullptr;
      },
      [&](const MaintenanceLubrication& v) {
        j = json{
          {"type", v.NAME},
          {"open", v.open},
          {"cogwheels", v.cogwheels},
          {"points", v.points},
        };
      },
      [&](const MaintenanceCleaning& v) {
        j = json{
          {"type", v.NAME},
          {"open", v.open},
          {"dirty_rects", v.dirty_rects},
        };
      },
      [&](const MaintenanceComponentReplacement& v) {
        j = json{
          {"type", v.NAME},
          {"open", v.open},
          {"slots", v.slots},
          {"broken", v.broken},
          {"fixed", v.fixed},
        };
      },
      [&](const MaintenanceCalibration& v) {
        j = json{
          {"type", v.NAME},
          {"open", v.open},
          {"range_low", v.range_low},
          {"range_high", v.range_high},
          {"value", v.value},
          {"t", v.t},
        };
      },
      [&](const MaintenanceMessagingSystem& v) {
        j = json{
          {"type", v.NAME},
        };
      },
    },
    m
  );
}

void from_json(const json& j, Maintenance& m) {
  if (j.is_null()) {
    m = std::monostate{};
    return;
  }

  auto type = j.at("type").get<std::string>();
  if (type == MaintenanceLubrication::NAME) {
    MaintenanceLubrication v{};
    j.at("open").get_to(v.open);
    j.at("cogwheels").get_to(v.cogwheels);
    j.at("points").get_to(v.points);
    m = v;
  } else if (type == MaintenanceCleaning::NAME) {
    MaintenanceCleaning v{};
    j.at("open").get_to(v.open);
    j.at("dirty_rects").get_to(v.dirty_rects);
    m = v;
  } else if (type == MaintenanceComponentReplacement::NAME) {
    MaintenanceComponentReplacement v{};
    j.at("open").get_to(v.open);
    j.at("slots").get_to(v.slots);
    j.at("broken").get_to(v.broken);
    j.at("fixed").get_to(v.fixed);
    m = v;
  } else if (type == MaintenanceCalibration::NAME) {
    MaintenanceCalibration v{};
    j.at("open").get_to(v.open);
    j.at("range_low").get_to(v.range_low);
    j.at("range_high").get_to(v.range_high);
    j.at("value").get_to(v.value);
    j.at("t").get_to(v.t);
    m = v;
  } else if (type == MaintenanceMessagingSystem::NAME) {
    MaintenanceMessagingSystem v{};
    m = v;
  }
}

void to_json(json& j, const EntityData& data) {
  std::visit(
    overloaded{
      [&](const Block&) {
        j = json{
          {"type", "block"},
        };
      },
      [&](const Player& v) {
        j = json{
          {"type", "player"},
          {"inventory", v.inventory},
          {"interaction_radius", v.interaction_radius},
          {"open_gui", v.open_gui},
          {"hand", v.hand},
        };
      },
      [&](const Storage& v) {
        j = json{
          {"type", "storage"},
          {"inventory", v.inventory},
        };
      },
      [&](const Conveyor& v) {
        j = json{
          {"type", "conveyor"},
          {"rotation", v.rotation},
          {"items", v.items},
        };
      },
      [&](const Item& v) {
        j = json{
          {"type", "item"},
          {"slot", v.slot},
        };
      },
      [&](const WorldTunnel& v) {
        j = json{
          {"type", "world_tunnel"},
          {"to", v.to},
          {"inventory", v.inventory},
        };
      },
      [&](const ResourceMessageSender& v) {
        j = json{
          {"type", "resource_message_sender"},
          {"maintenance", v.maintenance},
          {"page", v.page},
          {"msg_in_create", v.msg_in_create},
        };
      },
      [&](const ResourceMessageReceiver& v) {
        j = json{
          {"type", "resource_message_receiver"},
          {"maintenance", v.maintenance},
          {"inventory", v.inventory},
        };
      },
      [&](const Assembler& v) {
        j = json{
          {"type", "assembler"},
          {"maintenance", v.maintenance},
          {"selected_recipe_idx", v.selected_recipe_idx},
          {"inventory", v.inventory},
          {"t", v.t},
        };
      },
    },
    data
  );
}

void from_json(const json& j, EntityData& d) {
  auto type = j.at("type").get<std::string>();
  if (type == "block") {
    d = Block{};
  } else if (type == "player") {
    Player v{};
    j.at("inventory").get_to(v.inventory);
    j.at("interaction_radius").get_to(v.interaction_radius);
    j.at("open_gui").get_to(v.open_gui);
    j.at("hand").get_to(v.hand);
    d = v;
  } else if (type == "storage") {
    Storage v{};
    j.at("inventory").get_to(v.inventory);
    d = v;
  } else if (type == "conveyor") {
    Conveyor v{};
    j.at("rotation").get_to(v.rotation);
    j.at("items").get_to(v.items);
    d = v;
  } else if (type == "item") {
    Item v{};
    j.at("slot").get_to(v.slot);
    d = v;
  } else if (type == "world_tunnel") {
    WorldTunnel v{};
    j.at("to").get_to(v.to);
    j.at("inventory").get_to(v.inventory);
    d = v;
  } else if (type == "resource_message_sender") {
    ResourceMessageSender v{};
    j.at("maintenance").get_to(v.maintenance);
    j.at("page").get_to(v.page);
    j.at("msg_in_create").get_to(v.msg_in_create);
    d = v;
  } else if (type == "resource_message_receiver") {
    ResourceMessageReceiver v{};
    j.at("maintenance").get_to(v.maintenance);
    j.at("inventory").get_to(v.inventory);
    d = v;
  } else if (type == "assembler") {
    Assembler v{};
    j.at("maintenance").get_to(v.maintenance);
    j.at("selected_recipe_idx").get_to(v.selected_recipe_idx);
    j.at("inventory").get_to(v.inventory);
    j.at("t").get_to(v.t);
    d = v;
  } else {
    ASSERT(false, "invalid entity data type");
  }
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Entity, id, pos, world, data);
// TODO: could probably only serialize the entities vector
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EntityStore, free_slots, next_entity_idx, entities);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResourceMessageQueue, msgs);

void to_json(json& j, const State& s) {
  j = json{
    {"version", s.SERIALIZATION_VERSION},
    {"minutes", s.minutes},
    {"resource_message_queue", s.resource_message_queue},
    {"player_id", s.player_id},
    {"resource_message_receiver_id", s.resource_message_receiver_id},
    {"store", s.store},
  };
}

void from_json(const json& j, State& s) {
  ASSERT(j.at("version") == s.SERIALIZATION_VERSION, "invalid serialization version");
  j.at("minutes").get_to(s.minutes);
  j.at("resource_message_queue").get_to(s.resource_message_queue);
  j.at("player_id").get_to(s.player_id);
  j.at("resource_message_receiver_id").get_to(s.resource_message_receiver_id);
  j.at("store").get_to(s.store);
}

void save_state_to_file(const State& state, const std::filesystem::path& filepath) {
  json j(state);
  std::ofstream file{filepath};
  file << std::setw(4) << j << '\n';
}

void load_state_from_file(State& state, const std::filesystem::path& filepath) {
  std::ifstream file{filepath};
  json j         = json::parse(file);
  auto new_state = j.get<State>();

  // TODO: pull this state assigning out to a separate function?
  state.frame_input            = {};
  state.tick_input             = {};
  state.frame                  = {};
  state.ui_system              = {};
  state.minutes_accumulator    = {};
  state.current_place_rotation = {};
  state.debug                  = {};

  state.minutes                      = new_state.minutes;
  state.resource_message_queue       = new_state.resource_message_queue;
  state.player_id                    = new_state.player_id;
  state.resource_message_receiver_id = new_state.resource_message_receiver_id;
  state.store                        = new_state.store;
}
