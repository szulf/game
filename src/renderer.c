#include "renderer.h"

inline static void
setup_global_materials(Arena* perm_arena, Error* err)
{
  // TODO(szulf): can i somehow check if i ever exceed that?
  Error error = ERROR_SUCCESS;
  ARRAY_INIT(&g_materials, perm_arena, 100, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
}
