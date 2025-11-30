#ifndef ASSETS_H
#define ASSETS_H

struct AssetManager
{
  Array<Texture> textures;
  Array<Material> materials;
  Array<Mesh> meshes;
  Array<Model> models;

  Map<String, TextureHandle> texture_handles;
  Map<String, MaterialHandle> material_handles;
};

AssetManager asset_manager_make(Allocator& allocator);

static AssetManager* asset_manager_instance = nullptr;
Material& assets_get_material(MaterialHandle handle);
MaterialHandle assets_set_material(const Material& material);
Texture& assets_get_texture(TextureHandle handle);
TextureHandle assets_set_texture(const Texture& texture);
Mesh& assets_get_mesh(MeshHandle handle);
MeshHandle assets_set_mesh(const Mesh& mesh);
Model& assets_get_model(ModelHandle handle);
ModelHandle assets_set_model(const Model& model);

MaterialHandle assets_material_handle_get(const String& key);
void assets_material_handle_set(const String& key, MaterialHandle handle);
bool assets_material_handle_exists(const String& key);
TextureHandle assets_texture_handle_get(const String& key);
void assets_texture_handle_set(const String& key, TextureHandle handle);
bool assets_texture_handle_exists(const String& key);

ModelHandle assets_load_model(const char* path, Allocator& allocator, Error& out_error);

#endif
