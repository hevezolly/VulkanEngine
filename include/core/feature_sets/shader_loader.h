#pragma once
#include <feature_set.h>
#include <shader_source.h>
#include <string>
#include <allocator_feature.h>

struct API ShaderLoader: FeatureSet {
    ShaderLoader(RenderContext& ctx, const char* shaderIncludeRoot="shaders"): 
        FeatureSet(ctx), m_shaderIncludeRoot(shaderIncludeRoot) {}

    ShaderBinary Get(const std::string& path, Stage stage);

    const std::string& ShaderIncludeRoot() const {
        return m_shaderIncludeRoot;
    }
    
private:
    std::string m_shaderIncludeRoot;
};