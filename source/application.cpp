#include "application.hpp"

#include <imgui.h>
#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <vector>

namespace application {

namespace {

VkShaderModule vertex_shader = VK_NULL_HANDLE;
VkShaderModule fragment_shader = VK_NULL_HANDLE;
VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
VkPipeline graphics_pipeline = VK_NULL_HANDLE;

float rotation_y = 0.55f;

struct PushConstants
{
    float rotation_y;
};

std::vector<char> readFile(const char* filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Failed open file: " << filename << '\n';
        return {};
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));

    return buffer;
}

bool createShaderModule(const std::vector<char>& code, VkShaderModule& shader_module)
{
    if (code.empty())
    {
        return false;
    }

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkResult result = vkCreateShaderModule(graphics::internal::context.device, &create_info, nullptr, &shader_module);

    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed shader module: " << result << '\n';
        return false;
    }

    return true;
}

bool createPipeline()
{
    auto& context = graphics::internal::context;

    std::vector<char> vert_code = readFile("shaders/cone.vert.spv");
    std::vector<char> frag_code = readFile("shaders/cone.frag.spv");

    if (!createShaderModule(vert_code, vertex_shader))
    {
        return false;
    }

    if (!createShaderModule(frag_code, fragment_shader))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo shader_stages[2]{};

    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";

    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.rasterizerDiscardEnable = VK_FALSE;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    VkPushConstantRange push_range{};
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_range;

    if (vkCreatePipelineLayout(context.device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
    {
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};

    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterization;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = context.render_pass;
    pipeline_info.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline);

    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed graphics pipeline: " << result << '\n';
        return false;
    }

    return true;
}

}

bool initialize()
{
    std::cout << "Initializing application\n";

    if (!createPipeline())
    {
        return false;
    }

    std::cout << "Application initialized successfully\n";

    return true;
}

void shutdown()
{
    auto& context = graphics::internal::context;

    vkDeviceWaitIdle(context.device);

    if (graphics_pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(context.device, graphics_pipeline, nullptr);
        graphics_pipeline = VK_NULL_HANDLE;
    }

    if (pipeline_layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(context.device, pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
    }

    if (vertex_shader != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(context.device, vertex_shader, nullptr);
        vertex_shader = VK_NULL_HANDLE;
    }

    if (fragment_shader != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(context.device, fragment_shader, nullptr);
        fragment_shader = VK_NULL_HANDLE;
    }
}

void update([[maybe_unused]] double time)
{
    ImGui::Begin("Cone");

    ImGui::Text("menu");
    ImGui::Separator();

    ImGui::SliderFloat("rotation", &rotation_y, -3.14f, 3.14f);

    ImGui::End();
}

void render(const graphics::internal::FrameData& fd)
{
    auto& context = graphics::internal::context;

    if (fd.command_buffer == VK_NULL_HANDLE)
    {
        return;
    }

    vkResetCommandBuffer(fd.command_buffer, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(fd.command_buffer, &begin_info) != VK_SUCCESS)
    {
        return;
    }

    VkClearValue clear_values[2]{};

    clear_values[0].color.float32[0] = 0.05f;
    clear_values[0].color.float32[1] = 0.05f;
    clear_values[0].color.float32[2] = 0.08f;
    clear_values[0].color.float32[3] = 1.0f;

    clear_values[1].depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo render_pass_begin{};

    render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin.renderPass = context.render_pass;
    render_pass_begin.framebuffer = fd.framebuffer;
    render_pass_begin.renderArea.offset = { 0, 0 };
    render_pass_begin.renderArea.extent = context.swapchain_extent;
    render_pass_begin.clearValueCount = 2;
    render_pass_begin.pClearValues = clear_values;

    vkCmdBeginRenderPass(fd.command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(context.swapchain_extent.width);
    viewport.height = static_cast<float>(context.swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(fd.command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};

    scissor.offset = { 0, 0 };
    scissor.extent = context.swapchain_extent;

    vkCmdSetScissor(fd.command_buffer, 0, 1, &scissor);

    vkCmdBindPipeline(fd.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    PushConstants push_constants{};
    push_constants.rotation_y = rotation_y;

    vkCmdPushConstants(fd.command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push_constants);

    vkCmdDraw(fd.command_buffer, 300, 1, 0, 0);

    vkCmdEndRenderPass(fd.command_buffer);

    vkEndCommandBuffer(fd.command_buffer);
}

} // namespace application