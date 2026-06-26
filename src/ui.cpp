namespace ui {

struct RectCommand {
  ivec2 pos{};
  Color color{};
  ivec2 dims{};
};

struct TextCommand {
  ivec2 pos{};
  Color color{};
  std::string text{};
  i32 size{};
};

using Command = std::variant<RectCommand, TextCommand>;

void render(const std::vector<Command>& commands) {
  for (const auto& cmd : commands) {
    std::visit(
      overloaded{
        [&](const RectCommand& rect) {
          DrawRectangleV(vector2_from_ivec2(rect.pos), vector2_from_ivec2(rect.dims), rect.color);
        },
        [&](const TextCommand& text) {
          DrawText(text.text.c_str(), text.pos.x, text.pos.y, text.size, text.color);
        },
      },
      cmd
    );
  }
}

}
