#ifndef BASE_ENUM_ARRAY_H
#define BASE_ENUM_ARRAY_H

#include "base.h"

template <typename E, typename T>
struct enum_array
{
  static constexpr usize size = (usize) E::COUNT;
  T data[(usize) size];

  T& operator[](E e)
  {
    return data[(usize) e];
  }

  const T& operator[](E e) const
  {
    return data[(usize) e];
  }
};

#endif
