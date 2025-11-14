#pragma once

#include <string>

namespace core {

struct Material final {
  // NOTE(szulf): would prefer if this was an uuid, but i get it from the path to the image file in the obj parser so
  // not much i can do here ig
  std::string texture_name;
};

}
