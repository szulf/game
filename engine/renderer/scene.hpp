#pragma once

#include "renderer/model.hpp"
#include "renderer/shaders.hpp"

namespace core
{

struct Renderable
{
  Model model;
  Shader shader;
};

class Scene
{
public:
  inline auto renderables() const -> const std::vector<Renderable>&
  {
    return m_renderables;
  }
  inline auto view() const -> const math::mat4&
  {
    return m_view;
  }
  inline auto projection() const -> const math::mat4&
  {
    return m_proj;
  }

private:
  std::vector<Renderable> m_renderables{};
  math::mat4 m_view{};
  math::mat4 m_proj{};
};

}
