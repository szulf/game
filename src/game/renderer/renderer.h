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
  Mat4 model;
  assets::MeshHandle mesh;
  assets::MaterialHandle material;
  // TODO(szulf): get rid of this later
  bool emissive;
};

struct Pass
{
  Array<Item> items;
  Mat4 view;
  Mat4 projection;
};

Pass pass_make(Allocator& allocator);

void init(Allocator& allocator, Error& out_error);
void clear_screen();
void window_resize(u32 width, u32 height);
void queue_items(Pass& pass, const Array<Item>& render_items);
void draw(const Pass& pass);

}

#endif
