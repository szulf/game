#include "obj.h"

typedef struct ObjVertexHashMapEntry
{
  b32 set;
  Vertex key;
  u32 value;
} ObjVertexHashMapEntry;

typedef struct ObjVertexHashMap
{
  usize cap;
  ObjVertexHashMapEntry* entries;
} ObjVertexHashMap;

static ObjVertexHashMap
obj_vertex_hash_map_make(usize cap, Arena* arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  ObjVertexHashMap hash_map;
  hash_map.cap = cap;
  hash_map.entries = arena_alloc(arena, cap * sizeof(ObjVertexHashMapEntry), &error);
  mem_zero(hash_map.entries, cap * sizeof(ObjVertexHashMapEntry));
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, hash_map);
  return hash_map;
}

static u32
obj_vertex_hash_map_get(ObjVertexHashMap* map, Vertex* key, Error* err)
{
  usize idx = mem_hash_fnv_1a(key, sizeof(Vertex)) % map->cap;
  usize i = idx;
  b32 looped = false;
  while (true)
  {
    if (!map->entries[i].set)
    {
      *err = ERROR_NOT_FOUND;
      return (u32) -1;
    }

    if (mem_compare(&map->entries[i].key, key, sizeof(Vertex)))
    {
      *err = ERROR_SUCCESS;
      return map->entries[i].value;
    }
    ++i;
    if (i == map->cap)
    {
      i = 0;
      looped = true;
    }
    if (looped && i == idx)
    {
      *err = ERROR_OUT_OF_MEMORY;
      return (u32) -1;
    }
  }
  *err = ERROR_NOT_FOUND;
  return (u32) -1;
}

static void
obj_vertex_hash_map_set(ObjVertexHashMap* map, Vertex* key, u32 value, Error* err)
{
  usize idx = mem_hash_fnv_1a(key, sizeof(Vertex)) % map->cap;
  usize i = idx;
  b32 looped = false;
  while (true)
  {
    if (!map->entries[i].set)
    {
      map->entries[i].key = *key;
      map->entries[i].value = value;
      map->entries[i].set = true;
      *err = ERROR_SUCCESS;
      return;
    }
    ++i;
    if (i == map->cap)
    {
      i = 0;
      looped = true;
    }
    if (looped && i == idx)
    {
      *err = ERROR_OUT_OF_MEMORY;
      return;
    }
  }
}

typedef struct ObjContext
{
  StringArray lines;
  Vec3Array positions;
  Vec3Array normals;
  Vec2Array uvs;
  usize mesh_count;
  usize idx;
} ObjContext;

static void
obj_parse_mtl_file(String* mtl_file, Arena* temp_arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  StringArray lines = string_split(mtl_file, '\n', temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);

  b32 parsing = false;
  String* material_name = {0};
  for (usize i = 0; i < lines.len; ++i)
  {
    String* curr_line = &lines.items[i];
    switch (curr_line->data[0])
    {
      case 'n':
      {
        StringArray parts = string_split(curr_line, ' ', temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
        if (parts.items[0].len != 6) continue;
        if (!mem_compare(parts.items[0].data, "newmtl", 6)) continue;
        if (parts.len != 2) continue;
        if (!assets_material_exists(&parts.items[1]))
        {
          parsing = true;
          material_name = &parts.items[1];
        }
      } break;

      case 'm':
      {
        if (!parsing) continue;
        StringArray parts = string_split(curr_line, ' ', temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
        if (parts.items[0].len != 6) continue;
        if (!mem_compare(parts.items[0].data, "map_Kd", 6)) continue;
        if (parts.len != 2) continue;
        String texture_file_path = string_prepend(&parts.items[1], "assets/", temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
        Material mat = {0};
        if (!assets_texture_exists(&texture_file_path))
        {
          usize img_file_size;
          void* img_file = os_read_entire_file_bytes_read(texture_file_path.data, &img_file_size,
                                                          temp_arena, &error);
          ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
          Image img = image_decode_png(img_file, img_file_size, temp_arena, temp_arena, &error);
          ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
          mat.texture = texture_make(&img);
          assets_texture_set(&texture_file_path, &mat.texture);
        }
        else
        {
          Texture* texture = assets_texture_get(&texture_file_path, &error);
          ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
          mat.texture = *texture;
        }
        assets_material_set(material_name, &mat);
      } break;
    }
  }
}

static void
obj_parse_vertex(f32* points, u8 point_count, String* line, Arena* temp_arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  StringArray splits = string_split(line, ' ', temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
  ERROR_ASSERT(splits.len == point_count + 1u, *err, ERROR_OBJ_INVALID_DATA,);
  for (usize i = 0; i < point_count; ++i)
  {
    f32 point = string_parse_f32(&splits.items[i + 1], &error);
    ERROR_ASSERT(error == ERROR_SUCCESS, *err, error,);
    points[i] = point;
  }
}

static Mesh
obj_parse_object(ObjContext* ctx, Arena* temp_arena, Arena* perm_arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  Mesh mesh = {0};
  b32 parsing = true;
  for (usize i = ctx->idx; parsing && i < ctx->lines.len; ++i)
  {
    String* curr_line = &ctx->lines.items[i];
    switch (curr_line->data[0])
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
  ObjVertexHashMap vertex_hash_map = obj_vertex_hash_map_make(mesh.indices.cap * 2, temp_arena,
                                                              &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
  ARRAY_INIT(&mesh.vertices, mesh.indices.cap, perm_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
  ARRAY_INIT(&mesh.indices, mesh.indices.cap, perm_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);

  parsing = true;
  for (; parsing && ctx->idx < ctx->lines.len; ++ctx->idx)
  {
    String* curr_line = &ctx->lines.items[ctx->idx];
    switch (curr_line->data[0])
    {
      case 'f':
      {
        Error error = ERROR_SUCCESS;
        StringArray splits = string_split(curr_line, ' ', temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
        ERROR_ASSERT(splits.len == 4u, *err, ERROR_OBJ_INVALID_DATA, mesh);
        for (usize i = 0; i < 3; ++i)
        {
          StringArray parts = string_split(&splits.items[i + 1], '/', temp_arena, &error);
          ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
          ERROR_ASSERT(parts.len == 3u, *err, ERROR_OBJ_INVALID_DATA, mesh);
          u32 indices[3];
          for (usize j = 0; j < 3; ++j)
          {
            u32 idx = string_parse_u32(&parts.items[j], &error);
            ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
            indices[j] = idx - 1;
          }
          Vertex vertex;
          vertex.pos = ctx->positions.items[indices[0]];
          vertex.uv = ctx->uvs.items[indices[1]];
          vertex.normal = ctx->normals.items[indices[2]];
          u32 idx = obj_vertex_hash_map_get(&vertex_hash_map, &vertex, &error);
          if (error == ERROR_SUCCESS) ARRAY_PUSH(&mesh.indices, idx);
          else
          {
            obj_vertex_hash_map_set(&vertex_hash_map, &vertex, (u32) mesh.vertices.len, &error);
            ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
            ARRAY_PUSH(&mesh.indices, ((u32) mesh.vertices.len));
            ARRAY_PUSH(&mesh.vertices, vertex);
          }
        }
      } break;

      case 'v':
      {
        switch (curr_line->data[1])
        {
          case ' ':
          {
            f32 points[3];
            obj_parse_vertex(points, 3, curr_line, temp_arena, &error);
            ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
            ARRAY_PUSH(&ctx->positions, ((Vec3) {points[0], points[1], points[2]}));
          } break;

          case 'n':
          {
            f32 points[3];
            obj_parse_vertex(points, 3, curr_line, temp_arena, &error);
            ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
            ARRAY_PUSH(&ctx->normals, ((Vec3) {points[0], points[1], points[2]}));
          } break;

          case 't':
          {
            f32 points[2];
            obj_parse_vertex(points, 2, curr_line, temp_arena, &error);
            ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
            ARRAY_PUSH(&ctx->uvs, ((Vec2) {points[0], points[1]}));
          } break;
        }
      } break;

      case 'u':
      {
        StringArray parts = string_split(curr_line, ' ', temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
        ERROR_ASSERT(parts.items[0].len == 6, *err, error, mesh);
        ERROR_ASSERT(mem_compare(parts.items[0].data, "usemtl", 6), *err, ERROR_OBJ_INVALID_DATA,
                     mesh);
        Material* mat = assets_material_get(&parts.items[1], &error);
        // ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, mesh);
        if (mat)
        {
          mesh.material = *mat;
        }
      } break;

      case 'g':
      case 'o':
      {
        parsing = false;
        ctx->idx -= 2;
      } break;
    }
  }

  return mesh_make(&mesh.vertices, &mesh.indices, &mesh.material);
}

static Model
obj_parse(const char* path, Arena* temp_arena, Arena* perm_arena, Error* err)
{
  Error error = ERROR_SUCCESS;
  Model model = {0};
  model.model = mat4_make(1.0f);
  usize obj_file_size;
  char* obj_file_data = os_read_entire_file_bytes_read(path, &obj_file_size, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
  String obj_file = string_make_cstr_len(obj_file_data, obj_file_size);
  ObjContext ctx = {0};
  ctx.lines = string_split(&obj_file, '\n', temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);

  for (usize i = 0; i < ctx.lines.len; ++i)
  {
    String* curr_line = &ctx.lines.items[i];
    if (curr_line->len < 4) continue;
    switch (curr_line->data[0])
    {
      case 'g':
      case 'o':
      {
        ++ctx.mesh_count;
      } break;
      case 'v':
      {
        switch (curr_line->data[1])
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

  ARRAY_INIT(&model.meshes, ctx.mesh_count, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
  ARRAY_INIT(&ctx.positions, ctx.positions.cap, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
  ARRAY_INIT(&ctx.normals, ctx.normals.cap, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
  ARRAY_INIT(&ctx.uvs, ctx.uvs.cap, temp_arena, &error);
  ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);

  for (; ctx.idx < ctx.lines.len; ++ctx.idx)
  {
    String* curr_line = &ctx.lines.items[ctx.idx];
    switch (curr_line->data[0])
    {
      case 'o':
      case 'g':
      {
        ++ctx.idx;
        Mesh mesh = obj_parse_object(&ctx, temp_arena, perm_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
        ARRAY_PUSH(&model.meshes, mesh);
      } break;

      case 'm':
      {
        StringArray parts = string_split(curr_line, ' ', temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
        ERROR_ASSERT(parts.len == 2, *err, ERROR_OBJ_INVALID_DATA, model);
        ERROR_ASSERT(parts.items[0].len == 6, *err, ERROR_OBJ_INVALID_DATA, model);
        ERROR_ASSERT(mem_compare(parts.items[0].data, "mtllib", 6), *err, ERROR_OBJ_INVALID_DATA,
                     model);
        String mtl_file_path = string_prepend(&parts.items[1], "assets/", temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
        usize mtl_file_size;
        void* mtl_file_data = os_read_entire_file_bytes_read(mtl_file_path.data, &mtl_file_size,
                                                             temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
        String mtl_file = string_make_cstr_len(mtl_file_data, mtl_file_size);
        obj_parse_mtl_file(&mtl_file, temp_arena, &error);
        ERROR_ASSERT(error == ERROR_SUCCESS, *err, error, model);
      } break;
    }
  }

  return model;
}
