#ifndef VK_BUFFER_H
#define VK_BUFFER_H

#include <memory>
#include <stdexcept>
#include <cstring>

#include <volk.h>
#include "vk_mem_alloc.h"

#include "vk_device.h"

class VKBuffer{
private:
    std::shared_ptr<VKDevice> device;

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo alloc_info{};

    VkDeviceSize size = 0;
public:
    VKBuffer() = default;
    VKBuffer(std::shared_ptr<VKDevice> dev);
    ~VKBuffer();

    VKBuffer(const VKBuffer&) = delete;
    VKBuffer& operator=(const VKBuffer&) = delete;

    VKBuffer(VKBuffer&& other) noexcept;
    VKBuffer& operator=(VKBuffer&& other) noexcept;

    void create(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VmaMemoryUsage memory_usage,
        VmaAllocationCreateFlags flags = 0
    );

    void destroy();

    void write(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    void* map();
    void unmap();

    VkBuffer get() const { return buffer; }
    VkDeviceSize get_size() const { return size; }
    VmaAllocator get_allocator() const;
};
#endif // VK_BUFFER_H