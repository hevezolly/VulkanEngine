#pragma once
#include <buffer.h>
#include <resource_storage.h>

struct BufferRegion {
    ResourceRef<Buffer> buffer;
    uint32_t offset;
    uint32_t size;
};
