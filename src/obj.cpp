#include "obj.h"

namespace obj
{

struct Context
{
  std::ifstream file;
  std::unordered_map<Vertex, u32> vertex_map;
  std::vector<math::vec3> positions;
  std::vector<math::vec3> normals;
  std::vector<math::vec2> uvs;
  usize mesh_count;
  usize pos_count;
  usize normal_count;
  usize uv_count;
};

static void
parse_mtl_file(std::istream& mtl_file)
{
  std::string line;
  bool parsing{};
  std::string material_name{};
  while (std::getline(mtl_file, line))
  {
    switch (line[0])
    {
      case 'n':
      {
        auto parts = line | std::views::split(' ');
        auto it = parts.begin();
        if (it == parts.end())
        {
          throw std::runtime_error{"invalid mtl file"};
        }
        std::string_view header{std::begin(*it), std::end(*it)};
        if (header != "newmtl")
        {
          continue;
        }
        ++it;
        std::string data{std::begin(*it), std::end(*it)};
        if (!assets::material_exists(data))
        {
          parsing = true;
          material_name = std::move(data);
        }
      }
      break;

      case 'm':
      {
        if (!parsing)
        {
          continue;
        }
        auto parts = line | std::views::split(' ');
        auto it = parts.begin();
        if (it == parts.end())
        {
          throw std::runtime_error{"invalid map_Kd"};
        }
        std::string_view header{std::begin(*it), std::end(*it)};
        if (header != "map_Kd")
        {
          continue;
        }
        ++it;
        std::string data{std::begin(*it), std::end(*it)};
        std::filesystem::path texture_file_path{"assets"};
        texture_file_path /= data;
        Material mat{};
        if (!assets::texture_exists(texture_file_path))
        {
          auto img = png::Image::decode(texture_file_path);
          mat = Material{img};
          assets::texture_set(texture_file_path, mat.texture);
        }
        else
        {
          Texture* texture = assets::texture_get(texture_file_path);
          if (!texture)
          {
            throw std::runtime_error{"texture not found"};
          }
          mat.texture = *texture;
        }
        assets::material_set(material_name, mat);
      }
      break;
    }
  }
}

template <usize N>
static void
parse_vertex(std::array<f32, N>& points, const std::string& line)
{
  usize idx{};
  for (const auto& split : line | std::views::split(' ') | std::views::drop(1))
  {
    std::string_view s{split};
    f32 val;
    auto err = std::from_chars(s.begin(), s.end(), val).ec;
    if (err != std::errc{})
    {
      throw std::runtime_error{"[OBJ] couldnt parse string into index"};
    }
    points[idx] = val;
    ++idx;
  }
}

static Mesh
parse_object(Context& ctx)
{
  Mesh mesh{};
  usize indices_amount{};
  bool parsing{true};
  std::string line;
  auto pos = ctx.file.tellg();
  while (parsing && std::getline(ctx.file, line))
  {
    switch (line[0])
    {
      case 'f':
      {
        ++indices_amount;
      }
      break;
      case 'o':
      case 'g':
      {
        parsing = false;
      }
      break;
    }
  }
  ctx.file.clear();
  ctx.file.seekg(pos);
  indices_amount *= 3;
  mesh.indices.reserve(indices_amount);

  parsing = true;
  std::ifstream::pos_type back_pos{};
  while (parsing && std::getline(ctx.file, line))
  {
    auto pos = ctx.file.tellg();
    switch (line[0])
    {
      case 'f':
      {
        for (const auto& split : line | std::views::split(' ') | std::views::drop(1))
        {
          std::array<u32, 3> indices;
          usize idx = 0;
          for (const auto& part : split | std::views::split('/'))
          {
            std::string_view p{part};
            u32 vertex_idx;
            auto err = std::from_chars(p.begin(), p.end(), vertex_idx).ec;
            if (err != std::errc{})
            {
              throw std::runtime_error{"[OBJ] invalid data"};
            }
            indices[idx] = vertex_idx - 1;
            ++idx;
          }
          Vertex vertex;
          vertex.pos = ctx.positions[indices[0]];
          vertex.uv = ctx.uvs[indices[1]];
          vertex.normal = ctx.normals[indices[2]];
          if (ctx.vertex_map.contains(vertex))
          {
            mesh.indices.push_back(ctx.vertex_map[vertex]);
          }
          else
          {
            ctx.vertex_map[vertex] = static_cast<u32>(mesh.vertices.size());
            mesh.indices.push_back(static_cast<u32>(mesh.vertices.size()));
            mesh.vertices.push_back(vertex);
          }
        }
      }
      break;

      case 'v':
      {
        switch (line[1])
        {
          case ' ':
          {
            std::array<f32, 3> points{};
            parse_vertex(points, line);
            ctx.positions.push_back({points[0], points[1], points[2]});
          }
          break;

          case 'n':
          {
            std::array<f32, 3> points;
            parse_vertex(points, line);
            ctx.normals.push_back({points[0], points[1], points[2]});
          }
          break;

          case 't':
          {
            std::array<f32, 2> points;
            parse_vertex(points, line);
            ctx.uvs.push_back({points[0], points[1]});
          }
          break;
        }
      }
      break;

      case 'u':
      {
        auto parts = line | std::views::split(' ');
        auto it = parts.begin();
        if (it == parts.end())
        {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        std::string_view header{std::begin(*it), std::end(*it)};
        if (header != "usemtl")
        {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        ++it;
        if (it == parts.end())
        {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        std::string data{std::begin(*it), std::end(*it)};
        Material* mat = assets::material_get(data);
        if (mat == nullptr)
        {
          throw std::runtime_error{"[OBJ] not found"};
        }
        mesh.material = *mat;
      }
      break;

      case 'g':
      case 'o':
      {
        parsing = false;
        ctx.file.clear();
        ctx.file.seekg(back_pos);
      }
      break;
    }
    back_pos = pos;
  }

  return Mesh{std::move(mesh.vertices), std::move(mesh.indices), mesh.material};
}

static Model
parse(const std::filesystem::path& path)
{
  Model model{math::mat4{1.0f}};
  Context ctx = {};
  ctx.file = std::ifstream{path};
  std::string line;

  while (std::getline(ctx.file, line))
  {
    if (line.size() < 4)
    {
      continue;
    }
    switch (line[0])
    {
      case 'g':
      case 'o':
      {
        ++ctx.mesh_count;
      }
      break;
      case 'v':
      {
        switch (line[1])
        {
          case ' ':
          {
            ++ctx.pos_count;
          }
          break;
          case 'n':
          {
            ++ctx.normal_count;
          }
          break;
          case 't':
          {
            ++ctx.uv_count;
          }
          break;
        }
      }
    }
  }

  model.meshes.reserve(ctx.mesh_count);
  ctx.positions.reserve(ctx.pos_count);
  ctx.normals.reserve(ctx.normal_count);
  ctx.uvs.reserve(ctx.uv_count);

  ctx.file.clear();
  ctx.file.seekg(0);
  while (std::getline(ctx.file, line))
  {
    switch (line[0])
    {
      case 'o':
      case 'g':
      {
        Mesh mesh = parse_object(ctx);
        model.meshes.push_back(mesh);
      }
      break;

      case 'm':
      {
        auto parts = line | std::views::split(' ');
        auto it = parts.begin();
        if (it == parts.end())
        {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        std::string_view header{std::begin(*it), std::end(*it)};
        if (header != "mtllib")
        {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        ++it;
        if (it == parts.end())
        {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        std::filesystem::path mtl_file_path{"assets/"};
        mtl_file_path /= std::string_view{std::begin(*it), std::end(*it)};
        std::ifstream mtl_file{mtl_file_path};
        parse_mtl_file(mtl_file);
      }
      break;
    }
  }

  return model;
}

}
