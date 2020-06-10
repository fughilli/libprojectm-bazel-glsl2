#ifndef SHADER_HPP_
#define SHADER_HPP_

#include <map>
#include <memory>
#include <string>

#include "Texture.hpp"
#include "TextureManager.hpp"

class Shader {
public:
  // Compiles the provided vertex and fragment shader code into a new `Shader`
  // object. If the compilation, link, or validation steps fail, this function
  // returns `nullptr`.
  static std::shared_ptr<Shader>
  CompileShaderProgram(std::string vertex_shader_code,
                       std::string fragment_shader_code,
                       std::string_view shader_type_string);

  ~Shader() { glDeleteProgram(shader_program_id_); }

  GLuint GetId() const { return shader_program_id_; }

private:
  Shader(GLuint shader_program_id) : shader_program_id_(shader_program_id) {}

  GLuint shader_program_id_;
};

struct ShaderCache {
  std::map<std::string, TextureManager::TextureAndSampler>
      textures_and_samplers;

  std::string program_source;
  std::string preset_path;
  std::string file_name;
};

#endif /* SHADER_HPP_ */
