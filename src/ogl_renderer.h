#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

struct Texture {
  u32 id;

  Texture() {}
  Texture(const image::Image& img);
};

struct Material {
  Texture texture;
};

struct Mesh {
  std::vector<Vertex> vertices{};
  std::vector<u32> indices{};
  Material material{};

  u32 vao{};

  Mesh() {}
  // TODO(szulf): these vector should probably get moved and not copied
  Mesh(const std::vector<Vertex>& vertices, const std::vector<u32>& indices,
       const Material& material);

  static Mesh from_obj(const char* path, Error* err);

  // TODO(szulf): probably switch to some sort of a queue later on
  void draw(Shader shader) const;
};

std::optional<image::Image> g_img{};

#endif
