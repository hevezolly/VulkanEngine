#ifdef BLOCK

#include <common.h>
#include <image.h>
#include <shader_common.h>
#include <allocator_feature.h>
#include <render_context.h>
#include <resource_storage.h>
#include <render_node.h>
#include <subresources.h>

#ifndef BLOCK_NAME
#error "BLOCK_NAME must be defined"
#endif

//WRAPPER(name, sample_count, loadOp, storeOp, stenciLoadOp, stencilStoreOp, optimalLayout)

#ifndef MSAA
#define MSAA 1
#endif

struct BLOCK_NAME {

#define WRAPPER(name, sc, lo, so, slo, sso, ol) ImageSubresource name;
#if MSAA != 1
#define RESOLVE_WRAPPER(name) ImageSubresource name;
#endif
#include <define_attachments.h>
BLOCK
#include <reset_attachment_defines.h>

    struct Formats {
    
        #define WRAPPER(name, sc, lo, so, slo, sso, ol) VkFormat name;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    };

public:

    static constexpr uint32_t size_depth_stencil() {
        uint32_t counter = 0;
        #define WRAPPER(...)
        #define DS_WRAPPER(n, sc, lo, so, slo, sso, ol) counter++;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
        return counter;
    }

    static constexpr bool uses_msaa() {
#if MSAA == 1
        return false;
#else
        return true;
#endif 
    }

    static constexpr VkSampleCountFlagBits msaa_count() {
        return static_cast<VkSampleCountFlagBits>(MSAA);
    }

    static constexpr uint32_t size() {
        uint32_t counter = 0;
        #define WRAPPER(n, sc, lo, so, slo, sso, ol) counter++;
#if MSAA != 1
        #define RESOLVE_WRAPPER(n) counter++;
#endif
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
        return counter;
    }

    static constexpr uint32_t size_resolve() {
        uint32_t counter = 0;
        #define WRAPPER(n, sc, lo, so, slo, sso, ol)
        #define RESOLVE_WRAPPER(n) counter++;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
        return counter;
    }

    void write_outputs(NodeDependency* dependencies) {
        uint32_t index = 0;
        bool last_ds = false;
        #define WRAPPER(...)
        #define COLOR_WRAPPER(n, sc, lo, so, slo, sso, ol) \
        dependencies[index].resource = n##.image.id; \
        dependencies[index++].state = ResourceState{VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, ol};
        #define DS_WRAPPER(n, sc, lo, so, slo, sso, ol) \
        dependencies[index].resource = n##.image.id; \
        dependencies[index++].state = ResourceState{VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, ol};
#if MSAA != 1
        #define RESOLVE_WRAPPER(n) \
        dependencies[index].resource = n##.image.id; \
        dependencies[index++].state = dependencies[index - 2].state;
#endif
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    static void GetAttachmentDescriptions(std::vector<VkAttachmentDescription>& data, const Formats& formats) {
        uint32_t initialSize = data.size();
        data.resize(initialSize + size());
        uint32_t index;

        index = initialSize;
        #define WRAPPER(n, sc, lo, so, slo, sso, ol) \
        data[index].format = formats.##n; \
        data[index].samples = BLOCK_NAME::msaa_count(); \
        data[index].loadOp = lo; \
        data[index].storeOp = so; \
        data[index].stencilLoadOp = slo; \
        data[index].stencilStoreOp = sso; \
        data[index].initialLayout = ol; \
        ResolveAttachmentInitialLayout(data[index].initialLayout, data[index].loadOp, data[index].stencilLoadOp); \
        data[index++].finalLayout = ol;

#if MSAA != 1
        #define RESOLVE_WRAPPER(n) index++;
#endif
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>

        index = initialSize-1;
        #define WRAPPER(n, sc, lo, so, slo, sso, ol) index++;
        #define INITIAL_LAYOUT_WRAPPER(il) \
        data[index].initialLayout = il; \
        ResolveAttachmentInitialLayout(data[index].initialLayout, data[index].loadOp, data[index].stencilLoadOp);
        #define FINAL_LAYOUT_WRAPPER(fl) data[index].finalLayout = fl;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>

#if MSAA != 1
        index = initialSize-1;
        #define WRAPPER(n, sc, lo, so, slo, sso, ol) index++;
        #define RESOLVE_WRAPPER(n) \
        data[index] = data[index-1]; \
        data[index-1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; \
        data[index++].samples = VK_SAMPLE_COUNT_1_BIT;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
#endif
    }

    static void GetColorAttachmentReferences(std::vector<VkAttachmentReference>& data) {
        uint32_t initialSize = data.size();
        data.resize(initialSize + size() - size_depth_stencil() - size_resolve());
        uint32_t attachmentIndex;
        uint32_t refIndex;

        attachmentIndex = 0;
        refIndex = initialSize;
        #define WRAPPER(...)
        #define COLOR_WRAPPER(n, sc, lo, so, slo, sso, ol) \
        data[refIndex].attachment = attachmentIndex++; \
        data[refIndex++].layout = ol;
        #define DS_WRAPPER(...) attachmentIndex++;
        #define RESOLVE_WRAPPER(...) attachmentIndex++;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    static void GetColorResolveAttachmentReferences(std::vector<VkAttachmentReference>& data) {
        ASSERT(size_resolve() > 0);
        uint32_t initialSize = data.size();
        data.resize(initialSize + size_resolve());
        uint32_t attachmentIndex;
        uint32_t refIndex;
        VkImageLayout lastLayout;

        attachmentIndex = 0;
        refIndex = initialSize;
        #define WRAPPER(...)
        #define COLOR_WRAPPER(n, sc, lo, so, slo, sso, ol) \
        attachmentIndex++; \
        data[refIndex].attachment = VK_ATTACHMENT_UNUSED; \
        data[refIndex++].layout = VK_IMAGE_LAYOUT_UNDEFINED; \
        lastLayout = ol; 
        #define DS_WRAPPER(...) attachmentIndex++;
        #define RESOVE_WRAPPER(...) \
        data[refIndex-1].attachment = attachmentIndex++; \
        data[refIndex-1].layout = lastLayout;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    static VkAttachmentReference GetDepthStencilAttachmentReference() {
        ASSERT(size_depth_stencil() > 0);
        VkAttachmentReference result{};
        uint32_t attachmentIndex;

        attachmentIndex = 0;
        #define WRAPPER(...)
        #define COLOR_WRAPPER(...) attachmentIndex++;
        #define RESOLVE_WRAPPER(...) attachmentIndex++;
        #define DS_WRAPPER(n, sc, lo, so, slo, sso, ol) \
        result.attachment = attachmentIndex++; \
        result.layout = ol;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>

        return result;
    }

    void FillAttachments(VkImageView* views, VkClearValue* clearValues) {
        uint32_t index = 0;
        #define WRAPPER(n, sc, lo, so, slo, sso, ol) \
        views[index]=n##.vkView; \
        clearValues[index++]=n##.image->clearValue;
        #define RESOLVE_WRAPPER(n) \
        views[index]=n##.vkView; \
        clearValues[index++]=n##.image->clearValue;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    uint32_t width() {
        uint32_t index = 0;
        #define WRAPPER(name, sc, lo, so, slo, sso, ol) return name##.image->description.width;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    uint32_t height() {
        uint32_t index = 0;
        #define WRAPPER(name, sc, lo, so, slo, sso, ol) return name##.image->description.height;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    static constexpr bool check_layout_correctness() {

        uint32_t maxlAfterWrapper = 0;

        uint32_t ilAfterWrapper = 2;
        uint32_t flAfterWrapper = 2;

        #define WRAPPER(n, sc, lo, so, slo, sso, ol) \
        ilAfterWrapper = 0; \
        flAfterWrapper = 0;
        #define INITIAL_LAYOUT_WRAPPER(il) maxlAfterWrapper = std::max(maxlAfterWrapper, ++ilAfterWrapper);
        #define FINAL_LAYOUT_WRAPPER(fl) maxlAfterWrapper = std::max(maxlAfterWrapper, ++flAfterWrapper);
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>

        return maxlAfterWrapper <= 1;
    }

    static constexpr bool check_resolve_correctness() {
        bool resolveAfterDS = false;
        uint32_t maxResolveAfterColor = 0;
        uint32_t resolveAfterColor = 2;
        bool currentColor = true;

        #define WRAPPER(...)
        #define COLOR_WRAPPER(...) currentColor = true; resolveAfterColor = 0;
        #define DS_WRAPPER(...) currentColor = false;
        #define RESOLVE() \
        maxResolveAfterColor = std::max(maxResolveAfterColor, ++resolveAfterColor); \
        resolveAfterDS |= !currentColor;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>

        return !resolveAfterDS && (maxResolveAfterColor <= 1);
    }

};

static_assert(BLOCK_NAME::check_layout_correctness(), "INITIAL_LAYOUT and FINAL_LAYOUT are placed incorrectly");
static_assert(BLOCK_NAME::size_depth_stencil() <= 1, "only one depthstencil is supported");
#if MSAA == 1
static_assert(BLOCK_NAME::size_resolve() == 0, "RESOLVE_WITH can be used only when MSAA > 1");
#else
static_assert(MSAA == 0x00000002 ||
              MSAA == 0x00000004 ||
              MSAA == 0x00000008 ||
              MSAA == 0x00000010 ||
              MSAA == 0x00000020 ||
              MSAA == 0x00000040, "unsupported MSAA count")
static_assert(BLOCK_NAME::check_resolve_correctness(), "RESOLVE_WITH can be placed only after color attachment and only once");
#endif

#include <define_attachments.h>
#include <reset_attachment_defines.h>

#undef BLOCK
#undef BLOCK_NAME
#undef MSAA
#endif