/*
    Vulkan requires the GLFW library linkage for native Window handling on Microsoft Windows.
    GLFWV will only compile on both Release and Debug with the following options:

    C/C++ | Code Configuration | Runtime Libraries:
        RELEASE: /MD
        DEBUG  : /mtD

    Linker | Input | Additional Dependencies:
        RELEASE:vulkan-1.lib;glfw3.lib;
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

#define DEFAULT_VERTEX_SHADER "./Shader Files/sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./Shader Files/sample_frag.spv"

int MINIVULKAN_MAIN{
    /// ABOUT std::callback<...> and std::invokable<...>
    ///     invokable<...> takes in aone or multiple callback<...> with matching template parameters
    ///     and can call all callback members and their bound methods, instance methods or lambda methods.
    ///     This is similar to C# event system where you bind methods to C# events.
    /// 
    ///     This is used mainly to remove direct header/class dependencies between major parts of the system.
    minivk::MiniVkWindow window(640, 360, true, "HELLO WORLD");
    minivk::MiniVkInstance instance(std::callback<VkInstance&, VkSurfaceKHR&>(&window, &minivk::MiniVkWindow::CreateWindowSurface),
        window.GetRequiredExtensions(minivk::MiniVkInstance::enableValidationLayers), "HELLO WORLD", {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU});

    minivk::MiniVkSurfaceSupportDetails presentDetails;
    minivk::MiniVkSwapChain swapChain(instance, presentDetails, minivk::MiniVkBufferingMode::TRIPLE);
    swapChain.onGetFrameBufferSize += std::callback<int&, int&>(&window, &minivk::MiniVkWindow::GetFrameBufferSize);
    swapChain.onReCreateSwapChain += std::callback<>(&window, &minivk::MiniVkWindow::OnFrameBufferReSizeCallback);
    window.onResizeFrameBuffer += std::callback<>(&swapChain, &minivk::MiniVkSwapChain::OnWindowFrameBufferNotifyResizeCallback);

    minivk::MiniVkCommandPool commandPool(instance, (size_t) swapChain.bufferingMode);
    
    minivk::MiniVkShaderStages shaderStages(instance,
        { "./Shader Files/sample_vert.spv", "./Shader Files/sample_frag.spv" },
        { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT });
    
    minivk::MiniVkDynamicPipeline<minivk::MiniVkVertex, minivk::MiniVkUniform> dynamicPipeline(instance, shaderStages, swapChain.swapChainImageFormat,
        {},// {minivk::MiniVkDynamicPipeline<minivk::MiniVkVertex, minivk::MiniVkUniform>::SelectPushConstantRange(0, VK_SHADER_STAGE_ALL_GRAPHICS)},
        {},// {minivk::MiniVkDynamicPipeline<minivk::MiniVkVertex, minivk::MiniVkUniform>::SelectDescriptorSetLayout(instance, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)},
        VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
    minivk::MiniVkDynamicRenderer<minivk::MiniVkVertex, minivk::MiniVkUniform> dynamicRenderer(instance, commandPool, swapChain, dynamicPipeline);
    
    std::vector<minivk::MiniVkVertex> vbuff;
    vbuff.push_back(minivk::MiniVkVertex{ .pos = glm::vec2(-0.5, -0.5), .color = glm::vec3(0.0, .0, 1.0) });
    vbuff.push_back(minivk::MiniVkVertex{ .pos = glm::vec2(0.5, -0.5), .color = glm::vec3(.0, 1.0, .0) });
    vbuff.push_back(minivk::MiniVkVertex{ .pos = glm::vec2(0.5, 0.5), .color = glm::vec3(1.0, .0, 0.0) });
    vbuff.push_back(minivk::MiniVkVertex{ .pos = glm::vec2(-0.5, 0.5), .color = glm::vec3(0.0, 0.0, 0.0) });
    minivk::MiniVkVertexBuffer vbuffer(instance, vbuff);
    vbuffer.Stage(dynamicPipeline.graphicsQueue, commandPool.GetPool(), vbuff.data(), vbuffer.size);

    std::vector<uint32_t> ibuff = { 0, 1, 2, 2, 3, 0 };
    minivk::MiniVkIndexBuffer ibuffer(instance, vbuffer, ibuff );
    ibuffer.Stage(dynamicPipeline.graphicsQueue, commandPool.GetPool(), ibuff.data(), vbuffer.size);

    dynamicRenderer.onRenderEvents += std::callback<VkCommandBuffer>([&ibuff, &vbuff, &ibuffer, &vbuffer, &swapChain, &dynamicRenderer](VkCommandBuffer commandBuffer) {
        dynamicRenderer.BeginRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.swapChainImageViews[swapChain.currentFrame], swapChain.swapChainImages[swapChain.currentFrame], swapChain.swapChainExtent);

        VkBuffer vertexBuffers[] = { vbuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuff.size()), 1, 0, 0, 0);

        dynamicRenderer.EndRecordCommandBuffer(commandBuffer, swapChain.swapChainImages[swapChain.currentFrame]);
    });
    
    window.SetCallbackPointer(&instance);
    while (!window.ShouldClosePollEvents()) dynamicRenderer.RenderFrame();

    // FORCE DISPOSE ORDER: (Dispose is automatically called on destructor, but in our case order is explicitly senstive for Vulkan).
    ibuffer.Dispose();
    vbuffer.Dispose();
    swapChain.Dispose();
    dynamicRenderer.Dispose();
    dynamicPipeline.Dispose();
    shaderStages.Dispose();
    commandPool.Dispose();
    instance.Dispose();
    window.Dispose();
    return GLFW_NO_ERROR;
}