#pragma once

#ifdef GAME_DEBUG

#  include <print>
#  include <iostream>

#  define ASSERT(expr, msg)                                                                                            \
    do {                                                                                                               \
      if (!(expr)) {                                                                                                   \
        std::println(std::cerr, "assertion failed on expression: {}\nwith message: {}", #expr, (msg));                 \
        std::terminate();                                                                                              \
      }                                                                                                                \
    } while (false)

#else

#  define ASSERT(expr, msg)

#endif
