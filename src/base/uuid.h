#pragma once

#include "base.h"

class UUID
{
public:
  UUID();

  [[nodiscard]] inline constexpr u64 value() const noexcept
  {
    return m_value;
  }

  inline constexpr std::strong_ordering operator<=>(const UUID& other) const = default;

private:
  u64 m_value{};
};

template <std::derived_from<UUID> T>
struct std::hash<T>
{
  std::size_t operator()(const UUID& u) const noexcept
  {
    return u.value();
  }
};
