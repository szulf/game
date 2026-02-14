#include "parser.h"

#include <cctype>
#include <format>
#include <charconv>

namespace parser
{

void skip_whitespace(Pos& pos)
{
  while (pos.size_ok() && std::isspace(pos.curr_char()))
  {
    ++pos.pos;
  }
}

void expect_and_skip(Pos& pos, char c)
{
  ASSERT(pos.curr_char() == c, "Expected '{}', found '{}'.", c, pos.curr_char());
  ++pos.pos;
  skip_whitespace(pos);
}

std::string_view word(Pos& pos)
{
  usize word_length{};
  while (pos.size_ok() &&
         (std::isalnum(pos.curr_char()) || pos.curr_char() == '#' || pos.curr_char() == '.' ||
          pos.curr_char() == '_' || pos.curr_char() == '/'))
  {
    ++word_length;
    ++pos.pos;
  }

  auto word = pos.line.substr(pos.pos - word_length, word_length);
  skip_whitespace(pos);
  return word;
}

f32 number_f32(Pos& pos)
{
  usize num_length{};
  while (pos.size_ok() &&
         (std::isdigit(pos.curr_char()) || pos.curr_char() == '.' || pos.curr_char() == 'e' ||
          pos.curr_char() == 'E' || pos.curr_char() == '+' || pos.curr_char() == '-'))
  {
    ++num_length;
    ++pos.pos;
  }

  f32 result{};
  auto ec =
    std::from_chars(pos.line.data() + (pos.pos - num_length), pos.line.data() + pos.pos, result).ec;
  ASSERT(ec == std::errc{}, "Invalid f32.");
  skip_whitespace(pos);
  return result;
}

u32 number_u32(Pos& pos)
{
  usize num_length{};
  while (pos.size_ok() && std::isdigit(pos.curr_char()))
  {
    ++num_length;
    ++pos.pos;
  }

  u32 result{};
  auto ec =
    std::from_chars(pos.line.data() + (pos.pos - num_length), pos.line.data() + pos.pos, result).ec;
  ASSERT(ec == std::errc{}, "Invalid u32.");
  skip_whitespace(pos);
  return result;
}

bool boolean(Pos& pos)
{
  if (pos.line.size() - pos.pos >= 4 && pos.line.substr(pos.pos, 4) == "true")
  {
    return true;
  }
  else if (pos.line.size() - pos.pos >= 5 && pos.line.substr(pos.pos, 5) == "false")
  {
    return false;
  }
  ASSERT(false, "Invalid boolean.");
}

}
