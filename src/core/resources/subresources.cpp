#include <subresources.h>

ImageSubresource get(ResourceRef<Image> img, Mip mip) {
    return ImageSubresource(img, img->get(mip));
}

ImageSubresource get(ResourceRef<Image> img, Layer layer) {
    return ImageSubresource(img, img->get(layer));
}

ImageSubresource get(ResourceRef<Image> img, Mip mip, Layer layer) {
    return ImageSubresource(img, img->get(layer, mip));
}

ImageSubresource getSingle(ResourceRef<Image> img, Mip mip) {
    return ImageSubresource(img, img->getSingle(mip));
}

ImageSubresource getSingle(ResourceRef<Image> img, Layer layer) {
    return ImageSubresource(img, img->getSingle(layer));
}