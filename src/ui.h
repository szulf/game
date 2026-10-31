#pragma once

#include <variant>
#include <vector>
#include <unordered_map>

#include "raylib.h"

#include "core.h"
#include "math.h"
#include "input.h"

// NOTE: possibly im doing way too many iterations over the whole data set
// so if ever performance becomes an issue i could look at that

namespace ui {

enum LayoutDirection {
  LAYOUT_DIRECTION_HORIZONTAL,
  LAYOUT_DIRECTION_VERTICAL,
  LAYOUT_DIRECTION_STACK,
};

enum SizingType {
  SIZING_FIT,
  SIZING_FILL,
  SIZING_FIXED,
};

struct SizingAxis {
  SizingType type{};
  u16 fixed_px{};
};

inline SizingAxis sizing_fit() {
  return {.type = SIZING_FIT};
}

inline SizingAxis sizing_fill() {
  return {.type = SIZING_FILL};
}

inline SizingAxis sizing_fixed(u16 px) {
  return {.type = SIZING_FIXED, .fixed_px = px};
}

struct Sizing {
  SizingAxis width{};
  SizingAxis height{};
};

struct Padding {
  u16 top{};
  u16 down{};
  u16 left{};
  u16 right{};
};

inline Padding padding_all(u16 value) {
  return {value, value, value, value};
}

enum ChildAlignmentAxis {
  CHILD_ALIGNMENT_START,
  CHILD_ALIGNMENT_CENTER,
  CHILD_ALIGNMENT_END,
};

struct ChildAlignment {
  ChildAlignmentAxis x{};
  ChildAlignmentAxis y{};
};

#define SCROLL_SENSITIVITY 50

struct ElementConfigNormal {
  LayoutDirection layout_direction{};
  Sizing sizing{};
  Padding padding{};
  u16 child_gap{};
  ChildAlignment child_alignment{};
  Color bg_color{};

  // TODO: could pull these two into their own struct Texture or smth
  const Texture2D* texture{};
  bool flip_texture_vertically{};

  // TODO: should i have a separate corner_radius for each of the corners? (probably yes)
  f32 corner_radius{};
  i32* scroll_value{};
};

enum ElementType {
  ELEMENT_NORMAL,
  ELEMENT_TEXT,
};

// TODO: remove this?
static constexpr f32 TEXT_SPACING = 2;

struct ElementConfigText {
  std::string text{};
  f32 size{};
  Color color{};
};

using ElementConfig = std::variant<ElementConfigNormal, ElementConfigText>;

inline ElementConfigNormal* config_normal(ElementConfig& config) {
  return std::get_if<ElementConfigNormal>(&config);
}

template <typename T>
inline bool is(const ElementConfig& config) {
  return std::holds_alternative<T>(config);
}

struct StateOptions {
  bool* clicked{};
  bool* hovered{};
};

using ElementIdx = u32;
using Id         = const char*;
using IdInternal = u32;

struct Element {
  IdInternal id{};
  ElementIdx parent{};
  // NOTE: if idx == 0 then there is no child
  ElementIdx first_child{};
  // NOTE: if idx == 0 then there is no sibling
  ElementIdx next_sibling{};
  ElementConfig config{};
  vec2 dimensions{};
  vec2 pos{};
  Rectangle clip_rectangle{};
};

struct QuadCommand {
  vec2 pos{};
  vec2 dims{};
  f32 corner_radius{};
  Color tint{};
  std::optional<Rectangle> clip_rectangle{};
};

struct TextureCommand {
  vec2 pos{};
  vec2 dims{};
  const Texture2D* texture{};
  bool flip_vertically{};
  Color tint{};
  std::optional<Rectangle> clip_rectangle{};
};

struct TextCommand {
  vec2 pos{};
  std::string string{};
  f32 size{};
  Color tint{};
};

using Command = std::variant<QuadCommand, TextureCommand, TextCommand>;

struct System {
  std::vector<Command> cmds{};
  struct LastFrameData {
    std::unordered_map<IdInternal, ElementIdx> id_map{};
    std::vector<Element> elements{};
  };
  std::unordered_map<IdInternal, LastFrameData> last_frame_data{};
};

// NOTE: needs to be called once every frame, before the first begin_layout
void system_update(System& system);

struct Layout {
  IdInternal id{};
  System* system{};
  const Input* input{};
  // NOTE: weird hack to get the union working, because c++
  // (should probably switch to a std::variant somewhere to fix this properly)
  std::vector<std::string> strings{};
  std::vector<Element> elements{};
  vec2 pos{};
  vec2 max_dimensions{};

  bool _element{};
  ElementIdx _active_parent{};
};

enum Axis {
  AXIS_X,
  AXIS_Y,
};

static constexpr Id AUTO_ID = nullptr;

struct ElementSpec {
  ElementConfigNormal config{};
  StateOptions state_options{};
  Id id = AUTO_ID;
};

void element_resolve_state(Layout& layout, Id id, const StateOptions& state_options);
void element_begin(Layout& layout, Id id, const ElementConfigNormal& config);
vec2 element_get_pos(Layout& layout, Id id);
void element_end(Layout& layout);
void text(Layout& layout, std::string_view text, f32 size, Color color = BLACK);
void texture(
  Layout& layout,
  const Texture2D& texture,
  bool flip_vertically = false,
  Id id                = AUTO_ID
);
Layout layout_begin(
  Id id,
  System& system,
  const Input& input,
  const vec2& pos,
  const vec2& max_dimensions
);
void layout_end(Layout& layout);

struct Scope {
  template <typename F>
  Scope(Layout& _layout, F&& spec) : layout{_layout} {
    ElementSpec element_spec = spec();
    element_resolve_state(layout, element_spec.id, element_spec.state_options);

    element_spec = spec();
    element_begin(layout, element_spec.id, element_spec.config);
  }

  inline ~Scope() {
    element_end(layout);
  }

  Layout& layout;
};

#define UI_CAT_(a, b) a##b
#define UI_CAT(a, b) UI_CAT_(a, b)

#define UI(layout, ...)                                                                            \
  if (                                                                                             \
    ::ui::Scope UI_CAT(ui_s_, __COUNTER__){                                                        \
      (layout),                                                                                    \
      [&]() -> ::ui::ElementSpec {                                                                 \
        return {__VA_ARGS__};                                                                      \
      }                                                                                            \
    };                                                                                             \
    true                                                                                           \
  )

// NOTE: raylib renderer
void render(System& system);

}
