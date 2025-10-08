#include "renderer.h"

inline static void
setup_global_materials(Arena* perm_arena, Error* err)
{
  Error error = Error::Success;
  g_materials = Array<Material>::make(100, perm_arena, &error);
  ERROR_ASSERT(error == Error::Success, *err, error,);
}
