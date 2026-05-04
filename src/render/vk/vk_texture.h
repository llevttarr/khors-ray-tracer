#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <memory>
#include <stdexcept>

#include <volk.h>
#include "vk_mem_alloc.h"

#include "vk_device.h"

class VKTexture {
private:
    std::shared_ptr<VKDevice> device;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;

    VkFormat format;
    VkExtent3D extent;

public:
    VKTexture() = default;
    VKTexture(std::shared_ptr<VKDevice> dev);
    ~VKTexture();
    VKTexture(const VKTexture&) = delete;
    VKTexture& operator=(const VKTexture&) = delete;

    VKTexture(VKTexture&& other) noexcept;
    VKTexture& operator=(VKTexture&& other) noexcept;

    void create_image(VkExtent3D extent,VkFormat format,VkImageUsageFlags usage,VmaMemoryUsage memory_usage,
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
        uint32_t mip_levels = 1,
        uint32_t array_layers = 1,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
    );

    void create_view(VkImageAspectFlags aspect_flags, VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D,
        uint32_t mip_levels = 1, uint32_t layer_count = 1
    );

    void create_sampler(VkFilter filter = VK_FILTER_LINEAR,VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

    void destroy();

    VkImage get_image() const { return image; }
    VkImageView get_view() const { return view; }
    VkSampler get_sampler() const { return sampler; }

    VkFormat get_format() const { return format; }
    VkExtent3D get_extent() const { return extent; }
    VmaAllocator get_allocator() const;
};

#endif // VK_TEXTURE_H