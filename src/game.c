#include "game.h"

// TODO(szulf): implement this?
static void
log_(const char* file, usize line, const char* func, const char* fmt, ...)
{
  (void) file;
  (void) line;
  (void) func;
  (void) fmt;
}

// TODO(szulf): delete this later
static Scene
setup_simple_scene(const char* obj_path, Arena* perm_arena, Arena* temp_arena)
{
  Error error = ERROR_SUCCESS;

  Mesh mesh = mesh_from_obj(perm_arena, temp_arena, obj_path, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt load sphere mesh");

  MeshArray meshes = {};
  ARRAY_INIT(&meshes, perm_arena, 1, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt init meshes array");
  ARRAY_PUSH(&meshes, mesh);

  Model model = {};
  model.meshes = meshes;
  mat4_init(&model.model, 1.0f);
  DrawableArray drawables = {};
  ARRAY_INIT(&drawables, perm_arena, 1, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt init drawables array");
  ARRAY_PUSH(&drawables, ((Drawable) {model, SHADER_DEFAULT}));

  Scene scene = {};
  scene.drawables = drawables;
  mat4_init(&scene.view, 1.0f);
  mat4_init(&scene.proj, 1.0f);

  return scene;
}

static void
game_setup(Arena* perm_arena, Arena* temp_arena, GameState* state)
{
  Error error = ERROR_SUCCESS;

  setup_shaders(temp_arena, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt initialize shaders");

  Scene sphere_scene = setup_simple_scene("assets/sphere.obj", perm_arena, temp_arena);
  Scene cube_scene = setup_simple_scene("assets/cube.obj", perm_arena, temp_arena);

  state->current_scene_idx = 0;
  ARRAY_INIT(&state->scenes, perm_arena, 2, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt init scenes array");
  ARRAY_PUSH(&state->scenes, sphere_scene);
  ARRAY_PUSH(&state->scenes, cube_scene);

  arena_free_all(temp_arena);
}

Action keybind_map[] =
{
  [KEY_LMB] = ACTION_CHANGE_SCENE,
  [KEY_SPACE] = ACTION_MOVE,
};

static void
game_update(GameState* state, GameInput* input)
{
  static f32 degree = 0.0f;
  static f32 move = -3.0f;

  for (usize i = 0; i < input->input_events.len; ++i)
  {
    switch (keybind_map[input->input_events.items[i].key])
    {
      case ACTION_CHANGE_SCENE:
      {
        state->current_scene_idx += 1;
        state->current_scene_idx %= state->scenes.len;
      } break;
      case ACTION_MOVE:
      {
        move -= 1.0f;
      } break;
    };
  }
  input->input_events.len = 0;

  Scene* scene = &state->scenes.items[state->current_scene_idx];

  Vec3 translation_vec = {0.0f, 0.0f, move};
  mat4_translate(&scene->view, &translation_vec);

  WindowDimensions dimensions = get_window_dimensions();
  scene->proj = perspective(radians(45.0f), (f32) dimensions.width / (f32) dimensions.height,
                            0.1f, 100.0f);

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
