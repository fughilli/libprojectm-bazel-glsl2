#ifndef TextureManager_HPP
#define TextureManager_HPP

#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Texture.hpp"
#include "absl/types/span.h"
#include "projectM-opengl.h"

// Provides a memoized store of textures.
class TextureManager {
public:
  // A pair of texture and sampler. This type is returned by Load* and Get*
  // methods.
  struct TextureAndSampler {
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Sampler> sampler;
  };

  // Constructs a TextureManager, populating the store with any images under
  // `presets_url`, `data_url`/presets and `data_url`/textures. Also initializes
  // the main texture to `width` * `height` texels, and a series of blur
  // textures with sizes progressively halved from the main texture size.
  TextureManager(std::string presets_url, int width, int height,
                 std::string data_url);

  void Clear();

  std::optional<TextureAndSampler> LoadTextureAndSampler(std::string name);
  std::optional<TextureAndSampler>
  GetTextureAndSampler(std::string name, GLenum wrap_mode, GLenum filter_mode);

  std::shared_ptr<Texture> GetMainTexture();
  void UpdateMainTexture();

  absl::Span<std::shared_ptr<Texture>> GetBlurTextures();

  std::optional<TextureAndSampler>
  GetRandomTextureAndSampler(std::string random_name);

private:
  // Loads the ProjectM "M" logo and headphones textures from binary data.
  void LoadIdleTextures();

  // Returns a texture matching the provided name.
  std::shared_ptr<Texture> GetTexture(std::string name);

  // Returns a texture and sampler matching the provided name, reading from the
  // file at the provided texture path if nothing is already cached under
  // `name`.
  std::optional<TextureAndSampler>
  LoadTextureAndSampler(std::string name, std::string texture_path);

  // Attempts loading and caching all texture files under `directory_name`.
  void LoadTextureDirectory(std::string_view directory_name);
  void InsertNamedTexture(std::string name, Texture::ImageType image_type,
                          GLint width, GLint height, GLint depth,
                          bool is_user_texture, GLenum wrap_mode,
                          GLenum filter_mode, GLenum format, GLenum type,
                          void *data);
  void ParseTextureSettingsFromName(std::string_view qualified_name,
                                    GLenum *wrap_mode, GLenum *filter_mode,
                                    std::string *name);

  std::string presets_url_;
  std::map<std::string, std::shared_ptr<Texture>> named_textures_;
  std::vector<std::shared_ptr<Texture>> blur_textures_;
  std::shared_ptr<Texture> main_texture_;
  std::vector<std::string> random_textures_;
  std::vector<std::string> extensions_ = {".jpg", ".dds", ".png",
                                          ".tga", ".bmp", ".dib"};
};

#endif
