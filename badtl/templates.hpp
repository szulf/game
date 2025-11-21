#ifndef BADTL_TEMPLATES_HPP
#define BADTL_TEMPLATES_HPP

#include "types.hpp"

namespace btl {

template <typename T1, typename T2>
struct same {
  static constexpr bool value = false;
};

template <typename T>
struct same<T, T> {
  static constexpr bool value = true;
};

template <typename T>
struct remove_ref {
  using type = T;
};
template <typename T>
struct remove_ref<T&> {
  using type = T;
};
template <typename T>
struct remove_ref<T&&> {
  using type = T;
};

template <typename T>
struct remove_cv {
  using type = T;
};
template <typename T>
struct remove_cv<const T> {
  using type = T;
};
template <typename T>
struct remove_cv<volatile T> {
  using type = T;
};
template <typename T>
struct remove_cv<const volatile T> {
  using type = T;
};

template <typename T>
struct remove_cvref {
  using type = typename remove_cv<typename remove_ref<T>::type>::type;
};

template <typename T>
struct decay_helper {
  using type = remove_cvref<T>;
};
template <typename T, usize N>
struct decay_helper<T[N]> {
  using type = T*;
};
template <typename T, typename... Ts>
struct decay_helper<T(Ts...)> {
  using type = T (*)(Ts...);
};

template <typename T>
struct decay {
  using type = typename decay_helper<T>::type;
};

}

#endif
