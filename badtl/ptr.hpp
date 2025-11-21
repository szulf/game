#ifndef BADTL_PTR_HPP
#define BADTL_PTR_HPP

#include "types.hpp"

namespace btl {

template <typename T>
struct Ptr {
  T* ptr;
  usize size;
};

}

#endif
