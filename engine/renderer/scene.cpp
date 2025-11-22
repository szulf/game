#include "engine/renderer/scene.hpp"

namespace core {

Scene Scene::make(const Model& model, const Camera& camera, btl::Allocator& allocator) {
  Scene out = {};
  out.camera = camera;
  out.renderables = btl::List<Renderable>::make(1, allocator);
  out.renderables.push({model, Shader::Default});
  return out;
}

void write_formatted_type(btl::usize& buf_idx, char* buffer, btl::usize n, const core::Renderable&) {
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "model{"));
  // write_formatted_type(buf_idx, buffer, n, first.model);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, shader{"));
  // write_formatted_type(buf_idx, buffer, n, first.shader);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}"));
}

void write_formatted_type(btl::usize& buf_idx, char* buffer, btl::usize n, const core::Scene& first) {
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "camera{"));
  write_formatted_type(buf_idx, buffer, n, first.camera);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}, renderables{"));
  write_formatted_type(buf_idx, buffer, n, first.renderables);
  buf_idx += static_cast<btl::usize>(snprintf(buffer + buf_idx, n - buf_idx, "}"));
}

}
