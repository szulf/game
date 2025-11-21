#pragma once

#include "engine/renderer/scene.hpp"

namespace core {

// TODO(szulf): this should be more general, it shouldnt know what a scene is

namespace renderer {

void init();
void clearScreen();
void render(const Scene& scene);

}

}
