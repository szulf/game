#ifndef RENDERER_H
#define RENDERER_H

namespace renderer
{

// TODO(szulf): maybe textures, materials, meshes, models, shaders should be a part of renderer
// and assets would depend on renderer
// it would clean up the mess in mesh_make that vertex attrib pointer for instancing matrix buffer
// created

enum StaticModel
{
  STATIC_MODEL_BOUNDING_BOX = 1,
  STATIC_MODEL_RING,
  STATIC_MODEL_LINE,
};

void static_model_init(
  StaticModel static_model,
  assets::ShaderHandle shader,
  const Array<Vertex>& vertices,
  const Array<u32>& indices,
  assets::Primitive primitive,
  bool wireframe,
  vec3 color,
  Allocator& allocator
);

struct Item
{
  mat4 model;
  vec3 tint;
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
void queue_items(Pass& pass, const Item& render_item);
void queue_items(Pass& pass, const Array<Item>& render_items);
void sort_items(Pass& pass);
void draw(const Pass& pass);

}

#endif
