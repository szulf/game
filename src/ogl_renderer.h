#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

struct Texture
{
  u32 id;

  static Texture make(const png::Image& img);
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

  static Mesh make(const Array<Vertex>& vertices, const Array<u32>& indices, const Material& mat);

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw(Shader shader) const;
};

#endif
