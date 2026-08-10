#pragma once

#include <variant>
#include <unordered_map>

#include "raylib.h"

#include "core.h"
#include "math.h"
#include "utils.h"
#include "input.h"

// NOTE: possibly im doing way too many iterations over the whole data set
// so if ever performance becomes an issue i could look at that

// TODO: helper function to render textures?
// ui_element_begin(layout, UI_AUTO_ID);
// ui_element_end(
//   layout,
//   {.sizing = {ui_sizing_fixed(texture.width), ui_sizing_fixed(texture.height)}, .texture =
//   &texture}
// );

enum UI_LayoutDirection {
  UI_LAYOUT_DIRECTION_HORIZONTAL,
  UI_LAYOUT_DIRECTION_VERTICAL,
};

enum UI_SizingType {
  UI_SIZING_FIT,
  UI_SIZING_FILL,
  UI_SIZING_FIXED,
};

struct UI_SizingAxis {
  UI_SizingType type{};
  u16 fixed_px{};
};

inline UI_SizingAxis ui_sizing_fit() {
  return {.type = UI_SIZING_FIT};
}

inline UI_SizingAxis ui_sizing_fill() {
  return {.type = UI_SIZING_FILL};
}

inline UI_SizingAxis ui_sizing_fixed(u16 px) {
  return {.type = UI_SIZING_FIXED, .fixed_px = px};
}

struct UI_Sizing {
  UI_SizingAxis width{};
  UI_SizingAxis height{};
};

struct UI_Padding {
  u16 top{};
  u16 down{};
  u16 left{};
  u16 right{};
};

inline UI_Padding ui_padding_all(u16 value) {
  return {value, value, value, value};
}

enum UI_ChildAlignmentAxis {
  UI_CHILD_ALIGNMENT_START,
  UI_CHILD_ALIGNMENT_CENTER,
  UI_CHILD_ALIGNMENT_END,
};

struct UI_ChildAlignment {
  UI_ChildAlignmentAxis x{};
  UI_ChildAlignmentAxis y{};
};

#define SCROLL_SENSITIVITY 50

struct UI_ElementConfigNormal {
  UI_LayoutDirection layout_direction{};
  UI_Sizing sizing{};
  UI_Padding padding{};
  u16 child_gap{};
  UI_ChildAlignment child_alignment{};
  Color bg_color{};

  // TODO: could pull these two into their own struct UI_Texture or smth
  const Texture2D* texture{};
  bool flip_texture_vertically{};

  // TODO: should i have a separate corner_radius for each of the corners? (probably yes)
  f32 corner_radius{};
  i32* scroll_value{};
};

enum UI_ElementType {
  UI_ELEMENT_NORMAL,
  UI_ELEMENT_TEXT,
};

// TODO: remove this?
static constexpr f32 TEXT_SPACING = 2;

struct UI_ElementConfig {
  UI_ElementType type{};
  union {
    UI_ElementConfigNormal normal{};
    struct {
      u32 string_idx{};
      f32 size{};
      Color color{};
    } text;
  };
};

struct UI_StateOptions {
  bool* clicked{};
  bool* hovered{};
};

using UI_ElementIdx = u32;
using UI_Id         = const char*;
using UI_IdInternal = u32;

struct UI_Element {
  UI_IdInternal id{};
  UI_ElementIdx parent{};
  // NOTE: if idx == 0 then there is no child
  UI_ElementIdx first_child{};
  // NOTE: if idx == 0 then there is no sibling
  UI_ElementIdx next_sibling{};
  UI_ElementConfig config{};
  vec2 dimensions{};
  vec2 pos{};
  Rectangle clip_rectangle{};
};

struct UI_QuadCommand {
  vec2 pos{};
  vec2 dims{};
  f32 corner_radius{};
  Color tint{};
  std::optional<Rectangle> clip_rectangle{};
};

struct UI_TextureCommand {
  vec2 pos{};
  vec2 dims{};
  const Texture2D* texture{};
  bool flip_vertically{};
  Color tint{};
  std::optional<Rectangle> clip_rectangle{};
};

struct UI_TextCommand {
  vec2 pos{};
  std::string string{};
  f32 size{};
  Color tint{};
};

using UI_Command = std::variant<UI_QuadCommand, UI_TextureCommand, UI_TextCommand>;

struct UI_System {
  std::vector<UI_Command> ui_cmds{};
  struct LastFrameData {
    std::unordered_map<UI_IdInternal, UI_ElementIdx> id_map{};
    std::vector<UI_Element> elements{};
  };
  std::unordered_map<UI_IdInternal, LastFrameData> last_frame_data{};
};

// NOTE: needs to be called once every frame, before the first ui_begin_layout
void ui_system_update(UI_System& system);

struct UI_Layout {
  UI_IdInternal id{};
  UI_System* system{};
  const Input* input{};
  // NOTE: weird hack to get the union working, because c++
  // (should probably switch to a std::variant somewhere to fix this properly)
  std::vector<std::string> strings{};
  std::vector<UI_Element> elements{};
  vec2 pos{};
  vec2 max_dimensions{};

  bool _element{};
  UI_ElementIdx _active_parent{};
};

enum UI_Axis {
  UI_AXIS_X,
  UI_AXIS_Y,
};

static constexpr UI_Id UI_AUTO_ID = nullptr;

void ui_element_begin(UI_Layout& layout, UI_Id id, const UI_StateOptions& state_options = {});
vec2 ui_element_get_pos(UI_Layout& layout, UI_Id id);
void ui_element_end(UI_Layout& layout, const UI_ElementConfigNormal& config);
void ui_text(UI_Layout& layout, std::string_view text, f32 size, Color color = BLACK);
UI_Layout ui_layout_begin(
  UI_Id id,
  UI_System& system,
  const Input& input,
  const vec2& pos,
  const vec2& max_dimensions
);
void ui_layout_end(UI_Layout& layout);

// NOTE: raylib renderer
void ui_render(UI_System& system);
