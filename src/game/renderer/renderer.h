#ifndef RENDERER_H
#define RENDERER_H

namespace renderer
{

enum StaticModel
{
  STATIC_MODEL_BOUNDING_BOX = 1,
  STATIC_MODEL_RING,
};

void static_model_init(
  StaticModel static_model,
  assets::ShaderHandle shader,
  const Array<Vertex>& vertices,
  const Array<u32>& indices,
  assets::Primitive primitive,
  bool wireframe,
  Allocator& allocator
);

struct Item
{
  mat4 model;
  assets::MeshHandle mesh;
  assets::MaterialHandle material;
};

#define MAX_LIGHTS 32
struct Light
{
  vec3 pos;
  vec3 color;
};

struct Pass
{
  Array<Item> items;
  Array<Light> lights;

  Camera camera;

  u32 transforms_count;
  mat4* transforms;

  bool override_shader;
  assets::ShaderHandle shader;

  u32 framebuffer_id;

  assets::Texture* shadow_map;
  float shadow_map_camera_far_plane;

  u32 width;
  u32 height;
};

Pass pass_make(Allocator& allocator);

void init(Allocator& allocator, Error& out_error);
void queue_items(Pass& pass, const Array<Item>& render_items);
void draw(const Pass& pass);

}

#endif
