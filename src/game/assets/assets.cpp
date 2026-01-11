#include "assets.h"

template <>
usize hash(const usize& value)
{
  return value;
}

template <>
usize hash(const assets::ShaderHandle& value)
{
  return value;
}

namespace assets
{

Manager* Manager::instance = nullptr;

Manager Manager::make(Allocator& allocator)
{
  Manager out = {};
  out.textures = Array<Texture>::make(ArrayType::STATIC, 100, allocator);
  out.materials = Array<Material>::make(ArrayType::STATIC, 100, allocator);
  out.meshes = Array<Mesh>::make(ArrayType::STATIC, 100, allocator);
  out.models = Array<Model>::make(ArrayType::STATIC, 100, allocator);

  out.texture_handles = Map<String, TextureHandle>::make(100, allocator);
  out.material_handles = Map<String, MaterialHandle>::make(100, allocator);
  return out;
}

Texture& texture_get(TextureHandle handle)
{
  return Manager::instance->textures[handle];
}

TextureHandle texture_set(const Texture& texture)
{
  Manager::instance->textures.push(texture);
  return Manager::instance->textures.size - 1;
}

Mesh& mesh_get(MeshHandle handle)
{
  return Manager::instance->meshes[handle - 1];
}

MeshHandle mesh_set(const Mesh& mesh)
{
  Manager::instance->meshes.push(mesh);
  return Manager::instance->meshes.size;
}

Model& model_get(ModelHandle handle)
{
  return Manager::instance->models[handle - 1];
}

ModelHandle model_set(const Model& model)
{
  Manager::instance->models.push(model);
  return Manager::instance->models.size;
}

Material& material_get(MaterialHandle handle)
{
  return Manager::instance->materials[handle - 1];
}

MaterialHandle material_set(const Material& material)
{
  Manager::instance->materials.push(material);
  return Manager::instance->materials.size;
}

MaterialHandle material_handle_get(const String& key)
{
  return *Manager::instance->material_handles[key];
}

void material_handle_set(const String& key, MaterialHandle handle)
{
  Manager::instance->material_handles.set(key, handle);
}

bool material_handle_exists(const String& key)
{
  return Manager::instance->material_handles.contains(key);
}

TextureHandle texture_handle_get(const String& key)
{
  return *Manager::instance->texture_handles[key];
}

void texture_handle_set(const String& key, TextureHandle handle)
{
  Manager::instance->texture_handles.set(key, handle);
}

bool texture_handle_exists(const String& key)
{
  return Manager::instance->texture_handles.contains(key);
}

struct ObjContext
{
  Array<String> lines;
  usize idx;
  Map<Vertex, usize> vertex_map;
  Array<vec3> positions;
  Array<vec3> normals;
  Array<vec2> uvs;
  Allocator* allocator;
  usize mesh_count;
  usize pos_count;
  usize normal_count;
  usize uv_count;
};

static TextureHandle obj_get_texture_by_path(const String& path, Allocator& allocator)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  if (texture_handle_exists(path))
  {
    return texture_handle_get(path);
  }

  auto img = Image::from_file(path.to_cstr(scratch_arena.allocator), allocator, error);
  if (error != SUCCESS)
  {
    img = Image::error_placeholder();
  }
  auto texture_handle = texture_set({img});
  auto allocated_path = path.copy(allocator);
  texture_handle_set(allocated_path, texture_handle);
  return texture_handle;
}

static vec3 obj_color_from_parts(const Array<String>& parts, Error& out_error)
{
  Error error = SUCCESS;
  f32 r = parse_f32(parts[1], error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  f32 g = parse_f32(parts[2], error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  f32 b = parse_f32(parts[3], error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  return {r, g, b};
}

static void obj_set_shader(Material& material, const String& material_name)
{
  if (material_name == "light_bulb")
  {
    material.shader = SHADER_DEFAULT;
  }
  else
  {
    material.shader = SHADER_LIGHTING;
  }
}

static void obj_parse_mtl_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  bool parsing = false;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());
  usize file_size;
  void* file_ptr = platform::read_entire_file(path, scratch_arena.allocator, file_size, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  auto file = String::make((const char*) file_ptr, file_size);
  auto lines = file.split('\n', scratch_arena.allocator);

  String material_name = {};
  Material material = {};
  for (usize i = 0; i < lines.size; ++i)
  {
    const auto& line = lines[i];
    auto parts = line.split(' ', scratch_arena.allocator);

    if (parts[0] == "newmtl")
    {
      ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, );
      if (!material_handle_exists(parts[1]))
      {
        if (parsing)
        {
          obj_set_shader(material, material_name);
          auto material_handle = material_set(material);
          material_handle_set(material_name, material_handle);
        }

        parsing = true;
        material_name = parts[1].copy(allocator);
      }
    }
    else if (parts[0] == "map_Kd")
    {
      ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, );
      auto base_file_path = String::make("assets/");
      auto texture_file_path = base_file_path.append(parts[1], scratch_arena.allocator);

      auto texture_handle = obj_get_texture_by_path(texture_file_path, allocator);
      material.diffuse_map = texture_handle;
    }
    else if (parts[0] == "Kd")
    {
      ERROR_ASSERT(parts.size == 4, out_error, GLOBAL_ERROR_INVALID_DATA, );
      vec3 color = obj_color_from_parts(parts, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, );
      material.diffuse_color = color;
    }
    else if (parts[0] == "Ks")
    {
      ERROR_ASSERT(parts.size == 4, out_error, GLOBAL_ERROR_INVALID_DATA, );
      vec3 color = obj_color_from_parts(parts, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, );
      material.specular_color = color;
    }
    else if (parts[0] == "Ns")
    {
      ERROR_ASSERT(parts.size == 2, out_error, GLOBAL_ERROR_INVALID_DATA, );
      f32 exponent = parse_f32(parts[1], error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, );
      material.specular_exponent = exponent;
    }
  }

  obj_set_shader(material, material_name);
  auto material_handle = material_set(material);
  material_handle_set(material_name, material_handle);
}

static void obj_parse_vertex(f32* points, usize points_size, const String& line, Error& out_error)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());
  const auto parts = line.split(' ', scratch_arena.allocator);
  ASSERT(parts.size - 1 <= points_size, "out of bounds on points");
  for (usize i = 1; i < parts.size; ++i)
  {
    auto num = parse_f32(parts[i], error);
    if (error != SUCCESS)
    {
      out_error = error;
      return;
    }
    points[i - 1] = num;
  }
}

static MeshMaterialPair obj_parse_object(ObjContext& ctx, Error& out_error)
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
  auto mesh_indices = Array<u32>::make(ArrayType::STATIC, indices_amount, *ctx.allocator);
  auto mesh_vertices = Array<Vertex>::make(ArrayType::DYNAMIC, 1, *ctx.allocator);
  MaterialHandle material_handle = {};

  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  parsing = true;
  usize back_idx = 0;
  for (; parsing && ctx.idx < ctx.lines.size; ++ctx.idx)
  {
    auto& line = ctx.lines[ctx.idx];
    switch (line[0])
    {
      case 'f':
      {
        const auto splits = line.split(' ', scratch_arena.allocator);
        for (usize i = 1; i < splits.size; ++i)
        {
          u32 indices[3];
          usize idx = 0;
          const auto parts = splits[i].split('/', scratch_arena.allocator);
          for (usize part_idx = 0; part_idx < parts.size; ++part_idx)
          {
            auto num = parse_u32(parts[part_idx], error);
            if (error != SUCCESS)
            {
              out_error = error;
              return {};
            }
            indices[idx] = num - 1;
            ++idx;
          }
          Vertex vertex;
          vertex.pos = ctx.positions[indices[0]];
          vertex.uv = ctx.uvs[indices[1]];
          vertex.normal = ctx.normals[indices[2]];
          if (ctx.vertex_map.contains(vertex))
          {
            mesh_indices.push(*(u32*) ctx.vertex_map[vertex]);
          }
          else
          {
            ctx.vertex_map.set(vertex, mesh_vertices.size);
            mesh_indices.push((u32) mesh_vertices.size);
            mesh_vertices.push(vertex);
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
            vec3 vec = {points[0], points[1], points[2]};
            ctx.positions.push(vec);
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
            ctx.normals.push({points[0], points[1], points[2]});
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
            ctx.uvs.push({points[0], points[1]});
          }
          break;
        }
      }
      break;

      case 'u':
      {
        const auto parts = line.split(' ', scratch_arena.allocator);
        if (parts.size != 2 || parts[0] != "usemtl")
        {
          out_error = GLOBAL_ERROR_INVALID_DATA;
          return {};
        }
        ASSERT(
          material_handle_exists(parts[1]),
          "material with name %s does not exist",
          parts[1].to_cstr(scratch_arena.allocator)
        );
        material_handle = material_handle_get(parts[1]);
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

  return {
    mesh_set({mesh_vertices, mesh_indices, RenderingPrimitive::TRIANGLES}),
    material_handle,
  };
}

ModelHandle Model::from_file(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  Model model = {};
  ObjContext ctx = {};
  ctx.allocator = &allocator;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());
  usize file_size;
  void* file_ptr = platform::read_entire_file(path, scratch_arena.allocator, file_size, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  auto file = String::make((const char*) file_ptr, file_size);
  ctx.lines = file.split('\n', scratch_arena.allocator);

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

  model.parts = Array<MeshMaterialPair>::make(ArrayType::STATIC, ctx.mesh_count, allocator);
  ctx.positions = Array<vec3>::make(ArrayType::STATIC, ctx.pos_count, scratch_arena.allocator);
  ctx.normals = Array<vec3>::make(ArrayType::STATIC, ctx.normal_count, scratch_arena.allocator);
  ctx.uvs = Array<vec2>::make(ArrayType::STATIC, ctx.uv_count, scratch_arena.allocator);
  ctx.vertex_map = Map<Vertex, usize>::make(vertex_count * 3, scratch_arena.allocator);

  for (ctx.idx = 0; ctx.idx < ctx.lines.size; ++ctx.idx)
  {
    auto line = ctx.lines[ctx.idx];
    switch (line[0])
    {
      case 'o':
      case 'g':
      {
        ++ctx.idx;
        auto mesh_material_pair = obj_parse_object(ctx, error);
        if (error != SUCCESS)
        {
          out_error = error;
          return {};
        }
        model.parts.push(mesh_material_pair);
        --ctx.idx;
      }
      break;

      case 'm':
      {
        auto parts = line.split(' ', scratch_arena.allocator);
        if (parts.size != 2 || parts[0] != "mtllib")
        {
          out_error = GLOBAL_ERROR_INVALID_DATA;
          return {};
        }
        auto base_file_path = String::make("assets/");
        auto mtl_file_path = base_file_path.append(parts[1], scratch_arena.allocator);
        obj_parse_mtl_file(mtl_file_path.to_cstr(scratch_arena.allocator), allocator, error);
        if (error != SUCCESS)
        {
          out_error = error;
          return {};
        }
      }
      break;
    }
  }

  return model_set(model);
}

}
