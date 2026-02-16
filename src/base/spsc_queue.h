#pragma once

#include <atomic>

#include "base.h"

template <typename T>
class SPSCQueue
{
public:
  SPSCQueue(usize capacity) : m_capacity{capacity}
  {
    buffer = static_cast<Element*>(malloc(m_capacity * sizeof(T)));
    ASSERT(buffer, "Failed to alloc memory for SPSCQueue.");
  }

  ~SPSCQueue()
  {
    T temp;
    while (consume_one(temp))
    {
    }
    free(buffer);
  }

  bool push(const T& item)
  {
    const usize curr_tail = tail.load(std::memory_order_relaxed);
    const usize next_tail = increment(curr_tail);
    if (next_tail == head.load(std::memory_order_acquire))
    {
      return false;
    }

    std::construct_at(reinterpret_cast<T*>(&buffer[curr_tail]), item);
    tail.store(next_tail, std::memory_order_release);
    return true;
  }

  bool consume_one(T& out)
  {
    const usize curr_head = head.load(std::memory_order_relaxed);
    if (curr_head == tail.load(std::memory_order_acquire))
    {
      return false;
    }
    out = reinterpret_cast<T&>(std::move(buffer[curr_head]));
    std::destroy_at(&buffer[curr_head]);
    head.store(increment(curr_head), std::memory_order_release);
    return true;
  }

private:
  usize increment(usize idx)
  {
    return (idx + 1) % m_capacity;
  }

private:
  struct Element
  {
    alignas(T) std::byte storage[sizeof(T)];
  };
  std::atomic<usize> head{};
  std::atomic<usize> tail{};
  usize m_capacity{};
  Element* buffer;
};
