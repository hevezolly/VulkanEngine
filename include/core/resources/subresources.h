#pragma once
#include <buffer.h>
#include <image.h>
#include <resource_storage.h>

struct API ImageSubresource {
    ResourceRef<Image> image;
    VkImageView vkView;
    VkImageSubresourceRange range;

    ImageSubresource() = delete;
    ImageSubresource(ResourceRef<Image> i): 
        image(i), vkView(i->view().vkImageView), range(i->view().subresourceRange) {}

    ImageSubresource(ResourceRef<Image> i, ImageView& view):
        image(i), vkView(view.vkImageView), range(view.subresourceRange)
    {}

    static ImageSubresource Null() {
        return ImageSubresource({}, VK_NULL_HANDLE, {});
    }

    operator ResourceRef<Image>() const {return image;}
    operator VkImageView() const {return vkView;}

private:
    ImageSubresource(ResourceRef<Image> i, VkImageView view, VkImageSubresourceRange range):
        image(i), vkView(view), range(range)
    {}
};

ImageSubresource get(ResourceRef<Image> img, Mip mip);
ImageSubresource get(ResourceRef<Image> img, Layer layer);
ImageSubresource get(ResourceRef<Image> img, Mip mip, Layer layer);
ImageSubresource getSingle(ResourceRef<Image> img, Mip mip);
ImageSubresource getSingle(ResourceRef<Image> img, Layer layer);

struct API BufferRegion {
    ResourceRef<Buffer> buffer;
    uint64_t offset;
    uint64_t size;

    BufferRegion() = delete;

    BufferRegion(ResourceRef<Buffer> b, uint64_t o, uint64_t s): 
        buffer(b), offset(o), size(s){}

    BufferRegion(ResourceRef<Buffer> b): 
        buffer(b), offset(0), size(b->size_bytes()){}

    operator ResourceRef<Buffer>() const {return buffer;}

    static BufferRegion Null() {
        return BufferRegion({}, 0, 0);
    }
};

template<typename T>
struct NonDefaultRef {

    NonDefaultRef() = delete;
    NonDefaultRef(ResourceRef<T> inner): _ref(inner){}

    static NonDefaultRef Null() {
        return NonDefaultRef(ResourceRef<T>{});
    }

    ResourceId id() const {
        return _ref.id;
    }

    T* operator ->() {
        return &_ref.val();
    }

    const T* operator ->() const {
        return &_ref.val();
    }

private:
    ResourceRef<T> _ref;
};