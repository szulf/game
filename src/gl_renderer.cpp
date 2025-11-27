#include "gl_renderer.h"

template <>
b8 equal(const u64* v1, const u64* v2)
{
  return *v1 == *v2;
}

template <>
usize hash(const u64* value)
{
  return (usize) *value;
}

static u32 shader_setup(const char* path, ShaderType shader_type, Error* out_error)
{
  auto scratch_arena = scratch_arena_get();
  defer(scratch_arena_release(&scratch_arena));
  usize file_size;
  void* file = platform.read_file(path, scratch_arena.allocator, &file_size);

  u32 shader;
  switch (shader_type)
  {
    case SHADER_TYPE_VERTEX:
    {
      shader = gl.glCreateShader(GL_VERTEX_SHADER);
    }
    break;

    case SHADER_TYPE_FRAGMENT:
    {
      shader = gl.glCreateShader(GL_FRAGMENT_SHADER);
    }
    break;
  }

  auto shader_str = string_make_len((const char*) file, file_size);
  auto* shader_src = string_to_cstr(&shader_str, scratch_arena.allocator);
  gl.glShaderSource(shader, 1, &shader_src, nullptr);
  gl.glCompileShader(shader);
  GLint compiled;
  gl.glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    gl.glGetShaderInfoLog(shader, 1024, &log_length, message);
    switch (shader_type)
    {
      case SHADER_TYPE_VERTEX:
      {
        print("vertex shader compilation failed with message:\n%s\n", message);
        *out_error = SHADER_ERROR_COMPILATION;
        return (u32) -1;
      }
      break;
      case SHADER_TYPE_FRAGMENT:
      {
        print("fragment shader compilation failed with message:\n%s\n", message);
        *out_error = SHADER_ERROR_COMPILATION;
        return (u32) -1;
      }
      break;
    }
  }
  return shader;
}

static u32 shader_link(u32 vertex_shader, u32 fragment_shader, Error* out_error)
{
  GLuint program = gl.glCreateProgram();
  gl.glAttachShader(program, vertex_shader);
  gl.glAttachShader(program, fragment_shader);
  gl.glLinkProgram(program);

  gl.glDeleteShader(vertex_shader);
  gl.glDeleteShader(fragment_shader);

  GLint program_linked;
  gl.glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
  if (program_linked != GL_TRUE)
  {
    GLsizei log_length = 0;
    GLchar message[1024];
    gl.glGetProgramInfoLog(program, 1024, &log_length, message);
    print("failed to link shaders with message:\n%s\n", message);
    *out_error = SHADER_ERROR_LINKING;
    return (u32) -1;
  }
  return program;
}

void shader_init(u32* shader_map)
{
  Error error = SUCCESS;
  // TODO(szulf): should this function assert?
  {
    u32 v_shader = shader_setup("shaders/shader.vert", SHADER_TYPE_VERTEX, &error);
    ASSERT(error == SUCCESS, "failed to create vertex shader");
    u32 f_shader = shader_setup("shaders/shader.frag", SHADER_TYPE_FRAGMENT, &error);
    ASSERT(error == SUCCESS, "failed to create fragment shader");
    u32 shader = shader_link(v_shader, f_shader, &error);
    ASSERT(error == SUCCESS, "failed to link shaders");
    shader_map[SHADER_DEFAULT] = shader;
  }
}

template <>
usize hash(const Vertex* v)
{
  usize h = U64_FNV_OFFSET;
  mem_hash_fnv1(&h, &v->position.x, sizeof(v->position.x));
  mem_hash_fnv1(&h, &v->position.y, sizeof(v->position.y));
  mem_hash_fnv1(&h, &v->position.z, sizeof(v->position.z));
  mem_hash_fnv1(&h, &v->normal.x, sizeof(v->normal.x));
  mem_hash_fnv1(&h, &v->normal.y, sizeof(v->normal.y));
  mem_hash_fnv1(&h, &v->normal.z, sizeof(v->normal.z));
  mem_hash_fnv1(&h, &v->uv.x, sizeof(v->uv.x));
  mem_hash_fnv1(&h, &v->uv.y, sizeof(v->uv.y));
  return h;
}

template <>
b8 equal(const Vertex* va, const Vertex* vb)
{
  return equal(&va->position, &vb->position) && equal(&va->normal, &vb->normal) && equal(&va->uv, &vb->uv);
}

Texture texture_make(const Image* img)
{
  Texture out = {};
  gl.glGenTextures(1, &out.id);
  gl.glBindTexture(GL_TEXTURE_2D, out.id);

  // TODO(szulf): do i want to customize these
  gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  gl.glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA8,
    (GLsizei) img->width,
    (GLsizei) img->height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    img->data
  );
  gl.glGenerateMipmap(GL_TEXTURE_2D);
  return out;
}

Mesh mesh_make(const Array<Vertex>* vertices, const Array<u32>* indices, MaterialHandle material)
{
  Mesh out = {};
  out.vertices = *vertices;
  out.indices = *indices;
  out.material = material;

  gl.glGenVertexArrays(1, &out.vao);
  gl.glBindVertexArray(out.vao);

  gl.glGenBuffers(1, &out.vbo);
  gl.glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
  gl.glBufferData(GL_ARRAY_BUFFER, (GLsizei) (out.vertices.size * sizeof(Vertex)), out.vertices.data, GL_STATIC_DRAW);
  gl.glGenBuffers(1, &out.ebo);
  gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, out.ebo);
  gl.glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    (GLsizei) (out.indices.size * sizeof(u32)),
    out.indices.data,
    GL_STATIC_DRAW
  );

  gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
  gl.glEnableVertexAttribArray(0);
  gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (3 * sizeof(float)));
  gl.glEnableVertexAttribArray(1);
  gl.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (6 * sizeof(float)));
  gl.glEnableVertexAttribArray(2);

  return out;
}

static void camera_update_vectors(Camera* camera)
{
  camera->front.x = f32_cos(f32_radians(camera->yaw)) * f32_cos(f32_radians(camera->pitch));
  camera->front.y = f32_sin(f32_radians(camera->pitch));
  camera->front.z = f32_sin(f32_radians(camera->yaw)) * f32_cos(f32_radians(camera->pitch));
  camera->front = vec3_normalize(&camera->front);
  auto r = vec3_cross(&camera->front, &CAMERA_WORLD_UP);
  camera->right = vec3_normalize(&r);
  auto u = vec3_cross(&camera->right, &camera->front);
  camera->up = vec3_normalize(&u);
}

Camera camera_make(const Vec3* pos, u32 width, u32 height)
{
  Camera out = {};
  out.pos = *pos;

  out.front = {0.0f, 0.0f, -1.0f};
  out.yaw = -90.0f;
  out.fov = 45.0f;
  out.near_plane = 0.1f;
  out.far_plane = 1000.0f;

  out.viewport_width = width;
  out.viewport_height = height;

  camera_update_vectors(&out);
  return out;
}

Mat4 camera_look_at(const Camera* camera)
{
  Mat4 look_at = {};

  Vec3 back = vec3_negate(&camera->front);

  look_at.data[0] = camera->right.x;
  look_at.data[1] = camera->right.y;
  look_at.data[2] = camera->right.z;

  look_at.data[4] = camera->up.x;
  look_at.data[5] = camera->up.y;
  look_at.data[6] = camera->up.z;

  look_at.data[8] = back.x;
  look_at.data[9] = back.y;
  look_at.data[10] = back.z;

  look_at.data[12] = camera->pos.x;
  look_at.data[13] = camera->pos.y;
  look_at.data[14] = camera->pos.z;

  look_at.data[15] = 1.0f;

  return look_at;
}

Mat4 camera_projection(const Camera* camera)
{
  return mat4_perspective(
    camera->fov,
    (f32) camera->viewport_width / (f32) camera->viewport_height,
    camera->near_plane,
    camera->far_plane
  );
}

static Array<DrawCall>* renderer_queue_instance;

void renderer_init()
{
  gl.glEnable(GL_DEPTH_TEST);
}

void renderer_clear_screen()
{
  gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_queue_draw_call(const DrawCall* draw_call)
{
  // TODO(szulf): for now just pushing into the array, later actually try to sort it in place
  array_push(renderer_queue_instance, draw_call);
}

void renderer_draw()
{
  for (usize draw_call_idx = 0; draw_call_idx < renderer_queue_instance->size; ++draw_call_idx)
  {
    auto* draw_call = array_get(renderer_queue_instance, draw_call_idx);
    auto* model = assets_get_model(draw_call->model_handle);

    for (usize mesh_idx = 0; mesh_idx < model->meshes.size; ++mesh_idx)
    {
      auto* mesh = assets_get_mesh(*array_get(&model->meshes, mesh_idx));
      auto* material = assets_get_material(mesh->material);

      gl.glUseProgram(shader_map_instance[material->shader]);
      gl.glUniformMatrix4fv(
        gl.glGetUniformLocation(shader_map_instance[material->shader], "model"),
        1,
        false,
        draw_call->model.data
      );
      gl.glUniformMatrix4fv(
        gl.glGetUniformLocation(shader_map_instance[material->shader], "view"),
        1,
        false,
        draw_call->view.data
      );
      gl.glUniformMatrix4fv(
        gl.glGetUniformLocation(shader_map_instance[material->shader], "proj"),
        1,
        false,
        draw_call->projection.data
      );

      gl.glActiveTexture(GL_TEXTURE0);
      auto texture_id = assets_get_texture(assets_get_material(mesh->material)->texture)->id;
      gl.glBindTexture(GL_TEXTURE_2D, texture_id);
      gl.glUniform1i(gl.glGetUniformLocation(shader_map_instance[material->shader], "sampler"), 0);
      gl.glBindVertexArray(mesh->vao);
      gl.glDrawElements(GL_TRIANGLES, (GLsizei) mesh->indices.size, GL_UNSIGNED_INT, nullptr);
    }
  }
  renderer_queue_instance->size = 0;
}
