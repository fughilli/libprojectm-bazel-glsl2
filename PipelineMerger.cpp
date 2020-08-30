#include "PipelineMerger.hpp"
#include "RenderItemMatcher.hpp"
#include "RenderItemMergeFunction.hpp"

const double PipelineMerger::e(2.71828182845904523536);
const double PipelineMerger::s(0.5);

void PipelineMerger::DisableBlending(const Pipeline& pipeline) {
    for (auto& drawable : pipeline.drawables) {
        drawable->masterAlpha = 1.0f;
    }
}

void PipelineMerger::mergePipelines(Pipeline & a, Pipeline & b, Pipeline & out, RenderItemMatcher::MatchResults & results, RenderItemMergeFunction & mergeFunction, float ratio)

{

    const double invratio = 1.0 - ratio;

	out.textureWrap = ( ratio < 0.5 ) ? a.textureWrap : b.textureWrap;

	out.screenDecay = lerp ( b.screenDecay, a.screenDecay, ratio );

	out.drawables.clear();
	out.compositeDrawables.clear();

    for(auto& drawable : a.drawables) {
        drawable->masterAlpha = invratio;
        out.drawables.push_back(drawable);
    }

    for(auto & drawable : b.drawables) {
        drawable->masterAlpha = ratio;
        out.drawables.push_back(drawable);
    }

    if(ratio < 0.5)
    {
        const double local_ratio = (invratio - 0.5) * 2;

        for (auto& composite_drawable : a.compositeDrawables) {
            composite_drawable->masterAlpha = local_ratio;
            out.compositeDrawables.push_back(composite_drawable);
        }
    }
    else
    {
        const double local_ratio = (ratio - 0.5) * 2;

        for(auto& composite_drawable : b.compositeDrawables) {
            composite_drawable->masterAlpha = local_ratio;
            out.compositeDrawables.push_back(composite_drawable);
        }
    }

	/*
	for (RenderItemMatchList::iterator pos = results.matches.begin(); pos != results.matches.end(); ++pos) {

		RenderItem * itemA = pos->first;
		RenderItem * itemB = pos->second;

		RenderItem * itemC = mergeFunction(itemA, itemB, ratio);

		if (itemC == 0) {
			itemA->masterAlpha = ratio;
	        	out.drawables.push_back(itemA);
			itemB->masterAlpha = invratio;
	        	out.drawables.push_back(itemB);
		} else
			out.drawables.push_back(itemC);

	}


	for (std::vector<RenderItem*>::const_iterator pos = results.unmatchedLeft.begin();
		pos != results.unmatchedLeft.end(); ++pos)
	    {
	       (*pos)->masterAlpha = invratio;
	       out.drawables.push_back(*pos);
	    }

	for (std::vector<RenderItem*>::const_iterator pos = results.unmatchedRight.begin();
		pos != results.unmatchedRight.end(); ++pos)
		{
		  (*pos)->masterAlpha = ratio;
		   out.drawables.push_back(*pos);
		}
	*/

    if (a.static_per_pixel() && b.static_per_pixel()) {
      out.SetStaticPerPixel(a.mesh_width(), a.mesh_height());
      for (int x = 0; x < a.mesh_width(); x++) {
        for (int y = 0; y < a.mesh_height(); y++) {
          out.x_mesh_at(x, y) =
              a.x_mesh_at(x, y) * invratio + b.x_mesh_at(x, y) * ratio;
        }
      }
      for (int x = 0; x < a.mesh_width(); x++) {
        for (int y = 0; y < a.mesh_height(); y++) {
          out.y_mesh_at(x, y) =
              a.y_mesh_at(x, y) * invratio + b.y_mesh_at(x, y) * ratio;
        }
      }
    }

    if(ratio < 0.5)
    {
        auto warp = a.GetWarpShader().second;
        auto composite = a.GetCompositeShader().second;
        out.UpdateShaders(warp, composite);
    }
    else
    {
        auto warp = b.GetWarpShader().second;
        auto composite = b.GetCompositeShader().second;
        out.UpdateShaders(warp, composite);
    }
}
