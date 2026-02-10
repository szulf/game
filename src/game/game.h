#ifndef GAME_H
#define GAME_H

#include "base/base.h"
#include "os/os.h"

namespace game
{

struct Memory
{
  void* memory;
  usize size;
};

void init(Memory& memory, os::Window& window);
void update_tick(Memory& memory, os::Window& window, f32 dt);
void update_frame(Memory& memory, os::Window& input, f32 alpha);
void render(Memory& memory);

}

#endif
