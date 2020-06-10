#include <memory>

#include "Shader.hpp"

class StaticShaders {
public:
  static std::shared_ptr<StaticShaders> Get() {
    static auto instance_ = std::shared_ptr<StaticShaders>(new StaticShaders());
    return instance_;
  }

  GLint uniform_blur1_sampler_;
  GLint uniform_blur1_c0_;
  GLint uniform_blur1_c1_;
  GLint uniform_blur1_c2_;
  GLint uniform_blur1_c3_;

  GLint uniform_blur2_sampler_;
  GLint uniform_blur2_c0_;
  GLint uniform_blur2_c5_;
  GLint uniform_blur2_c6_;

  GLint uniform_v2f_c4f_vertex_tranformation_;
  GLint uniform_v2f_c4f_vertex_point_size_;
  GLint uniform_v2f_c4f_t2f_vertex_tranformation_;
  GLint uniform_v2f_c4f_t2f_frag_texture_sampler_;

  std::shared_ptr<Shader> program_v2f_c4f_;
  std::shared_ptr<Shader> program_v2f_c4f_t2f_;
  std::shared_ptr<Shader> program_blur1_;
  std::shared_ptr<Shader> program_blur2_;

private:
  StaticShaders();
};
