#include "base/base.h"
#include "os/os.h"

#include "game/game.h"

#define DT 0.05f

i32 main()
{
  os::init();
  auto window = os::Window::open("game", {1280, 720});
  window.init_rendering_api();

  game::Memory memory = {};
  memory.size = GB(2);
  memory.memory = os::alloc(memory.size);

  game::init(memory, window);

  f32 current_time = os::time_now();
  f32 accumulator = 0.0f;
  while (window.running)
  {
    f32 new_time = os::time_now();
    f32 frame_time = new_time - current_time;
    current_time = new_time;
    accumulator += frame_time;

    window.update();

    while (accumulator >= DT)
    {
      game::update_tick(memory, window, DT);
      accumulator -= DT;

      window.clear_input();
    }

    game::update_frame(memory, window, accumulator / DT);

    game::render(memory);
    window.swap_buffers();
  }

  return 0;
}
