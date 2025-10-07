#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

struct Texture
{
  u32 id;
};

struct Material
{
  Texture texture;
};

struct Mesh
{
  VertexArray vertices;
  U32Array indices;
  Material material;

  u32 vao;
};

#endif
