#include "assets.h"

#include "platform/platform.h"

template <>
void AssetType<MeshHandle, MeshData, 100>::destroy_all()
{
  size = StaticModel_COUNT;
  mem_set(data + StaticModel_COUNT, 0, (MAX - StaticModel_COUNT) * sizeof(MeshData));
}

Assets Assets::make(Allocator& allocator)
{
  Assets out = {};

  out.texture_handles = Map<String, TextureHandle>::make(100, allocator);
  out.material_handles = Map<String, MaterialHandle>::make(100, allocator);

  return out;
}

struct ObjContext
{
  MeshData out;

  Array<String> lines;
  usize idx;

  Map<Vertex, usize> vertex_map;
  Array<vec3> positions;
  Array<vec3> normals;
  Array<vec2> uvs;

  Allocator* allocator;
};

TextureHandle obj_get_texture_by_path_(const String& path, Assets& assets, Allocator& allocator)
{
  Error error = SUCCESS;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());

  if (assets.texture_handles.contains(path))
  {
    return *assets.texture_handles[path];
  }

  TextureData texture = {};
  auto img = Image::from_file(path.to_cstr(scratch_arena.allocator), allocator, error);
  if (error != SUCCESS)
  {
    img = Image::error_placeholder();
    texture.mag_filter = TextureFilteringOption::NEAREST;
    texture.min_filter = TextureFilteringOption::NEAREST;
    texture.wrap_s = TextureWrappingOption::REPEAT;
    texture.wrap_t = TextureWrappingOption::REPEAT;
  }
  texture.image = img;
  auto texture_handle = assets.textures.set(texture);
  auto allocated_path = path.copy(allocator);
  assets.texture_handles.set(allocated_path, texture_handle);
  return texture_handle;
}

vec3 obj_color_from_parts_(const Array<String>& parts, Error& out_error)
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

void obj_set_shader_(Material& material, const String& material_name)
{
  if (material_name == "light_bulb")
  {
    material.shader = ShaderHandle::DEFAULT;
  }
  else
  {
    material.shader = ShaderHandle::LIGHTING;
  }
}

void obj_parse_mtl_file_(const char* path, Assets& assets, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  bool parsing = false;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());
  auto file = platform::read_file_to_string(path, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, );
  auto lines = file.split('\n', scratch_arena.allocator);

  String material_name = {};
  Material material = {};
  for (usize i = 0; i < lines.size; ++i)
  {
    const auto& line = lines[i];
    auto parts = line.split(' ', scratch_arena.allocator);

    if (parts[0] == "newmtl")
    {
      ERROR_ASSERT(parts.size == 2, out_error, "Obj decoding error. Invalid newmtl.", );
      if (!assets.material_handles.contains(parts[1]))
      {
        if (parsing)
        {
          obj_set_shader_(material, material_name);
          auto material_handle = assets.materials.set(material);
          assets.material_handles.set(material_name, material_handle);
        }

        parsing = true;
        material_name = parts[1].copy(allocator);
      }
    }
    else if (parts[0] == "map_Kd")
    {
      ERROR_ASSERT(
        parts.size == 2,
        out_error,
        "Obj decoding error. Invalid map_Kd(diffuse map).",
      );
      auto base_file_path = String::make("assets/");
      auto texture_file_path = base_file_path.append(parts[1], scratch_arena.allocator);

      auto texture_handle = obj_get_texture_by_path_(texture_file_path, assets, allocator);
      material.diffuse_map = texture_handle;
    }
    else if (parts[0] == "Kd")
    {
      ERROR_ASSERT(parts.size == 4, out_error, "Obj decoding error. Invalid Kd(diffuse color).", );
      vec3 color = obj_color_from_parts_(parts, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, );
      material.diffuse_color = color;
    }
    else if (parts[0] == "Ks")
    {
      ERROR_ASSERT(parts.size == 4, out_error, "Obj decoding error. Invalid Ks(specular color).", );
      vec3 color = obj_color_from_parts_(parts, error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, );
      material.specular_color = color;
    }
    else if (parts[0] == "Ns")
    {
      ERROR_ASSERT(
        parts.size == 2,
        out_error,
        "Obj decoding error. Invalid Ns(specular_exponent)",
      );
      f32 exponent = parse_f32(parts[1], error);
      ERROR_ASSERT(error == SUCCESS, out_error, error, );
      material.specular_exponent = exponent;
    }
  }

  obj_set_shader_(material, material_name);
  auto material_handle = assets.materials.set(material);
  assets.material_handles.set(material_name, material_handle);
}

void obj_parse_vertex_(f32* points, usize points_size, const String& line, Error& out_error)
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

void obj_parse_object_(ObjContext& ctx, Assets& assets, Error& out_error)
{
  Error error = SUCCESS;
  Submesh submesh = {};
  submesh.index_offset = ctx.out.indices.size;
  bool parsing = true;
  auto idx_old = ctx.idx;
  for (; parsing && ctx.idx < ctx.lines.size; ++ctx.idx)
  {
    switch (ctx.lines[ctx.idx][0])
    {
      case 'o':
      {
        parsing = false;
      }
      break;
    }
  }
  ctx.idx = idx_old;

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
            ERROR_ASSERT(error == SUCCESS, out_error, error, );
            indices[idx] = num - 1;
            ++idx;
          }
          Vertex vertex;
          vertex.pos = ctx.positions[indices[0]];
          vertex.uv = ctx.uvs[indices[1]];
          vertex.normal = ctx.normals[indices[2]];
          if (ctx.vertex_map.contains(vertex))
          {
            ctx.out.indices.push(*(u32*) ctx.vertex_map[vertex]);
          }
          else
          {
            ctx.vertex_map.set(vertex, ctx.out.vertices.size);
            ctx.out.indices.push((u32) ctx.out.vertices.size);
            ctx.out.vertices.push(vertex);
          }
          ++submesh.index_count;
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
            obj_parse_vertex_(points, 3, line, error);
            ERROR_ASSERT(error == SUCCESS, out_error, error, );
            vec3 vec = {points[0], points[1], points[2]};
            ctx.positions.push(vec);
          }
          break;

          case 'n':
          {
            f32 points[3];
            obj_parse_vertex_(points, 3, line, error);
            ERROR_ASSERT(error == SUCCESS, out_error, error, );
            ctx.normals.push({points[0], points[1], points[2]});
          }
          break;

          case 't':
          {
            f32 points[2];
            obj_parse_vertex_(points, 2, line, error);
            ERROR_ASSERT(error == SUCCESS, out_error, error, );
            ctx.uvs.push({points[0], points[1]});
          }
          break;
        }
      }
      break;

      case 'u':
      {
        const auto parts = line.split(' ', scratch_arena.allocator);
        ERROR_ASSERT(
          parts.size == 2 && parts[0] == "usemtl",
          out_error,
          "Obj decoding error. Invalid usemtl.",
        );
        ASSERT(
          assets.material_handles.contains(parts[1]),
          "material with name %s does not exist",
          parts[1].to_cstr(scratch_arena.allocator)
        );
        submesh.material = *assets.material_handles[parts[1]];
      }
      break;

      case 'o':
      {
        parsing = false;
        ctx.idx = back_idx;
      }
      break;
    }
    back_idx = ctx.idx;
  }

  ctx.out.submeshes.push(submesh);
}

MeshHandle Assets::load_obj(const char* path, Allocator& allocator, Error& out_error)
{
  Error error = SUCCESS;
  ObjContext ctx = {};
  ctx.allocator = &allocator;
  auto scratch_arena = ScratchArena::get();
  defer(scratch_arena.release());
  auto file = platform::read_file_to_string(path, scratch_arena.allocator, error);
  ERROR_ASSERT(error == SUCCESS, out_error, error, {});
  ctx.lines = file.split('\n', scratch_arena.allocator);

  usize vertex_count = 0;
  usize submesh_count = 0;
  usize pos_count = 0;
  usize normal_count = 0;
  usize uv_count = 0;
  for (usize i = 0; i < ctx.lines.size; ++i)
  {
    const auto& line = ctx.lines[i];
    if (line.size < 2)
    {
      out_error = "Obj decoding error. Invalid line.";
      return {};
    }
    switch (line[0])
    {
      case 'o':
      {
        ++submesh_count;
      }
      break;
      case 'v':
      {
        switch (line[1])
        {
          case ' ':
          {
            ++pos_count;
          }
          break;
          case 'n':
          {
            ++normal_count;
          }
          break;
          case 't':
          {
            ++uv_count;
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

  ctx.out.submeshes = Array<Submesh>::make(ArrayType::STATIC, submesh_count, allocator);
  ctx.out.vertices = Array<Vertex>::make(ArrayType::DYNAMIC, 100, allocator);
  ctx.out.indices = Array<u32>::make(ArrayType::STATIC, vertex_count, allocator);

  ctx.positions = Array<vec3>::make(ArrayType::STATIC, pos_count, scratch_arena.allocator);
  ctx.normals = Array<vec3>::make(ArrayType::STATIC, normal_count, scratch_arena.allocator);
  ctx.uvs = Array<vec2>::make(ArrayType::STATIC, uv_count, scratch_arena.allocator);
  ctx.vertex_map = Map<Vertex, usize>::make(vertex_count * 2, scratch_arena.allocator);

  for (ctx.idx = 0; ctx.idx < ctx.lines.size; ++ctx.idx)
  {
    auto line = ctx.lines[ctx.idx];
    switch (line[0])
    {
      case 'o':
      {
        ++ctx.idx;
        obj_parse_object_(ctx, *this, error);
        ERROR_ASSERT(error == SUCCESS, out_error, error, {});
        --ctx.idx;
      }
      break;

      case 'm':
      {
        auto parts = line.split(' ', scratch_arena.allocator);
        if (parts.size != 2 || parts[0] != "mtllib")
        {
          out_error = "Obj decoding error. Invalid mtllib.";
          return {};
        }
        auto base_file_path = String::make("assets/");
        auto mtl_file_path = base_file_path.append(parts[1], scratch_arena.allocator);
        obj_parse_mtl_file_(
          mtl_file_path.to_cstr(scratch_arena.allocator),
          *this,
          allocator,
          error
        );
        if (error != SUCCESS)
        {
          out_error = error;
          return {};
        }
      }
      break;
    }
  }

  return meshes.set(ctx.out);
}

void Assets::destroy_all()
{
  textures.destroy_all();
  materials.destroy_all();
  meshes.destroy_all();

  texture_handles.clear();
  material_handles.clear();
}

template <>
usize hash(const usize& value)
{
  return value;
}

template <>
usize hash(const ShaderHandle& value)
{
  return (usize) value;
}
