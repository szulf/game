#pragma once

#include <array>
#include <concepts>
#include <type_traits>

#include "base/base.h"

template <typename E, typename T>
struct EnumArray
{
  static_assert(std::is_enum_v<E>, "E template argument has to be an enum");

  inline constexpr T* begin()
  {
    return m_data.begin();
  }

  inline constexpr T* end()
  {
    return m_data.end();
  }

  [[nodiscard]] inline constexpr T& operator[](E e)
  {
    return m_data[(std::size_t) e];
  }

  [[nodiscard]] inline constexpr const T& operator[](E e) const
  {
    return m_data[(std::size_t) e];
  }

  template <std::integral I>
  [[nodiscard]] inline constexpr T& operator[](I idx)
  {
    return m_data[idx];
  }

  template <std::integral I>
  [[nodiscard]] inline constexpr const T& operator[](I idx) const
  {
    return m_data[idx];
  }

  [[nodiscard]] inline constexpr static usize size()
  {
    return static_cast<usize>(E::COUNT);
  }

  [[nodiscard]] inline constexpr T* data()
  {
    return m_data.data();
  }

private:
  std::array<T, size()> m_data{};
};
