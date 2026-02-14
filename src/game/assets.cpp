#include "assets.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "parser.h"

template <>
void AssetType<MeshHandle, MeshData>::clear()
{
  m_data.erase(m_data.begin() + StaticModel_COUNT, m_data.end());
}

struct OBJContext
{
  MeshData out{};
  std::vector<vec3> positions;
  std::vector<vec3> normals;
  std::vector<vec2> uvs;
  std::unordered_map<Vertex, std::size_t> vertex_cache{};
};

static vec3 obj_parse_vec3(parser::Pos& pos)
{
  vec3 out{};
  out.x = parser::number_f32(pos);
  out.y = parser::number_f32(pos);
  out.z = parser::number_f32(pos);
  return out;
}

TextureHandle AssetManager::obj_get_texture_by_path(const std::filesystem::path& path)
{
  if (m_texture_handles.contains(path))
  {
    return m_texture_handles[path];
  }

  TextureData texture{};
  // TODO: create an error placeholder texture
  auto img = Image::from_file(path.c_str());
  texture.image = std::move(img);
  auto texture_handle = textures.set(std::move(texture));
  m_texture_handles.insert_or_assign(path.string(), texture_handle);
  return texture_handle;
}

void AssetManager::load_mtl_file(const std::filesystem::path& path)
{
  std::string mat_name{};
  Material mat{};
  bool parsing{};
  std::ifstream file{path};
  ASSERT(!file.fail(), "[MTL] File reading error. (path: {}).", path.string());
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty())
    {
      continue;
    }
    parser::Pos pos{.line = line};

    auto key = parser::word(pos);
    if (key == "newmtl")
    {
      if (parsing)
      {
        if (mat_name == "light_bulb")
        {
          mat.shader = ShaderHandle::DEFAULT;
        }
        else
        {
          mat.shader = ShaderHandle::LIGHTING;
        }
        auto material_handle = materials.set(std::move(mat));
        m_material_handles.insert_or_assign(mat_name, material_handle);
      }
      mat_name = parser::word(pos);
      if (m_material_handles.contains(mat_name))
      {
        parsing = false;
      }
      else
      {
        mat = {};
        parsing = true;
      }
    }
    else if (key == "Ka")
    {
      // NOTE: noop
    }
    else if (key == "map_Ka")
    {
      // NOTE: noop
    }
    else if (key == "Kd")
    {
      mat.diffuse_color = obj_parse_vec3(pos);
    }
    else if (key == "map_Kd")
    {
      auto filename = parser::word(pos);
      mat.diffuse_map = obj_get_texture_by_path(path.parent_path() / filename);
    }
    else if (key == "Ks")
    {
      mat.specular_color = obj_parse_vec3(pos);
    }
    else if (key == "map_Ks")
    {
      // NOTE: noop
    }
    else if (key == "Ns")
    {
      mat.specular_exponent = parser::number_f32(pos);
    }
    else if (key == "map_Ns")
    {
      // NOTE: noop
    }
    else if (key == "#")
    {
      continue;
    }
    else if (key == "Ke")
    {
      // NOTE: noop
    }
    else if (key == "Ni")
    {
      // NOTE: noop
    }
    else if (key == "d")
    {
      // NOTE: noop
    }
    else if (key == "illum")
    {
      // NOTE: noop
    }
    else
    {
      ASSERT(false, "Invalid key found. ({}).", key);
    }
  }
  if (parsing)
  {
    if (mat_name == "light_bulb")
    {
      mat.shader = ShaderHandle::DEFAULT;
    }
    else
    {
      mat.shader = ShaderHandle::LIGHTING;
    }
    auto material_handle = materials.set(std::move(mat));
    m_material_handles.insert_or_assign(mat_name, material_handle);
  }
}

static void obj_parse_vertex(OBJContext& ctx, parser::Pos& pos)
{
  auto position_idx = parser::number_u32(pos);
  parser::expect_and_skip(pos, '/');
  auto uv_idx = parser::number_u32(pos);
  parser::expect_and_skip(pos, '/');
  auto normal_idx = parser::number_u32(pos);

  Vertex v{
    .pos = ctx.positions[position_idx - 1],
    .normal = ctx.normals[normal_idx - 1],
    .uv = ctx.uvs[uv_idx - 1],
  };

  if (ctx.vertex_cache.contains(v))
  {
    ctx.out.indices.push_back((u32) ctx.vertex_cache[v]);
  }
  else
  {
    auto idx = ctx.out.vertices.size();
    ctx.vertex_cache.insert_or_assign(v, idx);
    ctx.out.indices.push_back((u32) idx);
    ctx.out.vertices.push_back(v);
  }
  ++ctx.out.submeshes[ctx.out.submeshes.size() - 1].index_count;
}

MeshHandle AssetManager::load_obj(const std::filesystem::path& path)
{
  OBJContext ctx{};
  std::ifstream file{path};
  ASSERT(!file.fail(), "File reading error. (path: {}).", path.string());
  std::string line{};
  while (std::getline(file, line))
  {
    if (line.empty() || line[0] == '#')
    {
      continue;
    }
    parser::Pos pos{.line = line};

    auto key = parser::word(pos);
    if (key == "mtllib")
    {
      auto mtl_filename = parser::word(pos);
      load_mtl_file(path.parent_path() / mtl_filename);
    }
    else if (key == "o")
    {
      ctx.out.submeshes.push_back({
        .index_offset = ctx.out.indices.size(),
      });
    }
    else if (key == "v")
    {
      auto position = obj_parse_vec3(pos);
      ctx.positions.push_back(position);
    }
    else if (key == "vn")
    {
      auto normal = obj_parse_vec3(pos);
      ctx.normals.push_back(normal);
    }
    else if (key == "vt")
    {
      vec2 uv{};
      uv.x = parser::number_f32(pos);
      uv.y = parser::number_f32(pos);
      ctx.uvs.push_back(uv);
    }
    else if (key == "s")
    {
      // NOTE: noop
    }
    else if (key == "usemtl")
    {
      auto name = parser::word(pos);
      ctx.out.submeshes[ctx.out.submeshes.size() - 1].material =
        m_material_handles[std::string{name}];
    }
    else if (key == "f")
    {
      obj_parse_vertex(ctx, pos);
      obj_parse_vertex(ctx, pos);
      obj_parse_vertex(ctx, pos);
    }
    else if (key == "#")
    {
      continue;
    }
    else
    {
      ASSERT(false, "Invalid key found. ({}).", key);
    }
  }
  return meshes.set(std::move(ctx.out));
}

void AssetManager::clear()
{
  textures.clear();
  materials.clear();
  meshes.clear();

  m_texture_handles.clear();
  m_material_handles.clear();
}
