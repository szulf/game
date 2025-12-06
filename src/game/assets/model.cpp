#include "model.h"

void static_model_init(
  StaticModel static_model,
  ShaderHandle shader,
  const Array<Vertex>& vertices,
  const Array<u32>& indices,
  Allocator& allocator
)
{
  Material material = {};
  material.shader = shader;
  auto material_handle = assets_set_material(material);
  Mesh mesh = mesh_make(vertices, indices, material_handle);
  auto mesh_handle = assets_set_mesh(mesh);
  Model model = {};
  model.matrix = mat4_make();
  model.meshes = array_make<MeshHandle>(ARRAY_TYPE_STATIC, 1, allocator);
  array_push(model.meshes, mesh_handle);
  auto handle = assets_set_model(model);
  ASSERT(handle == static_model, "failed to initalize a static model");
}
