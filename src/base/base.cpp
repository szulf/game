#include "base.h"

const char* key_to_cstr(Key key)
{
  switch (key)
  {
    case KEY_W:
      return "W";
    case KEY_S:
      return "S";
    case KEY_A:
      return "A";
    case KEY_D:
      return "D";
    case KEY_E:
      return "E";
    case KEY_F1:
      return "F1";
    case KEY_F2:
      return "F2";
    case KEY_SPACE:
      return "SPACE";
    case KEY_LSHIFT:
      return "LSHIFT";
  }
}

Key string_to_key(const String& str, Error& out_error)
{
  if (str == "W")
  {
    return KEY_W;
  }
  else if (str == "S")
  {
    return KEY_S;
  }
  else if (str == "A")
  {
    return KEY_A;
  }
  else if (str == "D")
  {
    return KEY_D;
  }
  else if (str == "E")
  {
    return KEY_E;
  }
  else if (str == "F1")
  {
    return KEY_F1;
  }
  else if (str == "F2")
  {
    return KEY_F2;
  }
  else if (str == "SPACE")
  {
    return KEY_SPACE;
  }
  else if (str == "LSHIFT")
  {
    return KEY_LSHIFT;
  }

  out_error = GLOBAL_ERROR_INVALID_DATA;
  return (Key) 0;
}

#include "math.cpp"
#include "memory.cpp"
#include "array.cpp"
#include "map.cpp"
#include "string.cpp"
#include "vertex.cpp"
