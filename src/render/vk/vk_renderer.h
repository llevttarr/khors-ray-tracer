#ifndef VK_RENDERER_H
#define VK_RENDERER_H

#include <memory>
#include <stdexcept>
#include <cstring>
#include <functional>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_cmanager.h"
#include "vk_descriptor.h"

#include "vk_texture.h"
#include "vk_buffer.h"

#include "vk_g_pipeline.h"
#include "vk_comp_pipeline.h"
#include "vk_rt_pipeline.h"

#include "renderer.h"
#include "scene.h"
#include "camera.h"

static constexpr uint32_t LOCAL_SIZE_X = 8;
static constexpr uint32_t LOCAL_SIZE_Y = 8;

struct ReSTIRPushConstants {
    uint32_t width;
    uint32_t height;
    uint32_t tric;
    uint32_t spherec;
    uint32_t bvhc;
    uint32_t matc;
    uint32_t lightc;
    uint32_t framec;
    uint32_t init_candidates;
};

struct alignas(16) CameraUBO {
    Vec4<float> pos;
    Vec4<float> forward;
    Vec4<float> right;
    Vec4<float> up;
    Vec4<float> prev_pos;
    Vec4<float> prev_forward;
    Vec4<float> prev_right;
    Vec4<float> prev_up;
};
struct alignas(4) PushConstants {
    uint32_t width;
    uint32_t height;
    uint32_t tric;
    uint32_t spherec;
    uint32_t bvhc;
    uint32_t matc;
    uint32_t lightc;
    uint32_t framec;
    uint32_t init_candidates;
};
struct PrevCameraState {
    Vec3<float> pos{};
    Vec3<float> forward{};
    Vec3<float> right{};
    Vec3<float> up{};
    float fov = 0.0f;
    float aspect = 0.0f;
};

/**
 * the Vulkan renderer; owns all GPU resources
 */
class VKRenderer : public Renderer{
private:
    std::shared_ptr<VKDevice> device;
    std::shared_ptr<VKSwapchain> swapchain;
    std::unique_ptr<VKCmanager> cmanager;

    /** PIPELINES */

    std::unique_ptr<VKRTPipeline>pipeline_res_sampling;
    std::unique_ptr<VKComputePipeline>pipeline_temp_reuse;
    std::unique_ptr<VKComputePipeline>pipeline_spat_reuse;
    std::unique_ptr<VKRTPipeline> pipeline_res_shade;
    std::unique_ptr<VKGraphicsPipeline> pipeline_present;
    
    void init_pipelines();
    void dispatch_res_sampling(VkCommandBuffer cmd, uint32_t dx, uint32_t dy);
    void dispatch_temp_reuse(VkCommandBuffer cmd, uint32_t dx, uint32_t dy);
    void dispatch_spat_reuse(VkCommandBuffer cmd, uint32_t dx, uint32_t dy);
    void dispatch_res_shade(VkCommandBuffer cmd, uint32_t dx, uint32_t dy);

    /** DESCRIPTOR */

    std::unique_ptr<VKDescriptorAllocator> descriptor_allocator;
    VkDescriptorSetLayout camera_dsl = VK_NULL_HANDLE;
    VkDescriptorSetLayout scene_dsl = VK_NULL_HANDLE;
    VkDescriptorSetLayout pingpong_dsl = VK_NULL_HANDLE;
    VkDescriptorSetLayout output_dsl = VK_NULL_HANDLE;
    VkDescriptorSetLayout present_dsl = VK_NULL_HANDLE;

    VkDescriptorSet camera_sets[MAX_FRAMES_IN_FLIGHT] = {};
    VkDescriptorSet scene_set = VK_NULL_HANDLE;
    VkDescriptorSet pingpong_sets[2] = {};
    VkDescriptorSet output_set = VK_NULL_HANDLE;
    VkDescriptorSet present_set = VK_NULL_HANDLE;

    void init_descriptor_layouts();
    void update_scene_descriptor();
    void update_output_descriptor();
    void update_present_descriptor();
    void update_pingpong_descriptors();
 
    /** GEOMETRY */
    VKBuffer tri_buf;
    VKBuffer sphr_buf;
    VKBuffer bvh_buf;
    VKBuffer mat_buf;
    VKBuffer prim_buf;
    VKBuffer light_buf;
 
    uint32_t tric = 0;
    uint32_t spherec = 0;
    uint32_t bvhc = 0;
    uint32_t matc = 0;
    uint32_t lightc = 0;

    /** MAIN BUFFERS */
    std::array<VKBuffer,2> reservoir_ab;
    VKBuffer reservoir_b;
    std::array<VKBuffer,2> gbuffer_ab;
    uint32_t pingpong_index = 0;
    VKTexture cbuff_tex;
    VKTexture accum_tex;
    VKTexture refl_accum_tex;

    /**  TEXTURE ARRAYS */
    VKTexture base_tex_arr;
    VKTexture normal_tex_arr;
    VKTexture specular_tex_arr;
    uint32_t  tex_layer_count = 0;

    /** FRAME STATE */
    uint32_t current_width = 0;
    uint32_t current_height = 0;
    bool framebuffer_resized = false;
    uint32_t framec= 0;
    uint32_t init_candidates_restir = 32;

    /** CAMERA */
    EulerCamera& camera;
    std::array<VKBuffer, MAX_FRAMES_IN_FLIGHT> camera_ubos;
    PrevCameraState prev_camera{};
    bool prev_camera_valid = false;
    void init_camera_ubos();
    void update_camera_ubo(uint32_t frame_idx);
    bool camera_moved() const;

    /** HELPERS */
    void upload_scene_buffers(RenderScene& scene);
    void create_texture_arrays(RenderScene& scene);
    void create_restir_buffers();
    void create_storage_images();
    void record_present_pass(VkCommandBuffer cmd);

    void one_time_submit(const std::function<void(VkCommandBuffer)>& fn);
    PushConstants make_push_constants() const;

    /** BARRIERS */
    void buf_barrier(VkCommandBuffer cmd,VkBuffer buf,VkDeviceSize size,
                     VkPipelineStageFlags2 src_stage,VkAccessFlags2 src_access,VkPipelineStageFlags2 dst_stage,VkAccessFlags2 dst_access);

    void img_barrier(VkCommandBuffer cmd,VkImage img,VkPipelineStageFlags2 src_stage,VkAccessFlags2 src_access,VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access,VkImageLayout old_layout,VkImageLayout new_layout);

public:
    VKRenderer(std::shared_ptr<VKDevice> device, std::shared_ptr<VKSwapchain> swapchain,std::unique_ptr<VKCmanager> cmanager, EulerCamera& camera);
    ~VKRenderer() override;
    VKRenderer(const VKRenderer&) = delete;
    VKRenderer& operator=(const VKRenderer&) = delete;
    VKRenderer(VKRenderer&&) = delete;
    VKRenderer& operator=(VKRenderer&&) = delete;


    void run_rs();
    void update_scene(RenderScene& scene) override;

    void on_window_resize(uint32_t w, uint32_t h);

    /**
     * block CPU for resizing/destruction
     */
    void wait_idle();
};

#endif // VK_RENDERER_H