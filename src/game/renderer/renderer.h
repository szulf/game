#ifndef RENDERER_H
#define RENDERER_H

enum StaticModel
{
  STATIC_MODEL_BOUNDING_BOX = 1,
  STATIC_MODEL_RING,
};

void static_model_init(
  StaticModel static_model,
  ShaderHandle shader,
  const Array<Vertex>& vertices,
  const Array<u32>& indices,
  Allocator& allocator
);

enum Primitive
{
  PRIMITIVE_TRIANGLES,
  PRIMITIVE_LINE_STRIP,
};

struct DrawCall
{
  Mat4 model;
  Mat4 view;
  Mat4 projection;

  ModelHandle model_handle;

  bool wireframe;
  Primitive primitive;
};

void renderer_init(Allocator& allocator, Error& out_error);
void renderer_clear_screen();
void renderer_window_resize(u32 width, u32 height);
void renderer_queue_draw_call(const Array<DrawCall>& queue, const DrawCall& scene);
void renderer_draw(const Array<DrawCall>& queue);

#endif
