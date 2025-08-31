#include "game.h"

static void
log_(const char* file, usize line, const char* func, const char* fmt, ...)
{
  (void) file;
  (void) line;
  (void) func;
  (void) fmt;
}

static Error
game_setup(Arena* perm_arena, Arena* temp_arena, GameState* state)
{
  Error shaders_err = setup_shaders(temp_arena);
  if (shaders_err != SUCCESS)
  {
    ASSERT(false, "couldnt initialize shaders");
    return shaders_err;
  }

  Mesh sphere_mesh = {};
  Error mesh_err = mesh_from_obj(&sphere_mesh, perm_arena, temp_arena, "assets/cube.obj");
  if (mesh_err != SUCCESS)
  {
    ASSERT(false, "couldnt load sphere mesh");
    return mesh_err;
  }
  MeshArray sphere_meshes = {};
  ARRAY_INIT(&sphere_meshes, perm_arena, 1);
  ARRAY_PUSH(&sphere_meshes, sphere_mesh);
  Model sphere_model = {};
  sphere_model.meshes = sphere_meshes;
  mat4_init(&sphere_model.model, 1.0f);
  DrawableArray sphere_drawables = {};
  ARRAY_INIT(&sphere_drawables, perm_arena, 1);
  ARRAY_PUSH(&sphere_drawables, ((Drawable) {sphere_model, SHADER_DEFAULT}));

  Scene scene = {};
  scene.drawables = sphere_drawables;
  mat4_init(&scene.view, 1.0f);
  mat4_init(&scene.proj, 1.0f);

  state->current_scene_idx = 0;
  ARRAY_INIT(&state->scenes, perm_arena, 1);
  ARRAY_PUSH(&state->scenes, scene);

  arena_free_all(temp_arena);
  return SUCCESS;
}

static void
game_update(GameState* state)
{
  static f32 degree = 0.0f;

  Scene* scene = &state->scenes.items[state->current_scene_idx];

  Vec3 translation_vec = {0.0f, 0.0f, -3.0f};
  mat4_translate(&scene->view, &translation_vec);

  WindowDimensions dimensions = get_window_dimensions();
  scene->proj = perspective(radians(45.0f), (f32) dimensions.width / (f32) dimensions.height,
                            0.1f, 100.0f);

  static u64 last_ms = 0;
  if (get_ms() - last_ms > 3000)
  {
    state->current_scene_idx += 1;
    state->current_scene_idx %= state->scenes.len;
    last_ms = get_ms();
  }

  Vec3 rotate_vec = {1.0f, 1.0f, 0.0f};
  model_rotate(&scene->drawables.items[0].model, degree, &rotate_vec);

  degree += 1.0f;
}

static void
game_render(GameState* state)
{
  Scene* curr_scene = &state->scenes.items[state->current_scene_idx];

  clear_screen();

  scene_draw(curr_scene);
}

static void
game_get_sound(GameSoundBuffer* sound_buffer)
{
  static u32 sample_index = 0;

  for (u32 i = 0; i < sound_buffer->sample_count; i += 2)
  {
    f32 t = (f32) sample_index / (f32) sound_buffer->samples_per_second;
    f32 frequency = 440.0f;
    f32 amplitude = 0.25f;
    s16 sine_value = (s16) (sin(2.0f * PI32 * t * frequency) * S16_MAX * amplitude);

    ++sample_index;

    s16* left  = sound_buffer->memory + i;
    s16* right = sound_buffer->memory + i + 1;

    *left  = sine_value;
    *right = sine_value;
  }
}
