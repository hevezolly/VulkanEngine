#pragma once
#include <common.h>
#include <feature_set.h>
#include <allocator_feature.h>

struct API RawImageData 
{
    unsigned char* data;
    int x;
    int y;
    int num_components;
    
    inline uint32_t size() {
        return x * y * num_components;
    }

    void Free();
};

struct API Registry: FeatureSet 
{
    Registry(RenderContext&, const char* resourcesFolder = "");

    MemChunk<char> LoadResource(const char* path);
    MemChunk<char> GetPath(const char* relativePath);
    std::string LoadText(const char* path);

    RawImageData LoadImage(const char* path, int forceNumberOfComponents=0);
    
private:
    std::string resourcesBase;
};
