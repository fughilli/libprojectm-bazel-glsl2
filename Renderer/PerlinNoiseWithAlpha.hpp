/*
 * PerlinNoise.hpp
 *
 *  Created and based on PerlinNoise.hpp
 *  Created on: Sep 14, 2019
 *      Author: hibengler
 */

/* The reason for this cousin class is that in Open GLES 2.0 environments,
the glTexImage2d cannot convert from GL_RGB into GL_RGBA
so the TextureManager has to use something that is pre-RGBA
*/
#ifndef PERLINNOISEWITHALPHA_HPP_
#define PERLINNOISEWITHALPHA_HPP_

#include <math.h>

#include <memory>
#include <utility>
#include <vector>

enum class Dimensionality : uint32_t {
  kDimensionality2d = 0,
  kDimensionality3d,
};

template <typename T>
class Image {
 public:
  static std::unique_ptr<Image<T>> Create(int width, int height,
                                          int num_channels) {
    auto return_pointer =
        std::unique_ptr<Image<T>>(new Image<T>(width, height, num_channels));
    return_pointer->data_.resize(width * height * num_channels, 0);
    return return_pointer;
  }

  static std::unique_ptr<Image<T>> Create(int width, int height, int depth,
                                          int num_channels) {
    auto return_pointer = std::unique_ptr<Image<T>>(
        new Image<T>(width, height, depth, num_channels));
    return_pointer->data_.resize(width * height * depth * num_channels, 0);
    return return_pointer;
  }

  T& at(int x, int y, int c) {
    assert(dimensionality_ == Dimensionality::kDimensionality2d);
    return data_[c + y * num_channels_ + x * num_channels_ * height_];
  }

  T& at(int x, int y, int z, int c) {
    assert(dimensionality_ == Dimensionality::kDimensionality3d);
    return data_[c + z * num_channels_ + y * num_channels_ * depth_ +
                 x * num_channels_ * depth_ * height_];
  }

  T* data() { return data_.data(); }

  int width() { return width_; }
  int height() { return height_; }
  int depth() { return depth_; }
  int num_channels() { return num_channels_; }
  Dimensionality dimensionality() { return dimensionality_; }

 private:
  Image(int width, int height, int num_channels)
      : dimensionality_(Dimensionality::kDimensionality2d),
        width_(width),
        height_(height),
        num_channels_(num_channels) {}
  Image(int width, int height, int depth, int num_channels)
      : dimensionality_(Dimensionality::kDimensionality3d),
        width_(width),
        height_(height),
        depth_(depth),
        num_channels_(num_channels) {}

  Dimensionality dimensionality_;
  int width_, height_, depth_, num_channels_;
  std::vector<T> data_;
};

void FillPerlin(Image<float>* image);
void FillPerlinScaled(float scale_x, float scale_y, Image<float>* image);

#endif /* PERLINNOISEWITHALPHA_HPP_ */
