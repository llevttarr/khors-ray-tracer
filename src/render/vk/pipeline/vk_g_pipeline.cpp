#include "vk_g_pipeline.h"
VKGraphicsPipelineBuilder& VKGraphicsPipelineBuilder::set_shaders(const std::string& vs_path, const std::string& fs_path) {
    VkDevice d = device->get_logic_device();

    VkShaderModule vs = VKPipeline::load_shader(d, vs_path);
    VkShaderModule fs = VKPipeline::load_shader(d, fs_path);

    shader_stages.clear();

    VkPipelineShaderStageCreateInfo vs_stage{};
    vs_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vs_stage.module = vs;
    vs_stage.pName = "main";

    VkPipelineShaderStageCreateInfo fs_stage{};
    fs_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs_stage.stage= VK_SHADER_STAGE_FRAGMENT_BIT;
    fs_stage.module = fs;
    fs_stage.pName  = "main";

    shader_stages.push_back(vs_stage);
    shader_stages.push_back(fs_stage);

    return *this;
}
VKGraphicsPipelineBuilder& VKGraphicsPipelineBuilder::set_topology(VkPrimitiveTopology topology) {
    input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = topology;
    return *this;
}
VKGraphicsPipelineBuilder& VKGraphicsPipelineBuilder::set_polygon_mode(VkPolygonMode mode) {
    rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = mode;
    rasterizer.lineWidth = 1.0f;
    return *this;
}

VKGraphicsPipelineBuilder& VKGraphicsPipelineBuilder::set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = frontFace;
    return *this;
}
VKGraphicsPipelineBuilder& VKGraphicsPipelineBuilder::disable_depth_test() {
    depth_stencil = {};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_FALSE;
    depth_stencil.depthWriteEnable = VK_FALSE;
    return *this;
}

VKGraphicsPipelineBuilder& VKGraphicsPipelineBuilder::enable_depth_test(VkCompareOp compareOp) {
    depth_stencil = {};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = compareOp;
    return *this;
}
VKGraphicsPipelineBuilder& VKGraphicsPipelineBuilder::set_render_formats(VkFormat color, VkFormat depth) {
    color_format = color;
    depth_format = depth;

    color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = 0xF;
    color_blend_attachment.blendEnable = VK_FALSE;

    return *this;
}

std::unique_ptr<VKGraphicsPipeline> VKGraphicsPipelineBuilder::build() {
    VkDevice d = device->get_logic_device();

    if (shader_stages.empty())
        throw std::runtime_error("graphics pipeline: shaders not set");

    if (color_format == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("graphics pipeline: render format not set");

    VkPipelineLayout layout = build_pipeline_layout();

    VkPipelineRenderingCreateInfo rendering{};
    rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &color_format;
    rendering.depthAttachmentFormat = depth_format;
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkDynamicState dyn_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dyn{};
    dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates = dyn_states;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &color_blend_attachment;

    VkGraphicsPipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = &rendering;
    info.stageCount = static_cast<uint32_t>(shader_stages.size());
    info.pStages = shader_stages.data();
    info.pVertexInputState = &vertex_input;
    info.pInputAssemblyState = &input_assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &rasterizer;
    info.pMultisampleState = &ms;
    info.pColorBlendState = &blend;
    info.pDepthStencilState = &depth_stencil;
    info.pDynamicState = &dyn;
    info.layout = layout;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(d, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    for (auto& s : shader_stages) {
        vkDestroyShaderModule(d, s.module, nullptr);
    }

    shader_stages.clear();

    return std::unique_ptr<VKGraphicsPipeline>(
        new VKGraphicsPipeline(device, pipeline, layout)
    );
}