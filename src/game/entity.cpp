#include "entity.h"

BoundingBox bounding_box_from_model(ModelHandle handle)
{
  Vec3 max = {F32_MIN, 0, F32_MIN};
  Vec3 min = {F32_MAX, 0, F32_MAX};
  auto& model = assets_get_model(handle);
  for (usize mesh_idx = 0; mesh_idx < model.meshes.size; ++mesh_idx)
  {
    auto& mesh = assets_get_mesh(model.meshes[mesh_idx]);
    for (usize vertex_idx = 0; vertex_idx < mesh.vertices.size; ++vertex_idx)
    {
      auto& vertex = mesh.vertices[vertex_idx];
      max.x = f32_max(max.x, vertex.position.x);
      min.x = f32_min(min.x, vertex.position.x);
      max.z = f32_max(max.z, vertex.position.z);
      min.z = f32_min(min.z, vertex.position.z);
    }
  }
  return {max.x - min.x, max.z - min.z};
}

bool collides(const Entity& ea, const Entity& eb)
{
  auto& ax = ea.position.x;
  auto& az = ea.position.z;
  auto& bx = eb.position.x;
  auto& bz = eb.position.z;

  return ax - (ea.bounding_box.width / 2.0f) < bx + (eb.bounding_box.width / 2.0f) &&
         ax + (ea.bounding_box.width / 2.0f) > bx - (eb.bounding_box.width / 2.0f) &&
         az - (ea.bounding_box.depth / 2.0f) < bz + (eb.bounding_box.depth / 2.0f) &&
         az + (ea.bounding_box.depth / 2.0f) > bz - (eb.bounding_box.depth / 2.0f);
}

DrawCall draw_call_entity(const Entity& entity, const Camera& camera)
{
  if (!entity.has_model)
  {
    return {};
  }
  DrawCall out = {};
  out.primitive = PRIMITIVE_TRIANGLES;
  out.model_handle = entity.model;
  out.model = mat4_make();
  mat4_scale(out.model, entity.scale);
  mat4_translate(out.model, entity.position);
  out.view = camera_look_at(camera);
  out.projection = camera_projection(camera);
  return out;
}

static Vertex bounding_box_vertices[] = {
  {{-0.500000, 0.500000, 0.500000},   {-1.000000, -0.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-1.000000, -0.000000, -0.000000}, {1.000000, 0.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-1.000000, -0.000000, -0.000000}, {0.000000, 0.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-0.000000, -0.000000, -1.000000}, {0.000000, 1.000000}},
  {{0.500000, -0.500000, -0.500000},  {-0.000000, -0.000000, -1.000000}, {1.000000, 0.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-0.000000, -0.000000, -1.000000}, {0.000000, 0.000000}},
  {{0.500000, 0.500000, -0.500000},   {1.000000, -0.000000, -0.000000},  {1.000000, 1.000000}},
  {{0.500000, -0.500000, 0.500000},   {1.000000, -0.000000, -0.000000},  {0.000000, 0.000000}},
  {{0.500000, -0.500000, -0.500000},  {1.000000, -0.000000, -0.000000},  {1.000000, 0.000000}},
  {{0.500000, 0.500000, 0.500000},    {-0.000000, -0.000000, 1.000000},  {1.000000, 1.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-0.000000, -0.000000, 1.000000},  {0.000000, 0.000000}},
  {{0.500000, -0.500000, 0.500000},   {-0.000000, -0.000000, 1.000000},  {1.000000, 0.000000}},
  {{0.500000, -0.500000, -0.500000},  {-0.000000, -1.000000, -0.000000}, {1.000000, 1.000000}},
  {{-0.500000, -0.500000, 0.500000},  {-0.000000, -1.000000, -0.000000}, {0.000000, 0.000000}},
  {{-0.500000, -0.500000, -0.500000}, {-0.000000, -1.000000, -0.000000}, {0.000000, 1.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-0.000000, 1.000000, -0.000000},  {0.000000, 1.000000}},
  {{0.500000, 0.500000, 0.500000},    {-0.000000, 1.000000, -0.000000},  {1.000000, 0.000000}},
  {{0.500000, 0.500000, -0.500000},   {-0.000000, 1.000000, -0.000000},  {1.000000, 1.000000}},
  {{-0.500000, 0.500000, -0.500000},  {-1.000000, -0.000000, -0.000000}, {1.000000, 1.000000}},
  {{0.500000, 0.500000, -0.500000},   {-0.000000, -0.000000, -1.000000}, {1.000000, 1.000000}},
  {{0.500000, 0.500000, 0.500000},    {1.000000, -0.000000, -0.000000},  {0.000000, 1.000000}},
  {{-0.500000, 0.500000, 0.500000},   {-0.000000, -0.000000, 1.000000},  {0.000000, 1.000000}},
  {{0.500000, -0.500000, 0.500000},   {-0.000000, -1.000000, -0.000000}, {1.000000, 0.000000}},
  {{-0.500000, 0.500000, 0.500000},   {-0.000000, 1.000000, -0.000000},  {0.000000, 0.000000}},
};

static u32 bounding_box_indices[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                                     12, 13, 14, 15, 16, 17, 0,  18, 1,  3,  19, 4,
                                     6,  20, 7,  9,  21, 10, 12, 22, 13, 15, 23, 16};

// TODO(szulf): i dont like this, wasting memory on normals and uvs
static Vertex ring_vertices[] = {
  {{0.000000f, 0.000000f, -0.500000f},  {}, {}},
  {{-0.097545f, 0.000000f, -0.490393f}, {}, {}},
  {{-0.191342f, 0.000000f, -0.461940f}, {}, {}},
  {{-0.277785f, 0.000000f, -0.415735f}, {}, {}},
  {{-0.353553f, 0.000000f, -0.353553f}, {}, {}},
  {{-0.415735f, 0.000000f, -0.277785f}, {}, {}},
  {{-0.461940f, 0.000000f, -0.191342f}, {}, {}},
  {{-0.490393f, 0.000000f, -0.097545f}, {}, {}},
  {{-0.500000f, 0.000000f, 0.000000f},  {}, {}},
  {{-0.490393f, 0.000000f, 0.097545f},  {}, {}},
  {{-0.461940f, 0.000000f, 0.191342f},  {}, {}},
  {{-0.415735f, 0.000000f, 0.277785f},  {}, {}},
  {{-0.353553f, 0.000000f, 0.353553f},  {}, {}},
  {{-0.277785f, 0.000000f, 0.415735f},  {}, {}},
  {{-0.191342f, 0.000000f, 0.461940f},  {}, {}},
  {{-0.097545f, 0.000000f, 0.490393f},  {}, {}},
  {{0.000000f, 0.000000f, 0.500000f},   {}, {}},
  {{0.097545f, 0.000000f, 0.490393f},   {}, {}},
  {{0.191342f, 0.000000f, 0.461940f},   {}, {}},
  {{0.277785f, 0.000000f, 0.415735f},   {}, {}},
  {{0.353553f, 0.000000f, 0.353553f},   {}, {}},
  {{0.415735f, 0.000000f, 0.277785f},   {}, {}},
  {{0.461940f, 0.000000f, 0.191342f},   {}, {}},
  {{0.490393f, 0.000000f, 0.097545f},   {}, {}},
  {{0.500000f, 0.000000f, 0.000000f},   {}, {}},
  {{0.490393f, 0.000000f, -0.097545f},  {}, {}},
  {{0.461940f, 0.000000f, -0.191342f},  {}, {}},
  {{0.415735f, 0.000000f, -0.277785f},  {}, {}},
  {{0.353553f, 0.000000f, -0.353553f},  {}, {}},
  {{0.277785f, 0.000000f, -0.415735f},  {}, {}},
  {{0.191342f, 0.000000f, -0.461940f},  {}, {}},
  {{0.097545f, 0.000000f, -0.490393f},  {}, {}},
};

static u32 ring_indices[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                             17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0};

DrawCall draw_call_entity_bounding_box(const Entity& entity, const Camera& camera)
{
  DrawCall out = {};
  out.primitive = PRIMITIVE_TRIANGLES;
  out.wireframe = true;
  out.model_handle = STATIC_MODEL_BOUNDING_BOX;
  out.model = mat4_make();
  mat4_scale(out.model, {entity.bounding_box.width, 1.0f, entity.bounding_box.depth});
  mat4_translate(out.model, entity.position);
  out.view = camera_look_at(camera);
  out.projection = camera_projection(camera);
  return out;
}

DrawCall draw_call_entity_interactable_radius(const Entity& entity, const Camera& camera)
{
  DrawCall out = {};
  out.primitive = PRIMITIVE_LINE_STRIP;
  out.model_handle = STATIC_MODEL_RING;
  out.model = mat4_make();
  mat4_scale(
    out.model,
    {interactable_info[entity.interactable_type].radius2,
     1.0f,
     interactable_info[entity.interactable_type].radius2}
  );
  mat4_translate(out.model, entity.position);
  out.view = camera_look_at(camera);
  out.projection = camera_projection(camera);
  return out;
}
