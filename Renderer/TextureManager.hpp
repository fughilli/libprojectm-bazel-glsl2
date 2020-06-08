#ifndef TextureManager_HPP
#define TextureManager_HPP

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "Texture.hpp"
#include "absl/types/span.h"
#include "projectM-opengl.h"

class TextureManager {

public:
  TextureManager(std::string presets_url, int width, int height,
                 std::string data_url);
  ~TextureManager();

  void Preload();
  void Clear();

  std::shared_ptr<Texture> TryLoadTexture(std::string_view name);
  std::shared_ptr<Texture> GetTexture(std::string name, GLenum wrap_mode,
                                      GLenum filter_mode);

  std::shared_ptr<Texture> GetMainTexture();
  void UpdateMainTexture();

  absl::Span<std::shared_ptr<Texture>> GetBlurTextures();

  std::shared_ptr<Texture> GetRandomTextureName(std::string random_name);
  void ClearRandomTextures();

private:
  void LoadTextureDirectory(std::string_view directory_name);
  std::shared_ptr<Texture> LoadTexture(std::string name,
                                       std::string image_url);
  void InsertNamedTexture(std::string name, Texture::ImageType image_type,
                          GLint width, GLint height, GLint depth,
                          bool is_user_texture, GLenum wrap_mode,
                          GLenum filter_mode, GLenum format, GLenum type,
                          void *data);
  void ParseTextureSettingsFromName(std::string_view qualified_name,
                                    GLint *wrap_mode, GLint *filter_mode,
                                    std::string *name);

  std::string presets_url_;
  std::map<std::string, std::shared_ptr<Texture>> named_textures_;
  std::vector<std::shared_ptr<Texture>> blur_textures_;
  std::shared_ptr<Texture> main_texture_;
  std::vector<std::string> random_textures_;
  std::map<std::string, std::string> random_names_mapping_;
  std::vector<std::string> extensions_ = {".jpg", ".dds", ".png",
                                          ".tga", ".bmp", ".dib"};
};

#endif
