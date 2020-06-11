#include "TextureManager.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

#include "Common.hpp"
#include "IdleTextures.hpp"
#include "PerlinNoiseWithAlpha.hpp"
#include "SOIL2/SOIL2.h"
#include "projectM-opengl.h"

namespace {
constexpr int kNumBlurTextures = 6;

std::string Lowercase(std::string_view name) {
  std::string lowercase_name(name);
  std::transform(name.begin(), name.end(), lowercase_name.begin(),
                 [](char c) { return std::tolower(c); });
  return lowercase_name;
}

void StripExtensions(std::string_view name, absl::Span<std::string> extensions,
                     std::string *name_without_extensions) {
  if (name_without_extensions == nullptr) {
    return;
  }

  *name_without_extensions = name;

  for (auto &extension : extensions) {
    size_t position = name.find(extension);
    if (position == std::string::npos) {
      continue;
    }
    name_without_extensions->replace(position, extension.size(), "");
  }
}

std::string SanitizeName(std::string_view name,
                         absl::Span<std::string> extensions) {
  std::string return_name;
  StripExtensions(Lowercase(name), extensions, &return_name);
  return return_name;
}

template <typename T> T RoundUp(T value, T multiple) {
  return ((value + multiple - 1) / multiple) * multiple;
}

template <typename T> T &SelectRandomly(absl::Span<T> items) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, items.size() - 1);
  auto iter = items.begin();
  std::advance(iter, dis(gen));
  return *iter;
}
} // namespace

void TextureManager::InsertNamedTexture(std::string name,
                                        Texture::ImageType image_type,
                                        GLint width, GLint height, GLint depth,
                                        bool is_user_texture, GLenum wrap_mode,
                                        GLenum filter_mode, GLenum format,
                                        GLenum type, void *data) {
  auto texture =
      std::make_shared<Texture>(name.c_str(), image_type, width, height, depth,
                                is_user_texture, format, type, data);
  texture->GetSamplerForModes(wrap_mode, filter_mode);
  std::cerr << "Inserting texture at " << std::hex
            << reinterpret_cast<intptr_t>(texture.get()) << std::dec
            << " under name: " << name << std::endl;
  named_textures_.emplace(std::make_pair(name, std::move(texture)));
}

TextureManager::TextureManager(std::string presets_url, int width, int height,
                               std::string data_url)
    : presets_url_(std::move(presets_url)) {
  LoadIdleTextures();

  if (data_url.empty()) {
    data_url = DATADIR_PATH;
  }

  LoadTextureDirectory(data_url + "/presets");
  LoadTextureDirectory(data_url + "/textures");
  LoadTextureDirectory(presets_url);

  // Create main texture and associated samplers
  main_texture_ = std::make_shared<Texture>("main", Texture::ImageType::k2d,
                                            width, height, 0, false);
  main_texture_->GetSamplerForModes(GL_REPEAT, GL_LINEAR);
  main_texture_->GetSamplerForModes(GL_REPEAT, GL_NEAREST);
  main_texture_->GetSamplerForModes(GL_CLAMP_TO_EDGE, GL_LINEAR);
  main_texture_->GetSamplerForModes(GL_CLAMP_TO_EDGE, GL_NEAREST);
  named_textures_["main"] = main_texture_;

  // Initialize blur textures
  for (int i = 0; i < kNumBlurTextures; i++) {
    // main VS = 1024
    // blur0 = 512
    // blur1 = 256  <-  user sees this as "blur1"
    // blur2 = 128
    // blur3 = 128  <-  user sees this as "blur2"
    // blur4 =  64
    // blur5 =  64  <-  user sees this as "blur3"
    if (!(i & 1) || (i < 2)) {
      width = std::max(16, width / 2);
      height = std::max(16, height / 2);
    }

    std::stringstream blur_texture_name_stream;
    blur_texture_name_stream << "blur" << (i / 2 + 1)
                             << ((i % 2) ? "" : "_internal");
    auto blur_texture_name = blur_texture_name_stream.str();
    auto blur_texture = std::make_shared<Texture>(
        blur_texture_name, Texture::ImageType::k2d, RoundUp(width, 16),
        RoundUp(height, 16), 0, false);
    blur_texture->GetSamplerForModes(GL_CLAMP_TO_EDGE, GL_LINEAR);
    named_textures_[blur_texture_name] = blur_texture;
    blur_textures_.push_back(blur_texture);
  }

  auto noise = std::make_unique<PerlinNoiseWithAlpha>();

  // TODO: populate these with a loop and a helper function
  InsertNamedTexture("noise_lq_lite", Texture::ImageType::k2d, 32, 32, 0, false,
                     GL_REPEAT, GL_LINEAR, GL_RGBA, GL_FLOAT,
                     noise->noise_lq_lite);
  InsertNamedTexture("noise_lq", Texture::ImageType::k2d, 256, 256, 0, false,
                     GL_REPEAT, GL_LINEAR, GL_RGBA, GL_FLOAT, noise->noise_lq);
  InsertNamedTexture("noise_mq", Texture::ImageType::k2d, 256, 256, 0, false,
                     GL_REPEAT, GL_LINEAR, GL_RGBA, GL_FLOAT, noise->noise_mq);
  InsertNamedTexture("noise_hq", Texture::ImageType::k2d, 256, 256, 0, false,
                     GL_REPEAT, GL_LINEAR, GL_RGBA, GL_FLOAT, noise->noise_hq);
  InsertNamedTexture("noisevol_lq", Texture::ImageType::k3d, 32, 32, 32, false,
                     GL_REPEAT, GL_LINEAR, GL_RGBA, GL_FLOAT, noise->noise_lq);
  InsertNamedTexture("noisevol_hq", Texture::ImageType::k3d, 32, 32, 32, false,
                     GL_REPEAT, GL_LINEAR, GL_RGBA, GL_FLOAT, noise->noise_lq);
}

void TextureManager::LoadIdleTextures() {
  int width, height;

  unsigned int texture_id = SOIL_load_OGL_texture_from_memory(
      M_data, M_bytes, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
      SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MULTIPLY_ALPHA, &width, &height);

  auto texture = std::make_shared<Texture>("m", Texture::ImageType::k2d,
                                           texture_id, width, height, 0, true);
  texture->GetSamplerForModes(GL_CLAMP_TO_EDGE, GL_LINEAR);
  named_textures_["m"] = texture;

  texture_id = SOIL_load_OGL_texture_from_memory(
      headphones_data, headphones_bytes, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
      SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MULTIPLY_ALPHA, &width, &height);

  texture = std::make_shared<Texture>("headphones", Texture::ImageType::k2d,
                                      texture_id, width, height, 0, true);
  texture->GetSamplerForModes(GL_CLAMP_TO_EDGE, GL_LINEAR);
  named_textures_["headphones"] = texture;
}

void TextureManager::Clear() {
  named_textures_.clear();
  LoadIdleTextures();
}

std::shared_ptr<Texture> TextureManager::GetTexture(std::string lookup_name) {
  if (named_textures_.find(lookup_name) == named_textures_.end()) {
    std::cerr << "Failed to find texture: " << lookup_name << std::endl;
    return nullptr;
  }
  std::cerr << "Found texture for name: " << lookup_name << std::endl;
  return named_textures_[lookup_name];
}

std::optional<TextureManager::TextureAndSampler>
TextureManager::GetTextureAndSampler(std::string name, GLenum default_wrap_mode,
                                     GLenum default_filter_mode) {
  std::cerr << std::hex << reinterpret_cast<intptr_t>(this) << std::dec
            << "->GetTextureAndSampler(" << name << ", " << default_wrap_mode
            << ", " << default_filter_mode << ")" << std::endl;
  std::string unqualified_name = name;
  ParseTextureSettingsFromName(name, &default_wrap_mode, &default_filter_mode,
                               &unqualified_name);
  auto lookup_name =
      SanitizeName(unqualified_name, absl::Span<std::string>(extensions_));
  auto return_texture = GetTexture(lookup_name);
  if (return_texture == nullptr) {
    std::cerr << "Could not find texture for name " << lookup_name << std::endl;
    return std::nullopt;
  }
  auto return_sampler = return_texture->GetSamplerForModes(default_wrap_mode,
                                                           default_filter_mode);
  if (return_sampler == nullptr) {
    std::cerr << "Could not find sampler for the given modes" << std::endl;
    return std::nullopt;
  }

  std::cerr << "Found texture for name " << lookup_name << std::endl;
  return TextureAndSampler({return_texture, return_sampler});
}

std::optional<TextureManager::TextureAndSampler>
TextureManager::LoadTextureAndSampler(std::string name) {
  ParseTextureSettingsFromName(name, nullptr, nullptr, &name);
  std::string texture_path = presets_url_ + PATH_SEPARATOR + name;
  return LoadTextureAndSampler(name, texture_path);
}

std::optional<TextureManager::TextureAndSampler>
TextureManager::LoadTextureAndSampler(std::string name,
                                      std::string texture_path) {
  GLenum wrap_mode = GL_REPEAT;
  GLenum filter_mode = GL_LINEAR;
  std::string unqualified_name = name;
  ParseTextureSettingsFromName(name, &wrap_mode, &filter_mode,
                               &unqualified_name);
  std::cerr << "LoadTexture(" << name << "): " << wrap_mode << ", "
            << filter_mode << " --> " << texture_path << std::endl;
  std::string sanitized_name =
      SanitizeName(unqualified_name, absl::Span<std::string>(extensions_));
  std::shared_ptr<Texture> texture = nullptr;
  if (named_textures_.find(sanitized_name) != named_textures_.end()) {
    texture = named_textures_[sanitized_name];
  } else {

    int width, height;
    unsigned int texture_id = SOIL_load_OGL_texture(
        texture_path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MULTIPLY_ALPHA, &width, &height);

    if (texture_id == 0) {
      return std::nullopt;
    }

    texture =
        std::make_shared<Texture>(unqualified_name, Texture::ImageType::k2d,
                                  texture_id, width, height, 0, true);
    named_textures_[sanitized_name] = texture;
  }
  if (texture == nullptr) {
    return std::nullopt;
  }

  auto sampler = texture->GetSamplerForModes(wrap_mode, filter_mode);
  if (sampler == nullptr) {
    return std::nullopt;
  }

  return TextureAndSampler({texture, sampler});
}

void TextureManager::LoadTextureDirectory(std::string_view directory_name) {
  try {
    for (const auto &directory_entry :
         std::filesystem::directory_iterator(directory_name)) {
      std::string file_name = directory_entry.path();
      if (file_name.length() > 0 && file_name[0] == '.') {
        continue;
      }

      LoadTextureAndSampler(
          std::string(directory_entry.path().filename()),
          (std::stringstream() << directory_name << PATH_SEPARATOR << file_name)
              .str());
    }
  } catch (const std::filesystem::filesystem_error &error) {
    std::cerr << "Failed to load from directory " << directory_name << ": "
              << error.what() << std::endl;
    return;
  }
}

std::optional<TextureManager::TextureAndSampler>
TextureManager::GetRandomTextureAndSampler(std::string random_name) {
  GLenum wrap_mode;
  GLenum filter_mode;
  std::string unqualified_name;

  ParseTextureSettingsFromName(random_name, &wrap_mode, &filter_mode,
                               &unqualified_name);
  unqualified_name = Lowercase(unqualified_name);

  std::vector<std::string> user_texture_names;
  size_t separator = unqualified_name.find("_");
  std::string texture_name_filter;

  if (separator != std::string::npos) {
    texture_name_filter = unqualified_name.substr(separator + 1);
    unqualified_name = unqualified_name.substr(0, separator);
  }

  for (auto k_v : named_textures_) {
    if (k_v.second->IsUserTexture()) {
      if (texture_name_filter.empty() ||
          k_v.first.find(texture_name_filter) == 0)
        user_texture_names.push_back(k_v.first);
    }
  }

  if (user_texture_names.size() > 0) {
    std::string random_texture_name =
        SelectRandomly(absl::Span<std::string>(user_texture_names));
    auto random_texture = named_textures_[random_texture_name];
    auto random_sampler =
        random_texture->GetSamplerForModes(wrap_mode, filter_mode);
    return TextureAndSampler({random_texture, random_sampler});
  }

  return std::nullopt;
}

void TextureManager::ParseTextureSettingsFromName(
    std::string_view name, GLenum *wrap_mode, GLenum *filter_mode,
    std::string *unqualified_name) {

  GLenum update_filter_mode = 0;
  GLenum update_wrap_mode = 0;
  bool invalid_name = false;

  if (name.size() > 3) {
    switch (name[0]) {
    case 'f':
    case 'F':
      update_filter_mode = GL_LINEAR;
      break;
    case 'p':
    case 'P':
      update_filter_mode = GL_NEAREST;
      break;
    default:
      std::cerr << "qualified name has invalid format specifier: " << name[0]
                << std::endl;
      invalid_name = true;
      break;
    }

    switch (name[1]) {
    case 'c':
    case 'C':
      update_wrap_mode = GL_CLAMP_TO_EDGE;
      break;
    case 'w':
    case 'W':
      update_wrap_mode = GL_REPEAT;
      break;
    default:
      std::cerr << "qualified name has invalid wrap specifier: " << name[0]
                << std::endl;
      invalid_name = true;
      break;
    }

    if (name[2] != '_') {
      invalid_name = true;
    }
  } else {
    invalid_name = true;
  }

  if (invalid_name) {
    *unqualified_name = name;
    return;
  }

  if (filter_mode != nullptr) {
    *filter_mode = update_filter_mode;
  }
  if (wrap_mode != nullptr) {
    *wrap_mode = update_wrap_mode;
  }
  *unqualified_name = name.substr(3);
}

std::shared_ptr<Texture> TextureManager::GetMainTexture() {
  return main_texture_;
}

absl::Span<std::shared_ptr<Texture>> TextureManager::GetBlurTextures() {
  return absl::Span<std::shared_ptr<Texture>>(blur_textures_);
}

void TextureManager::UpdateMainTexture() {
  glBindTexture(GL_TEXTURE_2D, main_texture_->GetId());
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, main_texture_->GetWidth(),
                      main_texture_->GetHeight());
  glBindTexture(GL_TEXTURE_2D, 0);
}
