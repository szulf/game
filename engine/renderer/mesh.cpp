#include "renderer/mesh.hpp"

namespace core
{

Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<std::uint32_t>&& indices, Material&& material)
  : m_vertices{std::move(vertices)}, m_indices{std::move(indices)}, m_material{std::move(material)}
{
}

}
