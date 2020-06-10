#include "Shader.hpp"

#include <sstream>

namespace {
bool CheckCompileStatus(GLuint shader, std::string_view shader_name) {
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE) {
    return true;
  }

  int info_log_length;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
  if (info_log_length > 0) {
    std::vector<char> FragmentShaderErrorMessage(info_log_length + 1);
    glGetShaderInfoLog(shader, info_log_length, NULL,
                       &FragmentShaderErrorMessage[0]);
    std::cerr << "Failed to compile shader '" << shader_name
              << "'. Error: " << &FragmentShaderErrorMessage[0] << std::endl;
  }

  return false;
}

bool ValidateProgram(GLuint shader_program_id) {
  GLint validation_result = GL_FALSE;
  int info_log_length;

  glValidateProgram(shader_program_id);

  glGetProgramiv(shader_program_id, GL_VALIDATE_STATUS, &validation_result);
  if (validation_result == GL_TRUE) {
    return true;
  }

  glGetProgramiv(shader_program_id, GL_INFO_LOG_LENGTH, &info_log_length);
  if (info_log_length > 0) {
    std::vector<char> program_error_message(info_log_length + 1);
    glGetProgramInfoLog(shader_program_id, info_log_length, nullptr,
                        &program_error_message[0]);
    fprintf(stderr, "%s\n", &program_error_message[0]);
  }

  return false;
}

bool LinkProgram(GLuint programID) {
  glLinkProgram(programID);

  GLint program_linked;
  glGetProgramiv(programID, GL_LINK_STATUS, &program_linked);
  if (program_linked == GL_TRUE) {
    return true; // success
  }

  int InfoLogLength;
  glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0) {
    std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
    glGetProgramInfoLog(programID, InfoLogLength, NULL,
                        &ProgramErrorMessage[0]);
    std::cerr << "Failed to link program: " << &ProgramErrorMessage[0]
              << std::endl;
  }

  return false;
}
} // namespace

std::shared_ptr<Shader>
Shader::CompileShaderProgram(std::string vertex_shader_code,
                             std::string fragment_shader_code,
                             std::string_view shader_type_string) {

  // Create the shaders
  GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

  // Compile Vertex Shader
  char const *vertex_source_ptr = vertex_shader_code.c_str();
  glShaderSource(vertex_shader_id, 1, &vertex_source_ptr, NULL);
  glCompileShader(vertex_shader_id);
  if (!CheckCompileStatus(
          vertex_shader_id,
          (std::stringstream() << "Vertex: " << shader_type_string).str())) {
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    return nullptr;
  }

  // Compile Fragment Shader
  char const *fragment_source_ptr = fragment_shader_code.c_str();
  glShaderSource(fragment_shader_id, 1, &fragment_source_ptr, NULL);
  glCompileShader(fragment_shader_id);
  if (!CheckCompileStatus(
          fragment_shader_id,
          (std::stringstream() << "Fragment: " << shader_type_string).str())) {
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
    return nullptr;
  }

  // Link the program
  GLuint shader_program_id = glCreateProgram();

  glAttachShader(shader_program_id, vertex_shader_id);
  glAttachShader(shader_program_id, fragment_shader_id);

  auto cleanup_fn = [&](void *) {
    glDetachShader(shader_program_id, vertex_shader_id);
    glDetachShader(shader_program_id, fragment_shader_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);
  };
  std::unique_ptr<void, decltype(cleanup_fn)> cleanup(nullptr, cleanup_fn);

  if (!LinkProgram(shader_program_id)) {
    return nullptr;
  }

  if (!ValidateProgram(shader_program_id)) {
    return nullptr;
  }

  return std::shared_ptr<Shader>(new Shader(shader_program_id));
}
