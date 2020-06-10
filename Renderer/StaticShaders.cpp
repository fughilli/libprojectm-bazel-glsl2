#include "StaticShaders.hpp"

#include "StaticGlShaders.h"

StaticShaders::StaticShaders() {
  std::shared_ptr<StaticGlShaders> static_gl_shaders = StaticGlShaders::Get();

  program_v2f_c4f_ = Shader::CompileShaderProgram(
      static_gl_shaders->GetV2fC4fVertexShader(),
      static_gl_shaders->GetV2fC4fFragmentShader(), "v2f_c4f");
  program_v2f_c4f_t2f_ = Shader::CompileShaderProgram(
      static_gl_shaders->GetV2fC4fT2fVertexShader(),
      static_gl_shaders->GetV2fC4fT2fFragmentShader(), "v2f_c4f_t2f");

  program_blur1_ = Shader::CompileShaderProgram(
      static_gl_shaders->GetBlurVertexShader(),
      static_gl_shaders->GetBlur1FragmentShader(), "blur1");
  program_blur2_ = Shader::CompileShaderProgram(
      static_gl_shaders->GetBlurVertexShader(),
      static_gl_shaders->GetBlur2FragmentShader(), "blur2");

  uniform_v2f_c4f_vertex_tranformation_ =
      glGetUniformLocation(program_v2f_c4f_->GetId(), "vertex_transformation");
  uniform_v2f_c4f_vertex_point_size_ =
      glGetUniformLocation(program_v2f_c4f_->GetId(), "vertex_point_size");
  uniform_v2f_c4f_t2f_vertex_tranformation_ = glGetUniformLocation(
      program_v2f_c4f_t2f_->GetId(), "vertex_transformation");
  uniform_v2f_c4f_t2f_frag_texture_sampler_ =
      glGetUniformLocation(program_v2f_c4f_t2f_->GetId(), "texture_sampler");

  uniform_blur1_sampler_ =
      glGetUniformLocation(program_blur1_->GetId(), "texture_sampler");
  uniform_blur1_c0_ = glGetUniformLocation(program_blur1_->GetId(), "_c0");
  uniform_blur1_c1_ = glGetUniformLocation(program_blur1_->GetId(), "_c1");
  uniform_blur1_c2_ = glGetUniformLocation(program_blur1_->GetId(), "_c2");
  uniform_blur1_c3_ = glGetUniformLocation(program_blur1_->GetId(), "_c3");

  uniform_blur2_sampler_ =
      glGetUniformLocation(program_blur2_->GetId(), "texture_sampler");
  uniform_blur2_c0_ = glGetUniformLocation(program_blur2_->GetId(), "_c0");
  uniform_blur2_c5_ = glGetUniformLocation(program_blur2_->GetId(), "_c5");
  uniform_blur2_c6_ = glGetUniformLocation(program_blur2_->GetId(), "_c6");
}
