#pragma once

#include "engine/key.hpp"
#include "badtl/types.hpp"

namespace core {

// TODO(szulf): think of a way to consume events

struct ResizeEvent {
  btl::u32 width;
  btl::u32 height;
};

struct KeydownEvent {
  Key key;
};

struct MouseMoveEvent {
  btl::u32 x;
  btl::u32 y;
};

struct Event {
  enum class Tag {
    Resize = 1,
    Keydown,
    MouseMove,
  };
  union Events {
    ResizeEvent resize;
    KeydownEvent keydown;
    MouseMoveEvent mouse_move;
  };

  static Event make(const ResizeEvent& e) {
    Event out;
    out.tag = Tag::Resize;
    out.event.resize = e;
    return out;
  }
  static Event make(const KeydownEvent& e) {
    Event out;
    out.tag = Tag::Keydown;
    out.event.keydown = e;
    return out;
  }
  static Event make(const MouseMoveEvent& e) {
    Event out;
    out.tag = Tag::MouseMove;
    out.event.mouse_move = e;
    return out;
  }

  // void consume() {
  //   tag = static_cast<Tag>(0);
  // }

  Tag tag;
  Events event;
};

}
