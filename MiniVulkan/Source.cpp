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
#define MINIVULKAN_SHORTERREF //mvk
#include "MiniVk.hpp"
using namespace mvk;

/// SHADER FILE REFERENCE PATHS
#define DEFAULT_VERTEX_SHADER "./Shader Files/sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./Shader Files/sample_frag.spv"

int MINIVULKAN_MAIN {
    /// MINI VULKAN & WINDOW INITIALIZATION /////////////////////////////////////////////////////////////////////////////////////////////////////////
    MiniVkWindow window(640, 360, true, "MINIVKWINDOW");
    MiniVkInstance mvkInstance(window.GetRequiredExtensions(MiniVkInstance::enableValidationLayers), "MINIVK", {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU});
    mvkInstance.Initialize(window.CreateWindowSurface(mvkInstance.instance));

    /// VULKAN MEMORY ALLOCATOR FOR BUFFER/IMAGE CREATION (OPTIONAL, RECOMMENDED) ///////////////////////////////////////////////////////////////////
    MiniVkMemAlloc memAlloc(mvkInstance);

    /// SWAPCHAIN & WINDOW/SWAPCHAIN COMMUNICATION UPDAITNG /////////////////////////////////////////////////////////////////////////////////////////
    MiniVkSwapChain swapChain(mvkInstance, MiniVkSurfaceSupportDetails(), MiniVkBufferingMode::TRIPLE);
    swapChain.onGetFrameBufferSize += std::callback<int&, int&>(&window, &MiniVkWindow::OnFrameBufferReSizeCallback);
    swapChain.onReCreateSwapChain += std::callback<int&, int&>(&window, &MiniVkWindow::OnFrameBufferReSizeCallback);
    window.onResizeFrameBuffer += std::callback<int, int>(&swapChain, &MiniVkSwapChain::OnFrameBufferResizeCallback);

    /// COMMAND POOL & COMMAND BUFFER INITIALIZATION FOR RENDERING //////////////////////////////////////////////////////////////////////////////////
    MiniVkCommandPool commandPool(mvkInstance, (size_t)swapChain.bufferingMode);

    /// SHADER SETUP ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MiniVkShaderStages shaderStages(mvkInstance,
        { "./Shader Files/sample_vert.spv", "./Shader Files/sample_frag.spv" },
        { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT });
    
    /// GRAPHICS PIPELINE SETUP (without push constant & uniform buffer input) //////////////////////////////////////////////////////////////////////
    MiniVkDynamicPipeline dynamicPipeline(mvkInstance, swapChain.swapChainImageFormat, shaderStages,
        MiniVkVertex::GetBindingDescription(), MiniVkVertex::GetAttributeDescriptions(), {}, {}, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
    
    /// GRAPHICS RENDERER ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MiniVkDynamicRenderer dynamicRenderer(mvkInstance, commandPool, swapChain, dynamicPipeline);
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<MiniVkVertex> vbuff;
    vbuff.push_back(MiniVkVertex{ .pos = glm::vec2(-0.5, -0.5), .color = glm::vec3(0.0, .0, 1.0) });
    vbuff.push_back(MiniVkVertex{ .pos = glm::vec2(0.5, -0.5), .color = glm::vec3(.0, 1.0, .0) });
    vbuff.push_back(MiniVkVertex{ .pos = glm::vec2(0.5, 0.5), .color = glm::vec3(1.0, .0, 0.0) });
    vbuff.push_back(MiniVkVertex{ .pos = glm::vec2(-0.5, 0.5), .color = glm::vec3(0.0, 0.0, 0.0) });
    std::vector<uint32_t> ibuff = { 0, 1, 2, 2, 3, 0 };

    MiniVkMemoryStrct* vbuffer = memAlloc.CreateVertexBuffer(vbuff);
    memAlloc.StageMemoryData(dynamicPipeline.graphicsQueue, commandPool.GetPool(), vbuffer, vbuff.data(), vbuffer->size, 0, 0);
    MiniVkMemoryStrct* ibuffer = memAlloc.CreateIndexBuffer(ibuff);
    memAlloc.StageMemoryData(dynamicPipeline.graphicsQueue, commandPool.GetPool(), ibuffer, ibuff.data(), ibuffer->size, 0, 0);

    dynamicRenderer.onRenderEvents += std::callback<VkCommandBuffer>([&vbuff, &ibuff, &vbuffer, &ibuffer, &memAlloc, &swapChain, &dynamicRenderer](VkCommandBuffer commandBuffer) {
        dynamicRenderer.BeginRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D()); \
        
        VkBuffer vertexBuffers[] = { vbuffer->buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, ibuffer->buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuff.size()), 1, 0, 0, 0);
        
        dynamicRenderer.EndRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
    }); // onRenderEvents are render functions that get called within RenderFrame() before the swapChain image is presented to the screen.
    while (!window.ShouldClosePollEvents()) dynamicRenderer.RenderFrame();
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    /// ORDER-FORCED DISPOSE ORDER //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    vkDeviceWaitIdle(mvkInstance.logicalDevice);
    memAlloc.DestroyMemory(ibuffer);
    memAlloc.DestroyMemory(vbuffer);
    swapChain.Dispose();
    dynamicRenderer.Dispose();
    dynamicPipeline.Dispose();
    shaderStages.Dispose();
    commandPool.Dispose();
    memAlloc.Dispose();
    mvkInstance.Dispose();
    window.Dispose();
    return VK_SUCCESS;
}