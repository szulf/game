#ifndef ASSETS_H
#define ASSETS_H

#include "shader.h"
#include "texture.h"
#include "material.h"
#include "mesh.h"
#include "model.h"

namespace assets
{

struct Manager
{
  Array<Shader> shaders;
  Array<Texture> textures;
  Array<Material> materials;
  Array<Mesh> meshes;
  Array<Model> models;

  Map<String, TextureHandle> texture_handles;
  Map<String, MaterialHandle> material_handles;
};

static Manager* manager_instance = nullptr;

Manager manager_make(Allocator& allocator);

Shader shader_get(ShaderHandle handle);
ShaderHandle shader_set(Shader shader);
Texture& texture_get(TextureHandle handle);
TextureHandle texture_set(const Texture& texture);
Material& material_get(MaterialHandle handle);
MaterialHandle material_set(const Material& material);
Mesh& mesh_get(MeshHandle handle);
MeshHandle mesh_set(const Mesh& mesh);
Model& model_get(ModelHandle handle);
ModelHandle model_set(const Model& model);

MaterialHandle material_handle_get(const String& key);
void material_handle_set(const String& key, MaterialHandle handle);
bool material_handle_exists(const String& key);
TextureHandle texture_handle_get(const String& key);
void texture_handle_set(const String& key, TextureHandle handle);
bool texture_handle_exists(const String& key);

ModelHandle model_from_file(const char* path, Allocator& allocator, Error& out_error);
ShaderHandle shader_from_file(const char* vert_path, const char* frag_path, Error& out_error);

}

#endif
