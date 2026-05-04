#include "vk_buffer.h"

VKBuffer::VKBuffer(std::shared_ptr<VKDevice> dev): device(dev) {}
VKBuffer::~VKBuffer() {destroy();}
VmaAllocator VKBuffer::get_allocator() const {
    return device->get_allocator();
}

void VKBuffer::create(VkDeviceSize bufferSize, VkBufferUsageFlags usage,VmaMemoryUsage memory_usage, VmaAllocationCreateFlags flags) {
    size = bufferSize;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = bufferSize;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_cinfo{};
    alloc_cinfo.usage = memory_usage;
    alloc_cinfo.flags = flags;

    if (vmaCreateBuffer(get_allocator(),&buffer_info,&alloc_cinfo,&buffer,&allocation,&alloc_info) != VK_SUCCESS) {
        throw std::runtime_error("VMA buffer init FAIL");
    }
}

void VKBuffer::destroy() {
    if (buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(get_allocator(), buffer, allocation);
        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
}

void* VKBuffer::map() {
    void* data;
    vmaMapMemory(get_allocator(), allocation, &data);
    return data;
}

void VKBuffer::unmap() {
    vmaUnmapMemory(get_allocator(), allocation);
}

void VKBuffer::write(const void* data, VkDeviceSize dataSize, VkDeviceSize offset) {
    void* dst = map();
    std::memcpy(static_cast<char*>(dst) + offset, data, dataSize);
    unmap();
}

VKBuffer::VKBuffer(VKBuffer&& other) noexcept {
    *this = std::move(other);
}

VKBuffer& VKBuffer::operator=(VKBuffer&& other) noexcept {
    if (this != &other) {
        destroy();

        device = std::move(other.device);
        buffer = other.buffer;
        allocation = other.allocation;
        alloc_info = other.alloc_info;
        size = other.size;

        other.buffer = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
    }
    return *this;
}
