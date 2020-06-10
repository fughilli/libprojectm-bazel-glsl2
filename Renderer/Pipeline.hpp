#ifndef Pipeline_HPP
#define Pipeline_HPP

#include <vector>
#include <mutex>
#include <utility>

#include "Common.hpp"
#include "PerPixelMesh.hpp"
#include "Renderable.hpp"
#include "Shader.hpp"

class PipelineContext
{ public:
	int fps;
    float time;
    float presetStartTime;
	int   frame;
	float progress;
};

//This class is the input to projectM's renderer
//
//Most implemenatations should implement PerPixel in order to get multi-threaded
//dynamic PerPixel equations.  If you MUST (ie Milkdrop compatability), you can use the
//setStaticPerPixel function and fill in x_mesh and y_mesh yourself.
class Pipeline
{
public:

	 //static per pixel stuff
	 bool staticPerPixel;
	 int gx;
	 int gy;

	 float** x_mesh;
	 float** y_mesh;
	 //end static per pixel

	 bool  textureWrap;
	 float screenDecay;

	 //variables passed to pixel shaders
	 float q[NUM_Q_VARIABLES];

	 //blur settings n=bias x=scale
	 float blur1n;
	 float blur2n;
	 float blur3n;
	 float blur1x;
	 float blur2x;
	 float blur3x;
	 float blur1ed;


	 std::vector<RenderItem*> drawables;
	 std::vector<RenderItem*> compositeDrawables;

	 Pipeline();
     void setStaticPerPixel(int _gx, int _gy);
	 virtual ~Pipeline();
	 virtual PixelPoint PerPixel(PixelPoint p, const PerPixelContext context);

     void UpdateShaders(ShaderCache warp_shader, ShaderCache composite_shader) {
       const std::lock_guard<std::mutex> lock(shader_mutex_);
       warp_shader_ = std::move(warp_shader);
       composite_shader_ = std::move(composite_shader);
     }

     std::pair<std::unique_lock<std::mutex>, ShaderCache &> GetWarpShader() {
       return {std::unique_lock<std::mutex>(shader_mutex_),
                             warp_shader_};
     }

     std::pair<std::unique_lock<std::mutex>, ShaderCache &>
     GetCompositeShader() {
       return {std::unique_lock<std::mutex>(shader_mutex_),
                             composite_shader_};
     }

private:
     std::mutex shader_mutex_;
	 ShaderCache warp_shader_;
	 ShaderCache composite_shader_;
};

float **alloc_mesh(size_t gx, size_t gy);
float **free_mesh(float **mesh);

#endif
