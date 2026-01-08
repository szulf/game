#include "base.h"

#include "math.cpp"
#include "memory.cpp"
#include "array.cpp"
#include "map.cpp"
#include "string.cpp"
#include "vertex.cpp"

const char* key_to_cstr(Key key)
{
  switch (key)
  {
    case Key::W:
      return "W";
    case Key::S:
      return "S";
    case Key::A:
      return "A";
    case Key::D:
      return "D";
    case Key::E:
      return "E";
    case Key::F1:
      return "F1";
    case Key::F2:
      return "F2";
    case Key::SPACE:
      return "SPACE";
    case Key::LSHIFT:
      return "LSHIFT";
  }
}

Key string_to_key(const String& str, Error& out_error)
{
  if (str == "W")
  {
    return Key::W;
  }
  else if (str == "S")
  {
    return Key::S;
  }
  else if (str == "A")
  {
    return Key::A;
  }
  else if (str == "D")
  {
    return Key::D;
  }
  else if (str == "E")
  {
    return Key::E;
  }
  else if (str == "F1")
  {
    return Key::F1;
  }
  else if (str == "F2")
  {
    return Key::F2;
  }
  else if (str == "SPACE")
  {
    return Key::SPACE;
  }
  else if (str == "LSHIFT")
  {
    return Key::LSHIFT;
  }

  out_error = GLOBAL_ERROR_INVALID_DATA;
  return (Key) 0;
}
