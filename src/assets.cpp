#include "assets.h"

AssetManager asset_manager_make(Allocator& allocator)
{
  AssetManager out = {};
  out.textures = map_make<MaterialHandle, Texture>(100, allocator);
  out.materials = map_make<TextureHandle, Material>(100, allocator);
  out.meshes = map_make<MeshHandle, Mesh>(100, allocator);
  out.models = map_make<ModelHandle, Model>(100, allocator);

  out.texture_handles = map_make<String, TextureHandle>(100, allocator);
  out.material_handles = map_make<String, MaterialHandle>(100, allocator);
  return out;
}

Material* assets_get_material(MaterialHandle handle)
{
  return map_get(asset_manager_instance->materials, handle);
}

MaterialHandle assets_set_material(const Material& material)
{
  MaterialHandle handle = u64_random();
  map_set(asset_manager_instance->materials, handle, material);
  return handle;
}

Texture* assets_get_texture(TextureHandle handle)
{
  return map_get(asset_manager_instance->textures, handle);
}

TextureHandle assets_set_texture(const Texture& texture)
{
  TextureHandle handle = u64_random();
  map_set(asset_manager_instance->textures, handle, texture);
  return handle;
}

Mesh* assets_get_mesh(MeshHandle handle)
{
  return map_get(asset_manager_instance->meshes, handle);
}

MeshHandle assets_set_mesh(const Mesh& mesh)
{
  MeshHandle handle = u64_random();
  map_set(asset_manager_instance->meshes, handle, mesh);
  return handle;
}

Model* assets_get_model(ModelHandle handle)
{
  return map_get(asset_manager_instance->models, handle);
}

ModelHandle assets_set_model(const Model& model)
{
  ModelHandle handle = u64_random();
  map_set(asset_manager_instance->models, handle, model);
  return handle;
}

MaterialHandle assets_material_handle_get(const String& key)
{
  return *map_get(asset_manager_instance->material_handles, key);
}

void assets_material_handle_set(const String& key, MaterialHandle handle)
{
  map_set(asset_manager_instance->material_handles, key, handle);
}

bool assets_material_handle_exists(const String& key)
{
  return map_contains(asset_manager_instance->material_handles, key);
}

TextureHandle assets_texture_handle_get(const String& key)
{
  return *map_get(asset_manager_instance->texture_handles, key);
}

void assets_texture_handle_set(const String& key, TextureHandle handle)
{
  map_set(asset_manager_instance->texture_handles, key, handle);
}

bool assets_texture_handle_exists(const String& key)
{
  return map_contains(asset_manager_instance->texture_handles, key);
}

struct ObjContext
{
  Array<String> lines;
  usize idx;
  Map<Vertex, usize> vertex_map;
  Array<Vec3> positions;
  Array<Vec3> normals;
  Array<Vec2> uvs;
  Allocator* allocator;
  usize mesh_count;
  usize pos_count;
  usize normal_count;
  usize uv_count;
};

static void obj_parse_mtl_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  bool parsing = false;
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));
  usize file_size;
  void* file_ptr = platform.read_file(path, &scratch_arena.allocator, &file_size);
  auto file = string_make_len((const char*) file_ptr, file_size);
  auto lines = string_split(file, '\n', scratch_arena.allocator);

  String material_name = {};
  Material material = {};
  for (usize i = 0; i < lines.size; ++i)
  {
    const auto& line = lines[i];
    switch (line[0])
    {
      case 'n':
      {
        auto parts = string_split(line, ' ', scratch_arena.allocator);
        if (parts.size != 2)
        {
          out_error = GLOBAL_ERROR_INVALID_DATA;
          return;
        }
        if (parts[0] != "newmtl")
        {
          continue;
        }
        if (!assets_material_handle_exists(parts[1]))
        {
          if (parsing)
          {
            // TODO(szulf): how to correctly initialize the shader in the future? for now hardcoded
            material.shader = SHADER_DEFAULT;
            auto material_handle = assets_set_material(material);
            assets_material_handle_set(material_name, material_handle);
          }

          parsing = true;
          material_name = string_copy(parts[1], allocator);
        }
      }
      break;

      case 'm':
      {
        if (!parsing)
        {
          continue;
        }
        auto parts = string_split(line, ' ', scratch_arena.allocator);
        if (parts.size != 2)
        {
          continue;
        }
        if (parts[0] != "map_Kd")
        {
          continue;
        }
        auto base_file_path = string_make("assets/");
        auto texture_file_path = string_append_str(base_file_path, parts[1], scratch_arena.allocator);
        if (!assets_texture_handle_exists(texture_file_path))
        {
          auto img =
            image_from_file(string_to_cstr(texture_file_path, scratch_arena.allocator), scratch_arena.allocator, error);
          // TODO(szulf): do i want to initialize it with an error image here instead?
          if (error != SUCCESS)
          {
            out_error = GLOBAL_ERROR_INVALID_DATA;
            return;
          }
          auto texture = texture_make(img);
          auto allocated_texture_file_path = string_copy(texture_file_path, allocator);
          auto texture_handle = assets_set_texture(texture);
          assets_texture_handle_set(allocated_texture_file_path, texture_handle);
          material.texture = texture_handle;
        }
        else
        {
          auto texture_handle = assets_texture_handle_get(texture_file_path);
          material.texture = texture_handle;
        }
      }
      break;
    }
  }

  // TODO(szulf): how to correctly initialize the shader in the future? for now hardcoded
  material.shader = SHADER_DEFAULT;
  auto material_handle = assets_set_material(material);
  assets_material_handle_set(material_name, material_handle);
}

static void obj_parse_vertex(f32* points, usize points_size, const String& line, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));
  const auto parts = string_split(line, ' ', scratch_arena.allocator);
  ASSERT(parts.size - 1 <= points_size, "out of bounds on points");
  for (usize i = 1; i < parts.size; ++i)
  {
    auto num = string_parse_f32(parts[i], error);
    if (error != SUCCESS)
    {
      out_error = error;
      return;
    }
    points[i - 1] = num;
  }
}

static Mesh obj_parse_object(ObjContext& ctx, Error& out_error)
{
  Error error = SUCCESS;
  usize indices_amount = 0;
  bool parsing = true;
  auto idx_old = ctx.idx;
  for (; parsing && ctx.idx < ctx.lines.size; ++ctx.idx)
  {
    switch (ctx.lines[ctx.idx][0])
    {
      case 'f':
      {
        indices_amount += 3;
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
  ctx.idx = idx_old;
  auto mesh_indices = array_make<u32>(indices_amount, *ctx.allocator);
  auto mesh_vertices = array_make<Vertex>(1, *ctx.allocator);
  MaterialHandle mesh_material_handle = {};

  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));

  parsing = true;
  usize back_idx = 0;
  for (; parsing && ctx.idx < ctx.lines.size; ++ctx.idx)
  {
    auto& line = ctx.lines[ctx.idx];
    switch (line[0])
    {
      case 'f':
      {
        const auto splits = string_split(line, ' ', scratch_arena.allocator);
        for (usize i = 1; i < splits.size; ++i)
        {
          u32 indices[3];
          usize idx = 0;
          const auto parts = string_split(splits[i], '/', scratch_arena.allocator);
          for (usize part_idx = 0; part_idx < parts.size; ++part_idx)
          {
            auto num = string_parse_u32(parts[part_idx], error);
            if (error != SUCCESS)
            {
              out_error = error;
              return {};
            }
            indices[idx] = num - 1;
            ++idx;
          }
          Vertex vertex;
          vertex.position = ctx.positions[indices[0]];
          vertex.uv = ctx.uvs[indices[1]];
          vertex.normal = ctx.normals[indices[2]];
          if (map_contains(ctx.vertex_map, vertex))
          {
            array_push(mesh_indices, *(u32*) map_get(ctx.vertex_map, vertex));
          }
          else
          {
            map_set(ctx.vertex_map, vertex, mesh_vertices.size);
            array_push(mesh_indices, (u32) mesh_vertices.size);
            array_push(mesh_vertices, vertex);
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
            f32 points[3];
            obj_parse_vertex(points, 3, line, error);
            if (error != SUCCESS)
            {
              out_error = error;
              return {};
            }
            array_push(ctx.positions, {points[0], points[1], points[2]});
          }
          break;

          case 'n':
          {
            f32 points[3];
            obj_parse_vertex(points, 3, line, error);
            if (error != SUCCESS)
            {
              out_error = error;
              return {};
            }
            array_push(ctx.normals, {points[0], points[1], points[2]});
          }
          break;

          case 't':
          {
            f32 points[2];
            obj_parse_vertex(points, 2, line, error);
            if (error != SUCCESS)
            {
              out_error = error;
              return {};
            }
            array_push(ctx.uvs, {points[0], points[1]});
          }
          break;
        }
      }
      break;

      case 'u':
      {
        const auto parts = string_split(line, ' ', scratch_arena.allocator);
        if (parts.size != 2 || parts[0] != "usemtl")
        {
          out_error = GLOBAL_ERROR_INVALID_DATA;
          return {};
        }
        ASSERT(
          assets_material_handle_exists(parts[1]),
          "material with name %s does not exist",
          string_to_cstr(parts[1], scratch_arena.allocator)
        );
        mesh_material_handle = assets_material_handle_get(parts[1]);
      }
      break;

      case 'g':
      case 'o':
      {
        parsing = false;
        ctx.idx = back_idx;
      }
      break;
    }
    back_idx = ctx.idx;
  }

  return mesh_make(mesh_vertices, mesh_indices, mesh_material_handle);
}

ModelHandle assets_load_model(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  Model model = {};
  model.matrix = mat4_make();
  ObjContext ctx = {};
  ctx.allocator = &allocator;
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(scratch_arena));
  usize file_size;
  void* file_ptr = platform.read_file(path, &scratch_arena.allocator, &file_size);
  if (file_ptr == nullptr)
  {
    out_error = GLOBAL_ERROR_NOT_FOUND;
    return {};
  }
  auto file = string_make_len((const char*) file_ptr, file_size);
  ctx.lines = string_split(file, '\n', scratch_arena.allocator);

  usize vertex_count = 0;
  for (usize i = 0; i < ctx.lines.size; ++i)
  {
    const auto& line = ctx.lines[i];
    if (line.size < 2)
    {
      out_error = GLOBAL_ERROR_INVALID_DATA;
      return {};
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
      break;
      case 'f':
      {
        vertex_count += 3;
      }
      break;
    }
  }

  model.meshes = array_make<MeshHandle>(ctx.mesh_count, allocator);
  ctx.positions = array_make<Vec3>(ctx.pos_count, scratch_arena.allocator);
  ctx.normals = array_make<Vec3>(ctx.normal_count, scratch_arena.allocator);
  ctx.uvs = array_make<Vec2>(ctx.uv_count, scratch_arena.allocator);
  ctx.vertex_map = map_make<Vertex, usize>(vertex_count * 3, scratch_arena.allocator);

  for (ctx.idx = 0; ctx.idx < ctx.lines.size; ++ctx.idx)
  {
    auto line = ctx.lines[ctx.idx];
    switch (line[0])
    {
      case 'o':
      case 'g':
      {
        ++ctx.idx;
        auto mesh = obj_parse_object(ctx, error);
        if (error != SUCCESS)
        {
          out_error = error;
          return {};
        }
        auto mesh_handle = assets_set_mesh(mesh);
        array_push(model.meshes, mesh_handle);
        --ctx.idx;
      }
      break;

      case 'm':
      {
        auto parts = string_split(line, ' ', scratch_arena.allocator);
        if (parts.size != 2 || parts[0] != "mtllib")
        {
          out_error = GLOBAL_ERROR_INVALID_DATA;
          return {};
        }
        auto base_file_path = string_make("assets/");
        auto mtl_file_path = string_append_str(base_file_path, parts[1], scratch_arena.allocator);
        obj_parse_mtl_file(string_to_cstr(mtl_file_path, scratch_arena.allocator), allocator, error);
        if (error != SUCCESS)
        {
          out_error = error;
          return {};
        }
      }
      break;
    }
  }

  return assets_set_model(model);
}
