#pragma once

#include <common.h>
#include <resource_memory.h>
#include <optional>
#include <glm/glm.hpp>
#include <resource_id.h>
#include <image_usage.h>
#include <hash_combine.h>

struct RenderContext;

void formatDepthStencilSupport(VkFormat format, bool& depth, bool& stencil);

struct API ImageDescription {
    VkFormat format;
    uint32_t width;
    uint32_t height;
    ImageUsage usage;
    uint32_t arrayLayers=1;
    uint32_t mipLevels=1;

    ImageDescription()=default;
    ImageDescription(VkFormat f, ImageUsage u, VkExtent2D extent, uint32_t d=1, uint32_t mips=1):
        usage(u), format(f), width(extent.width), height(extent.height), arrayLayers(d), mipLevels(mips){}
};

struct API Image;

struct API ImageView {
    VkImageView vkImageView;
    const Image* referencedImage;
    VkImageSubresourceRange subresourceRange;

    ImageView(): 
        vkImageView(VK_NULL_HANDLE), 
        referencedImage(nullptr),
        subresourceRange(),
        context(nullptr) {}
         
    ImageView(RenderContext& context, const Image* image, const VkImageSubresourceRange& subresourceRange);

    RULE_5(ImageView)

private:
    RenderContext* context;
};

struct HashImageSubresourceRange {
        std::size_t operator()(const VkImageSubresourceRange& val) const {
            size_t seed = 0;
            hash_combine(seed, val.aspectMask);
            hash_combine(seed, val.baseArrayLayer);
            hash_combine(seed, val.baseMipLevel);
            hash_combine(seed, val.layerCount);
            hash_combine(seed, val.levelCount);
            return seed;
        }
    };

struct EqualImageSubresourceRange{
    bool operator()(const VkImageSubresourceRange& val1, const VkImageSubresourceRange& val2) const {

        return val1.aspectMask == val2.aspectMask && 
               val1.baseArrayLayer == val2.baseArrayLayer &&
               val1.baseMipLevel == val2.baseMipLevel &&
               val1.layerCount == val2.layerCount &&
               val1.levelCount == val2.levelCount;
    }
};

struct Mip {
    uint32_t level;
    uint32_t count = 1;
};

struct Layer {
    uint32_t layer;
    uint32_t count = 1;
};

struct API Image {
    VkImage vkImage;
    ImageDescription description;
    VkClearValue clearValue;
    VkImageSubresourceRange mainRange;

    Image(VkImage readyImage, RenderContext& context, const ImageDescription& description);
    Image(VkImage img, RenderContext& context, Memory&& memory, const ImageDescription& description);

    ImageView& view() {
        return GetSubresource(mainRange);
    }

    ImageView& get(Mip mip, Layer layer);

    inline ImageView& get(Layer layer, Mip mip) {
        return get(mip, layer);
    }

    inline ImageView& get(Layer layer) {
        return get(Mip(mainRange.baseMipLevel, mainRange.levelCount), layer);
    }

    inline ImageView& get(Mip mip) {
        return get(mip, Layer(mainRange.baseArrayLayer, mainRange.layerCount));
    }

    inline ImageView& getSingle(Layer layer) {
        return get(Mip(mainRange.baseMipLevel), layer);
    }

    inline ImageView& getSingle(Mip mip) {
        return get(mip, Layer(mainRange.baseArrayLayer));
    }
    
    RULE_5(Image)

    ImageView& GetSubresource(const VkImageSubresourceRange& range); 

private:
    std::unordered_map<VkImageSubresourceRange, ImageView, HashImageSubresourceRange, EqualImageSubresourceRange> subresources; 
    RenderContext* context = nullptr;
    std::optional<Memory> memory;
};