#include "renderer.h"

void renderer_queue_draw_call(Array<DrawCall>& queue, const DrawCall& draw_call)
{
  // TODO(szulf): for now just pushing into the array, later actually try to sort it in place
  array_push(queue, draw_call);
}

#ifdef RENDERER_OPENGL
#  include "gl_renderer.cpp"
#else
#  error Unknown rendering backend.
#endif
