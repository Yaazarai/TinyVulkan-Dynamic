/*
    Vulkan requires the GLFW library linkage for native Window handling on Microsoft Windows.
    GLFW will only compile on both Release and Debug with the following options:

    C/C++ | Code Configuration | Runtime Libraries:
        RELEASE: /MD
        DEBUG  : /mtD

    Linker | Input | Additional Dependencies:
        RELEASE: vulkan-1.lib;glfw3.lib;
        DEBUG  : vulkan-1.lib;glfw3_mt.lib;

    C/C++ | General | Additional Include Directories:
        RELEASE: ...\VulkanSDK\1.3.211.0\Include;...\glfw-3.3.7.bin.WIN64\glfw-3.3.7.bin.WIN64\include;
        DEBUG  : ...\VulkanSDK\1.3.211.0\Include;...\glfw-3.3.7.bin.WIN64\glfw-3.3.7.bin.WIN64\include;

    Linker | General | Additional Library Directories:
        RELEASE: ...\glfw-3.3.7.bin.WIN64\lib-vc2022;$(VULKAN_SDK)\Lib;
        DEBUG  : ...\glfw-3.3.7.bin.WIN64\lib-vc2022;$(VULKAN_SDK)\Lib;
*/
#define MINIVULKAN_SHORTREF
#include "MiniVk.hpp"

#include <chrono>

// Returns the current time for benchmarking as type std::chrono::T.
template<typename T>
uint64_t now() {
    return std::chrono::duration_cast<T>
        (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#define DEFAULT_VERTEX_SHADER "./Shader Files/sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./Shader Files/sample_frag.spv"

int MINIVULKAN_MAIN{
    minivk::MiniVkWindow window(640, 360, true, "HELLO WORLD");;
    minivk::MiniVkInstance instance(window.GetRequiredExtensions(minivk::MiniVkInstance::enableValidationLayers), "HELLO TRIANGLE", {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU});
    instance.Initialize(window.CreateWindowSurface(instance.instance));

    minivk::MiniVkMemAlloc memoryAllocator(instance);

    minivk::MiniVkSurfaceSupportDetails presentDetails;
    minivk::MiniVkSwapChain swapChain(instance, presentDetails, minivk::MiniVkBufferingMode::TRIPLE);
    swapChain.onGetFrameBufferSize += std::callback<int&, int&>(&window, &minivk::MiniVkWindow::OnFrameBufferReSizeCallback);
    swapChain.onReCreateSwapChain += std::callback<int&, int&>(&window, &minivk::MiniVkWindow::OnFrameBufferReSizeCallback);
    window.onResizeFrameBuffer += std::callback<int, int>(&swapChain, &minivk::MiniVkSwapChain::OnFrameBufferResizeCallback);

    minivk::MiniVkCommandPool commandPool(instance, (size_t) swapChain.bufferingMode);

    minivk::MiniVkShaderStages shaderStages(instance,
        { "./Shader Files/sample_vert.spv", "./Shader Files/sample_frag.spv" },
        { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT });
    
    minivk::MiniVkDynamicPipeline dynamicPipeline(instance, swapChain.swapChainImageFormat, shaderStages,
        minivk::MiniVkVertex::GetBindingDescription(), minivk::MiniVkVertex::GetAttributeDescriptions(),
        {},// {minivk::MiniVkDynamicPipeline::SelectPushConstantRange(0, VK_SHADER_STAGE_ALL_GRAPHICS)},
        {},// {minivk::MiniVkDynamicPipeline::SelectDescriptorSetLayout(instance, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)},
        VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
    
    minivk::MiniVkDynamicRenderer dynamicRenderer(instance, commandPool, swapChain, dynamicPipeline);

    std::vector<minivk::MiniVkVertex> vbuff;
    vbuff.push_back(minivk::MiniVkVertex{ .pos = glm::vec2(-0.5, -0.5), .color = glm::vec3(0.0, .0, 1.0) });
    vbuff.push_back(minivk::MiniVkVertex{ .pos = glm::vec2(0.5, -0.5), .color = glm::vec3(.0, 1.0, .0) });
    vbuff.push_back(minivk::MiniVkVertex{ .pos = glm::vec2(0.5, 0.5), .color = glm::vec3(1.0, .0, 0.0) });
    vbuff.push_back(minivk::MiniVkVertex{ .pos = glm::vec2(-0.5, 0.5), .color = glm::vec3(0.0, 0.0, 0.0) });
    minivk::MiniVkVertexBuffer vbuffer(instance, vbuff);
    vbuffer.Stage(dynamicPipeline.graphicsQueue, commandPool.GetPool(), vbuff.data(), vbuffer.size);

    std::vector<uint32_t> ibuff = { 0, 1, 2, 2, 3, 0 };
    minivk::MiniVkIndexBuffer ibuffer(instance, vbuffer, ibuff);
    ibuffer.Stage(dynamicPipeline.graphicsQueue, commandPool.GetPool(), ibuff.data(), vbuffer.size);

    dynamicRenderer.onRenderEvents += std::callback<VkCommandBuffer>([&vbuffer, &ibuff, &ibuffer, &swapChain, &dynamicRenderer](VkCommandBuffer commandBuffer) {
        dynamicRenderer.BeginRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());\

        VkBuffer vertexBuffers[] = { vbuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuff.size()), 1, 0, 0, 0);

        dynamicRenderer.EndRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
    });
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    window.SetCallbackPointer(&instance);
    std::thread mythread([&window, &dynamicRenderer]() { while (!window.ShouldClose()) { dynamicRenderer.RenderFrame(); } });

    while (!window.ShouldClosePollEvents()) {
        //dynamicRenderer.RenderFrame();
    }
    mythread.join();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // FORCE DISPOSE ORDER: (Dispose is automatically called on destructor, but in our case order is explicitly senstive for Vulkan).
    ibuffer.Dispose();
    vbuffer.Dispose();
    swapChain.Dispose();
    dynamicRenderer.Dispose();
    dynamicPipeline.Dispose();
    shaderStages.Dispose();
    commandPool.Dispose();
    memoryAllocator.Dispose();
    instance.Dispose();
    window.Dispose();
    return GLFW_NO_ERROR;
}