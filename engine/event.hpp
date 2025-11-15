#pragma once

#include <variant>
#include <cstdint>

#include "action.hpp"

namespace core {

struct ResizeEvent final {
  std::uint32_t width{};
  std::uint32_t height{};
};

struct KeydownEvent final {
  Key key{};
};

struct MouseMoveEvent final {
  std::uint32_t x{};
  std::uint32_t y{};
};

using Event = std::variant<ResizeEvent, KeydownEvent, MouseMoveEvent>;

}
