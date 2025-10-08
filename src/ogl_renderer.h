#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

struct Texture
{
  u32 id;

  static Texture make(Image* img);
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

  static Mesh make(Array<Vertex>* vertices, Array<u32>* indices, Material* material);
  static Mesh from_obj(const char* path, Arena* temp_arena, Arena* perm_arena, Error* err);

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw(Shader shader) const;
};

#endif
