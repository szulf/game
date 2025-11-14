#pragma once

#include <type_traits>

namespace utils {

template <typename... Ts>
struct type_list {};

template <typename First, typename... Ts>
struct get_first_type {
  using type = First;
};

template <typename T, typename Ts>
constexpr bool type_list_contains = false;

template <typename T, typename... Ts>
constexpr bool type_list_contains<type_list<Ts...>, T> = (std::is_same_v<Ts, T> || ...);

}
