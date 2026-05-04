#ifndef VK_RENDERER_H
#define VK_RENDERER_H

#include <memory>
#include <volk.h>
#include "vk_mem_alloc.h"

#include "app_util.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_cmanager.h"

#include "vk_g_pipeline.h"
#include "vk_comp_pipeline.h"
#include "vk_rt_pipeline.h"

#include "renderer.h"
#include "camera.h"

/**
 * the Vulkan renderer; owns all GPU resources
 */
class VKRenderer : public Renderer{
private:
    std::shared_ptr<VKDevice> device;
    std::shared_ptr<VKSwapchain> swapchain;
    std::unique_ptr<VKCmanager> cmanager;

    std::unique_ptr<VKGraphicsPipeline> main_graphics_pipeline;
    std::unique_ptr<VKComputePipeline>  culling_compute_pipeline;
    std::unique_ptr<VKRTPipeline> raytracing_pipeline;

    bool is_framebuffer_resized = false;
    uint32_t current_width = 0;
    uint32_t current_height = 0;

    EulerCamera& camera;

    void init_pipelines();
public:
    VKRenderer(std::shared_ptr<VKDevice> device, std::shared_ptr<VKSwapchain> swapchain,std::shared_ptr<VKCmanager> cmanager, EulerCamera& camera);
    ~VKRenderer() override;

    void run_rs();
    void on_window_resize(uint32_t w, uint32_t h);

    /**
     * block CPU for resizing/destruction
     */
    void wait_idle();
};

#endif // VK_RENDERER_H