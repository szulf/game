#ifndef GENT_H
#define GENT_H

void entity_from_file(const Entity& entity);
Entity entity_to_file(const char* path, Allocator& allocator, Error& out_error);

#endif
