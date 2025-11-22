#pragma once

#include "engine/renderer/camera.hpp"
#include "engine/renderer/model.hpp"
#include "engine/renderer/shaders.hpp"

namespace core {

struct Renderable {
  Model model;
  Shader shader;
};

struct Scene {
  static Scene make(const Model& model, const Camera& camera, btl::Allocator& allocator);

  btl::List<Renderable> renderables;
  Camera camera;
};

void write_formatted_type(btl::usize& buf_idx, char* buffer, btl::usize n, const Renderable& first);
void write_formatted_type(btl::usize& buf_idx, char* buffer, btl::usize n, const Scene& first);

}
