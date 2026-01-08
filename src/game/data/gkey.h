#ifndef GKEY_H
#define GKEY_H

namespace data
{

game::Input keymap_from_file(const char* path, Error& out_error);
void keymap_to_file(const char* path, game::Input& key_map, Error& out_error);

}

#endif
