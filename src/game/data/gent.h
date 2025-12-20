#ifndef GENT_H
#define GENT_H

namespace data
{

enum EntityReadError
{
  ENTITY_READ_ERROR_INVALID_POSITION = GLOBAL_ERROR_COUNT,
  ENTITY_READ_ERROR_INVALID_MODEL,
  ENTITY_READ_ERROR_INVALID_TYPE,
  ENTITY_READ_ERROR_INVALID_INTERACTABLE_TYPE,
  ENTITY_READ_ERROR_DYNAMIC_BOUNDING_BOX_NO_MODEL,
  ENTITY_READ_ERROR_INVALID_PATH,
};

void entity_from_file(const Entity& entity, Error& out_error);
Entity entity_to_file(const char* path, Allocator& allocator, Error& out_error);

}

#endif
