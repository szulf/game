namespace ui {

void render(const std::vector<Command>& commands) {
  for (const auto& cmd : commands) {
    switch (cmd.type) {
      case CommandType::Rect: {
        DrawRectangleV(vector2_from_ivec2(cmd.pos), vector2_from_ivec2(cmd.dims), cmd.color);
      } break;

      case CommandType::Text:
        DrawText(cmd.text.c_str(), cmd.pos.x, cmd.pos.y, cmd.size, cmd.color);
        break;
    }
  }
}

}
