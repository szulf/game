#ifndef BADTL_ARRAY_HPP
#define BADTL_ARRAY_HPP

#include "utils.hpp"
#include "types.hpp"

namespace btl {

template <typename T, usize N>
struct Array {
  T& operator[](usize idx);
  const T& operator[](usize idx) const;

  T* begin();
  T* end();
  const T* begin() const;
  const T* end() const;

  T data[N];
  static constexpr usize size = N;
};

}

namespace btl {

template <typename T, usize N>
T& Array<T, N>::operator[](usize idx) {
  ASSERT(idx < N, "array index out of bounds");
  return data[idx];
}

template <typename T, usize N>
const T& Array<T, N>::operator[](usize idx) const {
  ASSERT(idx < N, "array index out of bounds");
  return data[idx];
}

template <typename T, usize N>
T* Array<T, N>::begin() {
  return data;
}

template <typename T, usize N>
T* Array<T, N>::end() {
  return data + N;
}

template <typename T, usize N>
const T* Array<T, N>::begin() const {
  return data;
}

template <typename T, usize N>
const T* Array<T, N>::end() const {
  return data + N;
}

}

#endif
