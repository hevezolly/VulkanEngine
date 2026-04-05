#pragma once
#include <buffer.h>
#include <image.h>
#include <resource_storage.h>

struct ImageSubresource {
    ResourceRef<Image> image;
    VkImageView vkView;
    VkImageSubresourceRange range;

    ImageSubresource(): image(), vkView(VK_NULL_HANDLE), range(){}
    ImageSubresource(ResourceRef<Image> i): 
        image(i), vkView(i->view().vkImageView), range(i->view().subresourceRange) {}

    ImageSubresource(ResourceRef<Image> i, ImageView& view):
        image(i), vkView(view.vkImageView), range(view.subresourceRange)
    {}

    operator ResourceRef<Image>() const {return image;}
    operator VkImageView() const {return vkView;}
};

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
