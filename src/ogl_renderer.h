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
  Array<Vertex> vertices;
  Array<u32> indices;
  Material material;

  u32 vao;
};

#endif
