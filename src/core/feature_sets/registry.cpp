#include <render_context.h>
#include <registry.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define FS_SEP std::filesystem::path::preferred_separator

void fixSeparators(char* data, uint32_t size) {
    for (int i = 0; i < size; i++) {
        if (data[i] == '\\' || data[i] == '/')
            data[i] = FS_SEP;
    }
}

void RawImageData::Free() 
{
    stbi_image_free(data);
    data = nullptr;
    x = 0;
    y = 0;
    num_components = 0;
}

Registry::Registry(RenderContext& ctx, const char* resourcesFolder):
    FeatureSet(ctx)
{
    resourcesBase = std::string(resourcesFolder);
    LOG(resourcesBase)
    if (resourcesBase.size() > 0)
        resourcesBase += FS_SEP;
    fixSeparators(resourcesBase.data(), resourcesBase.size());
}

RawMemChunk getFileName(Allocator& alloc, std::string& basePath, const char* path) {
    uint32_t nameSize = basePath.size() + strlen(path);
    RawMemChunk name = alloc.BumpAllocate<char>(nameSize + 1);

    memcpy(name.data, basePath.data(), basePath.size());
    memcpy(name.data + basePath.size(), path, nameSize - basePath.size());
    name[nameSize] = 0;

    fixSeparators(name.data + basePath.size(), nameSize - basePath.size());
    return name;
}

RawMemChunk Registry::LoadResource(const char* path) {
    Allocator& alloc = context.Get<Allocator>();
    RawMemChunk name = getFileName(alloc, resourcesBase, path);

    std::ifstream file(name.data, std::ios::binary | std::ios::ate);

    assert(file.is_open());

    uint32_t fileSize = static_cast<uint32_t>(file.tellg());
    file.seekg(0);

    RawMemChunk fileData = alloc.HeapAllocate<char>(fileSize);
    file.read(fileData.data, fileSize);

    alloc.Free(name, true);

    return fileData;
}

RawImageData Registry::LoadImage(const char* path, int forceNumberOfComponents) {
    assert(forceNumberOfComponents >= 0 && forceNumberOfComponents <= 4);
    
    Allocator& alloc = context.Get<Allocator>();
    RawMemChunk name = getFileName(alloc, resourcesBase, path);
    RawImageData data;

    data.data = stbi_load(name.data, &data.x, &data.y, &data.num_components, forceNumberOfComponents);
    
    assert(data.data != nullptr);

    if (forceNumberOfComponents != 0)
        data.num_components = forceNumberOfComponents;
    
    alloc.Free(name);
    return data;
}