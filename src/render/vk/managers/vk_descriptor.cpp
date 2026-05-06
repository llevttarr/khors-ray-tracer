#include "vk_descriptor.h"
#include <stdexcept>

VKDescriptorAllocator::VKDescriptorAllocator(std::shared_ptr<VKDevice> dev)
    : device(std::move(dev))
{
    pool_sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 16},
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64},
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32},
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 16},
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 8},
    };
}

VKDescriptorAllocator::~VKDescriptorAllocator() {
    VkDevice vkdev = device->get_logic_device();
    for (VkDescriptorPool p : used_pools)
        vkDestroyDescriptorPool(vkdev, p, nullptr);
    for (VkDescriptorPool p : free_pools)
        vkDestroyDescriptorPool(vkdev, p, nullptr);
    if (current_pool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(vkdev, current_pool, nullptr);
}

VkDescriptorPool VKDescriptorAllocator::grab_pool() {
    if (!free_pools.empty()) {
        VkDescriptorPool p = free_pools.back();
        free_pools.pop_back();
        return p;
    }

    constexpr uint32_t MAX_SETS = 128;
    VkDescriptorPoolCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.maxSets = MAX_SETS;
    ci.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    ci.pPoolSizes = pool_sizes.data();

    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(device->get_logic_device(), &ci, nullptr, &pool) != VK_SUCCESS)
        throw std::runtime_error("VKDescriptorAllocator: vkCreateDescriptorPool failed");
    return pool;
}

bool VKDescriptorAllocator::allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout) {
    if (current_pool == VK_NULL_HANDLE) {
        current_pool = grab_pool();
        used_pools.push_back(current_pool);
    }

    VkDescriptorSetAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = current_pool;
    ai.descriptorSetCount = 1;
    ai.pSetLayouts = &layout;

    VkResult result = vkAllocateDescriptorSets(device->get_logic_device(), &ai, set);

    if (result == VK_SUCCESS) return true;

    if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY) {
        current_pool = grab_pool();
        used_pools.push_back(current_pool);
        ai.descriptorPool = current_pool;
        result = vkAllocateDescriptorSets(device->get_logic_device(), &ai, set);
        if (result == VK_SUCCESS) return true;
    }

    return false;
}

void VKDescriptorAllocator::reset_pools() {
    VkDevice vkdev = device->get_logic_device();
    for (VkDescriptorPool p : used_pools) {
        vkResetDescriptorPool(vkdev, p, 0);
        free_pools.push_back(p);
    }
    used_pools.clear();
    current_pool = VK_NULL_HANDLE;
}

VKDescriptorBuilder::VKDescriptorBuilder(std::shared_ptr<VKDevice> dev,VKDescriptorAllocator* alloc): device(std::move(dev)), allocator(alloc) {}

VKDescriptorBuilder& VKDescriptorBuilder::bind_buffer(uint32_t binding,VkDescriptorBufferInfo* buffer_info,VkDescriptorType type,VkShaderStageFlags stage_flags){
    VkDescriptorSetLayoutBinding lb{};
    lb.binding = binding;
    lb.descriptorType = type;
    lb.descriptorCount = 1;
    lb.stageFlags = stage_flags;
    bindings.push_back(lb);

    VkWriteDescriptorSet w{};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.descriptorCount = 1;
    w.descriptorType = type;
    w.dstBinding = binding;
    w.pBufferInfo = buffer_info;
    writes.push_back(w);
    return *this;
}

VKDescriptorBuilder& VKDescriptorBuilder::bind_image(uint32_t binding,VkDescriptorImageInfo* image_info,VkDescriptorType type,VkShaderStageFlags stage_flags){
    VkDescriptorSetLayoutBinding lb{};
    lb.binding = binding;
    lb.descriptorType = type;
    lb.descriptorCount = 1;
    lb.stageFlags = stage_flags;
    bindings.push_back(lb);

    VkWriteDescriptorSet w{};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.descriptorCount = 1;
    w.descriptorType = type;
    w.dstBinding = binding;
    w.pImageInfo = image_info;
    writes.push_back(w);
    return *this;
}

VKDescriptorBuilder& VKDescriptorBuilder::bind_acceleration_structure(uint32_t binding,VkWriteDescriptorSetAccelerationStructureKHR* as_info,VkShaderStageFlags stage_flags)
{
    VkDescriptorSetLayoutBinding lb{};
    lb.binding = binding;
    lb.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    lb.descriptorCount = 1;
    lb.stageFlags = stage_flags;
    bindings.push_back(lb);

    VkWriteDescriptorSet w{};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    w.dstBinding = binding;
    w.pNext = as_info;
    writes.push_back(w);
    return *this;
}

bool VKDescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& layout) {
    VkDevice vkdev = device->get_logic_device();
    VkDescriptorSetLayoutCreateInfo layout_ci{};
    layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_ci.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_ci.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vkdev, &layout_ci, nullptr, &layout) != VK_SUCCESS)
        return false;

    if (!allocator->allocate(&set, layout))
        return false;
    for (auto& w : writes)
        w.dstSet = set;

    vkUpdateDescriptorSets(vkdev,
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return true;
}