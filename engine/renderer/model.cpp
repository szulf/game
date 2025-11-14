#include "renderer/model.hpp"

#include <fstream>
#include <unordered_map>
#include <ranges>

#include "image.hpp"
#include "renderer/vertex.hpp"
#include "asset_manager.hpp"

namespace core {

namespace obj_impl {

struct Context {
  std::ifstream file;
  std::unordered_map<Vertex, std::uint32_t> vertex_map;
  std::vector<math::vec3> positions;
  std::vector<math::vec3> normals;
  std::vector<math::vec2> uvs;
  std::size_t mesh_count;
  std::size_t pos_count;
  std::size_t normal_count;
  std::size_t uv_count;
};

static void parse_mtl_file(std::istream& mtl_file) {
  std::string line;
  bool parsing{};
  std::string material_name{};
  AssetManager& assets = AssetManager::instance();
  while (std::getline(mtl_file, line)) {
    switch (line[0]) {
      case 'n': {
        auto parts = line | std::views::split(' ');
        auto it = parts.begin();
        if (it == parts.end()) {
          throw std::runtime_error{"invalid mtl file"};
        }
        std::string_view header{std::begin(*it), std::end(*it)};
        if (header != "newmtl") {
          continue;
        }
        ++it;
        std::string data{std::begin(*it), std::end(*it)};
        if (!assets.materials.contains(data)) {
          parsing = true;
          material_name = std::move(data);
        }
      } break;

      case 'm': {
        if (!parsing) {
          continue;
        }
        auto parts = line | std::views::split(' ');
        auto it = parts.begin();
        if (it == parts.end()) {
          throw std::runtime_error{"invalid map_Kd"};
        }
        std::string_view header{std::begin(*it), std::end(*it)};
        if (header != "map_Kd") {
          continue;
        }
        ++it;
        std::string data{std::begin(*it), std::end(*it)};
        std::filesystem::path texture_file_path{"assets"};
        texture_file_path /= data;
        Material mat{};
        if (!assets.textures.contains(texture_file_path)) {
          auto img = Image{texture_file_path};
          Texture texture{img};
          assets.textures[texture_file_path] = std::move(texture);
        }
        mat.texture_name = texture_file_path;
        assets.materials[material_name] = std::move(mat);
      } break;
    }
  }
}

template <std::size_t N>
static void parse_vertex(std::array<float, N>& points, const std::string& line) {
  std::size_t idx{};
  for (const auto split : line | std::views::split(' ') | std::views::drop(1)) {
    std::string_view s{split};
    float val;
    auto err = std::from_chars(s.begin(), s.end(), val).ec;
    if (err != std::errc{}) {
      throw std::runtime_error{"[OBJ] couldnt parse string into index"};
    }
    points[idx] = val;
    ++idx;
  }
}

static Mesh parse_object(Context& ctx) {
  std::size_t indices_amount{};
  bool parsing{true};
  std::string line;
  auto pos = ctx.file.tellg();
  while (parsing && std::getline(ctx.file, line)) {
    switch (line[0]) {
      case 'f': {
        ++indices_amount;
      } break;
      case 'o':
      case 'g': {
        parsing = false;
      } break;
    }
  }
  ctx.file.clear();
  ctx.file.seekg(pos);
  indices_amount *= 3;
  std::vector<std::uint32_t> mesh_indices{};
  mesh_indices.reserve(indices_amount);
  std::vector<Vertex> mesh_vertices{};
  std::string mesh_material_name{};

  parsing = true;
  std::ifstream::pos_type back_pos{};
  while (parsing && std::getline(ctx.file, line)) {
    pos = ctx.file.tellg();
    switch (line[0]) {
      case 'f': {
        for (const auto split : line | std::views::split(' ') | std::views::drop(1)) {
          std::array<std::uint32_t, 3> indices;
          std::size_t idx = 0;
          for (const auto part : split | std::views::split('/')) {
            std::string_view p{part};
            std::uint32_t vertex_idx;
            auto err = std::from_chars(p.begin(), p.end(), vertex_idx).ec;
            if (err != std::errc{}) {
              throw std::runtime_error{"[OBJ] invalid data"};
            }
            indices[idx] = vertex_idx - 1;
            ++idx;
          }
          Vertex vertex;
          vertex.position = ctx.positions[indices[0]];
          vertex.uv = ctx.uvs[indices[1]];
          vertex.normal = ctx.normals[indices[2]];
          if (ctx.vertex_map.contains(vertex)) {
            mesh_indices.push_back(ctx.vertex_map[vertex]);
          } else {
            ctx.vertex_map[vertex] = static_cast<std::uint32_t>(mesh_vertices.size());
            mesh_indices.push_back(static_cast<std::uint32_t>(mesh_vertices.size()));
            mesh_vertices.push_back(vertex);
          }
        }
      } break;

      case 'v': {
        switch (line[1]) {
          case ' ': {
            std::array<float, 3> points{};
            parse_vertex(points, line);
            ctx.positions.push_back({points[0], points[1], points[2]});
          } break;

          case 'n': {
            std::array<float, 3> points;
            parse_vertex(points, line);
            ctx.normals.push_back({points[0], points[1], points[2]});
          } break;

          case 't': {
            std::array<float, 2> points;
            parse_vertex(points, line);
            ctx.uvs.push_back({points[0], points[1]});
          } break;
        }
      } break;

      case 'u': {
        auto parts = line | std::views::split(' ');
        auto it = parts.begin();
        if (it == parts.end()) {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        std::string_view header{std::begin(*it), std::end(*it)};
        if (header != "usemtl") {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        ++it;
        if (it == parts.end()) {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        mesh_material_name = {std::begin(*it), std::end(*it)};
      } break;

      case 'g':
      case 'o': {
        parsing = false;
        ctx.file.clear();
        ctx.file.seekg(back_pos);
      } break;
    }
    back_pos = pos;
  }

  return Mesh{std::move(mesh_vertices), std::move(mesh_indices), std::move(mesh_material_name)};
}

}

Model::Model(const std::filesystem::path& path) {
  obj_impl::Context ctx = {};
  ctx.file = std::ifstream{path};
  std::string line;

  while (std::getline(ctx.file, line)) {
    if (line.size() < 4) {
      continue;
    }
    switch (line[0]) {
      case 'g':
      case 'o': {
        ++ctx.mesh_count;
      } break;
      case 'v': {
        switch (line[1]) {
          case ' ': {
            ++ctx.pos_count;
          } break;
          case 'n': {
            ++ctx.normal_count;
          } break;
          case 't': {
            ++ctx.uv_count;
          } break;
        }
      }
    }
  }

  meshes.reserve(ctx.mesh_count);
  ctx.positions.reserve(ctx.pos_count);
  ctx.normals.reserve(ctx.normal_count);
  ctx.uvs.reserve(ctx.uv_count);

  ctx.file.clear();
  ctx.file.seekg(0);
  while (std::getline(ctx.file, line)) {
    switch (line[0]) {
      case 'o':
      case 'g': {
        Mesh mesh = obj_impl::parse_object(ctx);
        meshes.push_back(std::move(mesh));
      } break;

      case 'm': {
        auto parts = line | std::views::split(' ');
        auto it = parts.begin();
        if (it == parts.end()) {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        std::string_view header{std::begin(*it), std::end(*it)};
        if (header != "mtllib") {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        ++it;
        if (it == parts.end()) {
          throw std::runtime_error{"[OBJ] invalid data"};
        }
        std::filesystem::path mtl_file_path{"assets/"};
        mtl_file_path /= std::string_view{std::begin(*it), std::end(*it)};
        std::ifstream mtl_file{mtl_file_path};
        obj_impl::parse_mtl_file(mtl_file);
      } break;
    }
  }
}

}
