#ifndef Pipeline_HPP
#define Pipeline_HPP

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "Common.hpp"
#include "PerPixelMesh.hpp"
#include "Renderable.hpp"
#include "Shader.hpp"

class PipelineContext {
 public:
  int fps;
  float time;
  float presetStartTime;
  int frame;
  float progress;
};

// This class is the input to projectM's renderer
//
// Most implemenatations should implement PerPixel in order to get
// multi-threaded dynamic PerPixel equations.  If you MUST (ie Milkdrop
// compatability), you can use the SetStaticPerPixel function and fill in x_mesh
// and y_mesh yourself.
class Pipeline {
 public:
  // end static per pixel

  bool textureWrap;
  float screenDecay;

  // variables passed to pixel shaders
  float q[NUM_Q_VARIABLES];

  // blur settings n=bias x=scale
  float blur1n;
  float blur2n;
  float blur3n;
  float blur1x;
  float blur2x;
  float blur3x;
  float blur1ed;

  std::vector<std::shared_ptr<RenderItem>> drawables;
  std::vector<std::shared_ptr<RenderItem>> compositeDrawables;

  Pipeline();
  void SetStaticPerPixel(int _gx, int _gy);
  virtual ~Pipeline();
  virtual PixelPoint PerPixel(PixelPoint p, const PerPixelContext context);

  void UpdateShaders(ShaderCache warp_shader, ShaderCache composite_shader) {
    const std::lock_guard<std::mutex> lock(shader_mutex_);
    warp_shader_ = std::move(warp_shader);
    composite_shader_ = std::move(composite_shader);
  }

  std::pair<std::unique_lock<std::mutex>, ShaderCache&> GetWarpShader() {
    return {std::unique_lock<std::mutex>(shader_mutex_), warp_shader_};
  }

  std::pair<std::unique_lock<std::mutex>, ShaderCache&> GetCompositeShader() {
    return {std::unique_lock<std::mutex>(shader_mutex_), composite_shader_};
  }

  float& x_mesh_at(int x, int y) {
    return x_mesh_.get()[ComputeMeshOffset(x, y)];
  }

  float& y_mesh_at(int x, int y) {
    return y_mesh_.get()[ComputeMeshOffset(x, y)];
  }

  float x_mesh_at(int x, int y) const {
    return x_mesh_.get()[ComputeMeshOffset(x, y)];
  }

  float y_mesh_at(int x, int y) const {
    return y_mesh_.get()[ComputeMeshOffset(x, y)];
  }

  float mesh_width() const { return gx_; }

  float mesh_height() const { return gy_; }

  bool static_per_pixel() const { return static_per_pixel_; }

 protected:
  int ComputeMeshOffset(int x, int y) const {
    return (x + y * gx_) % (gx_ * gy_);
  }

  std::mutex shader_mutex_;
  ShaderCache warp_shader_;
  ShaderCache composite_shader_;

  std::unique_ptr<float> x_mesh_;
  std::unique_ptr<float> y_mesh_;

  // static per pixel stuff
  bool static_per_pixel_;
  int gx_;
  int gy_;
};

#endif
