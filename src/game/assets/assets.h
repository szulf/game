#ifndef ASSETS_H
#define ASSETS_H

#include "shader.h"
#include "texture.h"
#include "material.h"
#include "mesh.h"
#include "model.h"

struct AssetManager
{
  Array<Shader> shaders;
  Array<Texture> textures;
  Array<Material> materials;
  Array<Mesh> meshes;
  Array<Model> models;

  Map<String, TextureHandle> texture_handles;
  Map<String, MaterialHandle> material_handles;
};

static AssetManager* asset_manager_instance = nullptr;

AssetManager asset_manager_make(Allocator& allocator);

ShaderHandle assets_set_shader(Shader shader);
Shader assets_get_shader(ShaderHandle handle);
Texture& assets_get_texture(TextureHandle handle);
TextureHandle assets_set_texture(const Texture& texture);
Material& assets_get_material(MaterialHandle handle);
MaterialHandle assets_set_material(const Material& material);
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

ModelHandle model_from_file(const char* path, Allocator& allocator, Error& out_error);
ShaderHandle shader_from_file(const char* vert_path, const char* frag_path, Error& out_error);

#endif
