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

#include <math.h>;

class PerlinNoiseWithAlpha {
 public:
  float noise_lq[256][256][4];
  float noise_lq_lite[32][32][4];
  float noise_mq[256][256][4];
  float noise_hq[256][256][4];
  float noise_lq_vol[32][32][32][4];
  float noise_hq_vol[32][32][32][4];

  PerlinNoiseWithAlpha();
};

#endif /* PERLINNOISEWITHALPHA_HPP_ */
