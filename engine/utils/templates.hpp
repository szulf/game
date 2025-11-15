#pragma once

#include <type_traits>

namespace utils {

template <typename... ListTypes>
struct type_list {};

template <typename T, typename...>
struct type_list_first {
  using type = T;
};

template <typename T, typename... Ts>
struct type_list_first<type_list<T, Ts...>> {
  using type = T;
};

template <typename T, typename Ts>
constexpr bool type_list_contains = false;

template <typename T, typename... Ts>
constexpr bool type_list_contains<type_list<Ts...>, T> = (std::is_same_v<Ts, T> || ...);

}
