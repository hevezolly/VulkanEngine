#ifdef BLOCK

#include <common.h>
#include <image.h>
#include <shader_common.h>
#include <allocator_feature.h>
#include <render_context.h>

#ifndef BLOCK_NAME
#error "BLOCK_NAME must be defined"
#endif

#define FUNC_NAME(r) __##BLOCK_NAME##r

//WRAPPER(name, sample_count, loadOp, storeOp, stenciLoadOp, stencilStoreOp, optimalLayout)

struct BLOCK_NAME {

#define WRAPPER(name, sc, lo, so, slo, sso, ol) ImageView* name;
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

    static constexpr uint32_t size() {
        uint32_t counter = 0;
        #define WRAPPER(n, sc, lo, so, slo, sso, ol) counter++;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
        return counter;
    }

    static void GetAttachmentDescriptions(std::vector<VkAttachmentDescription>& data, const Formats& formats) {
        uint32_t initialSize = data.size();
        data.resize(initialSize + size());
        uint32_t index;

        index = initialSize;
        #define WRAPPER(n, sc, lo, so, slo, sso, ol) \
        data[index].format = formats.##n; \
        data[index].samples = sc; \
        data[index].loadOp = lo; \
        data[index].storeOp = so; \
        data[index].stencilLoadOp = slo; \
        data[index].stencilStoreOp = sso; \
        data[index].initialLayout = ol; \
        ResolveAttachmentInitialLayout(data[index].initialLayout, data[index].loadOp, data[index].stencilLoadOp); \
        data[index++].finalLayout = ol;
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
    }

    static void GetColorAttachmentReferences(std::vector<VkAttachmentReference>& data) {
        uint32_t initialSize = data.size();
        data.resize(initialSize + size() - size_depth_stencil());
        uint32_t attachmentIndex;
        uint32_t refIndex;

        attachmentIndex = 0;
        refIndex = initialSize;
        #define WRAPPER(...)
        #define COLOR_WRAPPER(n, sc, lo, so, slo, sso, ol) \
        data[refIndex].attachment = attachmentIndex++; \
        data[refIndex++].layout = ol;
        #define DS_WRAPPER(...) attachmentIndex++;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    static VkAttachmentReference GetDepthStencilAttachmentReference() {
        assert(size_depth_stencil() > 0);
        VkAttachmentReference result{};
        uint32_t attachmentIndex;

        attachmentIndex = 0;
        #define WRAPPER(...)
        #define COLOR_WRAPPER(...) attachmentIndex++;
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
        views[index]=n##->vkImageView; \
        clearValues[index++]=n##->referencedImage->clearValue;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    uint32_t width() {
        uint32_t index = 0;
        #define WRAPPER(name, sc, lo, so, slo, sso, ol) return name->referencedImage->description.width;
        #include <define_attachments.h>
        BLOCK
        #include <reset_attachment_defines.h>
    }

    uint32_t height() {
        uint32_t index = 0;
        #define WRAPPER(name, sc, lo, so, slo, sso, ol) return name->referencedImage->description.height;
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

};

static_assert(BLOCK_NAME::check_layout_correctness(), "INITIAL_LAYOUT and FINAL_LAYOUT are placed incorrectly");
static_assert(BLOCK_NAME::size_depth_stencil() <= 1, "only one depthstencil is supported");

#include <define_attachments.h>
#include <reset_attachment_defines.h>

#undef BLOCK
#undef BLOCK_NAME
#endif