#pragma once
#include <feature_set.h>
#include <shader_source.h>
#include <string>

struct ShaderLoader: FeatureSet {
    using FeatureSet::FeatureSet;

    ShaderBinary Get(const std::string& path, Stage stage);
};