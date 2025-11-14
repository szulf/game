#pragma once

#include "renderer/scene.hpp"

namespace core {

// TODO(szulf): this should be more general, it shouldnt know what a scene is

namespace renderer {

void init() noexcept;
void clearScreen() noexcept;
void render(const Scene& scene) noexcept;

}

}
