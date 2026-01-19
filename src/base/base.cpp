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
    case Key::A:
      return "A";
    case Key::B:
      return "B";
    case Key::C:
      return "C";
    case Key::D:
      return "D";
    case Key::E:
      return "E";
    case Key::F:
      return "F";
    case Key::G:
      return "G";
    case Key::H:
      return "H";
    case Key::I:
      return "I";
    case Key::J:
      return "J";
    case Key::K:
      return "K";
    case Key::L:
      return "L";
    case Key::M:
      return "M";
    case Key::N:
      return "N";
    case Key::O:
      return "O";
    case Key::P:
      return "P";
    case Key::Q:
      return "Q";
    case Key::R:
      return "R";
    case Key::S:
      return "S";
    case Key::T:
      return "T";
    case Key::U:
      return "U";
    case Key::V:
      return "V";
    case Key::W:
      return "W";
    case Key::X:
      return "X";
    case Key::Y:
      return "Y";
    case Key::Z:
      return "Z";
    case Key::F1:
      return "F1";
    case Key::F2:
      return "F2";
    case Key::F3:
      return "F3";
    case Key::F4:
      return "F4";
    case Key::F5:
      return "F5";
    case Key::F6:
      return "F6";
    case Key::F7:
      return "F7";
    case Key::F8:
      return "F8";
    case Key::F9:
      return "F9";
    case Key::F10:
      return "F10";
    case Key::F11:
      return "F11";
    case Key::F12:
      return "F12";
    case Key::SPACE:
      return "SPACE";
    case Key::LSHIFT:
      return "LSHIFT";
  }
}

Key string_to_key(const String& str, Error& out_error)
{
  if (str == "A")
  {
    return Key::A;
  }
  else if (str == "B")
  {
    return Key::B;
  }
  else if (str == "C")
  {
    return Key::C;
  }
  else if (str == "D")
  {
    return Key::D;
  }
  else if (str == "E")
  {
    return Key::E;
  }
  else if (str == "F")
  {
    return Key::F;
  }
  else if (str == "G")
  {
    return Key::G;
  }
  else if (str == "H")
  {
    return Key::H;
  }
  else if (str == "I")
  {
    return Key::I;
  }
  else if (str == "J")
  {
    return Key::J;
  }
  else if (str == "K")
  {
    return Key::K;
  }
  else if (str == "L")
  {
    return Key::L;
  }
  else if (str == "M")
  {
    return Key::M;
  }
  else if (str == "N")
  {
    return Key::N;
  }
  else if (str == "O")
  {
    return Key::O;
  }
  else if (str == "P")
  {
    return Key::P;
  }
  else if (str == "Q")
  {
    return Key::Q;
  }
  else if (str == "R")
  {
    return Key::R;
  }
  else if (str == "S")
  {
    return Key::S;
  }
  else if (str == "T")
  {
    return Key::T;
  }
  else if (str == "U")
  {
    return Key::U;
  }
  else if (str == "V")
  {
    return Key::V;
  }
  else if (str == "W")
  {
    return Key::W;
  }
  else if (str == "X")
  {
    return Key::X;
  }
  else if (str == "Y")
  {
    return Key::Y;
  }
  else if (str == "Z")
  {
    return Key::Z;
  }
  else if (str == "F1")
  {
    return Key::F1;
  }
  else if (str == "F2")
  {
    return Key::F2;
  }
  else if (str == "F3")
  {
    return Key::F3;
  }
  else if (str == "F4")
  {
    return Key::F4;
  }
  else if (str == "F5")
  {
    return Key::F5;
  }
  else if (str == "F6")
  {
    return Key::F6;
  }
  else if (str == "F7")
  {
    return Key::F7;
  }
  else if (str == "F8")
  {
    return Key::F8;
  }
  else if (str == "F9")
  {
    return Key::F9;
  }
  else if (str == "F10")
  {
    return Key::F10;
  }
  else if (str == "F11")
  {
    return Key::F11;
  }
  else if (str == "F12")
  {
    return Key::F12;
  }
  else if (str == "SPACE")
  {
    return Key::SPACE;
  }
  else if (str == "LSHIFT")
  {
    return Key::LSHIFT;
  }

  out_error = "Invalid key string.";
  return (Key) 0;
}
