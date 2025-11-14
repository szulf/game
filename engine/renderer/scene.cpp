#include "scene.hpp"

namespace core {

Scene::Scene(Model&& m, const Camera& c) : camera{c} {
  renderables.push_back({std::move(m), Shader::Default});
}

}
