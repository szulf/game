#include "engine/renderer/scene.hpp"

namespace core {

Scene Scene::make(const Model& model, const Camera& camera, btl::Allocator& allocator) {
  Scene out = {};
  out.camera = camera;
  out.renderables = btl::List<Renderable>::make(1, allocator);
  out.renderables.push({model, Shader::Default});
  return out;
}

}
