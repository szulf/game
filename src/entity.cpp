#include "entity.h"

DrawCall draw_call_make(const Entity& entity, const Camera& camera)
{
  if (!entity.has_model)
  {
    return {};
  }
  DrawCall out = {};
  out.model_handle = entity.model;
  out.model = mat4_make();
  // TODO(szulf): order of operations matter! is this the correct one?
  mat4_scale(out.model, entity.scale);
  mat4_translate(out.model, entity.position);
  out.view = camera_look_at(camera);
  out.projection = camera_projection(camera);
  return out;
}
