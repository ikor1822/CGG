#include "application.hpp"

#include <imgui.h>
#include <vulkan/vulkan.h>

#include <cmath>
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
float camera_x = 0.0f, camera_y = 0.0f, camera_z = 3.0f;
float camera_yaw = 0.0f, camera_pitch = 0.0f, fov = 1.0f;
float light_x = -0.5f, light_y = -1.0f, light_z = -0.7f;
float light_r = 1.0f, light_g = 1.0f, light_b = 1.0f;
float light_intensity = 0.9f;
float ambient = 0.22f;
float albedo_r = 0.1f, albedo_g = 0.7f, albedo_b = 0.9f;
float spec_r = 1.0f, spec_g = 1.0f, spec_b = 1.0f;
float shininess = 32.0f;

struct PushConstants {
    float camera_pos[3];
    float camera_yaw;
    float camera_pitch;
    float fov;

    float light_dir[3];
    float light_color[3];
    float light_intensity;
    float ambient;

    float albedo[3];
    float specular[3];
    float shininess;
    float rotation_y;
    float aspect;
};

std::vector<char> readFile(const char* filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed open file: " << filename << '\n';
        return {};
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(file_size));

    return buffer;
}

bool createShaderModule(
    const std::vector<char>& code,
    VkShaderModule& shader_module)
{
    if (code.empty()) {
        return false;
    }

    VkShaderModuleCreateInfo create_info{};
    create_info.sType =
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode =
        reinterpret_cast<const uint32_t*>(code.data());

    VkResult result = vkCreateShaderModule(
        graphics::internal::context.device,
        &create_info,
        nullptr,
        &shader_module);

    if (result != VK_SUCCESS) {
        std::cerr << "Failed shader module: " << result << '\n';
        return false;
    }

    return true;
}

bool createPipeline()
{
    auto& context = graphics::internal::context;

    auto vert_code = readFile("shaders/cone.vert.spv");
    auto frag_code = readFile("shaders/cone.frag.spv");

    if (!createShaderModule(vert_code, vertex_shader)) {
        return false;
    }

    if (!createShaderModule(frag_code, fragment_shader)) {
        return false;
    }

    VkPipelineShaderStageCreateInfo shader_stages[2]{};

    shader_stages[0].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";

    shader_stages[1].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology =
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.rasterizerDiscardEnable = VK_FALSE;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    VkPushConstantRange push_range{};
    push_range.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT |
        VK_SHADER_STAGE_FRAGMENT_BIT;
    push_range.offset = 0;
    push_range.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_range;

    if (vkCreatePipelineLayout(
            context.device,
            &pipeline_layout_info,
            nullptr,
            &pipeline_layout) != VK_SUCCESS) {
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType =
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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

    VkResult result = vkCreateGraphicsPipelines(
        context.device,
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        nullptr,
        &graphics_pipeline);

    if (result != VK_SUCCESS) {
        std::cerr << "Failed graphics pipeline: "
                  << result << '\n';
        return false;
    }

    return true;
}

}

bool initialize()
{
    std::cout << "Initializing application\n";

    if (!createPipeline()) {
        return false;
    }

    std::cout << "Application initialized successfully\n";
    return true;
}

void shutdown()
{
    auto& context = graphics::internal::context;

    vkDeviceWaitIdle(context.device);

    if (graphics_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(
            context.device, graphics_pipeline, nullptr);
        graphics_pipeline = VK_NULL_HANDLE;
    }

    if (pipeline_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(
            context.device, pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
    }

    if (vertex_shader != VK_NULL_HANDLE) {
        vkDestroyShaderModule(
            context.device, vertex_shader, nullptr);
        vertex_shader = VK_NULL_HANDLE;
    }

    if (fragment_shader != VK_NULL_HANDLE) {
        vkDestroyShaderModule(
            context.device, fragment_shader, nullptr);
        fragment_shader = VK_NULL_HANDLE;
    }
}

void update([[maybe_unused]] double time)
{
    ImGuiIO& io = ImGui::GetIO();

    if (!io.WantCaptureKeyboard) {
        float dt = io.DeltaTime;
        float move_speed = 2.5f * dt;
        float turn_speed = 1.5f * dt;

        float forward_x = -std::sin(camera_yaw);
        float forward_z = -std::cos(camera_yaw);
        float right_x = std::cos(camera_yaw);
        float right_z = -std::sin(camera_yaw);

        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            camera_x += forward_x * move_speed;
            camera_z += forward_z * move_speed;
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            camera_x -= forward_x * move_speed;
            camera_z -= forward_z * move_speed;
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            camera_x += right_x * move_speed;
            camera_z += right_z * move_speed;
        }
        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            camera_x -= right_x * move_speed;
            camera_z -= right_z * move_speed;
        }
        if (ImGui::IsKeyDown(ImGuiKey_Space) || ImGui::IsKeyDown(ImGuiKey_E)) {
            camera_y += move_speed;
        }
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_Q)) {
            camera_y -= move_speed;
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
            camera_yaw -= turn_speed;
        }
        if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
            camera_yaw += turn_speed;
        }
        if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
            camera_pitch -= turn_speed;
            if (camera_pitch < -1.5f) camera_pitch = -1.5f;
        }
        if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
            camera_pitch += turn_speed;
            if (camera_pitch > 1.5f) camera_pitch = 1.5f;
        }
    }

    ImGui::Begin("Cone");

    ImGui::Text("Camera and Blinn-Phong lighting");
    ImGui::Separator();

    ImGui::Text("Camera (WASD + Space/Shift + Arrows)");
    ImGui::SliderFloat("Camera X", &camera_x, -10.0f, 10.0f);
    ImGui::SliderFloat("Camera Y", &camera_y, -10.0f, 10.0f);
    ImGui::SliderFloat("Camera Z", &camera_z, -10.0f, 10.0f);
    ImGui::SliderFloat("Yaw", &camera_yaw, -3.14f, 3.14f);
    ImGui::SliderFloat("Pitch", &camera_pitch, -1.5f, 1.5f);
    ImGui::SliderFloat("Field of view", &fov, 0.3f, 2.2f);
    ImGui::SliderFloat(
        "Object rotation", &rotation_y, -3.14f, 3.14f);

    ImGui::Separator();
    ImGui::Text("Directional light");
    ImGui::SliderFloat("Light X", &light_x, -1.0f, 1.0f);
    ImGui::SliderFloat("Light Y", &light_y, -1.0f, 1.0f);
    ImGui::SliderFloat("Light Z", &light_z, -1.0f, 1.0f);
    ImGui::SliderFloat("Light red", &light_r, 0.0f, 1.0f);
    ImGui::SliderFloat("Light green", &light_g, 0.0f, 1.0f);
    ImGui::SliderFloat("Light blue", &light_b, 0.0f, 1.0f);
    ImGui::SliderFloat(
        "Light intensity", &light_intensity, 0.0f, 2.0f);
    ImGui::SliderFloat("Ambient light", &ambient, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Material (Blinn-Phong)");
    ImGui::SliderFloat("Albedo red", &albedo_r, 0.0f, 1.0f);
    ImGui::SliderFloat("Albedo green", &albedo_g, 0.0f, 1.0f);
    ImGui::SliderFloat("Albedo blue", &albedo_b, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular red", &spec_r, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular green", &spec_g, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular blue", &spec_b, 0.0f, 1.0f);
    ImGui::SliderFloat("Shininess", &shininess, 1.0f, 128.0f);

    ImGui::End();
}

void render(const graphics::internal::FrameData& fd)
{
    auto& context = graphics::internal::context;

    if (fd.command_buffer == VK_NULL_HANDLE) {
        return;
    }

    vkResetCommandBuffer(fd.command_buffer, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(
            fd.command_buffer, &begin_info) != VK_SUCCESS) {
        return;
    }

    VkClearValue clear_values[2]{};

    clear_values[0].color.float32[0] = 0.05f;
    clear_values[0].color.float32[1] = 0.05f;
    clear_values[0].color.float32[2] = 0.08f;
    clear_values[0].color.float32[3] = 1.0f;

    clear_values[1].depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo render_pass_begin{};
    render_pass_begin.sType =
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin.renderPass = context.render_pass;
    render_pass_begin.framebuffer = fd.framebuffer;
    render_pass_begin.renderArea.offset = {0, 0};
    render_pass_begin.renderArea.extent =
        context.swapchain_extent;
    render_pass_begin.clearValueCount = 2;
    render_pass_begin.pClearValues = clear_values;

    vkCmdBeginRenderPass(
        fd.command_buffer,
        &render_pass_begin,
        VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width =
        static_cast<float>(context.swapchain_extent.width);
    viewport.height =
        static_cast<float>(context.swapchain_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(fd.command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = context.swapchain_extent;

    vkCmdSetScissor(fd.command_buffer, 0, 1, &scissor);

    vkCmdBindPipeline(
        fd.command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphics_pipeline);

    PushConstants pc{};

    pc.camera_pos[0] = camera_x;
    pc.camera_pos[1] = camera_y;
    pc.camera_pos[2] = camera_z;
    pc.camera_yaw = camera_yaw;
    pc.camera_pitch = camera_pitch;
    pc.fov = fov;

    pc.light_dir[0] = light_x;
    pc.light_dir[1] = light_y;
    pc.light_dir[2] = light_z;

    pc.light_color[0] = light_r;
    pc.light_color[1] = light_g;
    pc.light_color[2] = light_b;

    pc.light_intensity = light_intensity;
    pc.ambient = ambient;

    pc.albedo[0] = albedo_r;
    pc.albedo[1] = albedo_g;
    pc.albedo[2] = albedo_b;

    pc.specular[0] = spec_r;
    pc.specular[1] = spec_g;
    pc.specular[2] = spec_b;

    pc.shininess = shininess;
    pc.rotation_y = rotation_y;
    pc.aspect = viewport.height > 0.0f ? (viewport.width / viewport.height) : 1.0f;

    vkCmdPushConstants(
        fd.command_buffer,
        pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT |
            VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstants),
        &pc);

    vkCmdDraw(fd.command_buffer, 300, 1, 0, 0);

    vkCmdEndRenderPass(fd.command_buffer);
    vkEndCommandBuffer(fd.command_buffer);
}

}