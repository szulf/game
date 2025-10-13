#ifndef ARRAY_H
#define ARRAY_H

// NOTE(szulf): example of an array
// struct TypeArray
// {
//   usize cap;
//   usize len;
//   Type* items;
// };

// TODO(szulf): do i want to actually handle the overflow case?
#define ARRAY_PUSH(arr, val) do {\
if ((arr)->len >= (arr)->cap) \
{ \
  ASSERT(false, "array cap exceeded"); \
} \
(arr)->items[(arr)->len] = (val); \
++(arr)->len; \
} while (0)

#define ARRAY_INIT(arr, capacity, arena, err) do { \
(arr)->cap = (capacity); \
(arr)->len = 0; \
(arr)->items = arena_alloc((arena), (capacity) * sizeof(*(arr)->items), (err)); \
} while (0)

#endif
