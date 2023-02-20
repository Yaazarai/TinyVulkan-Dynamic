#define MINIVULKAN_SHORTREF
#define QOI_IMPLEMENTATION
#include "./MiniVK.hpp"
#include "./QuiteOkImageFormat.h"
using namespace mvk;

#define DEFAULT_VERTEX_SHADER "./Shader Files/sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./Shader Files/sample_frag.spv"

int MINIVULKAN_WINDOWMAIN {
    try {
        const std::vector<VkPhysicalDeviceType> renderDeviceTypes = { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU };
        const MiniVkBufferingMode bufferingMode = MiniVkBufferingMode::TRIPLE;
        const std::tuple<VkShaderStageFlagBits, std::string> vertexShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, DEFAULT_VERTEX_SHADER };
        const std::tuple<VkShaderStageFlagBits, std::string> fragmentShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, DEFAULT_FRAGMENT_SHADER };
        const MiniVkVertexDescription vertexDescription = MiniVkVertex::GetVertexDescription();
        const std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings = { MiniVkDynamicPipeline::SelectPushDescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT) };
        const std::vector<VkPushConstantRange>& pushConstantRanges = { MiniVkDynamicPipeline::SelectPushConstantRange(sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT) };

        MiniVkInstance instance(MiniVkWindow::QueryRequiredExtensions(MVK_VALIDATION_LAYERS), "MINIVK");
        MiniVkWindow window("MINIVK WINDOW", 1920, 1080, true, true);
        MiniVkRenderDevice renderDevice(instance, window.CreateWindowSurface(instance.instance), renderDeviceTypes);
        MiniVkSwapChain swapChain(renderDevice, MiniVkSurfaceSupporter(), bufferingMode);
        MiniVkCommandPool cmdPool(renderDevice, static_cast<size_t>(bufferingMode));
        MiniVkVMAllocator memAlloc(instance, renderDevice);
        MiniVkShaderStages shaders(renderDevice, { vertexShader, fragmentShader });
        MiniVkDynamicPipeline dyPipe(renderDevice, swapChain.swapChainImageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges);
        MiniVkDynamicRenderer dyRender(renderDevice, memAlloc, cmdPool, swapChain, dyPipe);

        window.onResizeFrameBuffer += MiniVkCallback<int, int>(&swapChain, &MiniVkSwapChain::OnFrameBufferResizeCallback);
        swapChain.onGetFrameBufferSize += MiniVkCallback<int&, int&>(&window, &MiniVkWindow::OnFrameBufferReSizeCallback);

        std::vector<MiniVkVertex> triangle {
            MiniVkVertex({0.0,0.0}, {480.0,270.0, 0.5}, {1.0,1.0,1.0,1.0}),
            MiniVkVertex({1.0,0.0}, {1440.0,270.0, 0.5}, {1.0,1.0,1.0,1.0}),
            MiniVkVertex({1.0,1.0}, {1440.0,810.0, 0.5}, {1.0,1.0,1.0,1.0}),
            MiniVkVertex({0.0,1.0}, {480.0,810.0, 0.5}, {1.0,1.0,1.0,1.0}),
            MiniVkVertex({0.0,0.0}, {480.0 - 128.0,270.0 - 128.0, 1.0}, {1.0,1.0,1.0,0.5}),
            MiniVkVertex({0.5,0.0}, {1440.0 - 128.0,270.0 - 128.0, 1.0}, {1.0,1.0,1.0,0.5}),
            MiniVkVertex({0.5,0.5}, {1440.0 - 128.0,810.0 - 128.0, 1.0}, {1.0,1.0,1.0,0.5}),
            MiniVkVertex({0.0,0.5}, {480.0 - 128.0,810.0 - 128.0, 1.0}, {1.0,1.0,1.0,0.5})
        };
        std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 };

        MiniVkBuffer vbuffer(renderDevice, memAlloc, triangle.size() * sizeof(MiniVkVertex), MiniVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
        vbuffer.StageBufferData(dyPipe.graphicsQueue, cmdPool.GetPool(), triangle.data(), triangle.size() * sizeof(MiniVkVertex), 0, 0);
        MiniVkBuffer ibuffer(renderDevice, memAlloc, indices.size() * sizeof(indices[0]), MiniVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
        ibuffer.StageBufferData(dyPipe.graphicsQueue, cmdPool.GetPool(), indices.data(), triangle.size() * sizeof(MiniVkVertex), 0, 0);

        FILE* test = fopen("./Screeny.qoi", "rb");
        if (test == NULL) { std::cout << "NO QOI IMAGE" << std::endl; } else { fclose(test); }

        qoi_desc qoidesc;
        void* qoiPixels = qoi_read("./Screeny.qoi", &qoidesc, 4);
        VkDeviceSize dataSize = qoidesc.width * qoidesc.height * qoidesc.channels;

        MiniVkImage image = MiniVkImage(renderDevice, memAlloc, qoidesc.width, qoidesc.height, false, VK_FORMAT_R8G8B8A8_SRGB);
        image.StageImageData(dyPipe.graphicsQueue, cmdPool.GetPool(), qoiPixels, dataSize);
        QOI_FREE(qoiPixels);

        dyRender.onRenderEvents += MiniVkCallback<VkCommandBuffer>([&instance, &image, &window, &vbuffer, &ibuffer, &memAlloc, &swapChain, &dyRender, &dyPipe](VkCommandBuffer commandBuffer) {
            VkClearValue clearColor{};
            VkClearValue depthStencil{};
            clearColor.color = { 0.0, 0.0, 0.0, 1.0 };
            depthStencil.depthStencil = { 1.0f, 0 };

            dyRender.BeginRecordCommandBufferSwapChain(commandBuffer, clearColor, depthStencil);
            vkCmdSetColorBlendEnableEKHR(instance.instance, commandBuffer, 0U, 1U, { (VkBool32)1U });

            glm::mat4 projection = MvkMath::Project2D(window.GetWidth(), window.GetHeight(), 1.0, 0.0);
            VkDescriptorImageInfo imageInfo = image.GetImageDescriptor();
            VkWriteDescriptorSet writeDescriptorSets = MiniVkDynamicPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo);
            vkCmdPushDescriptorSetEKHR(instance.instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dyPipe.pipelineLayout, 0, 1, &writeDescriptorSets);
            vkCmdPushConstants(commandBuffer, dyPipe.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &projection);

            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbuffer.buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t) / 2, 1, 0, 0, 0);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t) / 2, 1, 0, 4, 0);

            dyRender.EndRecordCommandBufferSwapChain(commandBuffer, clearColor, depthStencil);
        });

        std::thread mythread([&window, &dyRender]() { while (!window.ShouldClose()) { dyRender.RenderFrame(); } });
        while (!window.ShouldClosePollEvents()) {}
        mythread.join();

        renderDevice.WaitIdle();
        MiniVkObject::DisposeOrdered({ &instance, &window, &renderDevice, &swapChain, &memAlloc, &cmdPool, &shaders, &dyPipe, &dyRender, &vbuffer, &ibuffer, &image }, false);
    } catch (std::runtime_error err) { std::cout << err.what() << std::endl; }
    return VK_SUCCESS;
}