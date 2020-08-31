/*
 * ShaderEngine.cpp
 *
 *  Created on: Jul 18, 2008
 *      Author: pete
 */
#include "ShaderEngine.hpp"

#include <algorithm>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>  // glm::mat4
#include <regex>
#include <set>

#include "BeatDetect.hpp"
#include "GLSLGenerator.h"
#include "HLSLParser.h"
#include "StaticGlShaders.h"
#include "StaticShaders.hpp"
#include "Texture.hpp"

#define FRAND ((rand() % 7381) / 7380.0f)

constexpr GLuint kDefaultNoiseTextureSamplingMode = GL_REPEAT;

ShaderEngine::ShaderEngine(std::function<void()> activateCompileContext,
                           std::function<void()> deactivateCompileContext)
    : activate_compile_context_(activateCompileContext),
      deactivate_compile_context_(deactivateCompileContext),
      compile_complete_(true) {
  // TODO: This is a complete hack to get the static shaders set up before we
  // call `enable*`.
  StaticShaders::Get();

  // Initialize Blur vao/vbo
  float pointsBlur[16] = {-1.0, -1.0, 0.0, 1.0, 1.0, -1.0, 1.0, 1.0,
                          -1.0, 1.0,  0.0, 0.0, 1.0, 1.0,  1.0, 0.0};

  glGenBuffers(1, &vboBlur);
  glGenVertexArrays(1, &vaoBlur);

  glBindVertexArray(vaoBlur);
  glBindBuffer(GL_ARRAY_BUFFER, vboBlur);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8 * 2, pointsBlur,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                        (void *)0);  // Positions

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                        (void *)(sizeof(float) * 2));  // Textures

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

ShaderEngine::~ShaderEngine() {
  glDeleteBuffers(1, &vboBlur);
  glDeleteVertexArrays(1, &vaoBlur);
}

void ShaderEngine::setParams(const int _texsizeX, const int _texsizeY,
                             BeatDetect *_beatDetect,
                             std::shared_ptr<TextureManager> texture_manager) {
  this->beatDetect = _beatDetect;
  texture_manager_ = texture_manager;

  aspectX = 1;
  aspectY = 1;
  if (_texsizeX > _texsizeY)
    aspectY = (float)_texsizeY / (float)_texsizeX;
  else
    aspectX = (float)_texsizeX / (float)_texsizeY;

  this->texsizeX = _texsizeX;
  this->texsizeY = _texsizeY;
}

namespace {
constexpr std::string_view kProgramStartBrace = "{";
constexpr std::string_view kProgramEndBrace = "}";
constexpr std::string_view kShaderBodyTag = "shader_body";

bool ApplyHlslShaderSourceTransformations(
    ShaderEngine::PresentShaderType shader_type, std::string program_source,
    std::string *out_program_source) {
  if (out_program_source == nullptr) {
    std::cerr << "Outparameter was null" << std::endl;
    return false;
  }
  if (program_source.empty()) {
    std::cerr << "Input program source is empty" << std::endl;
    return false;
  }

  size_t found = 0;
  if ((found = program_source.rfind(kProgramEndBrace.data())) ==
      std::string::npos) {
    std::cerr << "Could not find end brace" << std::endl;
    return false;
  }

  // replace "}" with return statement (this can probably be optimized for the
  // GLSL conversion...)
  program_source.replace(int(found), kProgramEndBrace.size(),
                         "_return_value = float4(ret.xyz, 1.0);\n"
                         "}\n");

  // replace shader_body with entry point function
  if ((found = program_source.find(kShaderBodyTag.data())) ==
      std::string::npos) {
    std::cerr << "Could not find body tag" << std::endl;
    return false;
  }

  std::stringstream program_function;
  program_function << "void PS(float4 _vDiffuse : COLOR, "
                   << ((shader_type ==
                        ShaderEngine::PresentShaderType::PresentWarpShader)
                           ? "float4"
                           : "float2")
                   << " _uv : TEXCOORD0, float2 _rad_ang : TEXCOORD1, out "
                      "float4 _return_value : COLOR)\n";
  program_source.replace(int(found), kShaderBodyTag.size(),
                         program_function.str().c_str());
  // replace "{" with some variable declarations
  if ((found = program_source.find(kProgramStartBrace.data(), found)) ==
      std::string::npos) {
    std::cerr << "Could not find start brace" << std::endl;
    return false;
  }
  program_source.replace(found, kProgramStartBrace.size(),
                         "{\nfloat3 ret = 0;\n");

  // prepend our HLSL template to the actual program source
  *out_program_source = StaticGlShaders::Get()->GetPresetShaderHeader();

  if (shader_type == ShaderEngine::PresentShaderType::PresentWarpShader) {
    out_program_source->append(
        "#define rad _rad_ang.x\n"
        "#define ang _rad_ang.y\n"
        "#define uv _uv.xy\n"
        "#define uv_orig _uv.zw\n");
  } else {
    out_program_source->append(
        "#define rad _rad_ang.x\n"
        "#define ang _rad_ang.y\n"
        "#define uv _uv.xy\n"
        "#define uv_orig _uv.xy\n"
        "#define hue_shader _vDiffuse.xyz\n");
  }

  out_program_source->append(program_source);
  return true;
}

// Transpile a user-defined HLSL shader from a preset into GLSL, and then
// compile the GLSL into a Shader object. returns a shared pointer to a valid
// Shader if successful.
std::shared_ptr<Shader> TranspilePresetShader(
    std::shared_ptr<TextureManager> texture_manager,
    ShaderEngine::PresentShaderType shader_type, std::string shader_filename,
    std::string shader_source, ShaderCache *shader) {
  ShaderCache new_shader;
  std::string transformed_hlsl_source;
  if (!ApplyHlslShaderSourceTransformations(shader_type, shader_source,
                                            &transformed_hlsl_source)) {
    std::cerr << "Failed to apply source transformations." << std::endl;
    return nullptr;
  }

  // Add builtin textures
  // TODO: Ensure that these are the sampler properties acquired for these
  // textures
  new_shader.textures_and_samplers["main"] =
      texture_manager->GetTextureAndSampler("main", GL_REPEAT, GL_LINEAR)
          .value();

  new_shader.textures_and_samplers["noise_lq"] =
      texture_manager
          ->GetTextureAndSampler("noise_lq", kDefaultNoiseTextureSamplingMode,
                                 GL_LINEAR)
          .value();
  new_shader.textures_and_samplers["noise_lq_lite"] =
      texture_manager
          ->GetTextureAndSampler("noise_lq_lite",
                                 kDefaultNoiseTextureSamplingMode, GL_LINEAR)
          .value();
  new_shader.textures_and_samplers["noise_mq"] =
      texture_manager
          ->GetTextureAndSampler("noise_mq", kDefaultNoiseTextureSamplingMode,
                                 GL_LINEAR)
          .value();
  new_shader.textures_and_samplers["noise_hq"] =
      texture_manager
          ->GetTextureAndSampler("noise_hq", kDefaultNoiseTextureSamplingMode,
                                 GL_LINEAR)
          .value();
  new_shader.textures_and_samplers["noisevol_lq"] =
      texture_manager
          ->GetTextureAndSampler("noisevol_lq",
                                 kDefaultNoiseTextureSamplingMode, GL_LINEAR)
          .value();
  new_shader.textures_and_samplers["noisevol_hq"] =
      texture_manager
          ->GetTextureAndSampler("noisevol_hq",
                                 kDefaultNoiseTextureSamplingMode, GL_LINEAR)
          .value();

  // set up texture samplers for all samplers references in the shader program
  size_t found = 0;
  found = transformed_hlsl_source.find("sampler_", found);
  while (found != std::string::npos) {
    found += 8;
    size_t end = transformed_hlsl_source.find_first_of(" ;,\n\r)", found);

    if (end != std::string::npos) {
      std::string sampler =
          transformed_hlsl_source.substr((int)found, (int)end - found);
      std::string lowerCaseName(sampler);
      std::transform(lowerCaseName.begin(), lowerCaseName.end(),
                     lowerCaseName.begin(), tolower);

      auto texture_and_sampler =
          texture_manager->GetTextureAndSampler(sampler, GL_REPEAT, GL_LINEAR);

      if (!texture_and_sampler.has_value()) {
        texture_and_sampler = texture_manager->LoadTextureAndSampler(sampler);
      }

      if (!texture_and_sampler.has_value()) {
        std::cerr << "Texture loading error for: " << sampler << std::endl;
      } else {
        if (new_shader.textures_and_samplers.find(sampler) ==
            new_shader.textures_and_samplers.end()) {
          new_shader.textures_and_samplers[sampler] =
              texture_and_sampler.value();
        }
      }
    }
    found = transformed_hlsl_source.find("sampler_", found);
  }

  found = transformed_hlsl_source.find("GetBlur3");
  if (found != std::string::npos) {
    new_shader.textures_and_samplers["blur3"] =
        texture_manager
            ->GetTextureAndSampler("blur3", GL_CLAMP_TO_EDGE, GL_LINEAR)
            .value();
    new_shader.textures_and_samplers["blur2"] =
        texture_manager
            ->GetTextureAndSampler("blur2", GL_CLAMP_TO_EDGE, GL_LINEAR)
            .value();
    new_shader.textures_and_samplers["blur1"] =
        texture_manager
            ->GetTextureAndSampler("blur1", GL_CLAMP_TO_EDGE, GL_LINEAR)
            .value();
  } else {
    found = transformed_hlsl_source.find("GetBlur2");
    if (found != std::string::npos) {
      new_shader.textures_and_samplers["blur2"] =
          texture_manager
              ->GetTextureAndSampler("blur2", GL_CLAMP_TO_EDGE, GL_LINEAR)
              .value();
      new_shader.textures_and_samplers["blur1"] =
          texture_manager
              ->GetTextureAndSampler("blur1", GL_CLAMP_TO_EDGE, GL_LINEAR)
              .value();
    } else {
      found = transformed_hlsl_source.find("GetBlur1");
      if (found != std::string::npos) {
        new_shader.textures_and_samplers["blur1"] =
            texture_manager
                ->GetTextureAndSampler("blur1", GL_CLAMP_TO_EDGE, GL_LINEAR)
                .value();
      }
    }
  }

  std::string shaderTypeString;
  switch (shader_type) {
    case ShaderEngine::PresentShaderType::PresentWarpShader:
      shaderTypeString = "Warp";
      break;
    case ShaderEngine::PresentShaderType::PresentCompositeShader:
      shaderTypeString = "Comp";
      break;
    case ShaderEngine::PresentShaderType::PresentBlur1Shader:
      shaderTypeString = "Blur1";
      break;
    case ShaderEngine::PresentShaderType::PresentBlur2Shader:
      shaderTypeString = "Blur2";
      break;
    default:
      shaderTypeString = "Other";
  }

  M4::GLSLGenerator generator;
  M4::Allocator allocator;

  M4::HLSLTree tree(&allocator);
  M4::HLSLParser parser(&allocator, &tree);

  // preprocess define macros
  std::string sourcePreprocessed;
  if (!parser.ApplyPreprocessor(
          shader_filename.c_str(), transformed_hlsl_source.c_str(),
          transformed_hlsl_source.size(), sourcePreprocessed)) {
    std::cerr << "Failed to preprocess HLSL(step1) " << shaderTypeString
              << " shader" << std::endl;

#if !DUMP_SHADERS_ON_ERROR
    std::cerr << "Source: " << std::endl
              << transformed_hlsl_source << std::endl;
#else
    std::ofstream out("/tmp/shader_" + shaderTypeString + "_step1.txt");
    out << transformed_hlsl_source;
    out.close();
#endif
    return nullptr;
  }

  // Remove previous shader declarations
  std::smatch matches;
  while (std::regex_search(sourcePreprocessed, matches,
                           std::regex("sampler(2D|3D|)(\\s+|\\().*"))) {
    sourcePreprocessed.replace(matches.position(), matches.length(), "");
  }

  // Remove previous texsize declarations
  while (std::regex_search(sourcePreprocessed, matches,
                           std::regex("float4\\s+texsize_.*"))) {
    sourcePreprocessed.replace(matches.position(), matches.length(), "");
  }

  // Declare samplers
  std::set<std::string> texsizes;
  for (auto &k_v : new_shader.textures_and_samplers) {
    auto texture = k_v.second.texture;

    if (texture->GetType() == GL_TEXTURE_3D) {
      sourcePreprocessed.insert(
          0, "uniform sampler3D sampler_" + k_v.first + ";\n");
    } else {
      sourcePreprocessed.insert(
          0, "uniform sampler2D sampler_" + k_v.first + ";\n");
    }

    texsizes.insert(k_v.first);
    texsizes.insert(std::string(texture->GetName()));
  }

  // Declare texsizes
  std::set<std::string>::const_iterator iter_texsizes = texsizes.cbegin();
  for (; iter_texsizes != texsizes.cend(); ++iter_texsizes) {
    sourcePreprocessed.insert(
        0, "uniform float4 texsize_" + *iter_texsizes + ";\n");
  }

  // transpile from HLSL (aka preset shader aka directX shader) to GLSL (aka
  // OpenGL shader lang)

  // parse
  if (!parser.Parse(shader_filename.c_str(), sourcePreprocessed.c_str(),
                    sourcePreprocessed.size())) {
    std::cerr << "Failed to parse HLSL(step2) " << shaderTypeString << " shader"
              << std::endl;

#if !DUMP_SHADERS_ON_ERROR
    std::cerr << "Source: " << std::endl << sourcePreprocessed << std::endl;
#else
    std::ofstream out2("/tmp/shader_" + shaderTypeString + "_step2.txt");
    out2 << sourcePreprocessed;
    out2.close();
#endif
    return nullptr;
  }

  // generate GLSL
  if (!generator.Generate(&tree, M4::GLSLGenerator::Target_FragmentShader,
                          StaticGlShaders::Get()->GetGlslGeneratorVersion(),
                          "PS")) {
    std::cerr << "Failed to transpile HLSL(step3) " << shaderTypeString
              << " shader to GLSL" << std::endl;
#if !DUMP_SHADERS_ON_ERROR
    std::cerr << "Source: " << std::endl << sourcePreprocessed << std::endl;
#else
    std::ofstream out2("/tmp/shader_" + shaderTypeString + "_step2.txt");
    out2 << sourcePreprocessed;
    out2.close();
#endif
    return nullptr;
  }

  // now we have GLSL source for the preset shader program (hopefully it's
  // valid!) copmile the preset shader fragment shader with the standard
  // vertex shader and cross our fingers
  std::shared_ptr<Shader> return_shader;
  if (shader_type == ShaderEngine::PresentShaderType::PresentWarpShader) {
    return_shader = Shader::CompileShaderProgram(
        StaticGlShaders::Get()->GetPresetWarpVertexShader(),
        generator.GetResult(),
        shaderTypeString);  // returns new program
  } else {
    return_shader = Shader::CompileShaderProgram(
        StaticGlShaders::Get()->GetPresetCompVertexShader(),
        generator.GetResult(),
        shaderTypeString);  // returns new program
  }

  if (return_shader == nullptr) {
    std::cerr << "Compilation error (step3) of " << shaderTypeString
              << std::endl;

#if !DUMP_SHADERS_ON_ERROR
    std::cerr << "Source:" << std::endl << generator.GetResult() << std::endl;
#else
    std::ofstream out3("/tmp/shader_" + shaderTypeString + "_step3.txt");
    out3 << generator.GetResult();
    out3.close();
#endif
    return nullptr;
  }

  std::cerr << "Successful compilation of " << shaderTypeString << std::endl;
  *shader = new_shader;
  return return_shader;
}
}  // namespace

void ShaderEngine::SetupShaderVariables(GLuint program,
                                        const Pipeline &pipeline,
                                        const PipelineContext &context) {
  // pass info from projectM to the shader uniforms
  // these are the inputs:
  // http://www.geisswerks.com/milkdrop/milkdrop_preset_authoring.html#3f6

  float time_since_preset_start = context.time - context.presetStartTime;
  float time_since_preset_start_wrapped =
      time_since_preset_start - (int)(time_since_preset_start / 10000) * 10000;
  float mip_x = logf((float)texsizeX) / logf(2.0f);
  float mip_y = logf((float)texsizeX) / logf(2.0f);
  float mip_avg = 0.5f * (mip_x + mip_y);

  glUniform4f(glGetUniformLocation(program, "rand_frame"), (rand() % 100) * .01,
              (rand() % 100) * .01, (rand() % 100) * .01, (rand() % 100) * .01);
  glUniform4f(glGetUniformLocation(program, "rand_preset"), rand_preset[0],
              rand_preset[1], rand_preset[2], rand_preset[3]);

  glUniform4f(glGetUniformLocation(program, "_c0"), aspectX, aspectY,
              1 / aspectX, 1 / aspectY);
  glUniform4f(glGetUniformLocation(program, "_c1"), 0.0, 0.0, 0.0, 0.0);
  glUniform4f(glGetUniformLocation(program, "_c2"),
              time_since_preset_start_wrapped, context.fps, context.frame,
              context.progress);
  glUniform4f(glGetUniformLocation(program, "_c3"), beatDetect->bass / 100,
              beatDetect->mid / 100, beatDetect->treb / 100,
              beatDetect->vol / 100);
  glUniform4f(glGetUniformLocation(program, "_c4"), beatDetect->bass_att / 100,
              beatDetect->mid_att / 100, beatDetect->treb_att / 100,
              beatDetect->vol_att / 100);
  glUniform4f(glGetUniformLocation(program, "_c5"),
              pipeline.blur1x - pipeline.blur1n, pipeline.blur1n,
              pipeline.blur2x - pipeline.blur2n, pipeline.blur2n);
  glUniform4f(glGetUniformLocation(program, "_c6"),
              pipeline.blur3x - pipeline.blur3n, pipeline.blur3n,
              pipeline.blur1n, pipeline.blur1x);
  glUniform4f(glGetUniformLocation(program, "_c7"), texsizeX, texsizeY,
              1 / (float)texsizeX, 1 / (float)texsizeY);

  glUniform4f(glGetUniformLocation(program, "_c8"),
              0.5f + 0.5f * cosf(context.time * 0.329f + 1.2f),
              0.5f + 0.5f * cosf(context.time * 1.293f + 3.9f),
              0.5f + 0.5f * cosf(context.time * 5.070f + 2.5f),
              0.5f + 0.5f * cosf(context.time * 20.051f + 5.4f));

  glUniform4f(glGetUniformLocation(program, "_c9"),
              0.5f + 0.5f * sinf(context.time * 0.329f + 1.2f),
              0.5f + 0.5f * sinf(context.time * 1.293f + 3.9f),
              0.5f + 0.5f * sinf(context.time * 5.070f + 2.5f),
              0.5f + 0.5f * sinf(context.time * 20.051f + 5.4f));

  glUniform4f(glGetUniformLocation(program, "_c10"),
              0.5f + 0.5f * cosf(context.time * 0.0050f + 2.7f),
              0.5f + 0.5f * cosf(context.time * 0.0085f + 5.3f),
              0.5f + 0.5f * cosf(context.time * 0.0133f + 4.5f),
              0.5f + 0.5f * cosf(context.time * 0.0217f + 3.8f));

  glUniform4f(glGetUniformLocation(program, "_c11"),
              0.5f + 0.5f * sinf(context.time * 0.0050f + 2.7f),
              0.5f + 0.5f * sinf(context.time * 0.0085f + 5.3f),
              0.5f + 0.5f * sinf(context.time * 0.0133f + 4.5f),
              0.5f + 0.5f * sinf(context.time * 0.0217f + 3.8f));

  glUniform4f(glGetUniformLocation(program, "_c12"), mip_x, mip_y, mip_avg, 0);
  glUniform4f(glGetUniformLocation(program, "_c13"), pipeline.blur2n,
              pipeline.blur2x, pipeline.blur3n, pipeline.blur3x);

  glm::mat4 temp_mat[24];

  // write matrices
  for (int i = 0; i < 20; i++) {
    glm::mat4 mx, my, mz, mxlate;

    mx = glm::rotate(glm::mat4(1.0f),
                     rot_base[i].x + rot_speed[i].x * context.time,
                     glm::vec3(1.0f, 0.0f, 0.0f));
    my = glm::rotate(glm::mat4(1.0f),
                     rot_base[i].y + rot_speed[i].y * context.time,
                     glm::vec3(0.0f, 1.0f, 0.0f));
    mz = glm::rotate(glm::mat4(1.0f),
                     rot_base[i].z + rot_speed[i].z * context.time,
                     glm::vec3(0.0f, 0.0f, 1.0f));

    mxlate = glm::translate(glm::mat4(1.0f),
                            glm::vec3(xlate[i].x, xlate[i].y, xlate[i].z));

    temp_mat[i] = mxlate * mx;
    temp_mat[i] = mz * temp_mat[i];
    temp_mat[i] = my * temp_mat[i];
  }

  // the last 4 are totally random, each frame
  for (int i = 20; i < 24; i++) {
    glm::mat4 mx, my, mz, mxlate;

    mx = glm::rotate(glm::mat4(1.0f), FRAND * 6.28f,
                     glm::vec3(1.0f, 0.0f, 0.0f));
    my = glm::rotate(glm::mat4(1.0f), FRAND * 6.28f,
                     glm::vec3(0.0f, 1.0f, 0.0f));
    mz = glm::rotate(glm::mat4(1.0f), FRAND * 6.28f,
                     glm::vec3(0.0f, 0.0f, 1.0f));

    mxlate = glm::translate(glm::mat4(1.0f), glm::vec3(FRAND, FRAND, FRAND));

    temp_mat[i] = mxlate * mx;
    temp_mat[i] = mz * temp_mat[i];
    temp_mat[i] = my * temp_mat[i];
  }

  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_s1"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[0])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_s2"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[1])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_s3"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[2])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_s4"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[3])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_d1"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[4])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_d2"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[5])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_d3"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[6])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_d4"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[7])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_f1"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[8])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_f2"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[9])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_f3"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[10])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_f4"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[11])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_vf1"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[12])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_vf2"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[13])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_vf3"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[14])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_vf4"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[15])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_uf1"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[16])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_uf2"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[17])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_uf3"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[18])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_uf4"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[19])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_rand1"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[20])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_rand2"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[21])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_rand3"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[22])));
  glUniformMatrix3x4fv(glGetUniformLocation(program, "rot_rand4"), 1, GL_FALSE,
                       glm::value_ptr(glm::mat4(temp_mat[23])));

  // set program uniform "_q[a-h]" values (_qa.x, _qa.y, _qa.z, _qa.w, _qb.x,
  // _qb.y ... ) alias q[1-32]
  for (int i = 0; i < 32; i += 4) {
    std::string varName = "_q";
    varName.push_back('a' + i / 4);
    glUniform4f(glGetUniformLocation(program, varName.c_str()), pipeline.q[i],
                pipeline.q[i + 1], pipeline.q[i + 2], pipeline.q[i + 3]);
  }
}

void ShaderEngine::SetupTextures(GLuint program, const ShaderCache &shader) {
  unsigned int texNum = 0;
  std::map<std::string, std::shared_ptr<Texture>> texsizes;

  // Set samplers
  for (auto &k_v : shader.textures_and_samplers) {
    auto texture = k_v.second.texture;
    auto sampler = k_v.second.sampler;

    std::string samplerName = "sampler_" + k_v.first;

    // https://www.khronos.org/opengl/wiki/Sampler_(GLSL)#Binding_textures_to_samplers
    GLint param = glGetUniformLocation(program, samplerName.c_str());
    if (param < 0) {
      // unused uniform have been optimized out by glsl compiler
      continue;
    }

    texsizes[k_v.first] = texture;
    texsizes[std::string(texture->GetName())] = texture;

    glActiveTexture(GL_TEXTURE0 + texNum);
    glBindTexture(texture->GetType(), texture->GetId());
    glBindSampler(texNum, sampler->GetId());

    glUniform1i(param, texNum);
    texNum++;
  }

  // Set texsizes
  for (auto &k_v : texsizes) {
    auto texture = k_v.second;

    std::string texsizeName =
        (std::stringstream() << "texsize_" << k_v.first).str();
    GLint textSizeParam = glGetUniformLocation(program, texsizeName.c_str());
    if (textSizeParam >= 0) {
      glUniform4f(textSizeParam, texture->GetWidth(), texture->GetHeight(),
                  1 / (float)texture->GetWidth(),
                  1 / (float)texture->GetHeight());
    } else {
      // unused uniform have been optimized out by glsl compiler
      continue;
    }
  }
}

void ShaderEngine::RenderBlurTextures(const Pipeline &pipeline,
                                      const PipelineContext &pipelineContext) {
  unsigned int passes = 6;

  const float w[8] = {4.0f, 3.8f, 3.5f, 2.9f,
                      1.9f, 1.2f, 0.7f, 0.3f};  //<- user can specify these
  float edge_darken = pipeline.blur1ed;
  float blur_min[3], blur_max[3];

  blur_min[0] = pipeline.blur1n;
  blur_min[1] = pipeline.blur2n;
  blur_min[2] = pipeline.blur3n;
  blur_max[0] = pipeline.blur1x;
  blur_max[1] = pipeline.blur2x;
  blur_max[2] = pipeline.blur3x;

  // check that precision isn't wasted in later blur passes [...min-max gap
  // can't grow!] also, if min-max are close to each other, push them apart:
  const float fMinDist = 0.1f;
  if (blur_max[0] - blur_min[0] < fMinDist) {
    float avg = (blur_min[0] + blur_max[0]) * 0.5f;
    blur_min[0] = avg - fMinDist * 0.5f;
    blur_max[0] = avg - fMinDist * 0.5f;
  }
  blur_max[1] = std::min(blur_max[0], blur_max[1]);
  blur_min[1] = std::max(blur_min[0], blur_min[1]);
  if (blur_max[1] - blur_min[1] < fMinDist) {
    float avg = (blur_min[1] + blur_max[1]) * 0.5f;
    blur_min[1] = avg - fMinDist * 0.5f;
    blur_max[1] = avg - fMinDist * 0.5f;
  }
  blur_max[2] = std::min(blur_max[1], blur_max[2]);
  blur_min[2] = std::max(blur_min[1], blur_min[2]);
  if (blur_max[2] - blur_min[2] < fMinDist) {
    float avg = (blur_min[2] + blur_max[2]) * 0.5f;
    blur_min[2] = avg - fMinDist * 0.5f;
    blur_max[2] = avg - fMinDist * 0.5f;
  }

  float fscale[3];
  float fbias[3];

  // figure out the progressive scale & bias needed, at each step,
  // to go from one [min..max] range to the next.
  float temp_min, temp_max;
  fscale[0] = 1.0f / (blur_max[0] - blur_min[0]);
  fbias[0] = -blur_min[0] * fscale[0];
  temp_min = (blur_min[1] - blur_min[0]) / (blur_max[0] - blur_min[0]);
  temp_max = (blur_max[1] - blur_min[0]) / (blur_max[0] - blur_min[0]);
  fscale[1] = 1.0f / (temp_max - temp_min);
  fbias[1] = -temp_min * fscale[1];
  temp_min = (blur_min[2] - blur_min[1]) / (blur_max[1] - blur_min[1]);
  temp_max = (blur_max[2] - blur_min[1]) / (blur_max[1] - blur_min[1]);
  fscale[2] = 1.0f / (temp_max - temp_min);
  fbias[2] = -temp_min * fscale[2];

  const auto blurTextures = texture_manager_->GetBlurTextures();
  const auto mainTexture = texture_manager_->GetMainTexture();

  glBlendFunc(GL_ONE, GL_ZERO);
  glBindVertexArray(vaoBlur);

  for (unsigned int i = 0; i < passes; i++) {
    // set pixel shader
    if ((i % 2) == 0) {
      glUseProgram(StaticShaders::Get()->program_blur1_->GetId());
      glUniform1i(StaticShaders::Get()->uniform_blur1_sampler_, 0);

    } else {
      glUseProgram(StaticShaders::Get()->program_blur2_->GetId());
      glUniform1i(StaticShaders::Get()->uniform_blur2_sampler_, 0);
    }

    glViewport(0, 0, blurTextures[i]->GetWidth(), blurTextures[i]->GetHeight());

    // hook up correct source texture - assume there is only one, at stage 0
    glActiveTexture(GL_TEXTURE0);
    if (i == 0) {
      glBindTexture(GL_TEXTURE_2D, mainTexture->GetId());
    } else {
      glBindTexture(GL_TEXTURE_2D, blurTextures[i - 1]->GetId());
    }

    float srcw =
        (i == 0) ? mainTexture->GetWidth() : blurTextures[i - 1]->GetWidth();
    float srch =
        (i == 0) ? mainTexture->GetHeight() : blurTextures[i - 1]->GetHeight();

    float fscale_now = fscale[i / 2];
    float fbias_now = fbias[i / 2];

    // set constants
    if (i % 2 == 0) {
      // pass 1 (long horizontal pass)
      //-------------------------------------
      const float w1 = w[0] + w[1];
      const float w2 = w[2] + w[3];
      const float w3 = w[4] + w[5];
      const float w4 = w[6] + w[7];
      const float d1 = 0 + 2 * w[1] / w1;
      const float d2 = 2 + 2 * w[3] / w2;
      const float d3 = 4 + 2 * w[5] / w3;
      const float d4 = 6 + 2 * w[7] / w4;
      const float w_div = 0.5f / (w1 + w2 + w3 + w4);
      //-------------------------------------
      // float4 _c0; // source texsize (.xy), and inverse (.zw)
      // float4 _c1; // w1..w4
      // float4 _c2; // d1..d4
      // float4 _c3; // scale, bias, w_div, 0
      //-------------------------------------
      glUniform4f(StaticShaders::Get()->uniform_blur1_c0_, srcw, srch,
                  1.0f / srcw, 1.0f / srch);
      glUniform4f(StaticShaders::Get()->uniform_blur1_c1_, w1, w2, w3, w4);
      glUniform4f(StaticShaders::Get()->uniform_blur1_c2_, d1, d2, d3, d4);
      glUniform4f(StaticShaders::Get()->uniform_blur1_c3_, fscale_now,
                  fbias_now, w_div, 0.0);
    } else {
      // pass 2 (short vertical pass)
      //-------------------------------------
      const float w1 = w[0] + w[1] + w[2] + w[3];
      const float w2 = w[4] + w[5] + w[6] + w[7];
      const float d1 = 0 + 2 * ((w[2] + w[3]) / w1);
      const float d2 = 2 + 2 * ((w[6] + w[7]) / w2);
      const float w_div = 1.0f / ((w1 + w2) * 2);
      //-------------------------------------
      // float4 _c0; // source texsize (.xy), and inverse (.zw)
      // float4 _c5; // w1,w2,d1,d2
      // float4 _c6; // w_div, edge_darken_c1, edge_darken_c2,
      // edge_darken_c3
      //-------------------------------------
      glUniform4f(StaticShaders::Get()->uniform_blur2_c0_, srcw, srch,
                  1.0f / srcw, 1.0f / srch);
      glUniform4f(StaticShaders::Get()->uniform_blur2_c5_, w1, w2, d1, d2);
      // note: only do this first time; if you do it many times,
      // then the super-blurred levels will have big black lines along the
      // top & left sides.
      if (i == 1)
        glUniform4f(StaticShaders::Get()->uniform_blur2_c6_, w_div,
                    (1 - edge_darken), edge_darken,
                    5.0f);  // darken edges
      else
        glUniform4f(StaticShaders::Get()->uniform_blur2_c6_, w_div, 1.0f, 0.0f,
                    5.0f);  // don't darken
    }

    // draw fullscreen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // save to blur texture
    glBindTexture(GL_TEXTURE_2D, blurTextures[i]->GetId());
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
                        blurTextures[i]->GetWidth(),
                        blurTextures[i]->GetHeight());
  }

  glBindVertexArray(0);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindTexture(GL_TEXTURE_2D, 0);
}

namespace {

void CompilePresetShaders(
    Pipeline *pipeline, std::shared_ptr<TextureManager> texture_manager,
    std::function<void()> activate_compile_context,
    std::function<void()> deactivate_compile_context,
    std::function<void(std::shared_ptr<Shader>, std::shared_ptr<Shader>,
                       ShaderCache, ShaderCache)>
        compile_completed_callback) {
  activate_compile_context();

  std::cout << "Starting shader compilation" << std::endl;

  std::shared_ptr<Shader> new_composite_shader = nullptr,
                          new_warp_shader = nullptr;
  ShaderCache new_composite_shader_cache, new_warp_shader_cache;
  // compile and link warp and composite shaders from pipeline
  std::string file_name, program_source;

  {
    auto shader = pipeline->GetWarpShader();
    file_name = shader.second.file_name;
    program_source = shader.second.program_source;
  }

  if (!program_source.empty()) {
    new_warp_shader = TranspilePresetShader(
        texture_manager, ShaderEngine::PresentShaderType::PresentWarpShader,
        file_name, program_source, &new_warp_shader_cache);
    if (new_warp_shader == nullptr) {
      std::cerr << "Failed to transpile warp shader, exiting compilation!"
                << std::endl;
      return;
    }
  }

  {
    auto shader = pipeline->GetCompositeShader();
    file_name = shader.second.file_name;
    program_source = shader.second.program_source;
  }

  if (!program_source.empty()) {
    new_composite_shader = TranspilePresetShader(
        texture_manager,
        ShaderEngine::PresentShaderType::PresentCompositeShader, file_name,
        program_source, &new_composite_shader_cache);
    if (new_composite_shader == nullptr) {
      std::cerr << "Failed to transpile composite shader, exiting compilation!"
                << std::endl;
      return;
    }
  }

  glFinish();

  std::cerr << "Finished shader compilation" << std::endl;
  compile_completed_callback(new_composite_shader, new_warp_shader,
                             new_composite_shader_cache, new_warp_shader_cache);
  deactivate_compile_context();
}
}  // namespace

void ShaderEngine::UpdateShaders(Pipeline *pipeline,
                                 std::shared_ptr<Shader> composite_shader,
                                 std::shared_ptr<Shader> warp_shader,
                                 ShaderCache composite_shader_cache,
                                 ShaderCache warp_shader_cache) {
  std::lock_guard<std::mutex> program_reference_lock(program_reference_mutex_);
  std::cout << "Updating shaders" << std::endl;
  ResetPerPresetState();

  pipeline->UpdateShaders(warp_shader_cache, composite_shader_cache);
  composite_shader_ = composite_shader;
  warp_shader_ = warp_shader;
  compile_complete_ = true;
  compile_complete_cv_.notify_one();
}

void ShaderEngine::LoadPresetShadersAsync(Pipeline &pipeline,
                                          std::string_view preset_name) {
  {
    std::unique_lock<std::mutex> program_reference_lock(
        program_reference_mutex_);
    if (compile_complete_) {
      if (compile_thread_.joinable()) {
        compile_thread_.join();
      }
    } else {
      std::cerr << "Compile thread from previous preset has not completed!"
                << std::endl;
      compile_complete_cv_.wait(program_reference_lock);
      compile_complete_ = false;
      if (compile_thread_.joinable()) {
        compile_thread_.join();
      }
    }
  }

  compile_thread_ =
      std::thread(&CompilePresetShaders, &pipeline, texture_manager_,
                  activate_compile_context_, deactivate_compile_context_,
                  std::bind(&ShaderEngine::UpdateShaders, this, &pipeline,
                            std::placeholders::_1, std::placeholders::_2,
                            std::placeholders::_3, std::placeholders::_4));
}

void ShaderEngine::ResetPerPresetState() {
  rand_preset[0] = FRAND;
  rand_preset[1] = FRAND;
  rand_preset[2] = FRAND;
  rand_preset[3] = FRAND;

  unsigned int k = 0;
  do {
    for (int i = 0; i < 4; i++) {
      float xlate_mult = 1;
      float rot_mult = 0.9f * powf(k / 8.0f, 3.2f);
      xlate[k].x = (FRAND * 2 - 1) * xlate_mult;
      xlate[k].y = (FRAND * 2 - 1) * xlate_mult;
      xlate[k].z = (FRAND * 2 - 1) * xlate_mult;
      rot_base[k].x = FRAND * 6.28f;
      rot_base[k].y = FRAND * 6.28f;
      rot_base[k].z = FRAND * 6.28f;
      rot_speed[k].x = (FRAND * 2 - 1) * rot_mult;
      rot_speed[k].y = (FRAND * 2 - 1) * rot_mult;
      rot_speed[k].z = (FRAND * 2 - 1) * rot_mult;
      k++;
    }
  } while (k < sizeof(xlate) / sizeof(xlate[0]));
}

// use the appropriate shader program for rendering the interpolation.
// it will use the preset shader if available, otherwise the textured shader
bool ShaderEngine::enableWarpShader(ShaderCache &shader,
                                    const Pipeline &pipeline,
                                    const PipelineContext &pipelineContext,
                                    const glm::mat4 &mat_ortho) {
  if (warp_shader_ != nullptr) {
    glUseProgram(warp_shader_->GetId());

    SetupTextures(warp_shader_->GetId(), shader);

    SetupShaderVariables(warp_shader_->GetId(), pipeline, pipelineContext);

    ;
    glUniformMatrix4fv(
        glGetUniformLocation(warp_shader_->GetId(), "vertex_transformation"), 1,
        GL_FALSE, glm::value_ptr(mat_ortho));

    return true;
  }

  glUseProgram(StaticShaders::Get()->program_v2f_c4f_t2f_->GetId());

  glUniformMatrix4fv(
      StaticShaders::Get()->uniform_v2f_c4f_t2f_vertex_tranformation_, 1,
      GL_FALSE, glm::value_ptr(mat_ortho));
  glUniform1i(StaticShaders::Get()->uniform_v2f_c4f_t2f_frag_texture_sampler_,
              0);

  return false;
}

bool ShaderEngine::enableCompositeShader(
    ShaderCache &shader, const Pipeline &pipeline,
    const PipelineContext &pipelineContext) {
  if (composite_shader_ != nullptr) {
    glUseProgram(composite_shader_->GetId());

    SetupTextures(composite_shader_->GetId(), shader);

    SetupShaderVariables(composite_shader_->GetId(), pipeline, pipelineContext);

    return true;
  }

  glUseProgram(StaticShaders::Get()->program_v2f_c4f_t2f_->GetId());

  return false;
}
