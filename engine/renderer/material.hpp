#pragma once

#include "badtl/string.hpp"

namespace core {

struct Material {
  // NOTE(szulf): would prefer if this was an uuid, but i get it from the path to the image file in the obj parser so
  // not much i can do here ig
  btl::String texture_name;
};

}
