/*
 * Shader.hpp
 *
 *  Created on: Jun 29, 2008
 *      Author: pete
 */

#ifndef SHADER_HPP_
#define SHADER_HPP_

#include "Texture.hpp"
#include <map>
#include <string>

struct Shader {
  std::map<std::string, TextureManager::TextureAndSampler>
      textures_and_samplers;

  std::string program_source;
  std::string preset_path;
};

#endif /* SHADER_HPP_ */
