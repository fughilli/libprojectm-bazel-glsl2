/*
 * Pipeline.cpp
 *
 *  Created on: Jun 17, 2008
 *      Author: pete
 */
#include "Pipeline.hpp"

#include <algorithm>

Pipeline::Pipeline()
    : static_per_pixel_(false),
      gx_(0),
      gy_(0),
      blur1n(1),
      blur2n(1),
      blur3n(1),
      blur1x(1),
      blur2x(1),
      blur3x(1),
      blur1ed(1),
      textureWrap(false),
      screenDecay(false) {
  std::fill(q, q + NUM_Q_VARIABLES, 0);
  x_mesh_.reset(new float[1]);
  y_mesh_.reset(new float[1]);
}

void Pipeline::SetStaticPerPixel(int gx, int gy) {
  static_per_pixel_ = true;
  gx_ = gx;
  gy_ = gy;

  x_mesh_.reset(new float[gx * gy]);
  y_mesh_.reset(new float[gx * gy]);
}

Pipeline::~Pipeline() {}

PixelPoint Pipeline::PerPixel(PixelPoint p, const PerPixelContext context) {
  return p;
}
