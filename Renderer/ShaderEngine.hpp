/*
 * ShaderEngine.hpp
 *
 *  Created on: Jul 18, 2008
 *      Author: pete
 */

#ifndef SHADERENGINE_HPP_
#define SHADERENGINE_HPP_

#include <cstdlib>
#include <functional>
#include <glm/vec3.hpp>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>

#include "BeatDetect.hpp"
#include "Common.hpp"
#include "Pipeline.hpp"
#include "Shader.hpp"
#include "TextureManager.hpp"
#include "projectM-opengl.h"

class ShaderEngine {
 public:
  enum PresentShaderType {
    PresentCompositeShader,
    PresentWarpShader,
    PresentBlur1Shader,
    PresentBlur2Shader,
  };

  ShaderEngine(std::function<void()> activateCompileContext,
               std::function<void()> deactivateCompileContext);
  virtual ~ShaderEngine();
  void LoadPresetShadersAsync(Pipeline &pipeline, std::string_view preset_name);
  bool enableWarpShader(ShaderCache &shader, const Pipeline &pipeline,
                        const PipelineContext &pipelineContext,
                        const glm::mat4 &mat_ortho);
  bool enableCompositeShader(ShaderCache &shader, const Pipeline &pipeline,
                             const PipelineContext &pipelineContext);
  void RenderBlurTextures(const Pipeline &pipeline,
                          const PipelineContext &pipelineContext);
  void setParams(const int _texsizeX, const int texsizeY,
                 BeatDetect *beatDetect,
                 std::shared_ptr<TextureManager> texture_manager);

 private:
  int texsizeX;
  int texsizeY;
  float aspectX;
  float aspectY;
  BeatDetect *beatDetect;
  std::shared_ptr<TextureManager> texture_manager_;

  GLuint vboBlur;
  GLuint vaoBlur;

  float rand_preset[4];
  glm::vec3 xlate[20];
  glm::vec3 rot_base[20];
  glm::vec3 rot_speed[20];

  void ResetPerPresetState();
  void SetupShaderVariables(GLuint program, const Pipeline &pipeline,
                            const PipelineContext &pipelineContext);
  void SetupTextures(GLuint program, const ShaderCache &shader);
  void UpdateShaders(Pipeline *pipeline,
                     std::shared_ptr<Shader> composite_shader,
                     std::shared_ptr<Shader> warp_shader,
                     ShaderCache composite_shader_cache,
                     ShaderCache warp_shader_cache);

  void ResetShaders();
  void ResetShadersAsync();

  std::mutex program_reference_mutex_;
  // programs generated from preset shader code
  std::shared_ptr<Shader> composite_shader_, warp_shader_;

  std::thread compile_thread_;
  bool compile_complete_;
  std::condition_variable compile_complete_cv_;

  std::function<void()> activate_compile_context_, deactivate_compile_context_;
};

#endif /* SHADERENGINE_HPP_ */
