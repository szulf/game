#ifndef GSCN_H
#define GSCN_H

namespace data
{

Array<Entity> scene_from_file(const char* path, Allocator& allocator, Error& out_error);
void scene_to_file(const char* path, const Array<Entity>& entities, Error& out_error);

}

#endif
