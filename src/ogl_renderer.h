#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

struct Texture
{
  u32 id;

  Texture() {}
  Texture(const image::Image& img);
};

struct Material
{
  Texture texture;
};

static std::pmr::vector<Material> g_materials{};

struct Mesh
{
  std::pmr::vector<Vertex> vertices{};
  std::pmr::vector<u32> indices{};
  Material material{};

  u32 vao{};

  Mesh() {}
  // TODO(szulf): these vector should probably get moved and not copied
  Mesh(const std::pmr::vector<Vertex>& vertices, const std::pmr::vector<u32>& indices,
       const Material& material);

  static Mesh from_obj(const char* path, Error* err);

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw(Shader shader) const;
};

#endif
