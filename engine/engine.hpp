#pragma once

#include <type_traits>
#include <print>
#include <vector>
#include <memory>

#define ASSERT(expr, msg)

namespace core
{

class Layer
{
public:
  virtual ~Layer() {}

  virtual auto onRender() -> void {}
  virtual auto onUpdate(float) -> void {}
  virtual auto onEvent() -> void {}
};

struct AppSpec final
{
  const char* name{};
  std::int32_t width{};
  std::int32_t height{};
};

class Engine final
{
public:
  Engine(const AppSpec& spec);
  ~Engine();

  Engine(const Engine& other) = delete;
  Engine& operator=(const Engine& other) = delete;

  void run();

  template <typename T>
    requires std::is_base_of_v<Layer, T>
  auto pushLayer() -> void
  {
    m_layers.push_back(std::make_unique<T>());
  }

private:
  bool m_running{};
  std::vector<std::unique_ptr<Layer>> m_layers{};

  // TODO(szulf): can i somehow avoid the dynamic allocation here?
  struct PlatformData;
  PlatformData* m_platform_data;
};

}
