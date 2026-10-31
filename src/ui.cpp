#include "ui.h"

#include "utils.h"

namespace ui {

void system_update(System& system) {
  system.cmds.clear();
}

static bool intersects(const vec2& point, const vec2& start, const vec2& dimensions) {
  return (point.x > start.x && point.x < start.x + dimensions.x) &&
         (point.y > start.y && point.y < start.y + dimensions.y);
}

constexpr static std::array<LayoutDirection, 2> layout_direction_from_axis = []() {
  std::array<LayoutDirection, 2> t{};
  t[AXIS_X] = LAYOUT_DIRECTION_HORIZONTAL;
  t[AXIS_Y] = LAYOUT_DIRECTION_VERTICAL;
  return t;
}();

constexpr static std::array<Axis, 3> axis_from_layout_direction = []() {
  std::array<Axis, 3> t{};
  t[LAYOUT_DIRECTION_HORIZONTAL] = AXIS_X;
  t[LAYOUT_DIRECTION_VERTICAL]   = AXIS_Y;
  // TODO: does it actually matter what i put in here?
  t[LAYOUT_DIRECTION_STACK] = AXIS_X;
  return t;
}();

static SizingAxis& sizing_from_axis(ElementConfigNormal& config, Axis axis) {
  if (axis == AXIS_X) {
    return config.sizing.width;
  } else if (axis == AXIS_Y) {
    return config.sizing.height;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32& dimension_from_axis(Element& elem, Axis axis) {
  if (axis == AXIS_X) {
    return elem.dimensions.x;
  } else if (axis == AXIS_Y) {
    return elem.dimensions.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 total_padding_from_axis(ElementConfigNormal& config, Axis axis) {
  if (axis == AXIS_X) {
    return config.padding.left + config.padding.right;
  } else if (axis == AXIS_Y) {
    return config.padding.top + config.padding.down;
  }
  ASSERT(false, "Invalid axis provided");
}

static void calculate_fit_fixed_sizing_axis(Layout& layout, ElementIdx idx, Axis axis) {
  auto& elem   = layout.elements[idx];
  auto& config = elem.config.normal;
  auto& sizing = sizing_from_axis(config, axis);
  switch (sizing.type) {
    case SIZING_FIXED:
      dimension_from_axis(elem, axis) = sizing.fixed_px;
      break;
    case SIZING_FIT:
    case SIZING_FILL:
      for (ElementIdx child_idx = elem.first_child; child_idx != 0;
           child_idx            = layout.elements[child_idx].next_sibling) {
        auto& child = layout.elements[child_idx];
        if (config.layout_direction == layout_direction_from_axis[axis]) {
          dimension_from_axis(elem, axis) += dimension_from_axis(child, axis) + config.child_gap;
        } else {
          f32& dim = dimension_from_axis(elem, axis);
          dim      = std::max(dim, dimension_from_axis(child, axis));
        }
      }
      break;
  }
  dimension_from_axis(elem, axis) += total_padding_from_axis(config, axis);
}

// NOTE: fill sizing elements are treated as fit sizing when
// the parent has a fit sizing on the same axis
// and the current layout direction corresponds to that axis (horizontal for x, vertical for y)
// NOT SURE if that is the 'correct' behaviour but it makes sense to me
static void calculate_text_fit_fixed_sizing(Layout& layout, ElementIdx idx = 0) {
  bool has_children = false;
  auto& elem        = layout.elements[idx];
  for (ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx            = layout.elements[child_idx].next_sibling) {
    has_children = true;
    calculate_text_fit_fixed_sizing(layout, child_idx);
  }

  switch (elem.config.type) {
    case ELEMENT_NORMAL: {
      calculate_fit_fixed_sizing_axis(layout, idx, AXIS_X);
      calculate_fit_fixed_sizing_axis(layout, idx, AXIS_Y);
      auto& config = elem.config.normal;
      if (has_children) {
        dimension_from_axis(elem, axis_from_layout_direction[config.layout_direction]) -=
          config.child_gap;
      }
    } break;

    case ELEMENT_TEXT: {
      auto& config    = elem.config.text;
      auto& str       = layout.strings[config.string_idx];
      elem.dimensions = vec2_from_raylib(
        MeasureTextEx(GetFontDefault(), str.c_str(), f32(config.size), TEXT_SPACING)
      );
    } break;
  }
}

static void calculate_fill_sizing_axis(Layout& layout, ElementIdx idx, Axis axis) {
  auto& elem   = layout.elements[idx];
  auto& config = elem.config.normal;
  u32 fill_child_count{};
  f32 available_space = dimension_from_axis(elem, axis) - total_padding_from_axis(config, axis);
  if (config.layout_direction == layout_direction_from_axis[axis]) {
    for (ElementIdx child_idx = elem.first_child; child_idx != 0;
         child_idx            = layout.elements[child_idx].next_sibling) {
      auto& child = layout.elements[child_idx];
      if (sizing_from_axis(child.config.normal, axis).type == SIZING_FILL) {
        ++fill_child_count;
      } else {
        available_space -= dimension_from_axis(child, axis);
      }
      available_space -= config.child_gap;
    }
    available_space += config.child_gap;
  }
  for (ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx            = layout.elements[child_idx].next_sibling) {
    auto& child = layout.elements[child_idx];
    if (child.config.type == ELEMENT_TEXT) {
      continue;
    }
    if (sizing_from_axis(child.config.normal, axis).type == SIZING_FILL) {
      if (config.layout_direction == layout_direction_from_axis[axis]) {
        if (sizing_from_axis(config, axis).type != SIZING_FIT) {
          dimension_from_axis(child, axis) = available_space / f32(fill_child_count);
        }
      } else {
        dimension_from_axis(child, axis) = available_space;
      }
    }
  }
}

static void calculate_fill_sizing(Layout& layout, ElementIdx idx = 0) {
  auto& elem = layout.elements[idx];
  if (elem.config.type == ELEMENT_TEXT) {
    return;
  }
  calculate_fill_sizing_axis(layout, idx, AXIS_X);
  calculate_fill_sizing_axis(layout, idx, AXIS_Y);
  for (ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx            = layout.elements[child_idx].next_sibling) {
    calculate_fill_sizing(layout, child_idx);
  }
}

static ChildAlignmentAxis child_alignment_from_axis(const ElementConfigNormal& config, Axis axis) {
  if (axis == AXIS_X) {
    return config.child_alignment.x;
  } else if (axis == AXIS_Y) {
    return config.child_alignment.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32& pos_from_axis(Element& elem, Axis axis) {
  if (axis == AXIS_X) {
    return elem.pos.x;
  } else if (axis == AXIS_Y) {
    return elem.pos.y;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 start_padding_from_axis(ElementConfigNormal& config, Axis axis) {
  if (axis == AXIS_X) {
    return config.padding.left;
  } else if (axis == AXIS_Y) {
    return config.padding.top;
  }
  ASSERT(false, "Invalid axis provided");
}

static f32 end_padding_from_axis(ElementConfigNormal& config, Axis axis) {
  if (axis == AXIS_X) {
    return config.padding.right;
  } else if (axis == AXIS_Y) {
    return config.padding.down;
  }
  ASSERT(false, "Invalid axis provided");
}

// TODO: check all the centering code, not sure if its correct!
static void calculate_position_axis(
  Layout& layout,
  ElementIdx idx,
  ElementIdx child_idx,
  f32& used_space,
  Axis axis
) {
  auto& elem   = layout.elements[idx];
  auto& config = elem.config.normal;
  auto& child  = layout.elements[child_idx];
  if (config.layout_direction == layout_direction_from_axis[axis]) {
    switch (child_alignment_from_axis(config, axis)) {
      case CHILD_ALIGNMENT_START:
        pos_from_axis(child, axis) =
          pos_from_axis(elem, axis) + start_padding_from_axis(config, axis) + used_space;
        used_space += dimension_from_axis(child, axis) + config.child_gap;
        break;
      case CHILD_ALIGNMENT_CENTER:
        pos_from_axis(child, axis) =
          pos_from_axis(elem, axis) + start_padding_from_axis(config, axis) + used_space;
        used_space += dimension_from_axis(child, axis) + config.child_gap;
        break;
      case CHILD_ALIGNMENT_END:
        pos_from_axis(child, axis) = pos_from_axis(elem, axis) + dimension_from_axis(elem, axis) -
                                     end_padding_from_axis(config, axis) -
                                     dimension_from_axis(child, axis) - used_space;
        used_space += dimension_from_axis(child, axis) + config.child_gap;
        break;
    }
  } else {
    switch (child_alignment_from_axis(config, axis)) {
      case CHILD_ALIGNMENT_START:
        pos_from_axis(child, axis) =
          pos_from_axis(elem, axis) + start_padding_from_axis(config, axis);
        break;
      case CHILD_ALIGNMENT_CENTER:
        pos_from_axis(child, axis) =
          (pos_from_axis(elem, axis) + start_padding_from_axis(config, axis)) +
          ((dimension_from_axis(elem, axis) - total_padding_from_axis(config, axis)) * 0.5f) -
          (dimension_from_axis(child, axis) * 0.5f);
        break;
      case CHILD_ALIGNMENT_END:
        pos_from_axis(child, axis) = pos_from_axis(elem, axis) + dimension_from_axis(elem, axis) -
                                     end_padding_from_axis(config, axis) -
                                     dimension_from_axis(child, axis);
        break;
    }
  }
}

static void adjust_centered_position(Layout& layout, ElementIdx idx, f32 used_space, Axis axis) {
  auto& elem   = layout.elements[idx];
  auto& config = elem.config.normal;
  if (
    config.layout_direction == layout_direction_from_axis[axis] &&
    child_alignment_from_axis(config, axis) == CHILD_ALIGNMENT_CENTER
  ) {
    f32 adjust_value = (dimension_from_axis(elem, axis) - start_padding_from_axis(config, axis) -
                        end_padding_from_axis(config, axis) - used_space) *
                       0.5f;
    for (ElementIdx child_idx = elem.first_child; child_idx != 0;
         child_idx            = layout.elements[child_idx].next_sibling) {
      auto& child = layout.elements[child_idx];
      pos_from_axis(child, axis) += adjust_value;
    }
  }
}

static void calculate_positions(Layout& layout, ElementIdx idx = 0) {
  auto& elem = layout.elements[idx];
  if (idx == 0) {
    elem.pos = {layout.pos.x, layout.pos.y};
  }
  if (elem.config.type == ELEMENT_TEXT) {
    return;
  }
  auto& config   = elem.config.normal;
  f32 used_space = 0;
  for (ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx            = layout.elements[child_idx].next_sibling) {
    auto& child = layout.elements[child_idx];
    calculate_position_axis(layout, idx, child_idx, used_space, AXIS_X);
    calculate_position_axis(layout, idx, child_idx, used_space, AXIS_Y);
    if (config.scroll_value) {
      child.pos.y += (f32) *config.scroll_value * SCROLL_SENSITIVITY;
    }
  }
  adjust_centered_position(layout, idx, used_space, AXIS_X);
  adjust_centered_position(layout, idx, used_space, AXIS_Y);
  for (ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx            = layout.elements[child_idx].next_sibling) {
    calculate_positions(layout, child_idx);
  }
}

static bool intersects(const Rectangle& a, const Rectangle& b) {
  return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;
}

static void calculate_scroll_value(Layout& layout, ElementIdx idx) {
  auto& elem   = layout.elements[idx];
  auto& config = elem.config.normal;
  f32 max_height{};
  switch (config.layout_direction) {
    case LAYOUT_DIRECTION_STACK:
    case LAYOUT_DIRECTION_HORIZONTAL: {
      for (ElementIdx child_idx = elem.first_child; child_idx != 0;
           child_idx            = layout.elements[child_idx].next_sibling) {
        auto& child = layout.elements[child_idx];
        max_height =
          std::max(max_height, child.dimensions.y + config.padding.top + config.padding.down);
      }
    } break;
    case LAYOUT_DIRECTION_VERTICAL: {
      for (ElementIdx child_idx = elem.first_child; child_idx != 0;
           child_idx            = layout.elements[child_idx].next_sibling) {
        auto& child = layout.elements[child_idx];
        max_height += child.dimensions.y + config.child_gap;
      }
      max_height -= config.child_gap;
      max_height += config.padding.top + config.padding.down;
    } break;
  }

  *config.scroll_value += layout.input->mouse_scroll;
  *config.scroll_value =
    std::clamp(*config.scroll_value, (i32) -std::floor(max_height / SCROLL_SENSITIVITY), 0);
}

static void handle_scroll(Layout& layout) {
  for (i32 idx = i32(layout.elements.size()) - 1; idx >= 0; --idx) {
    auto& elem = layout.elements[idx];
    if (elem.config.type != ELEMENT_NORMAL) {
      continue;
    }
    auto& config = elem.config.normal;
    if (intersects(layout.input->mouse_pos, elem.pos, elem.dimensions)) {
      if (config.scroll_value) {
        calculate_scroll_value(layout, (ElementIdx) idx);
      } else {
        for (ElementIdx parent_idx = elem.parent; parent_idx != 0;
             parent_idx            = layout.elements[parent_idx].parent) {
          auto& parent = layout.elements[parent_idx];
          if (parent.config.normal.scroll_value) {
            calculate_scroll_value(layout, parent_idx);
            break;
          }
        }
      }
      break;
    }
  }
}

static Rectangle intersection_rectangle(const Rectangle& a, const Rectangle& b) {
  f32 left  = std::max(a.x, b.x);
  f32 top   = std::max(a.y, b.y);
  f32 right = std::min(a.x + a.width, b.x + b.width);
  f32 down  = std::min(a.y + a.height, b.y + b.height);

  if (right > left && down > top) {
    return {
      .x      = left,
      .y      = top,
      .width  = right - left,
      .height = down - top,
    };
  }
  return {};
}

static void generate_render_cmds(Layout& layout, ElementIdx idx = 0) {
  // TODO: not sure where to place this line of code
  layout.elements[0].clip_rectangle =
    rect_from_vec2x2(layout.elements[0].pos, layout.elements[0].dimensions);
  auto& elem = layout.elements[idx];
  for (ElementIdx child_idx = elem.first_child; child_idx != 0;
       child_idx            = layout.elements[child_idx].next_sibling) {
    auto& child = layout.elements[child_idx];
    if (!intersects(
          rect_from_vec2x2(elem.pos, elem.dimensions),
          rect_from_vec2x2(child.pos, child.dimensions)
        )) {
      continue;
    }
    child.clip_rectangle =
      intersection_rectangle(rect_from_vec2x2(elem.pos, elem.dimensions), elem.clip_rectangle);
    switch (child.config.type) {
      case ELEMENT_NORMAL: {
        auto& config = child.config.normal;
        // TODO: this is not really render cmd generation, not sure if it belongs here
        layout.system->last_frame_data[layout.id].id_map.insert_or_assign(child.id, child_idx);
        if (config.texture) {
          // TODO: this is not really the ideal solution,
          // what if someone really wants to render a fully transparent texture?
          // (good enough for now tho)
          auto bg = config.bg_color != Color{} ? config.bg_color : WHITE;

          layout.system->cmds.push_back(
            TextureCommand{
              .pos             = child.pos,
              .dims            = child.dimensions,
              .texture         = config.texture,
              .flip_vertically = config.flip_texture_vertically,
              .tint            = bg,
              .clip_rectangle  = {child.clip_rectangle},
            }
          );
        } else {
          layout.system->cmds.push_back(
            QuadCommand{
              .pos            = child.pos,
              .dims           = child.dimensions,
              .corner_radius  = config.corner_radius,
              .tint           = config.bg_color,
              .clip_rectangle = {child.clip_rectangle},
            }
          );
        }
      } break;
      case ELEMENT_TEXT: {
        auto& config = child.config.text;
        auto& str    = layout.strings[config.string_idx];
        layout.system->cmds.push_back(
          TextCommand{
            .pos    = child.pos,
            .string = str,
            .size   = config.size,
            .tint   = config.color,
          }
        );
      } break;
    }
    generate_render_cmds(layout, child_idx);
  }
}

// TODO: i hate this, but currently i dont have a clue what could be better
static void set_first_child_or_next_sibling(Layout& layout) {
  auto& parent = layout.elements[layout._active_parent];
  if (parent.first_child == 0) {
    parent.first_child = layout.elements.size() - 1;
  } else {
    Element* prev_sibling{};
    for (ElementIdx idx = parent.first_child; idx != 0; idx = layout.elements[idx].next_sibling) {
      prev_sibling = &layout.elements[idx];
    }
    prev_sibling->next_sibling = layout.elements.size() - 1;
  }
}

void element_resolve_state(Layout& layout, Id id, const StateOptions& state_options) {
  IdInternal id_internal = id ? std::hash<std::string_view>{}(std::string_view{id})
                              : std::hash<ElementIdx>{}(layout.elements.size());
  if (layout.system->last_frame_data[layout.id].id_map.contains(id_internal)) {
    auto& idx  = layout.system->last_frame_data[layout.id].id_map[id_internal];
    auto& elem = layout.system->last_frame_data[layout.id].elements[idx];
    Rectangle interaction_rect =
      intersection_rectangle(rect_from_vec2x2(elem.pos, elem.dimensions), elem.clip_rectangle);
    bool hovered = intersects(
      layout.input->mouse_pos,
      pos_from_rect(interaction_rect),
      dims_from_rect(interaction_rect)
    );
    // TODO: should check if there are no non child elements on top of me
    // before setting hovered/clicked to true
    // tho currently i dont think there even is a way to have two non related elements overlap
    // but i do plan to add that
    if (hovered) {
      if (state_options.hovered) {
        *state_options.hovered = hovered;
      }
      if (state_options.clicked) {
        *state_options.clicked = hovered && layout.input->lmb.pressed();
      }
    }
  }
}

void element_begin(Layout& layout, Id id, const ElementConfigNormal& config) {
  IdInternal id_internal = id ? std::hash<std::string_view>{}(std::string_view{id})
                              : std::hash<ElementIdx>{}(layout.elements.size());
  // TODO: there has to be a way to write this assert without the outer if statement
  if (config.flip_texture_vertically) {
    ASSERT(config.texture, "cannot set flip_texture_vertically with no texture");
  }
  ASSERT(config.corner_radius == 0.0f, "textured thing cannot have corner radius");
  layout.elements.push_back({
    .id     = id_internal,
    .parent = layout._active_parent,
    .config = {.type = ELEMENT_NORMAL, .normal = config},
  });
  set_first_child_or_next_sibling(layout);
  layout._active_parent = layout.elements.size() - 1;
}

// NOTE: returns the position of the element from the last frame can be called whenever really
vec2 element_get_pos(Layout& layout, Id id) {
  ASSERT(id != AUTO_ID, "have to use a proper element id to get its last position");
  IdInternal id_internal = std::hash<std::string_view>{}(std::string_view{id});
  if (layout.system->last_frame_data[layout.id].id_map.contains(id_internal)) {
    auto& idx  = layout.system->last_frame_data[layout.id].id_map[id_internal];
    auto& elem = layout.system->last_frame_data[layout.id].elements[idx];
    return elem.pos;
  }
  // TODO: do i want to assert here? or maybe return an optional?
  return {};
}

// TODO: separate element_set_style() function?
void element_end(Layout& layout) {
  auto& elem = layout.elements[layout._active_parent];
  ASSERT(elem.config.type == ELEMENT_NORMAL, "Cannot close a not normal element");
  layout._active_parent = elem.parent;
}

void text(Layout& layout, std::string_view text, f32 size, Color color) {
  layout.strings.push_back(std::string{text});
  layout.elements.push_back({
    .parent = layout._active_parent,
    .config = {
      .type = ELEMENT_TEXT,
      .text = {.string_idx = u32(layout.strings.size() - 1), .size = size, .color = color}
    },
  });
  set_first_child_or_next_sibling(layout);
}

void texture(Layout& layout, const Texture2D& texture, bool flip_vertically, Id id) {
  UI(
    layout,
    .config =
      {
        .sizing  = {ui::sizing_fixed(texture.width), ui::sizing_fixed(texture.height)},
        .texture = &texture,
        .flip_texture_vertically = flip_vertically,
      },
    .id = id,
  ){};
}

// TODO: sometimes i dont want to pass max_dimensions
Layout layout_begin(
  Id id,
  System& system,
  const Input& input,
  const vec2& pos,
  const vec2& max_dimensions
) {
  ASSERT(id, "layouts cannot have auto ids");
  IdInternal id_internal = std::hash<std::string_view>{}(std::string_view{id});
  Layout layout          = {
    .id             = id_internal,
    .system         = &system,
    .input          = &input,
    .pos            = pos,
    .max_dimensions = max_dimensions,
  };
  element_begin(
    layout,
    AUTO_ID,
    {
      .sizing = {
        sizing_fixed((u16) layout.max_dimensions.x),
        sizing_fixed((u16) layout.max_dimensions.y),
      },
    }
  );
  return layout;
}

void layout_end(Layout& layout) {
  element_end(layout);
  calculate_text_fit_fixed_sizing(layout);
  calculate_fill_sizing(layout);
  calculate_positions(layout);
  handle_scroll(layout);
  layout.system->last_frame_data[layout.id].id_map.clear();
  generate_render_cmds(layout);
  layout.system->last_frame_data[layout.id].elements = layout.elements;
}

// NOTE: raylib renderer
void render(System& system) {
  for (const auto& cmd : system.cmds) {
    std::visit(
      overloaded{
        [](const QuadCommand& quad) {
          auto rect = rect_from_vec2x2(quad.pos, quad.dims);

          if (quad.clip_rectangle) {
            BeginScissorMode(
              i32(quad.clip_rectangle->x),
              i32(quad.clip_rectangle->y),
              i32(quad.clip_rectangle->width),
              i32(quad.clip_rectangle->height)
            );
          }
          DrawRectangleRounded(rect, quad.corner_radius, 0, quad.tint);
          if (quad.clip_rectangle) {
            EndScissorMode();
          }
        },
        [](const TextureCommand& texture) {
          auto source_rect = rect_from_vec2x2({}, dims_from_texture(*texture.texture));
          if (texture.flip_vertically) {
            source_rect.height *= -1;
          }
          auto dest_rect = rect_from_vec2x2(texture.pos, texture.dims);

          if (texture.clip_rectangle) {
            BeginScissorMode(
              i32(texture.clip_rectangle->x),
              i32(texture.clip_rectangle->y),
              i32(texture.clip_rectangle->width),
              i32(texture.clip_rectangle->height)
            );
          }
          DrawTexturePro(*texture.texture, source_rect, dest_rect, {}, 0, texture.tint);
          if (texture.clip_rectangle) {
            EndScissorMode();
          }
        },
        [](const TextCommand& text) {
          DrawTextEx(
            GetFontDefault(),
            text.string.c_str(),
            vec2_to_raylib(text.pos),
            text.size,
            TEXT_SPACING,
            text.tint
          );
        },
      },
      cmd
    );
  }
}

}
