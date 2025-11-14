#pragma once

#include "renderer/camera.hpp"
#include "renderer/model.hpp"
#include "renderer/shaders.hpp"

namespace core {

struct Renderable final {
  Model model;
  Shader shader;
};

struct Scene final {
public:
  Scene(Model&& model, const Camera& camera);

  std::vector<Renderable> renderables{};
  Camera camera;
};

}
