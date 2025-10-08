#include "game.h"

static void
log_(const char* file, usize line, const char* func, const char* fmt, ...)
{
  UNUSED(file);
  UNUSED(line);
  UNUSED(func);
  UNUSED(fmt);
}

static void
setup_default_keybinds()
{
  g_keybind_map[(usize) Key::LMB] = Action::ChangeScene;
  g_keybind_map[(usize) Key::Space] = Action::Move;
}

// TODO(szulf): delete this later
static Scene
setup_simple_scene(const char* obj_path, Arena* perm_arena, Arena* temp_arena)
{
  Error error = Error::Success;

  Mesh mesh = Mesh::from_obj(obj_path, temp_arena, perm_arena, &error);
  ASSERT(error == Error::Success, "couldnt load sphere mesh");

  Array<Mesh> meshes = Array<Mesh>::make(1, perm_arena, &error);
  ASSERT(error == Error::Success, "couldnt init meshes array");
  meshes.push(mesh);

  Model model = {};
  model.meshes = meshes;
  model.model = Mat4::make(1.0f);
  Array<Drawable> drawables = Array<Drawable>::make(1, perm_arena, &error);
  ASSERT(error == Error::Success, "couldnt init drawables array");
  drawables.push({model, Shader::Default});

  Scene scene = {};
  scene.drawables = drawables;
  scene.view = Mat4::make(1.0f);
  scene.proj = Mat4::make(1.0f);

  return scene;
}

static void
game_setup(Arena* perm_arena, Arena* temp_arena, State* state)
{
  Error error = Error::Success;

  setup_renderer();
  setup_default_keybinds();

  setup_shaders(temp_arena, &error);
  ASSERT(error == Error::Success, "couldnt initialize shaders");

  setup_global_materials(perm_arena, &error);
  ASSERT(error == Error::Success, "couldnt initialize global materials");

  Scene sphere_scene = setup_simple_scene("assets/sphere.obj", perm_arena, temp_arena);
  Scene cube_scene = setup_simple_scene("assets/cube.obj", perm_arena, temp_arena);
  Scene cone_scene = setup_simple_scene("assets/cone.obj", perm_arena, temp_arena);

  state->current_scene_idx = 0;
  state->scenes = Array<Scene>::make(3, perm_arena, &error);
  ASSERT(error == Error::Success, "couldnt init scenes array");
  state->scenes.push(sphere_scene);
  state->scenes.push(cube_scene);
  state->scenes.push(cone_scene);

  temp_arena->free_all();
}

static void
game_update(State* state, Input* input)
{
  static f32 degree = 0.0f;
  static f32 move = -3.0f;

  for (usize i = 0; i < input->input_events.len; ++i)
  {
    switch (g_keybind_map[(usize) input->input_events[i].key])
    {
      case Action::ChangeScene:
      {
        state->current_scene_idx += 1;
        state->current_scene_idx %= state->scenes.len;
      } break;
      case Action::Move:
      {
        move -= 1.0f;
      } break;
    };
  }
  input->input_events.len = 0;

  Scene* scene = &state->scenes[state->current_scene_idx];

  Vec3 translation_vec = {0.0f, 0.0f, move};
  scene->view.translate(&translation_vec);

  PlatformWindowDimensions dimensions = platform_get_window_dimensions();
  scene->proj = Mat4::perspective(radians(45.0f), (f32) dimensions.width / (f32) dimensions.height,
                                  0.1f, 100.0f);

  Vec3 rotate_vec = {1.0f, 1.0f, 0.0f};
  scene->drawables[0].model.rotate(degree, &rotate_vec);

  degree += 1.0f;
}

static void
game_render(State* state)
{
  Scene* curr_scene = &state->scenes[state->current_scene_idx];

  clear_screen();

  curr_scene->draw();
}

static void
game_get_sound(SoundBuffer* sound_buffer)
{
  static u32 sample_index = 0;

  for (u32 i = 0; i < sound_buffer->sample_count; i += 2)
  {
    f32 t = (f32) sample_index / (f32) sound_buffer->samples_per_second;
    f32 frequency = 440.0f;
    f32 amplitude = 0.25f;
    i16 sine_value = (i16) (fsin(2.0f * PI32 * t * frequency) * I16_MAX * amplitude);
    UNUSED(sine_value);

    ++sample_index;

    i16* left  = sound_buffer->memory + i;
    i16* right = sound_buffer->memory + i + 1;

    // *left  = sine_value;
    // *right = sine_value;
    *left  = 0;
    *right = 0;
  }
}
