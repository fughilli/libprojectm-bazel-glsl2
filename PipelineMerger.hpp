#ifndef PRESET_MERGE_HPP
#define PRESET_MERGE_HPP
#include "Preset.hpp"
#include "Pipeline.hpp"
#include "RenderItemMatcher.hpp"
#include "RenderItemMergeFunction.hpp"

class PipelineMerger
{
 template <class T> inline static T lerp(T a, T b, float ratio)
{
  return a * ratio + b * (1 - ratio);
}

public:
    
  static void mergePipelines(Pipeline &a,  Pipeline &b, Pipeline &out, 
	RenderItemMatcher::MatchResults & matching, RenderItemMergeFunction & merger, float ratio);

  static void DisableBlending(const Pipeline& pipeline);

private :

static const double s;
static const double e;

};


#endif
