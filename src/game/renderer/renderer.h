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

struct Light
{
  vec3 pos;
  vec3 color;
};

struct Pass
{
  Array<Item> items;
  // TODO(szulf): make this an array in the future
  Light light;
  mat4 view;
  mat4 projection;
  vec3 view_pos;
};

Pass pass_make(Allocator& allocator);

void init(Allocator& allocator, Error& out_error);
void clear_screen();
void window_resize(u32 width, u32 height);
void queue_items(Pass& pass, const Array<Item>& render_items);
void draw(const Pass& pass);

}

#endif
