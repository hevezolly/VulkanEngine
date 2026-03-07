#pragma once
#include <buffer.h>
#include <resource_storage.h>

struct BufferRegion {
    ResourceRef<Buffer> buffer;
    uint64_t offset;
    uint64_t size;

    BufferRegion(): 
        buffer(), offset(0), size(0){}

    BufferRegion(ResourceRef<Buffer> b, uint64_t o, uint64_t s): 
        buffer(b), offset(o), size(s){}

    BufferRegion(ResourceRef<Buffer> b): 
        buffer(b), offset(0), size(b->size_bytes()){}

    operator ResourceRef<Buffer>() const {return buffer;}
};
