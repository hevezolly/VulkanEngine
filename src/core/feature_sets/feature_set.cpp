#include <feature_set.h>

API std::atomic_int TypeIdCounter = 0;

void FeatureSet::Destroy() {}
void FeatureSet::GetRequiredExtentions(std::vector<const char*>& buffer) {}
void FeatureSet::GetRequiredLayers(std::vector<const char*>& buffer) {}
FeatureSet::FeatureSet(RenderContext& renderContext): context(renderContext){}