#include "renderer.h"

inline static void
setup_global_materials(Arena* perm_arena, Error* err)
{
  // TODO(szulf): probably instead of this i will want to actually store materials on the scene
  // arena(when i will get to that) and just precalculate how many materials i
  // have to allocate memory for
  Error error = ERROR_SUCCESS;
  ARRAY_INIT(&g_materials, 100, perm_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
}
