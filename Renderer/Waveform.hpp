/*
 * Waveform.hpp
 *
 *  Created on: Jun 25, 2008
 *      Author: pete
 */

#ifndef WAVEFORM_HPP_
#define WAVEFORM_HPP_

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <vector>

#include "Renderable.hpp"

class ColoredPoint {
 public:
  glm::vec2 position;
  glm::vec4 color;

  ColoredPoint()
      : position(glm::vec2(0.5, 0.5)), color(glm::vec4(1, 1, 1, 1)) {}

  static constexpr ptrdiff_t kPositionOffset = 0;
  static constexpr ptrdiff_t kColorOffset = sizeof(glm::vec4);
};

class WaveformContext {
 public:
  float sample;
  int samples;
  int sample_int;
  float left;
  float right;
  BeatDetect* music;

  WaveformContext(int samples, BeatDetect* music)
      : samples(samples), music(music) {}
};

class Waveform : public RenderItem {
 public:
  int samples;   /* number of samples associated with this wave form. Usually
                    powers of 2 */
  bool spectrum; /* spectrum data or pcm data */
  bool dots;     /* draw wave as dots or lines */
  bool thick;    /* draw thicker lines */
  bool additive; /* add color values together */

  float scaling;   /* scale factor of waveform */
  float smoothing; /* smooth factor of waveform */
  int sep;         /* no idea what this is yet... */

  Waveform(int _samples);
  void InitVertexAttrib();
  void Draw(RenderContext &context);

 private:
  virtual ColoredPoint PerPoint(ColoredPoint p,
                                const WaveformContext& context) = 0;
  std::vector<ColoredPoint> points;
  std::vector<float> pointContext;
  std::vector<float> left_channel_buffer_;
  std::vector<float> right_channel_buffer_;
};
#endif /* WAVEFORM_HPP_ */
