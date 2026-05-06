#ifndef VK_ACCELSTRUCT_H
#define VK_ACCELSTRUCT_H

#include <memory>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cstring>
#include <volk.h>

#include "vk_device.h"
#include "vk_buffer.h"
#include "scene.h"
class VKAccelStructure {
    friend class VKAccelBuilder;
private:
    std::shared_ptr<VKDevice>  device;
    VKBuffer buffer;
    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    VkDeviceAddress device_address = 0;
public:
    VKAccelStructure() = default;
    ~VKAccelStructure();

    VKAccelStructure(const VKAccelStructure&) = delete;
    VKAccelStructure& operator=(const VKAccelStructure&) = delete;
    VKAccelStructure(VKAccelStructure&&) noexcept;
    VKAccelStructure& operator=(VKAccelStructure&&) noexcept;

    VkAccelerationStructureKHR get() const { return handle;}
    VkDeviceAddress get_address() const {return device_address; }
    bool is_valid() const { return handle != VK_NULL_HANDLE; }
};
class VKAccelBuilder {
public:
    explicit VKAccelBuilder(std::shared_ptr<VKDevice> device,std::function<void(const std::function<void(VkCommandBuffer)>&)> submit);

    std::function<void(const std::function<void(VkCommandBuffer)>&)> submit_fn;

    std::unique_ptr<VKAccelStructure> build_blas(const std::vector<RenderTri>& tris);
    std::unique_ptr<VKAccelStructure> build_tlas(const VKAccelStructure& blas);
    std::unique_ptr<VKAccelStructure> build_empty_tlas();
private:
    std::shared_ptr<VKDevice> device;
    VkDeviceSize scratch_alignment() const;

    std::unique_ptr<VKAccelStructure> alloc_accel_struct(VkAccelerationStructureTypeKHR type,VkDeviceSize size);

    VKBuffer alloc_scratch(VkDeviceSize size);
};



#endif // VK_ACCELSTRUCT_H