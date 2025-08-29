#include "game.h"

template <typename... Args>
void log_(const char* file, i64 line, const char* func, const char* fmt, const Args&... args)
{
  char s_buf[1024]{};
  String s{};
  s.data = s_buf;
  s.cap = 1024;
  s.len = 0;

  format(s, fmt, args...);

  char buffer_buf[1024]{};
  String buffer{};
  buffer.data = buffer_buf;
  buffer.cap = 1024;
  buffer.len = 0;
  format(buffer, "[{}:{} ({})] {}", file, line, func, s);

  platform::print(buffer.data);
}

void write(String& buf, const char* str, usize size)
{
  ASSERT(size <= buf.cap - buf.len - 1, "buf size exceeded");
  mem::copy(buf.data + buf.len, str, size);
  buf.len += size;
}

void write_val(String& buf, const char* val);
void write_val(String& buf, const String& val);
void write_val(String& buf, i64 val);
void write_val(String& buf, usize val);
void write_val(String& buf, u32 val);
void write_val(String& buf, const game::Mesh& val);
void write_val(String& buf, const game::Vertex& val);
void write_val(String& buf, const math::Vec3& val);
template <typename T>
void write_val(String& buf, const Array<T>& val);
void write_val(String& buf, f32 val);
void write_val(String& buf, char val);
template <typename T>
void write_val(String& buf, const Result<T>& val);
void write_val(String& buf, Error val);
void write_val(String& buf, bool val);

void write_val(String& buf, const char* val)
{
  write(buf, val, c_str_len(val));
}

void write_val(String& buf, const String& val)
{
  write(buf, val.data, val.len);
}

void write_val(String& buf, i64 val)
{
  if (val == 0)
  {
    write(buf, "0", 1);
    return;
  }

  if (val < 0)
  {
    write(buf, "-", 1);
    val = -val;
  }

  char digits[20]{};
  usize i = sizeof(digits) - 1;
  while (val > 0)
  {
    digits[i--] = (val % 10) + '0';
    val /= 10;
  }
  write(buf, digits + i + 1, sizeof(digits) - i - 1);
}

void write_val(String& buf, u32 val)
{
  write_val(buf, static_cast<usize>(val));
}

void write_val(String& buf, usize val)
{
  if (val == 0)
  {
    write(buf, "0", 1);
    return;
  }

  char digits[20]{};
  usize i = sizeof(digits) - 1;
  while (val > 0)
  {
    digits[i--] = (val % 10) + '0';
    val /= 10;
  }
  write(buf, digits + i + 1, sizeof(digits) - i - 1);
}

void write_val(String& buf, const game::Mesh& val)
{
  write(buf, "Mesh{ vertices: ", 16);
  write_val(buf, val.vertices);
  write(buf, ", indices: ", 11);
  write_val(buf, val.indices);
  write(buf, ", vao: ", 7);
  write_val(buf, static_cast<i64>(val.vao));
  write(buf, ", vbo: ", 7);
  write_val(buf, static_cast<i64>(val.vbo));
  write(buf, ", ebo: ", 7);
  write_val(buf, static_cast<i64>(val.ebo));
  write(buf, " }", 2);
}

void write_val(String& buf, const game::Vertex& val)
{
  write(buf, "Vertex{ pos: ", 13);
  write_val(buf, val.pos);
  write(buf, " }", 2);
}

void write_val(String& buf, const math::Vec3& val)
{
  write(buf, "Vec3{ x: ", 9);
  write_val(buf, val.x);
  write(buf, ", y: ", 5);
  write_val(buf, val.y);
  write(buf, ", z: ", 5);
  write_val(buf, val.z);
  write(buf, " }", 2);
}

template <typename T>
void write_val(String& buf, const Array<T>& val)
{
  write(buf, "[ ", 2);
  for (const auto& v : val)
  {
    write_val(buf, v);
    write(buf, ", ", 2);
  }
  write(buf, " ]", 2);
}

void write_val(String& buf, char val)
{
  write(buf, &val, 1);
}

template <typename T>
void write_val(String& buf, const Result<T>& val)
{
  write(buf, "Result{ has_error: ", 19);
  write_val(buf, static_cast<bool>(val.has_error));
  if (!val.has_error)
  {
    write(buf, ", val: ", 7);
    write_val(buf, val.val);
  }
  else
  {
    write(buf, ", err: ", 7);
    write_val(buf, val.err);
  }
  write(buf, " }", 2);
}

void write_val(String& buf, Error val)
{
  switch (val)
  {
    case Error::OutOfMemory: {
      write(buf, "OutOfMemory", 11);
      break;
    }
    case Error::InvalidParameter: {
      write(buf, "InvalidParameter", 16);
      break;
    }
    case Error::FileReadingError: {
      write(buf, "FileReadingError", 16);
      break;
    }
    case Error::ShaderCompilation: {
      write(buf, "ShaderCompilation", 17);
      break;
    }
    case Error::ShaderLinking: {
      write(buf, "ShaderLinking", 13);
      break;
    }
    case Error::NotFound: {
      write(buf, "NotFound", 8);
      break;
    }
  }
}

void write_val(String& buf, bool val)
{
  if (val)
  {
    write(buf, "true", 4);
  }
  else
  {
    write(buf, "false", 5);
  }
}

bool next_hole(String& buf, const char*& fmt)
{
  auto prefix = fmt;
  while (*fmt)
  {
    if (*fmt == '{')
    {
      usize len = static_cast<usize>(fmt - prefix);
      ++fmt;
      if (*fmt == '}')
      {
        ++fmt;
        write(buf, prefix, len);
        return true;
      }
      if (*fmt == '{')
      {
        write(buf, prefix, len);
        prefix = fmt;
        ++fmt;
      }
    }
    ++fmt;
  }
  write(buf, prefix, static_cast<usize>(fmt - prefix));
  return false;
}

template <typename T>
void format_val(String& buf, const char*& fmt, const T& val)
{
  if (next_hole(buf, fmt))
  {
    write_val(buf, val);
  }
}

template <typename... Args>
void format(String& buf, const char* fmt, const Args&... args)
{
  (format_val(buf, fmt, args), ...);
  while (next_hole(buf, fmt)) {}
}

namespace game
{

void setup(mem::Arena& perm_arena, mem::Arena& temp_arena, State& state)
{
  setup_shaders(temp_arena);

  Mesh meshes[] = {Mesh::from_obj(perm_arena, temp_arena, "assets/cube.obj").val};
  state.model = Model{Array<Mesh>{perm_arena, meshes, 1}};

  temp_arena.free_all();
}

void render(State& state)
{
  Renderer::clear_screen();

  state.model.rotateY(static_cast<f32>(platform::get_ms()));
  state.model.draw(Shader::DefaultShader);
}

void get_sound(SoundBuffer& sound_buffer)
{
  static u32 sample_index = 0;

  for (u32 i = 0; i < sound_buffer.sample_count; i += 2)
  {
    f32 t = static_cast<f32>(sample_index) / static_cast<f32>(sound_buffer.samples_per_second);
    f32 frequency = 440.0f;
    f32 amplitude = 0.25f;
    i16 sine_value = static_cast<i16>(math::sin(2.0f * PI32 * t * frequency) * I16_MAX * amplitude);

    ++sample_index;

    i16* left  = sound_buffer.memory + i;
    i16* right = sound_buffer.memory + i + 1;

    *left  = sine_value;
    *right = sine_value;
  }
}

}
