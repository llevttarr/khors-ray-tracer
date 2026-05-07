#include "vk_renderer.h"


VKRenderer::VKRenderer(std::shared_ptr<VKDevice> dev,std::shared_ptr<VKSwapchain>swap,std::unique_ptr<VKCmanager>cmgr,EulerCamera& cam)
    : device(std::move(dev)),swapchain(std::move(swap)),
cmanager(std::move(cmgr)),camera(cam),current_width (static_cast<uint32_t>(cam.get_w())),current_height(static_cast<uint32_t>(cam.get_h())), tri_buf  (device), sphr_buf(device), bvh_buf(device),
mat_buf  (device), prim_buf(device), light_buf(device), reservoir_b(device),
    cbuff_tex(device), accum_tex(device), refl_accum_tex(device),
    base_tex_arr(device), normal_tex_arr(device), specular_tex_arr(device)
{
    for (int i = 0; i < 2; ++i) {
        reservoir_ab[i] = VKBuffer(device);
        gbuffer_ab[i] = VKBuffer(device);
    }
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        camera_ubos[i] = VKBuffer(device);
    }

    descriptor_allocator = std::make_unique<VKDescriptorAllocator>(device);

    accel_builder = std::make_unique<VKAccelBuilder>(device,[this](const std::function<void(VkCommandBuffer)>& fn){ one_time_submit(fn); });

    init_descriptor_layouts();
    init_camera_ubos();
    init_pipelines();

    create_restir_buffers();
    create_storage_images();
    update_pingpong_descriptors();
    update_output_descriptor();
    update_present_descriptor();
}

VKRenderer::~VKRenderer() {
    wait_idle();
    VkDevice vkdev = device->get_logic_device();
    for (auto dsl : { camera_dsl, scene_dsl, pingpong_dsl, output_dsl, present_dsl }) {
        if (dsl != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(vkdev, dsl, nullptr);
    }
}

void VKRenderer::init_pipelines() {
    std::cout<<"init pipeliens"<<std::endl;
    constexpr VkShaderStageFlags PC_STAGES_COMPUTE =VK_SHADER_STAGE_COMPUTE_BIT;
    constexpr VkShaderStageFlags PC_STAGES_RT =VK_SHADER_STAGE_RAYGEN_BIT_KHR|VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR|VK_SHADER_STAGE_MISS_BIT_KHR;

    pipeline_res_sampling = VKRTPipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_RT, 0, sizeof(PushConstants))
        .add_raygen_shader("assets/shaders/vk_res_sampling.rgen.spv")
        .add_miss_shader("assets/shaders/vk_res_sampling.rmiss.spv")
        .add_closest_hit_shader("assets/shaders/vk_res_sampling.rchit.spv")
        .build();
    pipeline_temp_reuse = VKComputePipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_COMPUTE, 0, sizeof(PushConstants))
        .set_shader("assets/shaders/vk_temp_reuse.comp.spv")
        .build();
    pipeline_spat_reuse = VKComputePipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_push_constant(PC_STAGES_COMPUTE, 0, sizeof(PushConstants))
        .set_shader("assets/shaders/vk_spat_reuse.comp.spv")
        .build();
    pipeline_res_shade = VKRTPipelineBuilder(device)
        .add_descriptor_set_layout(camera_dsl)
        .add_descriptor_set_layout(scene_dsl)
        .add_descriptor_set_layout(pingpong_dsl)
        .add_descriptor_set_layout(output_dsl)
        .add_push_constant(PC_STAGES_RT, 0, sizeof(PushConstants))
        .add_raygen_shader("assets/shaders/vk_res_shade.rgen.spv")
        .add_miss_shader("assets/shaders/vk_res_shade.rmiss.spv")
        .add_miss_shader("assets/shaders/vk_res_shade_refl.rmiss.spv")
        .add_closest_hit_shader("assets/shaders/vk_res_shade.rchit.spv")
        .build();
    pipeline_present = VKGraphicsPipelineBuilder(device)
        .add_descriptor_set_layout(present_dsl)
        .set_shaders("assets/shaders/vk_vs.vert.spv","assets/shaders/vk_fs.frag.spv")
        .set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .set_polygon_mode(VK_POLYGON_MODE_FILL)
        .set_cull_mode (VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .disable_depth_test()
        .set_render_formats(swapchain->get_format(), VK_FORMAT_UNDEFINED)
        .build();
}

void VKRenderer::init_descriptor_layouts() {
    VkDevice vkdev = device->get_logic_device();
    auto binding = [](uint32_t b, VkDescriptorType type,VkShaderStageFlags stages,uint32_t count = 1) -> VkDescriptorSetLayoutBinding{
        VkDescriptorSetLayoutBinding r{};
        r.binding = b;
        r.descriptorType = type;
        r.descriptorCount = count;
        r.stageFlags = stages;
        return r;
    };
 
    auto create_layout = [&](const std::vector<VkDescriptorSetLayoutBinding>& bindings,VkDescriptorSetLayout& out){
        VkDescriptorSetLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ci.bindingCount = static_cast<uint32_t>(bindings.size());
        ci.pBindings = bindings.data();
        if (vkCreateDescriptorSetLayout(vkdev, &ci, nullptr, &out) != VK_SUCCESS)
            throw std::runtime_error("vkCreateDescriptorSetLayout failed");
    };
 
    constexpr VkShaderStageFlags COMPUTE_RT =VK_SHADER_STAGE_COMPUTE_BIT |VK_SHADER_STAGE_RAYGEN_BIT_KHR |VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |VK_SHADER_STAGE_MISS_BIT_KHR;
 
    // camera
    create_layout({
        binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, COMPUTE_RT),
    }, camera_dsl);
 
    // geometry
    create_layout({
        binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // tri
        binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), //sphr
        binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), //bvh
        binding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // mats
        binding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // prims
        binding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // lights
        binding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, COMPUTE_RT), // base
        binding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, COMPUTE_RT), // normal
        binding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, COMPUTE_RT), // spec
        binding(9, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, COMPUTE_RT), // tlas
    }, scene_dsl);
    // gbuff
    create_layout({
        binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // reservoir_current
        binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // reservoir_b
        binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // reservoir_history
        binding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // gbuffer_current
        binding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, COMPUTE_RT), // gbuffer_history
    }, pingpong_dsl);

    // output
    create_layout({
        binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, COMPUTE_RT), // cbuff 
        binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, COMPUTE_RT), // accum 
        binding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, COMPUTE_RT), // refl_accum 
    }, output_dsl);
 
    create_layout({
        binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
    }, present_dsl);
}
 
void VKRenderer::init_camera_ubos() {
    VkDevice vkdev = device->get_logic_device();
 
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        camera_ubos[i].create(
            sizeof(CameraUBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
        descriptor_allocator->allocate(&camera_sets[i], camera_dsl);
 
        VkDescriptorBufferInfo buf_info{};
        buf_info.buffer = camera_ubos[i].get();
        buf_info.offset = 0;
        buf_info.range = sizeof(CameraUBO);
 
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = camera_sets[i];
        w.dstBinding = 0;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.pBufferInfo = &buf_info;
        vkUpdateDescriptorSets(vkdev, 1, &w, 0, nullptr);
    }
}
void VKRenderer::update_camera_ubo(uint32_t frame_idx) {
    const float aspect = static_cast<float>(current_width) /static_cast<float>(current_height);
 
    CameraUBO ubo{};
    ubo.pos = Vec4<float>(camera.get_pos().x,camera.get_pos().y,camera.get_pos().z,camera.get_fov());
    ubo.forward = Vec4<float>(camera.get_forward().x,camera.get_forward().y,camera.get_forward().z,aspect);
    ubo.right = Vec4<float>(camera.get_right().x,camera.get_right().y,camera.get_right().z,0.0f);
    ubo.up =Vec4<float>(camera.get_up().x,camera.get_up().y,camera.get_up().z,0.0f);
 
    if (prev_camera_valid) {
        ubo.prev_pos = Vec4<float>(prev_camera.pos.x,prev_camera.pos.y,prev_camera.pos.z,prev_camera.fov);
        ubo.prev_forward = Vec4<float>(prev_camera.forward.x,prev_camera.forward.y,prev_camera.forward.z,prev_camera.aspect);
        ubo.prev_right = Vec4<float>(prev_camera.right.x,prev_camera.right.y,prev_camera.right.z,0.0f);
        ubo.prev_up = Vec4<float>(prev_camera.up.x,prev_camera.up.y,prev_camera.up.z,0.0f);
    } else {
        ubo.prev_pos = ubo.pos;
        ubo.prev_forward = ubo.forward;
        ubo.prev_right = ubo.right;
        ubo.prev_up= ubo.up;
    }
    camera_ubos[frame_idx].write(&ubo, sizeof(CameraUBO));
}


void VKRenderer::update_scene_descriptor() {
    VkDevice vkdev = device->get_logic_device();
    descriptor_allocator->allocate(&scene_set, scene_dsl);
 
    auto buf_write = [&](uint32_t binding, VkBuffer buf, VkDeviceSize size)-> VkWriteDescriptorSet {
        static VkDescriptorBufferInfo infos[6];
        infos[binding].buffer = buf;
        infos[binding].offset = 0;
        infos[binding].range  = size;
 
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = scene_set;
        w.dstBinding = binding;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        w.pBufferInfo = &infos[binding];
        return w;
    };
    VkDescriptorBufferInfo bi_tri{ tri_buf.get(), 0, tri_buf.get_size()};
    VkDescriptorBufferInfo bi_sphr{ sphr_buf.get(), 0, sphr_buf.get_size()};
    VkDescriptorBufferInfo bi_bvh{ bvh_buf.get(), 0, bvh_buf.get_size()};
    VkDescriptorBufferInfo bi_mat{ mat_buf.get(), 0, mat_buf.get_size()};
    VkDescriptorBufferInfo bi_prim{ prim_buf.get(),0, prim_buf.get_size()};
    VkDescriptorBufferInfo bi_lght{ light_buf.get(), 0, light_buf.get_size()};
 
    VkDescriptorImageInfo ii_base {
        base_tex_arr.get_sampler(), base_tex_arr.get_view(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkDescriptorImageInfo ii_norm {
        normal_tex_arr.get_sampler(), normal_tex_arr.get_view(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkDescriptorImageInfo ii_spec {
        specular_tex_arr.get_sampler(), specular_tex_arr.get_view(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
 
    auto sbuf = [](VkDescriptorSet set, uint32_t b, VkDescriptorBufferInfo* info)-> VkWriteDescriptorSet{
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = set;
        w.dstBinding = b;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        w.pBufferInfo = info;
        return w;
    };
    auto simg = [](VkDescriptorSet set, uint32_t b, VkDescriptorImageInfo* info)-> VkWriteDescriptorSet{
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = set;
        w.dstBinding = b;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.pImageInfo = info;
        return w;
    };

    VkWriteDescriptorSetAccelerationStructureKHR as_write{};
    as_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    as_write.accelerationStructureCount = 1;
    VkAccelerationStructureKHR tlas_handle = tlas->get();
    as_write.pAccelerationStructures = &tlas_handle;

    VkWriteDescriptorSet w_as{};
    w_as.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w_as.pNext = &as_write;
    w_as.dstSet = scene_set;
    w_as.dstBinding = 9;
    w_as.descriptorCount = 1;
    w_as.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    std::array<VkWriteDescriptorSet, 10> writes = {
        sbuf(scene_set, 0, &bi_tri),
        sbuf(scene_set, 1, &bi_sphr),
        sbuf(scene_set, 2, &bi_bvh),
        sbuf(scene_set, 3, &bi_mat),
        sbuf(scene_set, 4, &bi_prim),
        sbuf(scene_set, 5, &bi_lght),
        simg(scene_set, 6, &ii_base),
        simg(scene_set, 7, &ii_norm),
        simg(scene_set, 8, &ii_spec),
        w_as,
    };
    vkUpdateDescriptorSets(vkdev,
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
 
void VKRenderer::update_pingpong_descriptors() {
    VkDevice vkdev = device->get_logic_device();
 
    for (uint32_t i = 0; i < 2; ++i) {
        const uint32_t cur = i;
        const uint32_t his = 1 - i;
 
        descriptor_allocator->allocate(&pingpong_sets[i], pingpong_dsl);
 
        VkDescriptorBufferInfo bi_res_cur { reservoir_ab[cur].get(), 0,reservoir_ab[cur].get_size() };
        VkDescriptorBufferInfo bi_res_b{ reservoir_b.get(),0,reservoir_b.get_size()};
        VkDescriptorBufferInfo bi_res_his { reservoir_ab[his].get(), 0,reservoir_ab[his].get_size() };
        VkDescriptorBufferInfo bi_gbuf_cur{ gbuffer_ab[cur].get(), 0,gbuffer_ab[cur].get_size()   };
        VkDescriptorBufferInfo bi_gbuf_his{ gbuffer_ab[his].get(),0,gbuffer_ab[his].get_size()   };
 
        auto w = [&](uint32_t b, VkDescriptorBufferInfo* info) -> VkWriteDescriptorSet {
            VkWriteDescriptorSet wd{};
            wd.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            wd.dstSet= pingpong_sets[i];
            wd.dstBinding = b;
            wd.descriptorCount = 1;
            wd.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            wd.pBufferInfo = info;
            return wd;
        };
 
        std::array<VkWriteDescriptorSet, 5> writes = {
            w(0, &bi_res_cur),
            w(1, &bi_res_b),
            w(2, &bi_res_his),
            w(3, &bi_gbuf_cur),
            w(4, &bi_gbuf_his),
        };
        vkUpdateDescriptorSets(vkdev,
            static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}
 
void VKRenderer::update_output_descriptor() {
    VkDevice vkdev = device->get_logic_device();
    descriptor_allocator->allocate(&output_set, output_dsl);
 
    VkDescriptorImageInfo ii_cbuff{ VK_NULL_HANDLE, cbuff_tex.get_view(), VK_IMAGE_LAYOUT_GENERAL };
    VkDescriptorImageInfo ii_accum { VK_NULL_HANDLE, accum_tex.get_view(), VK_IMAGE_LAYOUT_GENERAL };
    VkDescriptorImageInfo ii_refl_accum{ VK_NULL_HANDLE, refl_accum_tex.get_view(), VK_IMAGE_LAYOUT_GENERAL };
 
    auto simg = [&](uint32_t b, VkDescriptorImageInfo* info) -> VkWriteDescriptorSet {
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet = output_set;
        w.dstBinding = b;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        w.pImageInfo = info;
        return w;
    };
 
    std::array<VkWriteDescriptorSet, 3> writes = {
        simg(0, &ii_cbuff),
        simg(1, &ii_accum),
        simg(2, &ii_refl_accum),
    };
    vkUpdateDescriptorSets(vkdev,
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
 
void VKRenderer::update_present_descriptor() {
    VkDevice vkdev = device->get_logic_device();
    descriptor_allocator->allocate(&present_set, present_dsl);
    VkDescriptorImageInfo ii{
        cbuff_tex.get_sampler(),
        cbuff_tex.get_view(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
 
    VkWriteDescriptorSet w{};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet = present_set;
    w.dstBinding = 0;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo = &ii;
    vkUpdateDescriptorSets(vkdev, 1, &w, 0, nullptr);
}


void VKRenderer::create_restir_buffers() {
    const VkDeviceSize res_size = static_cast<VkDeviceSize>(current_width)* current_height * sizeof(Reservoir);
    const VkDeviceSize gbuf_size = static_cast<VkDeviceSize>(current_width)* current_height * sizeof(GBufferPixel);
 
    constexpr VkBufferUsageFlags BUF_FLAGS =VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
 
    for (int i = 0; i < 2; ++i) {
        reservoir_ab[i].destroy();
        reservoir_ab[i].create(res_size,BUF_FLAGS, VMA_MEMORY_USAGE_GPU_ONLY);
        gbuffer_ab[i].destroy();
        gbuffer_ab[i].create(gbuf_size, BUF_FLAGS, VMA_MEMORY_USAGE_GPU_ONLY);
    }
    reservoir_b.destroy();
    reservoir_b.create(res_size, BUF_FLAGS, VMA_MEMORY_USAGE_GPU_ONLY);
}
 
void VKRenderer::create_storage_images() {
    const VkExtent3D ext{ current_width, current_height, 1 };
    constexpr VkFormat FMT = VK_FORMAT_R32G32B32A32_SFLOAT;
    cbuff_tex.destroy();
    cbuff_tex.create_image(ext, FMT,VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,VMA_MEMORY_USAGE_GPU_ONLY);
    cbuff_tex.create_view (VK_IMAGE_ASPECT_COLOR_BIT);
    cbuff_tex.create_sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    accum_tex.destroy();
    accum_tex.create_image(ext, FMT, VK_IMAGE_USAGE_STORAGE_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    accum_tex.create_view (VK_IMAGE_ASPECT_COLOR_BIT);
 
    refl_accum_tex.destroy();
    refl_accum_tex.create_image(ext, FMT, VK_IMAGE_USAGE_STORAGE_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    refl_accum_tex.create_view (VK_IMAGE_ASPECT_COLOR_BIT);
 
    one_time_submit([&](VkCommandBuffer cmd) {
        for (VkImage img : { cbuff_tex.get_image(),accum_tex.get_image(),refl_accum_tex.get_image() }) {
            img_barrier(cmd, img,VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        }
    });
}

void VKRenderer::update_scene(RenderScene& scene) {
    wait_idle();
    tric = static_cast<uint32_t>(scene.tri_v.size());
    std::cout<<"tric: "<<tric<<std::endl;
    spherec = static_cast<uint32_t>(scene.sphr_v.size());
    bvhc = static_cast<uint32_t>(scene.bvh_v.size());
    std::cout<<"bvhc: "<<bvhc<<std::endl;
    matc = static_cast<uint32_t>(scene.mat_v.size());
    lightc = static_cast<uint32_t>(scene.light_v.size());
 
    upload_scene_buffers(scene);
    if (tric!=0){
        blas = accel_builder->build_blas(scene.tri_v);
        tlas = accel_builder->build_tlas(*blas);
    }else{
        blas.reset();
        tlas = accel_builder->build_empty_tlas();
    }

    create_texture_arrays(scene);
    update_scene_descriptor();
}
 
void VKRenderer::upload_scene_buffers(RenderScene& scene) {
    auto upload = [&](VKBuffer& dst, const void* data, VkDeviceSize size) {
        const VkDeviceSize safe_size = (size == 0) ? 16 : size;

        dst.destroy();
        dst.create(
            safe_size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        if (size == 0) {
            return;
        }
        VKBuffer staging(device);
        staging.create(
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );
        staging.write(data, size);
        one_time_submit([&](VkCommandBuffer cmd) {
            VkBufferCopy region{ 0, 0, size };
            vkCmdCopyBuffer(cmd, staging.get(), dst.get(), 1, &region);

            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = dst.get();
            barrier.offset = 0;
            barrier.size = safe_size;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr
            );
        });
    };
 
    upload(tri_buf,scene.tri_v.data(),scene.tri_v.size() * sizeof(RenderTri));
    upload(sphr_buf,scene.sphr_v.data(), scene.sphr_v.size() * sizeof(Sphr));
    upload(bvh_buf,scene.bvh_v.data(), scene.bvh_v.size() * sizeof(BVH));
    upload(mat_buf,scene.mat_v.data(),scene.mat_v.size()* sizeof(Mat));
    upload(prim_buf,scene.prim_v.data(), scene.prim_v.size() * sizeof(uint32_t));
    upload(light_buf,scene.light_v.data(),scene.light_v.size()* sizeof(Light));
}

void VKRenderer::dispatch_res_sampling(VkCommandBuffer cmd,uint32_t dx, uint32_t dy) {
    pipeline_res_sampling->bind(cmd);
 
    const VkPipelineLayout layout = pipeline_res_sampling->get_layout();
    const std::array<VkDescriptorSet, 3> sets = {
        camera_sets[cmanager->get_current_frame()],
        scene_set,
        pingpong_sets[pingpong_index],
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
 
    const auto pc = make_push_constants();
    vkCmdPushConstants(cmd, layout,VK_SHADER_STAGE_RAYGEN_BIT_KHR |VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR|VK_SHADER_STAGE_MISS_BIT_KHR,0, sizeof(pc), &pc);
 
    const auto& sbt = pipeline_res_sampling->get_sbt();
    vkCmdTraceRaysKHR(cmd,&sbt.raygen, &sbt.miss, &sbt.hit, &sbt.callable,current_width, current_height, 1);
}
 
void VKRenderer::dispatch_temp_reuse(VkCommandBuffer cmd,uint32_t dx, uint32_t dy) {
    pipeline_temp_reuse->bind(cmd);
    const VkPipelineLayout layout = pipeline_temp_reuse->get_layout();
    const std::array<VkDescriptorSet, 3> sets = {
        camera_sets[cmanager->get_current_frame()],
        scene_set,
        pingpong_sets[pingpong_index],
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    const auto pc = make_push_constants();
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(pc), &pc);
    vkCmdDispatch(cmd, dx, dy, 1);
}
 
void VKRenderer::dispatch_spat_reuse(VkCommandBuffer cmd,uint32_t dx, uint32_t dy) {
    pipeline_spat_reuse->bind(cmd);
    const VkPipelineLayout layout = pipeline_spat_reuse->get_layout();
    const std::array<VkDescriptorSet, 3> sets = {
        camera_sets[cmanager->get_current_frame()],
        scene_set,
        pingpong_sets[pingpong_index],
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
        layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    const auto pc = make_push_constants();
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(pc), &pc);
    vkCmdDispatch(cmd, dx, dy, 1);
}
 
void VKRenderer::dispatch_res_shade(VkCommandBuffer cmd,uint32_t dx, uint32_t dy) {
    pipeline_res_shade->bind(cmd);
    const VkPipelineLayout layout = pipeline_res_shade->get_layout();
    const std::array<VkDescriptorSet, 4> sets = {
        camera_sets[cmanager->get_current_frame()],
        scene_set,
        pingpong_sets[pingpong_index],
        output_set,   // cbuf; accum;refl_accum
    };
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
    const auto pc = make_push_constants();
    vkCmdPushConstants(cmd, layout,VK_SHADER_STAGE_RAYGEN_BIT_KHR |VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR|VK_SHADER_STAGE_MISS_BIT_KHR,0, sizeof(pc), &pc);
 
    const auto& sbt = pipeline_res_shade->get_sbt();
    vkCmdTraceRaysKHR(cmd,&sbt.raygen, &sbt.miss, &sbt.hit, &sbt.callable,current_width, current_height, 1);
}
 
void VKRenderer::record_present_pass(VkCommandBuffer cmd) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(current_width);
    viewport.height = static_cast<float>(current_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { current_width, current_height };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    pipeline_present->bind(cmd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_present->get_layout(), 0, 1, &present_set, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}
// void VKRenderer::run_rs_test() {
//     if (framebuffer_resized || static_cast<uint32_t>(camera.get_w()) != current_width || static_cast<uint32_t>(camera.get_h()) != current_height) {
//         on_window_resize(camera.get_w(), camera.get_h());
//     }
//     if (camera_moved()) {
//         framec = 0;
//     }
//     std::cout << "run_rs" << std::endl;
 
//     uint32_t image_index = 0;
//     VkCommandBuffer cmd  = cmanager->begin_frame(image_index);
//     if (cmd == VK_NULL_HANDLE) return;
 
//     const uint32_t frame_idx = cmanager->get_current_frame();
//     update_camera_ubo(frame_idx);

//     img_barrier(cmd, cbuff_tex.get_image(),
//         VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, 
//         VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
//         VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

//     const VkExtent2D ext = swapchain->get_extent();
    
//     cmanager->begin_rendering(cmd, swapchain->get_image_view(image_index), ext, image_index);
    
//     record_present_pass(cmd); 
    
//     cmanager->end_rendering(cmd, image_index);
//     img_barrier(cmd, cbuff_tex.get_image(),
//         VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
//         VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
//         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

//     cmanager->end_frame_and_submit(cmd, image_index);
 
//     framec++;
//     prev_camera = {
//         camera.get_pos(),
//         camera.get_forward(),
//         camera.get_right(),
//         camera.get_up(),
//         camera.get_fov(),
//         static_cast<float>(current_width) / static_cast<float>(current_height)
//     };
//     prev_camera_valid = true;
//     pingpong_index ^= 1;
// }

void VKRenderer::run_rs(std::function<void(VkCommandBuffer)> ui_draw_fn) {
    if (framebuffer_resized ||static_cast<uint32_t>(camera.get_w()) != current_width ||static_cast<uint32_t>(camera.get_h()) != current_height) {
        on_window_resize(camera.get_w(), camera.get_h());
    }
    if (camera_moved()) {
        framec = 0;
    }
 
    uint32_t image_index = 0;
    VkCommandBuffer cmd  = cmanager->begin_frame(image_index);
    if (cmd == VK_NULL_HANDLE) return;
 
    const uint32_t frame_idx = cmanager->get_current_frame();
 
    update_camera_ubo(frame_idx);
 
    const uint32_t dx = (current_width  + LOCAL_SIZE_X - 1) / LOCAL_SIZE_X;
    const uint32_t dy = (current_height + LOCAL_SIZE_Y - 1) / LOCAL_SIZE_Y;
 
    dispatch_res_sampling(cmd, dx, dy);
 
    buf_barrier(cmd, reservoir_ab[pingpong_index].get(), VK_WHOLE_SIZE,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
    buf_barrier(cmd, gbuffer_ab[pingpong_index].get(), VK_WHOLE_SIZE,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        VK_ACCESS_2_SHADER_READ_BIT| VK_ACCESS_2_TRANSFER_READ_BIT);
 
    if (framec == 0) {
        VkBufferCopy region{};
        region.size = static_cast<VkDeviceSize>(current_width)
                    * current_height * sizeof(GBufferPixel);
        vkCmdCopyBuffer(cmd,
            gbuffer_ab[pingpong_index].get(),
            gbuffer_ab[1 - pingpong_index].get(),
            1, &region);
        buf_barrier(cmd, gbuffer_ab[1 - pingpong_index].get(), VK_WHOLE_SIZE,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
    }
 
    dispatch_temp_reuse(cmd, dx, dy);
    buf_barrier(cmd, reservoir_b.get(), VK_WHOLE_SIZE,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

    dispatch_spat_reuse(cmd, dx, dy);
    buf_barrier(cmd, reservoir_ab[pingpong_index].get(), VK_WHOLE_SIZE,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_READ_BIT);

    dispatch_res_shade(cmd, dx, dy);
    img_barrier(cmd, cbuff_tex.get_image(),
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,VK_ACCESS_2_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    const VkExtent2D ext = swapchain->get_extent();
    cmanager->begin_rendering(cmd, swapchain->get_image_view(image_index), ext, image_index);
    record_present_pass(cmd);
    if(ui_draw_fn){
        ui_draw_fn(cmd);
    }
    cmanager->end_rendering(cmd, image_index);
    img_barrier(cmd, cbuff_tex.get_image(),
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    cmanager->end_frame_and_submit(cmd, image_index);
 
    framec++;
    prev_camera = {
        camera.get_pos(),
        camera.get_forward(),
        camera.get_right(),
        camera.get_up(),
        camera.get_fov(),
        static_cast<float>(current_width) / static_cast<float>(current_height)
    };
    prev_camera_valid = true;
    pingpong_index ^= 1;
}

void VKRenderer::create_texture_arrays(RenderScene& scene) {
    auto build = [&](VKTexture& dst, const std::vector<Image>& img_v) {
        dst.destroy();
        if (img_v.empty()) {
            const VkExtent3D ext1{ 1, 1, 1 };
            dst.create_image(ext1, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY,
                VK_IMAGE_TILING_OPTIMAL,1,1);
            VKBuffer staging(device);
            staging.create(4,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY,
                VMA_ALLOCATION_CREATE_MAPPED_BIT);
            const uint32_t black = 0x00000000;
            staging.write(&black, 4);
 
            one_time_submit([&](VkCommandBuffer cmd) {
                img_barrier(cmd, dst.get_image(),
                    VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
                    VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
 
                VkBufferImageCopy region{};
                region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                region.imageExtent = { 1, 1, 1 };
                vkCmdCopyBufferToImage(cmd, staging.get(), dst.get_image(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
 
                img_barrier(cmd, dst.get_image(),
                    VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                    VK_ACCESS_2_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            });
 
            dst.create_view(VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1, 1);
            dst.create_sampler(VK_FILTER_LINEAR,
                VK_SAMPLER_ADDRESS_MODE_REPEAT);
            return;
        }
        const uint32_t layer_w = static_cast<uint32_t>(img_v[0].w);
        const uint32_t layer_h = static_cast<uint32_t>(img_v[0].h);
        const uint32_t layers = static_cast<uint32_t>(img_v.size());
        constexpr uint32_t CHANNELS = 4;
        const VkDeviceSize layer_bytes = static_cast<VkDeviceSize>(layer_w)* layer_h * CHANNELS;
        const VkDeviceSize total_bytes = layer_bytes * layers;
        const VkExtent3D ext{ layer_w, layer_h, 1 };
 
        dst.create_image(ext, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,VK_IMAGE_TILING_OPTIMAL,1,layers);
        VKBuffer staging(device);
        staging.create(total_bytes,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY,
            VMA_ALLOCATION_CREATE_MAPPED_BIT);
        {
            void* mapped = staging.map();
            auto* dst_ptr = static_cast<uint8_t*>(mapped);
            for (uint32_t i = 0; i < layers; ++i) {
                const Image& img = img_v[i];
                if (static_cast<uint32_t>(img.w) != layer_w ||
                    static_cast<uint32_t>(img.h) != layer_h) {
                    staging.unmap();
                    throw std::runtime_error(
                        "create_texture_arrays: all images must be the same size");
                }
                if (img.channels == 4) {
                    std::memcpy(dst_ptr + i*layer_bytes,
                                img.data.data(), layer_bytes);
                } else {
                    const uint8_t* src = img.data.data();
                    uint8_t* dst_row = dst_ptr + i * layer_bytes;
                    for (uint32_t px = 0; px < layer_w * layer_h; ++px) {
                        dst_row[px * 4 + 0] = (img.channels > 0) ? src[px * img.channels + 0] : 0;
                        dst_row[px * 4 + 1] = (img.channels > 1) ? src[px * img.channels + 1] : 0;
                        dst_row[px * 4 + 2] = (img.channels > 2) ? src[px * img.channels + 2] : 0;
                        dst_row[px * 4 + 3] = 255;
                    }
                }
            }
            staging.unmap();
        }

        std::vector<VkBufferImageCopy> regions(layers);
        for (uint32_t i = 0; i < layers; ++i) {
            auto& r = regions[i];
            r.bufferOffset = layer_bytes * i;
            r.bufferRowLength = 0;
            r.bufferImageHeight = 0;
            r.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, i, 1 };
            r.imageOffset = { 0, 0, 0 };
            r.imageExtent = ext;
        }
 
        one_time_submit([&](VkCommandBuffer cmd) {
            img_barrier(cmd, dst.get_image(),
                VK_PIPELINE_STAGE_2_NONE,VK_ACCESS_2_NONE,VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            vkCmdCopyBufferToImage(cmd, staging.get(), dst.get_image(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>(regions.size()), regions.data());
            img_barrier(cmd, dst.get_image(),
                VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });
 
        dst.create_view(VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1, layers);
        dst.create_sampler(VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT);
    };
 
    build(base_tex_arr, scene.tex_manager.get_base());
    build(normal_tex_arr, scene.tex_manager.get_normal());
    build(specular_tex_arr, scene.tex_manager.get_specular());
}
 

bool VKRenderer::camera_moved() const {
    if (!prev_camera_valid){
        return false;
    }
    return camera.get_pos()!= prev_camera.pos||
           camera.get_forward() != prev_camera.forward ||
           camera.get_fov() != prev_camera.fov;
}
void VKRenderer::one_time_submit(const std::function<void(VkCommandBuffer)>& fn){
    VkCommandPoolCreateInfo pool_ci{};
    pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_ci.queueFamilyIndex = device->get_graphics_family();
 
    VkCommandPool tmp_pool = VK_NULL_HANDLE;
    vkCreateCommandPool(device->get_logic_device(), &pool_ci, nullptr, &tmp_pool);
 
    VkCommandBufferAllocateInfo alloc_ci{};
    alloc_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_ci.commandPool = tmp_pool;
    alloc_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_ci.commandBufferCount = 1;
 
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device->get_logic_device(), &alloc_ci, &cmd);
 
    VkCommandBufferBeginInfo begin_ci{};
    begin_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_ci.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_ci);
 
    fn(cmd);

    vkEndCommandBuffer(cmd); 
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    vkQueueSubmit(device->get_graphicsq(), 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->get_graphicsq());
    vkDestroyCommandPool(device->get_logic_device(), tmp_pool, nullptr);
}
PushConstants VKRenderer::make_push_constants() const {
    return PushConstants{ current_width,current_height,tric,spherec,bvhc,matc,lightc,framec,init_candidates_restir, };
}

void VKRenderer::buf_barrier(VkCommandBuffer cmd,VkBuffer buf,VkDeviceSize size,VkPipelineStageFlags2 src_stage,VkAccessFlags2 src_access,VkPipelineStageFlags2 dst_stage,VkAccessFlags2 dst_access){
    VkBufferMemoryBarrier2 b{};
    b.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    b.srcStageMask = src_stage;
    b.srcAccessMask = src_access;
    b.dstStageMask = dst_stage;
    b.dstAccessMask = dst_access;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.buffer = buf;
    b.offset= 0;
    b.size = size;
 
    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.bufferMemoryBarrierCount = 1;
    dep.pBufferMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &dep);

}
void VKRenderer::img_barrier(VkCommandBuffer cmd,VkImage img,VkPipelineStageFlags2 src_stage,VkAccessFlags2 src_access,VkPipelineStageFlags2 dst_stage, VkAccessFlags2 dst_access,VkImageLayout old_layout,VkImageLayout new_layout){
    VkImageMemoryBarrier2 b{};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    b.srcStageMask = src_stage;
    b.srcAccessMask = src_access;
    b.dstStageMask = dst_stage;
    b.dstAccessMask = dst_access;
    b.oldLayout = old_layout;
    b.newLayout = new_layout;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img;
    b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
 
    VkDependencyInfo dep{};
    dep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &b;
    vkCmdPipelineBarrier2(cmd, &dep);

}
void VKRenderer::on_window_resize(uint32_t w, uint32_t h){
    wait_idle();
 
    current_width = w;
    current_height = h;
    framebuffer_resized = false;
    framec = 0;
 
    create_restir_buffers();
    create_storage_images();
 
    descriptor_allocator->reset_pools();
 
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        descriptor_allocator->allocate(&camera_sets[i], camera_dsl);
 
        VkDescriptorBufferInfo bi{ camera_ubos[i].get(), 0, sizeof(CameraUBO) };
        VkWriteDescriptorSet w{};
        w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet= camera_sets[i];
        w.dstBinding = 0;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w.pBufferInfo= &bi;
        vkUpdateDescriptorSets(device->get_logic_device(), 1, &w, 0, nullptr);
    }

    update_scene_descriptor();
    update_pingpong_descriptors();
    update_output_descriptor();
    update_present_descriptor();

}
void VKRenderer::wait_idle(){
    vkDeviceWaitIdle(device->get_logic_device());
}
