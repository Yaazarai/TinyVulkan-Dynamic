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

struct MiniVkVertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(MiniVkVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(MiniVkVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(MiniVkVertex, color);
        return attributeDescriptions;
    }
};

struct MiniVkIndexer {
    std::vector<MiniVkVertex> vertices;
    std::vector<uint32_t> indices;
};

struct ProjectionUniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

int MINIVULKAN_MAIN {
    /// MINI VULKAN & WINDOW INITIALIZATION /////////////////////////////////////////////////////////////////////////////////////////////////////////
    MiniVkWindow window(640, 360, true, "MINIVKWINDOW");
    MiniVkInstance mvkInstance(window.GetRequiredExtensions(MiniVkInstance::enableValidationLayers), "MINIVK", {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU});
    mvkInstance.Initialize(window.CreateWindowSurface(mvkInstance.instance));

    /// SWAPCHAIN & WINDOW/SWAPCHAIN COMMUNICATION UPDAITNG /////////////////////////////////////////////////////////////////////////////////////////
    MiniVkSwapChain swapChain(mvkInstance, MiniVkSurfaceSupportDetails(), MiniVkBufferingMode::TRIPLE);
    swapChain.onGetFrameBufferSize += std::callback<int&, int&>(&window, &MiniVkWindow::OnFrameBufferReSizeCallback);
    swapChain.onReCreateSwapChain += std::callback<int&, int&>(&window, &MiniVkWindow::OnFrameBufferReSizeCallback);
    window.onResizeFrameBuffer += std::callback<int, int>(&swapChain, &MiniVkSwapChain::OnFrameBufferResizeCallback);

    //
    //  CORE MINIVK SETUP
    //      MiniVkMemAlloc -> Dynamic Memory Allocator (using VMA).
    //      MiniVkCommandPool -> For Getting Command Buffers for rendering.
    //      MiniVkShaderStages -> Shaders that render commands passthrough in the graphics pipeline.
    //      VkDescriptorSetLayout -> Defines the data layout of large info being sent to shaders.
    //      MiniVkDynamicPipeline -> Render Pipeline structure that defines the rendering format.
    //      MiniVkDynamicRenderer -> Performs the actual rendering commands and submits them to the swapchain (screen).
    //
    MiniVkMemAlloc memAlloc(mvkInstance);
    MiniVkCommandPool commandPool(mvkInstance, (size_t)swapChain.bufferingMode);
    MiniVkShaderStages shaderStages(mvkInstance, { "./Shader Files/sample_vert.spv", "./Shader Files/sample_frag.spv" }, { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT });
    
    //VkPushConstantRange fragmentColorSetLayout = MiniVkDynamicPipeline::SelectPushConstantRange(32, VK_SHADER_STAGE_FRAGMENT_BIT);
    MiniVkDynamicPipeline dynamicPipeline(mvkInstance, swapChain.swapChainImageFormat, shaderStages, MiniVkVertex::GetBindingDescription(), MiniVkVertex::GetAttributeDescriptions(), {  }, {  });
    MiniVkDynamicRenderer dynamicRenderer(mvkInstance, commandPool, swapChain, dynamicPipeline);
    
    /*
    std::vector<MiniVkBuffer> InFlightUniformBuffers;
    for(int i = 0; i < static_cast<int>(swapChain.bufferingMode); i++)
        InFlightUniformBuffers.push_back(MiniVkBuffer(mvkInstance, memAlloc, sizeof(ProjectionUniformBufferObject), MiniVkBufferType::VKVMA_BUFFER_TYPE_UNIFORM));
    */
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    dynamicRenderer.onRenderEvents += std::callback<VkCommandBuffer>([&memAlloc, &swapChain, &dynamicRenderer](VkCommandBuffer commandBuffer) {
        dynamicRenderer.BeginRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());


        
        dynamicRenderer.EndRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
    }); // onRenderEvents are render functions that get called within RenderFrame() before the swapChain image is presented to the screen.
    while (!window.ShouldClosePollEvents()) dynamicRenderer.RenderFrame();
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    /// ORDER-FORCED DISPOSE ORDER //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    mvkInstance.WaitIdleLogicalDevice();

    //for(auto &ubo : InFlightUniformBuffers) ubo.Dispose();
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

/*
*   QOI EXAMPLE USAGE:
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
    MiniVkImage image = MiniVkImage(mvkInstance, memAlloc, qoidesc.width, qoidesc.height);
    image.StageImageData(dynamicPipeline.graphicsQueue, commandPool.GetPool(), qoiPixels, dataSize);
    QOI_FREE(qoiPixels);

    image.Dispose();
*/