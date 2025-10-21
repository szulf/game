#include "obj.h"

namespace obj
{

struct VertexHashMapEntry
{
  b32 set;
  Vertex key;
  u32 value;
};

struct VertexHashMap
{
  usize cap;
  VertexHashMapEntry* entries;

  static VertexHashMap make(usize cap, mem::Arena& arena, Error* err);

  u32 get(const Vertex& key, Error* err) const;
  void set(const Vertex& key, u32 value, Error* err);
};

VertexHashMap
VertexHashMap::make(usize cap, mem::Arena& arena, Error* err)
{
  Error error = Error::SUCCESS;
  VertexHashMap hash_map;
  hash_map.cap = cap;
  hash_map.entries = (VertexHashMapEntry*)
    arena.alloc(cap * sizeof(VertexHashMapEntry), &error);
  mem::zero(hash_map.entries, cap * sizeof(VertexHashMapEntry));
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, hash_map);
  return hash_map;
}

u32
VertexHashMap::get(const Vertex& key, Error* err) const
{
  usize idx = mem::hash_fnv_1a(&key, sizeof(Vertex)) % cap;
  usize i = idx;
  b32 looped = false;
  while (true)
  {
    if (!entries[i].set)
    {
      *err = Error::NOT_FOUND;
      return (u32) -1;
    }

    if (mem::compare(&entries[i].key, &key, sizeof(Vertex)))
    {
      *err = Error::SUCCESS;
      return entries[i].value;
    }
    ++i;
    if (i == cap)
    {
      i = 0;
      looped = true;
    }
    if (looped && i == idx)
    {
      *err = Error::OUT_OF_MEMORY;
      return (u32) -1;
    }
  }
  *err = Error::NOT_FOUND;
  return (u32) -1;
}

void
VertexHashMap::set(const Vertex& key, u32 value, Error* err)
{
  usize idx = mem::hash_fnv_1a(&key, sizeof(Vertex)) % cap;
  usize i = idx;
  b32 looped = false;
  while (true)
  {
    if (!entries[i].set)
    {
      entries[i].key = key;
      entries[i].value = value;
      entries[i].set = true;
      *err = Error::SUCCESS;
      return;
    }
    ++i;
    if (i == cap)
    {
      i = 0;
      looped = true;
    }
    if (looped && i == idx)
    {
      *err = Error::OUT_OF_MEMORY;
      return;
    }
  }
}

struct Context
{
  Array<String> lines;
  Array<Vec3> positions;
  Array<Vec3> normals;
  Array<Vec2> uvs;
  usize mesh_count;
  usize idx;
};

static void
parse_mtl_file(const String& mtl_file, mem::Arena& temp_arena, Error* err)
{
  Error error = Error::SUCCESS;
  Array<String> lines = mtl_file.split('\n', temp_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error,);

  b32 parsing = false;
  String material_name = {};
  for (const auto& curr_line : lines)
  {
    switch (curr_line.data[0])
    {
      case 'n':
      {
        Array<String> parts = curr_line.split(' ', temp_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
        if (parts[0].len != 6) continue;
        if (!mem::compare(parts[0].data, "newmtl", 6)) continue;
        if (parts.len != 2) continue;
        if (!assets::material_exists(parts[1]))
        {
          parsing = true;
          material_name = parts[1];
        }
      } break;

      case 'm':
      {
        if (!parsing) continue;
        Array<String> parts = curr_line.split(' ', temp_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
        if (parts[0].len != 6) continue;
        if (!mem::compare(parts[0].data, "map_Kd", 6)) continue;
        if (parts.len != 2) continue;
        String texture_file_path = parts[1].prepend("assets/", temp_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
        Material mat = {0};
        if (!assets::texture_exists(texture_file_path))
        {
          auto img = png::decode(texture_file_path.data, temp_arena, temp_arena, &error);
          ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
          mat.texture = Texture::make(img);
          assets::texture_set(texture_file_path, mat.texture);
        }
        else
        {
          Texture* texture = assets::texture_get(texture_file_path, &error);
          ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
          mat.texture = *texture;
        }
        assets::material_set(material_name, mat);
      } break;
    }
  }
}

static void
parse_vertex(f32* points, u8 point_count, const String& line, mem::Arena& temp_arena, Error* err)
{
  Error error = Error::SUCCESS;
  Array<String> splits = line.split(' ', temp_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
  ERROR_ASSERT(splits.len == point_count + 1u, *err, Error::OBJ_INVALID_DATA,);
  for (usize i = 0; i < point_count; ++i)
  {
    f32 point = splits[i + 1].parse<f32>(&error);
    ERROR_ASSERT(error == Error::SUCCESS, *err, error,);
    points[i] = point;
  }
}

static Mesh
parse_object(Context& ctx, mem::Arena& temp_arena, mem::Arena& perm_arena, Error* err)
{
  Error error = Error::SUCCESS;
  Mesh mesh = {};
  b32 parsing = true;
  for (usize i = ctx.idx; parsing && i < ctx.lines.len; ++i)
  {
    String& curr_line = ctx.lines[i];
    switch (curr_line.data[0])
    {
      case 'f':
      {
        ++mesh.indices.cap;
      } break;
      case 'o':
      case 'g':
      {
        parsing = false;
      } break;
    }
  }
  mesh.indices.cap *= 3;
  VertexHashMap vertex_hash_map = VertexHashMap::make(mesh.indices.cap * 2, temp_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
  // TODO(szulf): wasting a bunch of memory here i think
  mesh.vertices = Array<Vertex>::make(mesh.indices.cap, perm_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
  mesh.indices = Array<u32>::make(mesh.indices.cap, perm_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);

  parsing = true;
  for (; parsing && ctx.idx < ctx.lines.len; ++ctx.idx)
  {
    String& curr_line = ctx.lines[ctx.idx];
    switch (curr_line.data[0])
    {
      case 'f':
      {
        Error error = Error::SUCCESS;
        Array<String> splits = curr_line.split(' ', temp_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
        ERROR_ASSERT(splits.len == 4u, *err, Error::OBJ_INVALID_DATA, mesh);
        for (usize i = 0; i < 3; ++i)
        {
          Array<String> parts = splits[i + 1].split('/', temp_arena, &error);
          ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
          ERROR_ASSERT(parts.len == 3u, *err, Error::OBJ_INVALID_DATA, mesh);
          u32 indices[3];
          for (usize j = 0; j < 3; ++j)
          {
            u32 idx = parts[j].parse<u32>(&error);
            ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
            indices[j] = idx - 1;
          }
          Vertex vertex;
          vertex.pos = ctx.positions[indices[0]];
          vertex.uv = ctx.uvs[indices[1]];
          vertex.normal = ctx.normals[indices[2]];
          u32 idx = vertex_hash_map.get(vertex, &error);
          if (error == Error::SUCCESS) mesh.indices.push(idx);
          else
          {
            vertex_hash_map.set(vertex, (u32) mesh.vertices.len, &error);
            ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
            mesh.indices.push((u32) mesh.vertices.len);
            mesh.vertices.push(vertex);
          }
        }
      } break;

      case 'v':
      {
        switch (curr_line.data[1])
        {
          case ' ':
          {
            f32 points[3];
            parse_vertex(points, 3, curr_line, temp_arena, &error);
            ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
            ctx.positions.push({points[0], points[1], points[2]});
          } break;

          case 'n':
          {
            f32 points[3];
            parse_vertex(points, 3, curr_line, temp_arena, &error);
            ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
            ctx.normals.push({points[0], points[1], points[2]});
          } break;

          case 't':
          {
            f32 points[2];
            parse_vertex(points, 2, curr_line, temp_arena, &error);
            ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
            ctx.uvs.push({points[0], points[1]});
          } break;
        }
      } break;

      case 'u':
      {
        Array<String> parts = curr_line.split(' ', temp_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
        ERROR_ASSERT(parts[0].len == 6, *err, error, mesh);
        ERROR_ASSERT(mem::compare(parts[0].data, "usemtl", 6), *err, Error::OBJ_INVALID_DATA,
                     mesh);
        Material* mat = assets::material_get(parts[1], &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error, mesh);
        if (mat)
        {
          mesh.material = *mat;
        }
      } break;

      case 'g':
      case 'o':
      {
        parsing = false;
        ctx.idx -= 2;
      } break;
    }
  }

  return Mesh::make(mesh.vertices, mesh.indices, mesh.material);
}

static Model
parse(const char* path, mem::Arena& temp_arena, mem::Arena& perm_arena, Error* err)
{
  Error error = Error::SUCCESS;
  Model model = {};
  model.mat = Mat4::make(1.0f);
  usize obj_file_size;
  auto* obj_file_data = (const char*) os::read_entire_file(path, temp_arena, &error, &obj_file_size);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
  String obj_file = String::make(obj_file_data, obj_file_size);
  Context ctx = {};
  ctx.lines = obj_file.split('\n', temp_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);

  for (const auto& curr_line : ctx.lines)
  {
    if (curr_line.len < 4) continue;
    switch (curr_line.data[0])
    {
      case 'g':
      case 'o':
      {
        ++ctx.mesh_count;
      } break;
      case 'v':
      {
        switch (curr_line.data[1])
        {
          case ' ':
          {
            ++ctx.positions.cap;
          } break;
          case 'n':
          {
            ++ctx.normals.cap;
          } break;
          case 't':
          {
            ++ctx.uvs.cap;
          } break;
        }
      }
    }
  }

  model.meshes = Array<Mesh>::make(ctx.mesh_count, temp_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
  ctx.positions = Array<Vec3>::make(ctx.positions.cap, temp_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
  ctx.normals = Array<Vec3>::make(ctx.normals.cap, temp_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
  ctx.uvs = Array<Vec2>::make(ctx.uvs.cap, temp_arena, &error);
  ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);

  for (; ctx.idx < ctx.lines.len; ++ctx.idx)
  {
    String& curr_line = ctx.lines[ctx.idx];
    switch (curr_line.data[0])
    {
      case 'o':
      case 'g':
      {
        ++ctx.idx;
        Mesh mesh = parse_object(ctx, temp_arena, perm_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
        model.meshes.push(mesh);
      } break;

      case 'm':
      {
        Array<String> parts = curr_line.split(' ', temp_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
        ERROR_ASSERT(parts.len == 2, *err, Error::OBJ_INVALID_DATA, model);
        ERROR_ASSERT(parts[0].len == 6, *err, Error::OBJ_INVALID_DATA, model);
        ERROR_ASSERT(mem::compare(parts[0].data, "mtllib", 6), *err, Error::OBJ_INVALID_DATA,
                     model);
        String mtl_file_path = parts[1].prepend("assets/", temp_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
        usize mtl_file_size;
        void* mtl_file_data = os::read_entire_file(mtl_file_path.data, temp_arena, &error,
                                                   &mtl_file_size);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
        String mtl_file = String::make((const char*) mtl_file_data, mtl_file_size);
        parse_mtl_file(mtl_file, temp_arena, &error);
        ERROR_ASSERT(error == Error::SUCCESS, *err, error, model);
      } break;
    }
  }

  return model;
}

}

