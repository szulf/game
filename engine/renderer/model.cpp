#include "engine/renderer/model.hpp"

#include "engine/image.hpp"
#include "engine/renderer/vertex.hpp"
#include "engine/asset_manager.hpp"
#include "badtl/array.hpp"
#include "badtl/utils.hpp"
#include "badtl/files.hpp"
#include "badtl/map.hpp"
#include "badtl/vec2.hpp"
#include "badtl/vec3.hpp"
#include "badtl/math.hpp"

namespace core {

namespace obj_impl {

struct Context {
  btl::List<btl::String> lines;
  btl::usize idx;
  btl::Map<Vertex, btl::usize> vertex_map;
  btl::List<btl::Vec3> positions;
  btl::List<btl::Vec3> normals;
  btl::List<btl::Vec2> uvs;
  btl::Allocator* allocator;
  btl::usize mesh_count;
  btl::usize pos_count;
  btl::usize normal_count;
  btl::usize uv_count;
};

static btl::Result<void, ModelError> parse_mtl_file(const btl::String& mtl_file, btl::Allocator& allocator) {
  bool parsing = false;
  btl::String material_name = {};
  auto scratch_arena = btl::ScratchArena::get();
  defer(scratch_arena.release());
  AssetManager& assets = *AssetManager::instance;
  auto lines = mtl_file.split('\n', scratch_arena.allocator);

  for (const auto& line : lines) {
    switch (line[0]) {
      case 'n': {
        auto parts = line.split(' ', scratch_arena.allocator);
        if (parts.size != 2) {
          return btl::err<void>(ModelError::InvalidMTLFile);
        }
        if (parts[0] != "newmtl") {
          continue;
        }
        if (!assets.materials.contains(parts[1])) {
          parsing = true;
          // NOTE(szulf): weird way to copy a string, change it pls
          material_name = parts[1].copy(allocator);
        }
      } break;

      case 'm': {
        if (!parsing) {
          continue;
        }
        auto parts = line.split(' ', scratch_arena.allocator);
        if (parts.size != 2) {
          continue;
        }
        if (parts[0] != "map_Kd") {
          continue;
        }
        auto base_file_path = btl::String::make("assets/");
        auto texture_file_path = base_file_path.append(parts[1], scratch_arena.allocator);
        Material mat = {};
        if (!assets.textures.contains(texture_file_path)) {
          auto img = Image::from_file(texture_file_path, scratch_arena.allocator).value_or(Image::error_image());
          auto texture = Texture::make(img);
          auto allocated_texture_file_path = texture_file_path.copy(allocator);
          assets.textures.set(allocated_texture_file_path, texture);
          mat.texture_name = allocated_texture_file_path;
        } else {
          auto& entry = assets.textures.get_entry(texture_file_path);
          mat.texture_name = entry.key;
        }
        assets.materials.set(material_name, mat);
      } break;
    }
  }
  return btl::ok<ModelError>();
}

template <btl::usize N>
static btl::Result<void, ModelError> parse_vertex(btl::Array<float, N>& points, const btl::String& line) {
  auto scratch_arena = btl::ScratchArena::get();
  defer(scratch_arena.release());
  btl::usize idx = 0;
  const auto parts = line.split(' ', scratch_arena.allocator);
  for (btl::usize i = 1; i < parts.size; ++i) {
    auto res = parts[i].parse<btl::f32>();
    if (res.has_err) {
      return btl::err<void>(ModelError::InvalidVertex);
    }
    points[idx] = res.value.success;
    ++idx;
  }
  return btl::ok<ModelError>();
}

static btl::Result<Mesh, ModelError> parse_object(Context& ctx) {
  btl::usize indices_amount = 0;
  bool parsing = true;
  auto idx_old = ctx.idx;
  for (; parsing && ctx.idx < ctx.lines.size; ++ctx.idx) {
    switch (ctx.lines[ctx.idx][0]) {
      case 'f': {
        ++indices_amount;
      } break;
      case 'o':
      case 'g': {
        parsing = false;
      } break;
    }
  }
  ctx.idx = idx_old;
  indices_amount *= 3;
  auto mesh_indices = btl::List<btl::u32>::make(indices_amount, *ctx.allocator);
  auto mesh_vertices = btl::List<Vertex>::make(1, *ctx.allocator);
  btl::String mesh_material_name;

  auto scratch_arena = btl::ScratchArena::get();
  defer(scratch_arena.release());

  parsing = true;
  btl::usize back_idx = 0;
  for (; parsing && ctx.idx < ctx.lines.size; ++ctx.idx) {
    switch (ctx.lines[ctx.idx][0]) {
      case 'f': {
        const auto splits = ctx.lines[ctx.idx].split(' ', scratch_arena.allocator);
        for (btl::usize i = 1; i < splits.size; ++i) {
          btl::Array<btl::u32, 3> indices;
          btl::usize idx = 0;
          const auto parts = splits[i].split('/', scratch_arena.allocator);
          for (const auto& part : parts) {
            auto res = part.parse<btl::u32>();
            if (res.has_err) {
              return btl::err<Mesh>(ModelError::InvalidInput);
            }
            indices[idx] = res.value.success - 1;
            ++idx;
          }
          Vertex vertex;
          vertex.position = ctx.positions[indices[0]];
          vertex.uv = ctx.uvs[indices[1]];
          vertex.normal = ctx.normals[indices[2]];
          if (ctx.vertex_map.contains(vertex)) {
            mesh_indices.push(static_cast<btl::u32>(ctx.vertex_map[vertex]));
          } else {
            ctx.vertex_map.set(vertex, mesh_vertices.size);
            mesh_indices.push(static_cast<btl::u32>(mesh_vertices.size));
            mesh_vertices.push(vertex);
          }
        }
      } break;

      case 'v': {
        switch (ctx.lines[ctx.idx][1]) {
          case ' ': {
            btl::Array<float, 3> points;
            auto parse_res = parse_vertex(points, ctx.lines[ctx.idx]);
            if (parse_res.has_err) {
              return btl::err<Mesh>(parse_res.value.error);
            }
            ctx.positions.push({points[0], points[1], points[2]});
          } break;

          case 'n': {
            btl::Array<float, 3> points;
            auto parse_res = parse_vertex(points, ctx.lines[ctx.idx]);
            if (parse_res.has_err) {
              return btl::err<Mesh>(parse_res.value.error);
            }
            ctx.normals.push({points[0], points[1], points[2]});
          } break;

          case 't': {
            btl::Array<float, 2> points;
            auto parse_res = parse_vertex(points, ctx.lines[ctx.idx]);
            if (parse_res.has_err) {
              return btl::err<Mesh>(parse_res.value.error);
            }
            ctx.uvs.push({points[0], points[1]});
          } break;
        }
      } break;

      case 'u': {
        const auto parts = ctx.lines[ctx.idx].split(' ', scratch_arena.allocator);
        if (parts.size != 2 || parts[0] != "usemtl") {
          return btl::err<Mesh>(ModelError::InvalidInput);
        }
        mesh_material_name = parts[1].copy(*ctx.allocator);
      } break;

      case 'g':
      case 'o': {
        parsing = false;
        ctx.idx = back_idx;
      } break;
    }
    back_idx = ctx.idx;
  }

  return btl::ok<ModelError>(Mesh::make(mesh_vertices, mesh_indices, mesh_material_name));
}

}

btl::Result<Model, ModelError> Model::from_file(const btl::String& path, btl::Allocator& allocator) {
  Model model = {};
  model.matrix = btl::Mat4::make();
  obj_impl::Context ctx = {};
  ctx.allocator = &allocator;
  auto scratch_arena = btl::ScratchArena::get();
  defer(scratch_arena.release());
  auto file_ptr = btl::read_file(path, scratch_arena.allocator);
  auto file = btl::String::make(static_cast<const char*>(file_ptr.ptr), file_ptr.size);
  ctx.lines = file.split('\n', scratch_arena.allocator);

  btl::usize vertex_count = 0;
  for (const auto& line : ctx.lines) {
    if (line.size < 2) {
      return btl::err<Model>(ModelError::InvalidInput);
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
      } break;
      case 'f': {
        vertex_count += 3;
      } break;
    }
  }

  model.meshes = btl::List<Mesh>::make(ctx.mesh_count, allocator);
  ctx.positions = btl::List<btl::Vec3>::make(ctx.pos_count, scratch_arena.allocator);
  ctx.normals = btl::List<btl::Vec3>::make(ctx.normal_count, scratch_arena.allocator);
  ctx.uvs = btl::List<btl::Vec2>::make(ctx.uv_count, scratch_arena.allocator);
  ctx.vertex_map = btl::Map<Vertex, btl::usize>::make(vertex_count * 2, scratch_arena.allocator);

  for (ctx.idx = 0; ctx.idx < ctx.lines.size; ++ctx.idx) {
    switch (ctx.lines[ctx.idx][0]) {
      case 'o':
      case 'g': {
        ++ctx.idx;
        auto object_res = obj_impl::parse_object(ctx);
        if (object_res.has_err) {
          return btl::err<Model>(object_res.value.error);
        }
        model.meshes.push(object_res.value.success);
        --ctx.idx;
      } break;

      case 'm': {
        auto parts = ctx.lines[ctx.idx].split(' ', scratch_arena.allocator);
        if (parts.size != 2 || parts[0] != "mtllib") {
          return btl::err<Model>(ModelError::InvalidInput);
        }
        auto base_file_path = btl::String::make("assets/");
        auto mtl_file_path = base_file_path.append(parts[1], scratch_arena.allocator);
        auto mtl_file = btl::read_file(mtl_file_path.c_string(allocator), scratch_arena.allocator);
        auto mtl_file_res =
          obj_impl::parse_mtl_file(btl::String::make(static_cast<const char*>(mtl_file.ptr), mtl_file.size), allocator);
        if (mtl_file_res.has_err) {
          return btl::err<Model>(mtl_file_res.value.error);
        }
      } break;
    }
  }

  return btl::ok<ModelError>(model);
}

}
