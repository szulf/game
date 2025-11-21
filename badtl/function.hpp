#ifndef BADLT_FUNCTION_HPP
#define BADLT_FUNCTION_HPP

namespace btl {

struct Function {
  void* operator()() const {
    return fn(args);
  }

  void* (*fn)(void* args);
  void* args;
};

}

#endif
