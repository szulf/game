#pragma once

#include <cstdint>

namespace core {

struct Event {
  enum class Type : std::uint8_t {
    WindowResize,
  };

  Event(Type t) : type{t} {}

  Type type{};
};

struct WindowResizeEvent final : public Event {
  constexpr WindowResizeEvent(std::uint32_t w, std::uint32_t h) noexcept
    : Event{Event::Type::WindowResize}, width{w}, height{h} {}

  std::uint32_t width{};
  std::uint32_t height{};
};

}
