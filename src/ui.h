#ifndef UI_H
#define UI_H

namespace ui {

enum class CommandType {
  Rect,
  Text,
};

struct Command {
  CommandType type{};
  ivec2 pos{};
  Color color{};

  // NOTE: type - Rect
  ivec2 dims{};

  // NOTE: type - Text
  std::string text{};
  i32 size{};
};

void render(const std::vector<Command>& commands);

}

#endif
