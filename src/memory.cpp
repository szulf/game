#include "memory.h"

AllocatedBuffer::AllocatedBuffer(usize size) : size{size}
{
  std::pmr::memory_resource* allocator = std::pmr::get_default_resource();
  data = allocator->allocate(size);
}

AllocatedBuffer::~AllocatedBuffer()
{
  std::pmr::memory_resource* allocator = std::pmr::get_default_resource();
  allocator->deallocate(data, size);
}

AllocatedBuffer::AllocatedBuffer(AllocatedBuffer&& other) : data{other.data}, size{other.size}
{
  other.data = nullptr;
  other.size = 0;
}

AllocatedBuffer& 
AllocatedBuffer::operator=(AllocatedBuffer&& other)
{
  data = other.data;
  other.data = nullptr;
  size = other.size;
  other.size = 0;
  return *this;
}

AllocatedBuffer::operator void*()
{
  return data;
}

AllocatedBuffer::operator char*()
{
  return static_cast<char*>(data);
}

AllocatedBuffer::operator u8*()
{
  return static_cast<u8*>(data);
}
