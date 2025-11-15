#pragma once

#include "utils/assert.hpp"
#include <array>

namespace core {

enum class Key {
  E,
  Count,
};

template <typename Action>
  requires std::is_enum_v<Action>
struct KeyMap final {
  Action operator[](Key key) const noexcept {
    ASSERT(key < Key::Count, "Invalid key value provided");
    return map[static_cast<std::size_t>(key)];
  }

  constexpr void bind(Key key, Action action) noexcept {
    ASSERT(key < Key::Count, "Invalid key value provided.");
    map[static_cast<std::size_t>(key)] = action;
  }

  static constexpr KeyMap<Action>& instance() noexcept {
    static KeyMap<Action> k;
    return k;
  }

  std::array<Action, static_cast<std::size_t>(Key::Count)> map{};
};

}
