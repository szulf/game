#pragma once

#include "engine/renderer/vertex.hpp"
#include "badtl/string.hpp"
#include "badtl/list.hpp"

namespace core {

// TODO(szulf): think how to free gpu memory
struct Mesh {
#ifdef GAME_OPENGL
  struct BackendData {
    btl::u32 vao;
    btl::u32 vbo;
    btl::u32 ebo;
  };
#else
#  error Unknown rendering backend
#endif

  static Mesh
  make(const btl::List<Vertex>& vertices, const btl::List<btl::u32>& indices, const btl::String& material_name);

  btl::List<Vertex> vertices;
  btl::List<btl::u32> indices;
  btl::String material_name;
  BackendData backend_data;
};

}
