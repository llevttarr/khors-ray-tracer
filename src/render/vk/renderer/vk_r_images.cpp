#include "vk_renderer.h"
 
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
            img_barrier(cmd, img,VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, VK_ACCESS_2_SHADER_WRITE_BIT,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        }
    });
}

void VKRenderer::create_texture_arrays(RenderScene& scene) {

    auto build = [&](VKTexture& dst, const std::vector<Image>& img_v,VkFormat fmt) {
        dst.destroy();

        if (img_v.empty()) {
            const VkExtent3D ext1{ 1, 1, 1 };
            dst.create_image(ext1, fmt,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_TILING_OPTIMAL, 1, 1);

            VKBuffer staging(device);
            staging.create(4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT);
            const uint32_t black = 0x00000000;
            staging.write(&black, 4);

            one_time_submit([&](VkCommandBuffer cmd) {
                img_barrier(cmd, dst.get_image(),
                    VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
                    VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                VkBufferImageCopy r{};
                r.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                r.imageExtent = { 1, 1, 1 };
                vkCmdCopyBufferToImage(cmd, staging.get(), dst.get_image(),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);

                img_barrier(cmd, dst.get_image(),
                    VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                    VK_ACCESS_2_SHADER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            });

            dst.create_view(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1, 1);
            dst.create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
            return;
        }

        uint32_t max_w = 0, max_h = 0;
        for (const auto& img : img_v) {
            max_w = std::max(max_w, static_cast<uint32_t>(img.w));
            max_h = std::max(max_h, static_cast<uint32_t>(img.h));
        }

        const uint32_t layers = static_cast<uint32_t>(img_v.size());
        constexpr uint32_t CHANNELS = 4;
        const VkDeviceSize layer_bytes = static_cast<VkDeviceSize>(max_w) * max_h * CHANNELS;
        const VkDeviceSize total_bytes = layer_bytes * layers;
        const VkExtent3D ext{ max_w, max_h, 1 };

        dst.create_image(ext, fmt,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY, VK_IMAGE_TILING_OPTIMAL, 1, layers);

        VKBuffer staging(device);
        staging.create(total_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT);

        {
            uint8_t* dst_ptr = static_cast<uint8_t*>(staging.map());

            std::memset(dst_ptr, 0, total_bytes);

            for (uint32_t i = 0; i < layers; ++i) {
                const Image& img = img_v[i];
                const uint32_t img_w = static_cast<uint32_t>(img.w);
                const uint32_t img_h = static_cast<uint32_t>(img.h);
                uint8_t* layer_dst = dst_ptr + i * layer_bytes;

                for (uint32_t row = 0; row < img_h; ++row) {
                    uint8_t* dst_row = layer_dst + row * max_w * CHANNELS;

                    if (img.channels == 4) {
                        const uint8_t* src_row =
                            img.data.data() + row * img_w * CHANNELS;
                        std::memcpy(dst_row, src_row, img_w * CHANNELS);
                    } else {
                        const uint8_t* src_row =
                            img.data.data() + row * img_w * img.channels;
                        for (uint32_t px = 0; px < img_w; ++px) {
                            dst_row[px*4+0] = img.channels > 0 ? src_row[px*img.channels+0] : 0;
                            dst_row[px*4+1] = img.channels > 1 ? src_row[px*img.channels+1] : 0;
                            dst_row[px*4+2] = img.channels > 2 ? src_row[px*img.channels+2] : 0;
                            dst_row[px*4+3] = 255;
                        }
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
                VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            vkCmdCopyBufferToImage(cmd, staging.get(), dst.get_image(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>(regions.size()), regions.data());

            img_barrier(cmd, dst.get_image(),
                VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
                VK_ACCESS_2_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

        dst.create_view(VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1, layers);
        dst.create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    };

    build(base_tex_arr, scene.tex_manager.get_base(), VK_FORMAT_R8G8B8A8_SRGB);
    build(normal_tex_arr, scene.tex_manager.get_normal(), VK_FORMAT_R8G8B8A8_UNORM);
    build(specular_tex_arr, scene.tex_manager.get_specular(), VK_FORMAT_R8G8B8A8_SRGB);
}
