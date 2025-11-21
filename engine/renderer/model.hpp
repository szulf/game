#pragma once

#include "engine/renderer/mesh.hpp"
#include "badtl/allocator.hpp"
#include "badtl/mat4.hpp"

namespace core {

enum class ModelError {
  InvalidMTLFile,
  InvalidInput,
  InvalidVertex,
};

struct Model {
  static btl::Result<Model, ModelError> from_file(const btl::String& path, btl::Allocator& allocator);
  static Model error_model();

  btl::List<Mesh> meshes;
  btl::Mat4 matrix;
};

}
