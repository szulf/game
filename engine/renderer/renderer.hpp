#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <memory>

#include "math.hpp"
#include "renderer/scene.hpp"

namespace core
{

namespace renderer
{

void init();
void clearScreen();
void render(const Scene& scene);

}

}
