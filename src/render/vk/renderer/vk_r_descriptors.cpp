#include "vk_renderer.h"

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
        // binding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, COMPUTE_RT), // refl_accum 
    }, output_dsl);

    // refl output
    create_layout({
        binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, COMPUTE_RT), // refl_accum
    }, refl_output_dsl);

    // postp
    create_layout({
        binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT), // cbuff
        binding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT), // accum
        binding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT), // refl_accum
        binding(3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT), // bloom_tex_a
        binding(4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT), // bloom_tex_b
    }, postp_dsl);
 
    create_layout({
        binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
    }, present_dsl);
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
    // VkDescriptorImageInfo ii_refl_accum{ VK_NULL_HANDLE, refl_accum_tex.get_view(), VK_IMAGE_LAYOUT_GENERAL };
 
    std::array<VkWriteDescriptorSet, 2> writes = {
        simg(output_set, 0, &ii_cbuff),
        simg(output_set, 1, &ii_accum),
        // simg(2, &ii_refl_accum),
    };
    vkUpdateDescriptorSets(vkdev,
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
 
void VKRenderer::update_postp_descriptor() {
    VkDevice vkdev = device->get_logic_device();
    descriptor_allocator->allocate(&postp_set, postp_dsl);
 
    VkDescriptorImageInfo ii_cbuff{ VK_NULL_HANDLE, cbuff_tex.get_view(), VK_IMAGE_LAYOUT_GENERAL };
    VkDescriptorImageInfo ii_accum { VK_NULL_HANDLE, accum_tex.get_view(), VK_IMAGE_LAYOUT_GENERAL };
    VkDescriptorImageInfo ii_refl_accum{ VK_NULL_HANDLE, refl_accum_tex.get_view(), VK_IMAGE_LAYOUT_GENERAL };
    VkDescriptorImageInfo ii_bloom_a{ VK_NULL_HANDLE, bloom_tex_a.get_view(), VK_IMAGE_LAYOUT_GENERAL };
    VkDescriptorImageInfo ii_bloom_b{ VK_NULL_HANDLE, bloom_tex_b.get_view(), VK_IMAGE_LAYOUT_GENERAL };
 
    std::array<VkWriteDescriptorSet, 5> writes = {
        simg(postp_set, 0, &ii_cbuff),
        simg(postp_set, 1, &ii_accum),
        simg(postp_set, 2, &ii_refl_accum),
        simg(postp_set, 3, &ii_bloom_a),
        simg(postp_set, 4, &ii_bloom_b),
    };
    vkUpdateDescriptorSets(vkdev,
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
void VKRenderer::update_refl_output_descriptor(){
    VkDevice vkdev = device->get_logic_device();
    descriptor_allocator->allocate(&refl_output_set, refl_output_dsl);
 
    VkDescriptorImageInfo ii_refl_accum{ VK_NULL_HANDLE, refl_accum_tex.get_view(), VK_IMAGE_LAYOUT_GENERAL };
 
    std::array<VkWriteDescriptorSet, 1> writes = {
        simg(refl_output_set, 0, &ii_refl_accum),
    };
    vkUpdateDescriptorSets(vkdev,
        static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
VkWriteDescriptorSet VKRenderer::simg(VkDescriptorSet set, uint32_t b, VkDescriptorImageInfo* info){
    VkWriteDescriptorSet w{};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet = set;
    w.dstBinding = b;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w.pImageInfo = info;
    return w;
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
