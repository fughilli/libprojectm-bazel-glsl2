#ifndef PRESET_FRAME_IO_HPP
#define PRESET_FRAME_IO_HPP
#include <vector>
#include "Renderer/MilkdropWaveform.hpp"
#include "Renderer/Pipeline.hpp"
#include "Renderer/Filters.hpp"
#include "CustomShape.hpp"
#include "CustomWave.hpp"
#include "Renderer/VideoEcho.hpp"


/// Container for all *read only* engine variables a preset requires to
/// evaluate milkdrop equations. Every preset object needs a reference to one of these.
class PresetInputs : public PipelineContext {

public:
    /* PER_PIXEL VARIBLES BEGIN */

    float x_per_pixel;
    float y_per_pixel;
    float rad_per_pixel;
    float ang_per_pixel;

    /* PER_PIXEL VARIBLES END */

    float bass;
    float mid;
    float treb;
    float bass_att;
    float mid_att;
    float treb_att;

    /* variables were added in milkdrop 1.04 */
    int gx, gy;

    float **x_mesh;
    float **y_mesh;
    float **rad_mesh;
    float **theta_mesh;

    float **origtheta;  //grid containing interpolated mesh reference values
    float **origrad;
    float **origx;  //original mesh
    float **origy;

    void resetMesh();

    ~PresetInputs();
    PresetInputs();

    /// Initializes this preset inputs given a mesh size.
    /// \param gx the width of the mesh 
    /// \param gy the height of the mesh
    /// \note This must be called before reading values from this class
    void Initialize(int _gx, int _gy);

    /// Updates this preset inputs with the latest values from the
    /// the pipeline context and beat detection unit
    void update (const BeatDetect & music, const PipelineContext & context);

    private:
};


/// Container class for all preset writeable engine variables. This is the important glue
/// between the presets and renderer to facilitate smooth preset switching
/// Every preset object needs a reference to one of these.
class PresetOutputs : public Pipeline {
public:
    std::vector<std::shared_ptr<CustomWave>> customWaves;
    std::vector<std::shared_ptr<CustomShape>> customShapes;

    void Initialize(int _gx, int _gy);
    PresetOutputs();
    ~PresetOutputs();
    virtual void Render(const BeatDetect &music, const PipelineContext &context);
    void PerPixelMath( const PipelineContext &context);
    /* PER FRAME VARIABLES BEGIN */

    float zoom;
    float zoomexp;
    float rot;
    float warp;

    float sx;
    float sy;
    float dx;
    float dy;
    float cx;
    float cy;

    std::shared_ptr<VideoEcho> videoEcho;

    std::shared_ptr<MilkdropWaveform> wave;
    std::shared_ptr<Border> border;
    std::shared_ptr<MotionVectors> mv;
    std::shared_ptr<DarkenCenter> darkenCenter;

    std::shared_ptr<Brighten> brighten;
    std::shared_ptr<Darken> darken;
    std::shared_ptr<Invert> invert;
    std::shared_ptr<Solarize> solarize;

    /* PER_FRAME VARIABLES END */

    float fRating;
    float fGammaAdj;

    bool bDarkenCenter;
    bool bRedBlueStereo;
    bool bBrighten;
    bool bDarken;
    bool bSolarize;
    bool bInvert;
    bool bMotionVectorsOn;

    float fWarpAnimSpeed;
    float fWarpScale;
    float fShader;

    float **zoom_mesh;
    float **zoomexp_mesh;
    float **rot_mesh;

    float **sx_mesh;
    float **sy_mesh;
    float **dx_mesh;
    float **dy_mesh;
    float **cx_mesh;
    float **cy_mesh;
    float **warp_mesh;

    float **orig_x;  //original mesh
    float **orig_y;
    float **rad_mesh;

private:
    void PerPixelMath_c( const PipelineContext &context);
#ifdef __SSE2__
    void PerPixelMath_sse( const PipelineContext &context);
#endif
};


#endif
