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

#define ARRAY_INIT(arr, arena, capacity, err) ARRAY_INIT2(arr, arena, capacity, err, __COUNTER__)
#define ARRAY_INIT2(arr, arena, capacity, err, counter) ARRAY_INIT3(arr, arena, capacity, err, counter)
#define ARRAY_INIT3(arr, arena, capacity, err, counter) do { \
(arr)->cap = (capacity); \
(arr)->len = 0; \
(arr)->items = arena_alloc((arena), (capacity) * sizeof(*(arr)->items), err); \
} while (0)

#endif
