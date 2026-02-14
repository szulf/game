#pragma once

#include "base/base.h"

#include <string_view>

namespace parser
{

struct Pos
{
  std::string_view line;
  usize pos;

  char curr_char()
  {
    return line[pos];
  }

  bool size_ok()
  {
    return line.size() > pos;
  }
};

void skip_whitespace(Pos& pos);
void expect_and_skip(Pos& pos, char c);

std::string_view word(Pos& pos);
f32 number_f32(Pos& pos);
u32 number_u32(Pos& pos);
bool boolean(Pos& pos);

}
