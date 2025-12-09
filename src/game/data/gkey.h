#ifndef GKEY_H
#define GKEY_H

GameKeymap keymap_from_file(const char* path, Error& out_error);
void keymap_to_file(const char* path, GameKeymap& key_map);

#endif
