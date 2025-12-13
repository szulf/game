#ifndef GKEY_H
#define GKEY_H

GameInput keymap_from_file(const char* path, Error& out_error);
void keymap_to_file(const char* path, GameInput& key_map);

#endif
