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
/*
#define MINIVULKAN_SHORTREF
#include "MiniVulkan.hpp"

void RunMain(void* minivk) {
    minivk::MiniVkLayer* vulkan = reinterpret_cast<minivk::MiniVkLayer*>(minivk);
    vulkan->DrawFrame();
}

//void RunRefresh(GLFWwindow* window) {}

void ExitMain(void* minivk) {
    minivk::MiniVkLayer* vulkan = reinterpret_cast<minivk::MiniVkLayer*>(minivk);
    vulkan->LogicalDeviceWaitIdle();
}

int MINIVULKAN_MAIN {
    /// Create the window and subscribe function handle events.
    minivk::Window window(640, 480, true, "Mini Vulkan");
    
    /// Create the render engine and subscribe the window's framebuffer resize callback when recreating the swap chain.
    minivk::MiniVkLayer Vulkan(&window, "Mini Vulkan",
        {"./Shader Files/sample_vert.spv", "./Shader Files/sample_frag.spv"});
    
    // These are required to notify the render & window engine for framebuffer size changes.
    //    When the frame is drawn is requests to resize the swap chain and in turn calling the window to check if the size changed.
    //    This check is done via an invokable event call to static call: &minivk::Window::OnFrameBufferReSizeCallback
    //
    //    If the size did change GLFW triggers an event call to static callback: Window::OnFrameBufferNotifyReSizeCallback
    //        ** (This callback is set in the Window constructor)
    //    
    //        ** Because GLFW is a C library you can't pass instance-methods to function callbacks. The way to get around this is
    //        to have GLFW call the Window library's C-style static callback which then calls an invokable event that handles
    //        calling the instance-callback to our render engine.
    //
    //    The "Window::OnFrameBufferNotifyReSizeCallback" calls an instance-method to our render engine: &minivk::MiniVkLayer::OnFrameBufferNotifyResizeCallback
    //        which in turn notifys the render engine that the window was resized and as such needs to recreate the swap chain.
    //
    //    TL;DR when you're limited to static callbacks invokable<> is an absolutely invaluable library to skirt asround that.
    Vulkan.onReCreateSwapChain += std::callback<void*, int, int>(&minivk::Window::OnFrameBufferReSizeCallback);
    window.onResize += std::callback<>(&Vulkan, &minivk::MiniVkLayer::OnFrameBufferNotifyResizeCallback);

    window.onRunMain += std::callback<void*>(&RunMain);
    window.onExitMain += std::callback<void*>(&ExitMain);
    return window.RunMain(&Vulkan);
}
//*/
//*
#define MINIVULKAN_SHORTREF
#include "MiniVk.hpp"

//    RENDERING PIPELINE:
//        Swap Chain: Handles Driver borrowed images to render to the screen.
//        Graphics Pipeline: Represents a set of shader stages for rendering.
//            You need a UNIQUE graphics pipeline for each set of shaders you render to.
//            You can render to EITHER custom VkImages (surfaces) or pull images from the SwapChain.
//            You do NOT need fences/semaphores for rendering to custom VkImages, just for the swapchain.
//        Shader Stages: Represents a single Vertex/Fragment shader pair that can be bound to a graphics pipeline.
//        Command Pool: List of Buffers that hold render commands for the graphics pipeline.
//            Each Graphics pipeline should have its own command pool.

#define DEFAULT_VERTEX_SHADER "./Shader Files/sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./Shader Files/sample_frag.spv"

int MINIVULKAN_MAIN {
    minivk::MiniVkWindow window(640, 360, true, "HELLO WORLD");
    minivk::MiniVkInstance instance(&window, "HELLO WORLD", { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU } );

    minivk::MiniVkSurfaceSupportDetails presentDetails;
    minivk::MiniVkSwapChain swapChain(instance, presentDetails, minivk::MiniVkBufferingMode::TRIPLE);
    swapChain.onReCreateSwapChain += std::callback<void*, int, int>(&minivk::MiniVkWindow::OnFrameBufferReSizeCallback);
    window.onResize += std::callback<>(&swapChain, &minivk::MiniVkSwapChain::OnWindowFrameBufferNotifyResizeCallback);

    minivk::MiniVkCommandPool commandPool(instance, (size_t) swapChain.bufferingMode);
    minivk::MiniVkShaderStages shaderStages(instance,
        { "./Shader Files/sample_vert.spv", "./Shader Files/sample_frag.spv" },
        { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT });
    minivk::MiniVkDynamicPipeline<minivk::MiniVkVertex, minivk::MiniVkUniform> dynamicPipeline(instance, shaderStages, swapChain.swapChainImageFormat, 0, 15U, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
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

    // FORCE DISPOSE ORDER:
    ibuffer.Dispose();
    vbuffer.Dispose();
    swapChain.Dispose();
    dynamicRenderer.Dispose();
    dynamicPipeline.Dispose();
    shaderStages.Dispose();
    commandPool.Dispose();
    instance.Dispose();
    return GLFW_NO_ERROR;
}
//*/