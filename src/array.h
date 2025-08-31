#ifndef ARRAY_H
#define ARRAY_H

// NOTE(szulf): example of an array
// struct TypeArray
// {
//   usize cap;
//   usize len;
//   Type* items;
// };

#define ARRAY_PUSH(arr, val) do {\
if ((arr)->len + 1 >= (arr)->cap) \
{ \
} \
(arr)->items[(arr)->len] = (val); \
++(arr)->len; \
} while (0)

#define ARRAY_INIT(arr, arena, capacity) ARRAY_INIT2(arr, arena, capacity, __COUNTER__)
#define ARRAY_INIT2(arr, arena, capacity, counter) ARRAY_INIT3(arr, arena, capacity, counter)
#define ARRAY_INIT3(arr, arena, capacity, counter) do { \
(arr)->cap = (capacity); \
(arr)->len = 0; \
Error alloc_err##counter = arena_alloc((void**) &(arr)->items, (arena), (capacity) * sizeof(*(arr)->items)); \
if (alloc_err##counter != SUCCESS) \
{ \
  return alloc_err##counter; \
} \
} while (0)

#endif
