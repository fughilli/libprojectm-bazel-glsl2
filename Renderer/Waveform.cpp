/*
 * Waveform.hpp
 *
 *  Created on: Jun 25, 2008
 *      Author: pete
 */

#include "Waveform.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

#include "BeatDetect.hpp"
#include "ShaderEngine.hpp"
#include "StaticShaders.hpp"
#include "projectM-opengl.h"
#ifdef WIN32
#include <functional>
#endif /** WIN32 */

namespace {
constexpr int kDefaultTextureSize = 512;
constexpr int kPositionVertexAttribIndex = 0;
constexpr int kColorVertexAttribIndex = 1;
constexpr float kFreqDomainCoefficient = 0.015f;
constexpr float kTimeDomainCoefficient = 1.0f;
constexpr int kThickLineMultiplier = 2;
constexpr int kThinLineMultiplier = 1;
}  // namespace

Waveform::Waveform(int _samples)
    : RenderItem(),
      samples(_samples),
      points(_samples),
      pointContext(_samples),
      left_channel_buffer_(_samples),
      right_channel_buffer_(_samples) {
  spectrum = false; /* spectrum data or pcm data */
  dots = false;     /* draw wave as dots or lines */
  thick = false;    /* draw thicker lines */
  additive = false; /* add color values together */

  scaling = 1;   /* scale factor of waveform */
  smoothing = 0; /* smooth factor of waveform */
  sep = 0;

  Init();
}

void Waveform::InitVertexAttrib() {
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(
      0, 2, GL_FLOAT, GL_FALSE, sizeof(ColoredPoint),
      reinterpret_cast<void *>(ColoredPoint::kPositionOffset));  // points
  glVertexAttribPointer(
      1, 4, GL_FLOAT, GL_FALSE, sizeof(ColoredPoint),
      reinterpret_cast<void *>(ColoredPoint::kColorOffset));  // colors
}

void Waveform::Draw(RenderContext &context) {
  // scale PCM data based on vol_history to make it more or less independent of
  // the application output volume
  const float vol_scale =
      context.beatDetect->getPCMScale() * scaling *
      (spectrum ? kFreqDomainCoefficient : kTimeDomainCoefficient);

  context.beatDetect->pcm->getPCM(left_channel_buffer_.data(), samples, 0,
                                  spectrum, smoothing, 0);
  context.beatDetect->pcm->getPCM(right_channel_buffer_.data(), samples, 1,
                                  spectrum, smoothing, 0);

  for (auto &sample : left_channel_buffer_) {
    sample *= vol_scale;
  }
  for (auto &sample : right_channel_buffer_) {
    sample *= vol_scale;
  }

  WaveformContext wave_context(samples, context.beatDetect);

  for (int x = 0; x < samples; ++x) {
    wave_context.sample = x / static_cast<float>(samples - 1);
    wave_context.sample_int = x;
    wave_context.left = vol_scale * left_channel_buffer_[x];
    wave_context.right = vol_scale * right_channel_buffer_[x];

    points[x] = PerPoint(points[x], wave_context);
  }

  std::vector<ColoredPoint> points_transformed = points;

  for (auto &point : points_transformed) {
    point.position.y = 1 - point.position.y;
    point.color.a *= masterAlpha;
  }

  glBindBuffer(GL_ARRAY_BUFFER, m_vboID);

  glBufferData(GL_ARRAY_BUFFER, sizeof(ColoredPoint) * samples, nullptr,
               GL_DYNAMIC_DRAW);
  glBufferData(GL_ARRAY_BUFFER, sizeof(ColoredPoint) * samples,
               points_transformed.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glUseProgram(StaticShaders::Get()->program_v2f_c4f_->GetId());

  glUniformMatrix4fv(
      StaticShaders::Get()->uniform_v2f_c4f_vertex_tranformation_, 1, GL_FALSE,
      glm::value_ptr(context.mat_ortho));

  if (additive) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  } else {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  int thick_line_width =
      context.texsize <= kDefaultTextureSize
          ? kThickLineMultiplier
          : kThickLineMultiplier * context.texsize / kDefaultTextureSize;
  int thin_line_width =
      context.texsize <= kDefaultTextureSize
          ? kThinLineMultiplier
          : kThinLineMultiplier * context.texsize / kDefaultTextureSize;
  if (thick) {
    glLineWidth(thick_line_width);
#ifndef GL_TRANSITION
    glPointSize(thick_line_width);
#endif
    glUniform1f(StaticShaders::Get()->uniform_v2f_c4f_vertex_point_size_,
                thick_line_width);
  } else {
#ifndef GL_TRANSITION
    glPointSize(thin_line_width);
#endif
    glUniform1f(StaticShaders::Get()->uniform_v2f_c4f_vertex_point_size_,
                thin_line_width);
  }

  glBindVertexArray(m_vaoID);

  if (dots) {
    glDrawArrays(GL_POINTS, 0, samples);
  } else {
    glDrawArrays(GL_LINE_STRIP, 0, samples);
  }

  glBindVertexArray(0);
  glLineWidth(thin_line_width);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
