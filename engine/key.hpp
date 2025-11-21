#pragma once

#include "badtl/types.hpp"
#include "badtl/array.hpp"

namespace core {

enum class Key {
  E,
  T,
  Count,
};

template <typename Action>
struct KeyMap final {
  Action operator[](Key key) const noexcept {
    return map[static_cast<btl::usize>(key)];
  }

  void bind(Key key, Action action) noexcept {
    map[static_cast<btl::usize>(key)] = action;
  }

  btl::Array<Action, static_cast<btl::usize>(Key::Count)> map;
};

}
