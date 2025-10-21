#ifndef OBJ_H
#define OBJ_H

namespace obj
{

static Model parse(const char* path, mem::Arena& temp_arena, mem::Arena& perm_arena, Error* err);

}

#endif
