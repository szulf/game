#ifndef BADTL_FILES_HPP
#define BADTL_FILES_HPP

#include "allocator.hpp"
#include "ptr.hpp"
#include "string.hpp"

namespace btl {

Ptr<void> readFile(const String& path, Allocator& allocator);
Ptr<void> readFile(const char* path, Allocator& allocator);

}

#endif
