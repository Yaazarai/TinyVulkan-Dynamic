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

    int channels = 4;
    qoi_desc qoidesc;
    FILE* test = fopen("./Screeny.qoi", "rb");
    if (test == NULL) { std::cout << "NO QOI IMAGE" << std::endl; } else { fclose(test); }
    void* qoiPixels = qoi_read("./Screeny.qoi", &qoidesc, channels);

    std::cout << "QOI WIDTH: " << qoidesc.width << std::endl;
    std::cout << "QOI HEIGHT: " << qoidesc.height << std::endl;
    std::cout << "QOI CHANNELS: " << (uint32_t)qoidesc.channels << std::endl;
    
    VkDeviceSize dataSize = qoidesc.width * qoidesc.height * qoidesc.channels;
    std::cout << "QOI SIZE: " << (uint32_t)dataSize << std::endl;
    MiniVkMemoryStrct* image = memAlloc.CreateImage(qoidesc.width, qoidesc.height);
    memAlloc.StageImageData(dynamicPipeline.graphicsQueue, commandPool.GetPool(), image, qoiPixels, dataSize);
    QOI_FREE(qoiPixels);
    
    dynamicRenderer.onRenderEvents += std::callback<VkCommandBuffer>([&memAlloc, &swapChain, &dynamicRenderer](VkCommandBuffer commandBuffer) {
        dynamicRenderer.BeginRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D()); \
        
        
        
        dynamicRenderer.EndRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
    }); // onRenderEvents are render functions that get called within RenderFrame() before the swapChain image is presented to the screen.
    while (!window.ShouldClosePollEvents()) dynamicRenderer.RenderFrame();
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    /// ORDER-FORCED DISPOSE ORDER //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    memAlloc.DestroyMemory(image);

    mvkInstance.WaitIdleLogicalDevice();
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