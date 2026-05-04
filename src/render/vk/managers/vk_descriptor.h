#ifndef VK_DESCRIPTOR_H
#define VK_DESCRIPTOR_H

#include <vector>
#include <memory>
#include <volk.h>
#include "vk_device.h"

class VKDescriptorAllocator {
private:
    std::shared_ptr<VKDevice> device;
    
    VkDescriptorPool current_pool{VK_NULL_HANDLE};
    std::vector<VkDescriptorPool> used_pools;
    std::vector<VkDescriptorPool> free_pools;

    std::vector<VkDescriptorPoolSize> pool_sizes;

    VkDescriptorPool grab_pool();

public:
    VKDescriptorAllocator(std::shared_ptr<VKDevice> dev);
    ~VKDescriptorAllocator();

    void reset_pools();

    bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);
};

class VKDescriptorBuilder {
private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkWriteDescriptorSet> writes;

    VKDescriptorAllocator* allocator;
    std::shared_ptr<VKDevice> device;

public:
    VKDescriptorBuilder(std::shared_ptr<VKDevice> dev, VKDescriptorAllocator* alloc);

    VKDescriptorBuilder& bind_buffer(uint32_t binding, 
                                     VkDescriptorBufferInfo* buffer_info, 
                                     VkDescriptorType type, 
                                     VkShaderStageFlags stage_flags);

    VKDescriptorBuilder& bind_image(uint32_t binding, 
                                    VkDescriptorImageInfo* image_info, 
                                    VkDescriptorType type, 
                                    VkShaderStageFlags stage_flags);

    VKDescriptorBuilder& bind_acceleration_structure(uint32_t binding, 
                                                     VkWriteDescriptorSetAccelerationStructureKHR* as_info, 
                                                     VkShaderStageFlags stage_flags);

    bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
};

#endif // VK_DESCRIPTOR_H