#include "vk_accelstruct.h"
VKAccelStructure::~VKAccelStructure() {
    if (handle != VK_NULL_HANDLE && device) {
        vkDestroyAccelerationStructureKHR(device->get_logic_device(), handle, nullptr);
        handle = VK_NULL_HANDLE;
    }
}
 VKAccelStructure::VKAccelStructure(VKAccelStructure&& o) noexcept
    : device(std::move(o.device)), buffer(std::move(o.buffer)), handle(o.handle), device_address(o.device_address){
    o.handle = VK_NULL_HANDLE;
    o.device_address = 0;
}
VKAccelStructure& VKAccelStructure::operator=(VKAccelStructure&& o) noexcept {
    if (this != &o) {
        this->~VKAccelStructure();
        device = std::move(o.device);
        buffer = std::move(o.buffer);
        handle = o.handle;
        device_address = o.device_address;
        o.handle = VK_NULL_HANDLE;
        o.device_address = 0;
    }
    return *this;
}

VKAccelBuilder::VKAccelBuilder(std::shared_ptr<VKDevice> dev,std::function<void(const std::function<void(VkCommandBuffer)>&)> submit): device(std::move(dev)), submit_fn(std::move(submit))
{}

 VkDeviceSize VKAccelBuilder::scratch_alignment() const {
    VkPhysicalDeviceAccelerationStructurePropertiesKHR as_props{};
    as_props.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 props2{};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &as_props;
    vkGetPhysicalDeviceProperties2(device->get_phys_device(), &props2);

    return as_props.minAccelerationStructureScratchOffsetAlignment;
}

VKBuffer VKAccelBuilder::alloc_scratch(VkDeviceSize size) {
    VKBuffer buf(device);
    buf.create(
        size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,VMA_MEMORY_USAGE_GPU_ONLY,VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    return buf;
}

std::unique_ptr<VKAccelStructure> VKAccelBuilder::alloc_accel_struct(VkAccelerationStructureTypeKHR type,VkDeviceSize size){
    auto as  = std::make_unique<VKAccelStructure>();
    as->device = device;
    as->buffer = VKBuffer(device);
    as->buffer.create(size,VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,VMA_MEMORY_USAGE_GPU_ONLY);

    VkAccelerationStructureCreateInfoKHR ci{};
    ci.sType= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    ci.buffer = as->buffer.get();
    ci.size = size;
    ci.type = type;
    if (vkCreateAccelerationStructureKHR(
            device->get_logic_device(), &ci, nullptr, &as->handle) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateAccelerationStructureKHR failed");
    }

    VkAccelerationStructureDeviceAddressInfoKHR addr_info{};
    addr_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addr_info.accelerationStructure = as->handle;
    as->device_address = vkGetAccelerationStructureDeviceAddressKHR(
        device->get_logic_device(), &addr_info);

    return as;
}
std::unique_ptr<VKAccelStructure> VKAccelBuilder::build_empty_tlas(){
    VkAccelerationStructureBuildGeometryInfoKHR build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = nullptr;

    uint32_t instance_count = 0;

    VkAccelerationStructureBuildSizesInfoKHR size_info{};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    vkGetAccelerationStructureBuildSizesKHR(
        device->get_logic_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info,
        &instance_count,
        &size_info
    );

    auto tlas_as = alloc_accel_struct(
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        size_info.accelerationStructureSize
    );

    const VkDeviceSize scratch_align = scratch_alignment();
    const VkDeviceSize scratch_size =
        (size_info.buildScratchSize + scratch_align - 1) & ~(scratch_align - 1);

    VKBuffer scratch = alloc_scratch(scratch_size);

    VkBufferDeviceAddressInfo scratch_dai{};
    scratch_dai.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratch_dai.buffer = scratch.get();

    VkDeviceAddress scratch_addr =
        vkGetBufferDeviceAddress(device->get_logic_device(), &scratch_dai);

    build_info.dstAccelerationStructure = tlas_as->handle;
    build_info.scratchData.deviceAddress = scratch_addr;

    const VkAccelerationStructureBuildRangeInfoKHR* p_range = nullptr;

    submit_fn([&](VkCommandBuffer cmd) {
        vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, &p_range);

        VkMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        barrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

        VkDependencyInfo dep{};
        dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.memoryBarrierCount = 1;
        dep.pMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(cmd, &dep);
    });

    return tlas_as;  
}
std::unique_ptr<VKAccelStructure> VKAccelBuilder::build_blas(const std::vector<RenderTri>& tris){
    if (tris.empty())
        throw std::runtime_error("VKAccelBuilder::build_blas: no triangles");

    const uint32_t tri_count  = static_cast<uint32_t>(tris.size());
    const uint32_t vert_count = tri_count * 3;
    struct Vert { float x, y, z; };
    std::vector<Vert> verts;
    verts.reserve(vert_count);
    for (const auto& t : tris) {
        verts.push_back({ t.v0.x, t.v0.y, t.v0.z });
        verts.push_back({ t.v1.x, t.v1.y, t.v1.z });
        verts.push_back({ t.v2.x, t.v2.y, t.v2.z });
    }

    const VkDeviceSize vert_bytes = verts.size() * sizeof(Vert);

    VKBuffer vert_staging(device);
    vert_staging.create(vert_bytes,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        VMA_ALLOCATION_CREATE_MAPPED_BIT);
    vert_staging.write(verts.data(), vert_bytes);

    VKBuffer vert_buf(device);
    vert_buf.create(vert_bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VMA_MEMORY_USAGE_GPU_ONLY);

    submit_fn([&](VkCommandBuffer cmd) {
        VkBufferCopy r{ 0, 0, vert_bytes };
        vkCmdCopyBuffer(cmd, vert_staging.get(), vert_buf.get(), 1, &r);
    });

    VkBufferDeviceAddressInfo dai{};
    dai.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    dai.buffer = vert_buf.get();
    VkDeviceAddress vert_addr = vkGetBufferDeviceAddress(device->get_logic_device(), &dai);

    VkAccelerationStructureGeometryTrianglesDataKHR tri_data{};
    tri_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    tri_data.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    tri_data.vertexData.deviceAddress = vert_addr;
    tri_data.vertexStride = sizeof(Vert);
    tri_data.maxVertex = vert_count - 1;
    tri_data.indexType = VK_INDEX_TYPE_NONE_KHR;

    VkAccelerationStructureGeometryKHR geom{};
    geom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geom.geometry.triangles = tri_data;
    geom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    VkAccelerationStructureBuildGeometryInfoKHR build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode= VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &geom;

    VkAccelerationStructureBuildSizesInfoKHR size_info{};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device->get_logic_device(),VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info,&tri_count,&size_info);
    auto blas = alloc_accel_struct(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,size_info.accelerationStructureSize);

    const VkDeviceSize scratch_align = scratch_alignment();
    const VkDeviceSize scratch_size  =(size_info.buildScratchSize + scratch_align - 1) & ~(scratch_align - 1);
    VKBuffer scratch = alloc_scratch(scratch_size);

    VkBufferDeviceAddressInfo scratch_dai{};
    scratch_dai.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratch_dai.buffer = scratch.get();
    VkDeviceAddress scratch_addr = vkGetBufferDeviceAddress(device->get_logic_device(), &scratch_dai);

    build_info.dstAccelerationStructure = blas->handle;
    build_info.scratchData.deviceAddress = scratch_addr;
    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = tri_count;
    range.primitiveOffset = 0;
    range.firstVertex = 0;
    range.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* p_range = &range;

    submit_fn([&](VkCommandBuffer cmd) {
        vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, &p_range);

        VkMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        barrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstStageMask= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

        VkDependencyInfo dep{};
        dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.memoryBarrierCount = 1;
        dep.pMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cmd, &dep);
    });
    return blas;
}
std::unique_ptr<VKAccelStructure> VKAccelBuilder::build_tlas(
    const VKAccelStructure& blas){
    VkAccelerationStructureInstanceKHR instance{};
    instance.transform.matrix[0][0] = 1.0f;
    instance.transform.matrix[1][1] = 1.0f;
    instance.transform.matrix[2][2] = 1.0f;

    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = blas.get_address();
    const VkDeviceSize inst_bytes = sizeof(VkAccelerationStructureInstanceKHR);

    VKBuffer inst_staging(device);
    inst_staging.create(inst_bytes,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VMA_MEMORY_USAGE_CPU_ONLY,VMA_ALLOCATION_CREATE_MAPPED_BIT);
    inst_staging.write(&instance, inst_bytes);

    VKBuffer inst_buf(device);
    inst_buf.create(inst_bytes,VK_BUFFER_USAGE_TRANSFER_DST_BIT |VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,VMA_MEMORY_USAGE_GPU_ONLY);

    submit_fn([&](VkCommandBuffer cmd) {
        VkBufferCopy r{ 0, 0, inst_bytes };
        vkCmdCopyBuffer(cmd, inst_staging.get(), inst_buf.get(), 1, &r);

        VkMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

        VkDependencyInfo dep{};
        dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.memoryBarrierCount = 1;
        dep.pMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cmd, &dep);
    });

    VkBufferDeviceAddressInfo dai{};
    dai.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    dai.buffer = inst_buf.get();
    VkDeviceAddress inst_addr = vkGetBufferDeviceAddress(device->get_logic_device(), &dai);

    VkAccelerationStructureGeometryInstancesDataKHR inst_data{};
    inst_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    inst_data.arrayOfPointers = VK_FALSE;
    inst_data.data.deviceAddress = inst_addr;

    VkAccelerationStructureGeometryKHR geom{};
    geom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geom.geometry.instances = inst_data;

    constexpr uint32_t instance_count = 1;

    VkAccelerationStructureBuildGeometryInfoKHR build_info{};
    build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_info.type= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &geom;

    VkAccelerationStructureBuildSizesInfoKHR size_info{};
    size_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(device->get_logic_device(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_info,
        &instance_count,
        &size_info);
    auto tlas_as = alloc_accel_struct(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,size_info.accelerationStructureSize);

    const VkDeviceSize scratch_align = scratch_alignment();
    const VkDeviceSize scratch_size  =(size_info.buildScratchSize + scratch_align - 1) & ~(scratch_align - 1);
    VKBuffer scratch = alloc_scratch(scratch_size);

    VkBufferDeviceAddressInfo scratch_dai{};
    scratch_dai.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratch_dai.buffer = scratch.get();
    VkDeviceAddress scratch_addr = vkGetBufferDeviceAddress(
        device->get_logic_device(), &scratch_dai);

    build_info.dstAccelerationStructure = tlas_as->handle;
    build_info.scratchData.deviceAddress = scratch_addr;

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = instance_count;
    const VkAccelerationStructureBuildRangeInfoKHR* p_range = &range;

    submit_fn([&](VkCommandBuffer cmd) {vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, &p_range);
        VkMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
        barrier.srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstStageMask= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;

        VkDependencyInfo dep{};
        dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dep.memoryBarrierCount = 1;
        dep.pMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(cmd, &dep);
    });
    return tlas_as;
}
