#ifndef TEXTURE_HPP_
#define TEXTURE_HPP_

#include "projectM-opengl.h"
#include <map>
#include <string>
#include <string_view>
#include <vector>

// Wrapper for an OpenGL sampler instance.
class Sampler {
public:
  // Constructs a new sampler, initializing its wrap and filter modes to the
  // provided values.
  Sampler(GLint wrap_mode, GLint filter_mode);
  ~Sampler();

  GLint GetId() const { return sampler_id_; }

private:
  GLuint sampler_id_;
  GLint wrap_mode_;
  GLint filter_mode_;
};

class Texture {
public:
  enum ImageType { k2d = 0, k3d };

  Texture(std::string name, ImageType image_type, int width, int height,
          int depth, bool is_user_texture);
  Texture(std::string name, ImageType image_type, int width, int height,
          int depth, bool is_user_texture, GLenum data_format, GLenum data_type,
          void *data);
  Texture(std::string name, ImageType image_type, GLuint texture_id,
          GLenum texture_type, int width, int height, int depth,
          bool is_user_texture);
  ~Texture();

  // Gets a sampler for this texture with the specified modes.
  std::shared_ptr<Sampler> GetSamplerForModes(GLint wrap_mode,
                                              GLint filter_mode);

  std::shared_ptr<Sampler> GetFirstSampler();

  bool IsUserTexture() const { return is_user_texture_; }
  GLint GetId() const { return texture_id_; }
  GLenum GetType() const { return texture_type_; }
  std::string_view GetName() const { return name_; }
  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }
  int GetDepth() const { return depth_; }

private:
  std::string name_;
  ImageType image_type_;
  GLuint texture_id_;
  GLenum texture_type_;
  int width_;
  int height_;
  int depth_;
  bool is_user_texture_;
  std::map<std::pair<GLint, GLint>, std::shared_ptr<Sampler>> samplers_;
};

#endif /* TEXTURE_HPP_ */
