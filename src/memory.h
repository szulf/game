#ifndef MEMORY_H
#define MEMORY_H

struct AllocatedBuffer
{
  void* data;
  usize size;

  AllocatedBuffer() {}
  AllocatedBuffer(usize size);
  ~AllocatedBuffer();

  AllocatedBuffer(const AllocatedBuffer& other) = delete;
  AllocatedBuffer& operator=(const AllocatedBuffer& other) = delete;
  AllocatedBuffer(AllocatedBuffer&& other);
  AllocatedBuffer& operator=(AllocatedBuffer&& other);

  operator void*();
  operator char*();
  operator u8*();
};

#endif
