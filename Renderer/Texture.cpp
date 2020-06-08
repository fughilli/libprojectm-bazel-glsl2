#include "Texture.hpp"

Sampler::Sampler(GLint wrap_mode, GLint filter_mode)
    : wrap_mode_(wrap_mode), filter_mode_(filter_mode) {
  glGenSamplers(1, &sampler_id_);
  glSamplerParameteri(sampler_id_, GL_TEXTURE_MIN_FILTER, filter_mode_);
  glSamplerParameteri(sampler_id_, GL_TEXTURE_MAG_FILTER, filter_mode_);
  glSamplerParameteri(sampler_id_, GL_TEXTURE_WRAP_S, wrap_mode_);
  glSamplerParameteri(sampler_id_, GL_TEXTURE_WRAP_T, wrap_mode_);
}

Sampler::~Sampler() { glDeleteSamplers(1, &sampler_id_); }

Texture::Texture(std::string name, ImageType image_type, int width, int height,
                 int depth, bool is_user_texture)
    : Texture(name, image_type, width, height, depth, is_user_texture, GL_RGB,
              GL_UNSIGNED_BYTE, nullptr) {}

Texture::Texture(std::string name, ImageType image_type, int width, int height,
                 int depth, bool is_user_texture, GLenum data_format,
                 GLenum data_type, void *data)
    : Texture(std::move(name), image_type, 0, GL_TEXTURE_2D, width, height,
              depth, is_user_texture) {
  glGenTextures(1, &texture_id_);
  glBindTexture(texture_type_, texture_id_);
  switch (image_type) {
  case ImageType::k2d:
    glTexImage2D(texture_type_, 0, GL_RGB, width_, height_, 0, data_format,
                 data_type, data);
    break;
  case ImageType::k3d:
    glTexImage3D(texture_type_, 0, GL_RGB, width_, height_, depth_, 0,
                 data_format, data_type, data);
    break;
  }
  glBindTexture(texture_type_, 0);
}

Texture::Texture(std::string name, ImageType image_type, GLuint texture_id,
                 GLenum texture_type, int width, int height, int depth,
                 bool is_user_texture)
    : name_(std::move(name)), image_type_(image_type), texture_id_(texture_id),
      texture_type_(texture_type), width_(width), height_(height),
      depth_(depth), is_user_texture_(is_user_texture) {}

Texture::~Texture() { glDeleteTextures(1, &texture_id_); }

std::shared_ptr<Sampler> Texture::GetSamplerForModes(GLint wrap_mode,
                                                     GLint filter_mode) {
  auto sampler = samplers_.find({wrap_mode, filter_mode});
  if (sampler == samplers_.end()) {
    auto insert_result =
        samplers_.insert({{wrap_mode, filter_mode},
                          std::make_shared<Sampler>(wrap_mode, filter_mode)});
    if (!insert_result.second) {
      return nullptr;
    }
    return insert_result.first->second;
  }
  return sampler->second;
}

std::shared_ptr<Sampler> Texture::GetFirstSampler() {
  for (auto &k_v : samplers_) {
    return k_v.second;
  }
  return nullptr;
}
