#include "vk_texture.h"
#include "vk_texture.h"

VKTexture::VKTexture(std::shared_ptr<VKDevice> dev): device(dev) {}
VKTexture::~VKTexture() {
    destroy();
}
VmaAllocator VKTexture::get_allocator() const {
    return device->get_allocator();
}
void VKTexture::create_image(
    VkExtent3D img_extent,
    VkFormat fmt,
    VkImageUsageFlags usage,
    VmaMemoryUsage memory_usage,
    VkImageTiling tiling,
    uint32_t mip_levels,
    uint32_t array_layers,
    VkSampleCountFlagBits samples){
    extent = img_extent;
    format = fmt;

    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent = img_extent;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = array_layers;
    image_info.format = fmt;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = samples;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = memory_usage;

    if (vmaCreateImage(get_allocator(),&image_info,&alloc_info,&image,&allocation,nullptr) != VK_SUCCESS) {
        throw std::runtime_error("image creation fail");
    }
}

void VKTexture::create_view(VkImageAspectFlags aspect_flags,VkImageViewType view_type,
    uint32_t mip_levels,uint32_t layer_count){
    VkImageViewCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_info.image = image;
    image_info.viewType = view_type;
    image_info.format = format;

    image_info.subresourceRange.aspectMask = aspect_flags;
    image_info.subresourceRange.baseMipLevel = 0;
    image_info.subresourceRange.levelCount = mip_levels;
    image_info.subresourceRange.baseArrayLayer = 0;
    image_info.subresourceRange.layerCount = layer_count;

    if (vkCreateImageView(device->get_logic_device(),&image_info,nullptr,&view)!= VK_SUCCESS) {
        throw std::runtime_error("imageview creation fail");
    }
}

void VKTexture::create_sampler(VkFilter filter,VkSamplerAddressMode address_mode) {
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = filter;
    sampler_info.minFilter = filter;

    sampler_info.addressModeU = address_mode;
    sampler_info.addressModeV = address_mode;
    sampler_info.addressModeW = address_mode;

    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;

    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;

    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    if (vkCreateSampler(device->get_logic_device(),&sampler_info,nullptr,&sampler) != VK_SUCCESS) {
        throw std::runtime_error("Sampler creation fail");
    }
}

void VKTexture::destroy() {
    VkDevice dev = device->get_logic_device();

    if (sampler) {
        vkDestroySampler(dev, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    if (view) {
        vkDestroyImageView(dev, view, nullptr);
        view = VK_NULL_HANDLE;
    }
    if (image) {
        vmaDestroyImage(get_allocator(), image, allocation);
        image = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
}

VKTexture::VKTexture(VKTexture&& other) noexcept {
    *this = std::move(other);
}

VKTexture& VKTexture::operator=(VKTexture&& other) noexcept {
    if (this != &other) {
        destroy();

        device = std::move(other.device);
        image = other.image;
        allocation = other.allocation;
        view = other.view;
        sampler = other.sampler;
        format = other.format;
        extent = other.extent;

        other.image = VK_NULL_HANDLE;
        other.view = VK_NULL_HANDLE;
        other.sampler = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
    }
    return *this;
}