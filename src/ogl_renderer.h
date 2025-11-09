#ifndef OGL_RENDERER_H
#define OGL_RENDERER_H

class Texture
{
  Texture(const png::Image& img);

private:
  Texture() {}

  u32 id;

  friend Material;
  friend Renderer;
};

namespace obj
{

static void parse_mtl_file(std::istream& mtl_file);
struct Context;
static Mesh parse_object(obj::Context& ctx);

}

class Material
{
  Material(const png::Image& img) : texture{img} {}

private:
  Material() {}

  Texture texture;

  friend void obj::parse_mtl_file(std::istream& mtl_file);
  friend Mesh;
  friend Renderer;
};

class Mesh
{
  Mesh(std::vector<Vertex>&& vertices, std::vector<u32>&& indices, const Material& mat);

private:
  Mesh() {}

  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  Material material;
  u32 vao;

  friend Mesh obj::parse_object(obj::Context& ctx);
  friend Renderer;
};

#endif
