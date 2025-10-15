#include "game.h"

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

  Model model = obj_parse(obj_path, temp_arena, perm_arena, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt load obj model");

  RenderableArray renderables = {0};
  ARRAY_INIT(&renderables, 1, perm_arena, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt init renderables array");
  ARRAY_PUSH(&renderables, ((Renderable) {model, SHADER_DEFAULT}));

  Scene scene = {0};
  scene.renderables = renderables;
  scene.view = mat4_make(1.0f);
  scene.proj = mat4_make(1.0f);

  return scene;
}

static void
setup(State* state, Arena* temp_arena, Arena* perm_arena)
{
  Error error = ERROR_SUCCESS;

  setup_renderer();

  setup_shaders(temp_arena, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt initialize shaders");

  setup_assets(perm_arena, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt initialize assets");

  Scene backpack_scene = setup_simple_scene("assets/backpack.obj", perm_arena, temp_arena);
  Scene sphere_scene = setup_simple_scene("assets/sphere.obj", perm_arena, temp_arena);
  Scene cube_scene = setup_simple_scene("assets/cube.obj", perm_arena, temp_arena);
  Scene cone_scene = setup_simple_scene("assets/cone.obj", perm_arena, temp_arena);

  state->current_scene_idx = 0;
  ARRAY_INIT(&state->scenes, 4, perm_arena, &error);
  ASSERT(error == ERROR_SUCCESS, "couldnt init scenes array");
  ARRAY_PUSH(&state->scenes, backpack_scene);
  ARRAY_PUSH(&state->scenes, sphere_scene);
  ARRAY_PUSH(&state->scenes, cube_scene);
  ARRAY_PUSH(&state->scenes, cone_scene);

  arena_free_all(temp_arena);
}

static void
update(State* state, Input* input)
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

  WindowDimensions dimensions = os_get_window_dimensions();
  scene->proj = mat4_perspective(radians(45.0f), (f32) dimensions.width / (f32) dimensions.height,
                            0.1f, 100.0f);

  Vec3 rotate_vec = {1.0f, 1.0f, 0.0f};
  model_rotate(&scene->renderables.items[0].model, degree, &rotate_vec);


  degree += 1.0f;
}

static void
render(State* state)
{
  Scene* curr_scene = &state->scenes.items[state->current_scene_idx];

  clear_screen();

  scene_draw(curr_scene);
}

static void
get_sound(SoundBuffer* sound_buffer)
{
  static u32 sample_index = 0;

  for (u32 i = 0; i < sound_buffer->sample_count; i += 2)
  {
    f32 t = (f32) sample_index / (f32) sound_buffer->samples_per_second;
    f32 frequency = 440.0f;
    f32 amplitude = 0.25f;
    i16 sine_value = (i16) (sin(2.0f * PI32 * t * frequency) * I16_MAX * amplitude);
    (void) sine_value;

    ++sample_index;

    i16* left  = sound_buffer->memory + i;
    i16* right = sound_buffer->memory + i + 1;

    // *left  = sine_value;
    // *right = sine_value;
    *left  = 0;
    *right = 0;
  }
}
