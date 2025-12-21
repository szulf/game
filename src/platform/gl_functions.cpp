#ifdef RENDERER_OPENGL

#  include "gl_functions.h"

void setup_gl_functions(RenderingAPI* api)
{
  api->glViewport = (PFNGLVIEWPORTPROC) SDL_GL_GetProcAddress("glViewport");
  api->glGenTextures = (PFNGLGENTEXTURESPROC) SDL_GL_GetProcAddress("glGenTextures");
  api->glBindTexture = (PFNGLBINDTEXTUREPROC) SDL_GL_GetProcAddress("glBindTexture");
  api->glTexParameteri = (PFNGLTEXPARAMETERIPROC) SDL_GL_GetProcAddress("glTexParameteri");
  api->glTexImage2D = (PFNGLTEXIMAGE2DPROC) SDL_GL_GetProcAddress("glTexImage2D");
  api->glEnable = (PFNGLENABLEPROC) SDL_GL_GetProcAddress("glEnable");
  api->glClearColor = (PFNGLCLEARCOLORPROC) SDL_GL_GetProcAddress("glClearColor");
  api->glClear = (PFNGLCLEARPROC) SDL_GL_GetProcAddress("glClear");
  api->glActiveTexture = (PFNGLACTIVETEXTUREPROC) SDL_GL_GetProcAddress("glActiveTexture");
  api->glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) SDL_GL_GetProcAddress("glGenVertexArrays");
  api->glDrawElements = (PFNGLDRAWELEMENTSPROC) SDL_GL_GetProcAddress("glDrawElements");
  api->glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) SDL_GL_GetProcAddress("glGenVertexArrays");
  api->glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) SDL_GL_GetProcAddress("glBindVertexArray");
  api->glGenBuffers = (PFNGLGENBUFFERSPROC) SDL_GL_GetProcAddress("glGenBuffers");
  api->glBindBuffer = (PFNGLBINDBUFFERPROC) SDL_GL_GetProcAddress("glBindBuffer");
  api->glBufferData = (PFNGLBUFFERDATAPROC) SDL_GL_GetProcAddress("glBufferData");
  api->glVertexAttribPointer =
    (PFNGLVERTEXATTRIBPOINTERPROC) SDL_GL_GetProcAddress("glVertexAttribPointer");
  api->glEnableVertexAttribArray =
    (PFNGLENABLEVERTEXATTRIBARRAYPROC) SDL_GL_GetProcAddress("glEnableVertexAttribArray");
  api->glUniform1i = (PFNGLUNIFORM1IPROC) SDL_GL_GetProcAddress("glUniform1i");
  api->glGetUniformLocation =
    (PFNGLGETUNIFORMLOCATIONPROC) SDL_GL_GetProcAddress("glGetUniformLocation");
  api->glUniform1f = (PFNGLUNIFORM1FPROC) SDL_GL_GetProcAddress("glUniform1f");
  api->glUniform3f = (PFNGLUNIFORM3FPROC) SDL_GL_GetProcAddress("glUniform3f");
  api->glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) SDL_GL_GetProcAddress("glUniformMatrix4fv");
  api->glUseProgram = (PFNGLUSEPROGRAMPROC) SDL_GL_GetProcAddress("glUseProgram");
  api->glCreateShader = (PFNGLCREATESHADERPROC) SDL_GL_GetProcAddress("glCreateShader");
  api->glShaderSource = (PFNGLSHADERSOURCEPROC) SDL_GL_GetProcAddress("glShaderSource");
  api->glCompileShader = (PFNGLCOMPILESHADERPROC) SDL_GL_GetProcAddress("glCompileShader");
  api->glGetShaderiv = (PFNGLGETSHADERIVPROC) SDL_GL_GetProcAddress("glGetShaderiv");
  api->glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) SDL_GL_GetProcAddress("glGetShaderInfoLog");
  api->glCreateProgram = (PFNGLCREATEPROGRAMPROC) SDL_GL_GetProcAddress("glCreateProgram");
  api->glAttachShader = (PFNGLATTACHSHADERPROC) SDL_GL_GetProcAddress("glAttachShader");
  api->glLinkProgram = (PFNGLLINKPROGRAMPROC) SDL_GL_GetProcAddress("glLinkProgram");
  api->glDeleteShader = (PFNGLDELETESHADERPROC) SDL_GL_GetProcAddress("glDeleteShader");
  api->glGetProgramiv = (PFNGLGETPROGRAMIVPROC) SDL_GL_GetProcAddress("glGetProgramiv");
  api->glGetProgramInfoLog =
    (PFNGLGETPROGRAMINFOLOGPROC) SDL_GL_GetProcAddress("glGetProgramInfoLog");
  api->glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) SDL_GL_GetProcAddress("glGenerateMipmap");
  api->glDebugMessageCallback =
    (PFNGLDEBUGMESSAGECALLBACKPROC) SDL_GL_GetProcAddress("glDebugMessageCallback");
  api->glDeleteVertexArrays =
    (PFNGLDELETEVERTEXARRAYSPROC) SDL_GL_GetProcAddress("glDeleteVertexArrays");
  api->glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) SDL_GL_GetProcAddress("glDeleteBuffers");
  api->glGetError = (PFNGLGETERRORPROC) SDL_GL_GetProcAddress("glGetError");
  api->glPolygonMode = (PFNGLPOLYGONMODEPROC) SDL_GL_GetProcAddress("glPolygonMode");
  api->glBindBufferBase = (PFNGLBINDBUFFERBASEPROC) SDL_GL_GetProcAddress("glBindBufferBase");
  api->glBufferSubData = (PFNGLBUFFERSUBDATAPROC) SDL_GL_GetProcAddress("glBufferSubData");
}

#  ifdef MODE_DEBUG
void APIENTRY debug_callback(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* user
)
{
  (void) length;
  (void) user;

  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
  {
    return;
  }
  const char* source_str;
  switch (source)
  {
    case GL_DEBUG_SOURCE_API:
      source_str = "API";
      break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      source_str = "Window System";
      break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
      source_str = "Shader Compiler";
      break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
      source_str = "Third Party";
      break;
    case GL_DEBUG_SOURCE_APPLICATION:
      source_str = "Application";
      break;
    case GL_DEBUG_SOURCE_OTHER:
      source_str = "Other";
      break;
    default:
      source_str = "Unknown";
      break;
  }

  const char* type_str;
  switch (type)
  {
    case GL_DEBUG_TYPE_ERROR:
      type_str = "Error";
      break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      type_str = "Deprecated Behaviour";
      break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      type_str = "Undefined Behaviour";
      break;
    case GL_DEBUG_TYPE_PORTABILITY:
      type_str = "Portability";
      break;
    case GL_DEBUG_TYPE_PERFORMANCE:
      type_str = "Performance";
      break;
    case GL_DEBUG_TYPE_MARKER:
      type_str = "Marker";
      break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
      type_str = "Push Group";
      break;
    case GL_DEBUG_TYPE_POP_GROUP:
      type_str = "Pop Group";
      break;
    case GL_DEBUG_TYPE_OTHER:
      type_str = "Other";
      break;
    default:
      type_str = "Unknown";
      break;
  }

  const char* severity_str;
  switch (severity)
  {
    case GL_DEBUG_SEVERITY_HIGH:
      severity_str = "HIGH";
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      severity_str = "MEDIUM";
      break;
    case GL_DEBUG_SEVERITY_LOW:
      severity_str = "LOW";
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      severity_str = "NOTIFICATION";
      break;
    default:
      severity_str = "UNKNOWN";
      break;
  }

  print(
    "[OpenGL Debug] Source: %s | Type: %s | Severity: %s | ID: %u\n    Message: %s\n",
    source_str,
    type_str,
    severity_str,
    id,
    message
  );
}

#  endif

#endif
