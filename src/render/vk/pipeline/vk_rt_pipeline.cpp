#include "vk_rt_pipeline.h"
VKRTPipeline::~VKRTPipeline() {
    VkDevice d = device->get_logic_device();

    if (sbt_buffer) {
        vmaDestroyBuffer(device->get_allocator(), sbt_buffer, sbt_alloc);
    }
}
VKRTPipelineBuilder& VKRTPipelineBuilder::add_raygen_shader(const std::string& spv_path) {
    VkDevice d = device->get_logic_device();

    VkShaderModule module = VKPipeline::load_shader(d, spv_path);

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stage.module = module;
    stage.pName = "main";

    uint32_t index = static_cast<uint32_t>(shader_stages.size());
    shader_stages.push_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = index;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    shader_groups.push_back(group);

    return *this;
}

VKRTPipelineBuilder& VKRTPipelineBuilder::add_miss_shader(const std::string& spv_path) {
    VkDevice d = device->get_logic_device();

    VkShaderModule module = VKPipeline::load_shader(d, spv_path);

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stage.module = module;
    stage.pName = "main";

    uint32_t index = static_cast<uint32_t>(shader_stages.size());
    shader_stages.push_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = index;
    group.closestHitShader = VK_SHADER_UNUSED_KHR;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    shader_groups.push_back(group);

    return *this;
}

VKRTPipelineBuilder& VKRTPipelineBuilder::add_closest_hit_shader(const std::string& spv_path) {
    VkDevice d = device->get_logic_device();

    VkShaderModule module = VKPipeline::load_shader(d, spv_path);

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stage.module = module;
    stage.pName = "main";

    uint32_t index = static_cast<uint32_t>(shader_stages.size());
    shader_stages.push_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.generalShader = VK_SHADER_UNUSED_KHR;
    group.closestHitShader = index;
    group.anyHitShader = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    shader_groups.push_back(group);

    return *this;
}

VKRTPipelineBuilder& VKRTPipelineBuilder::set_max_recursion_depth(uint32_t depth) {
    max_recursion_depth = depth;
    return *this;
}


std::unique_ptr<VKRTPipeline> VKRTPipelineBuilder::build() {
    VkDevice d = device->get_logic_device();

    if (shader_stages.empty())
        throw std::runtime_error("RT pipeline shader stages empty");

    VkPipelineLayout layout = build_pipeline_layout();

    VkRayTracingPipelineCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    info.stageCount = static_cast<uint32_t>(shader_stages.size());
    info.pStages = shader_stages.data();
    info.groupCount = static_cast<uint32_t>(shader_groups.size());
    info.pGroups = shader_groups.data();
    info.maxPipelineRayRecursionDepth = max_recursion_depth;
    info.layout = layout;

    VkPipeline pipeline;
    if (vkCreateRayTracingPipelinesKHR(d,VK_NULL_HANDLE,VK_NULL_HANDLE,1,&info,nullptr,&pipeline) != VK_SUCCESS) {
        throw std::runtime_error("RT pipeline creation fail");
    }

    /** SBT */

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_props{};
    rt_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 props2{};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &rt_props;

    vkGetPhysicalDeviceProperties2(device->get_phys_device(), &props2);
    uint32_t group_count = static_cast<uint32_t>(shader_groups.size());

    uint32_t handle_size = rt_props.shaderGroupHandleSize;
    uint32_t handle_size_aligned =(handle_size + rt_props.shaderGroupHandleAlignment - 1) &~(rt_props.shaderGroupHandleAlignment - 1);
    uint32_t sbt_size = group_count * handle_size_aligned;

    std::vector<uint8_t> handles(sbt_size);

    if (vkGetRayTracingShaderGroupHandlesKHR(d,pipeline,0,group_count,sbt_size,handles.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to get RT shader handles");
    }

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = sbt_size;
    buffer_info.usage =VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer sbt_buffer;
    VmaAllocation sbt_alloc;
    VmaAllocationInfo alloc_info_out{};

    vmaCreateBuffer(
        device->get_allocator(),
        &buffer_info,
        &alloc_info,
        &sbt_buffer,
        &sbt_alloc,
        &alloc_info_out
    );
    uint8_t* mapped = static_cast<uint8_t*>(alloc_info_out.pMappedData);

    for (uint32_t i = 0; i < group_count; i++) {
        memcpy(mapped + i * handle_size_aligned,handles.data() + i * handle_size,handle_size);
    }
    VkBufferDeviceAddressInfo addr_info{};
    addr_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addr_info.buffer = sbt_buffer;

    VkDeviceAddress sbt_address =vkGetBufferDeviceAddress(d, &addr_info);

    SBTRegionsRT regions{};

    uint32_t stride = handle_size_aligned;

    regions.raygen.deviceAddress = sbt_address;
    regions.raygen.stride = stride;
    regions.raygen.size = stride;

    regions.miss.deviceAddress =sbt_address + stride * 1;
    regions.miss.stride = stride;
    regions.miss.size = stride *n_miss;

    regions.hit.deviceAddress =regions.miss.deviceAddress + regions.miss.size;
    regions.hit.stride = stride;
    regions.hit.size = stride *n_hit;

    regions.callable = {};

    for (auto& stage : shader_stages) {
        vkDestroyShaderModule(d, stage.module, nullptr);
    }
    shader_stages.clear();
    shader_groups.clear();

    return std::unique_ptr<VKRTPipeline>(
        new VKRTPipeline(device, pipeline, layout)
    );
}