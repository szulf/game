#ifndef BADLT_FUNCTION_HPP
#define BADLT_FUNCTION_HPP

namespace btl {

template <typename R, typename... Ts>
struct Function {
  R operator()(Ts... args) const;

  void* bound_args;
  R (*fn)(void* args, Ts...);
};

}

namespace btl {

template <typename R, typename... Ts>
R Function<R, Ts...>::operator()(Ts... args) const {
  return fn(bound_args, args...);
}

}

#endif
