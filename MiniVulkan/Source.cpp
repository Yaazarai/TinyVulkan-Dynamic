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
    glm::vec2 position;
    glm::vec4 color;

    MiniVkVertex(glm::vec2 pos, glm::vec4 col) : position(pos), color(col) {}

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription(1);
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
        attributeDescriptions[0].offset = offsetof(MiniVkVertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(MiniVkVertex, color);
        return attributeDescriptions;
    }
};

struct MiniVkIndexer {
    std::vector<MiniVkVertex> vertices;
    std::vector<uint32_t> indices;
};

int MINIVULKAN_MAIN {
    /// MINI VULKAN & WINDOW INITIALIZATION /////////////////////////////////////////////////////////////////////////////////////////////////////////
    MiniVkWindow window(1920, 1080, true, "MINIVK WINDOW", false);
    MiniVkInstance mvkInstance(window.GetRequiredExtensions(MiniVkInstance::enableValidationLayers), "MINIVK", {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU});
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
    //      MiniVkDynamicPipeline -> Render Pipeline structure that defines the rendering format.
    //      MiniVkDynamicRenderer -> Performs the actual rendering commands and submits them to the swapchain (screen).
    //
    MiniVkMemAlloc memAlloc(mvkInstance);
    MiniVkCommandPool commandPool(mvkInstance, (size_t)swapChain.bufferingMode);
    MiniVkShaderStages shaderStages(mvkInstance, { "./Shader Files/sample_vert.spv", "./Shader Files/sample_frag.spv" }, { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT });
    
    MiniVkDynamicPipeline dynamicPipeline(mvkInstance, swapChain.swapChainImageFormat, shaderStages, MiniVkVertex::GetBindingDescription(), MiniVkVertex::GetAttributeDescriptions(),
        {MiniVkDynamicPipeline::SelectPushConstantRange(sizeof(glm::mat4),VK_SHADER_STAGE_VERTEX_BIT)});
    MiniVkDynamicRenderer dynamicRenderer(mvkInstance, commandPool, swapChain, dynamicPipeline);
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<MiniVkVertex> triangle {
        {{480.0,270.0}, {1.0,0.0,0.0,0.0}},
        {{1440.0,270.0}, {0.0,1.0,0.0,1.0}},
        {{1440.0,810.0}, {0.0,0.0,1.0,0.0}},
        {{480.0,810.0}, {0.0,1.0,1.0,1.0}}
    };
    std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

    MiniVkBuffer vbuffer(mvkInstance, memAlloc, triangle.size() * sizeof(MiniVkVertex), MiniVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
    vbuffer.StageBufferData(dynamicPipeline.graphicsQueue, commandPool.GetPool(), triangle.data(), triangle.size() * sizeof(MiniVkVertex), 0, 0);
    MiniVkBuffer ibuffer(mvkInstance, memAlloc, indices.size() * sizeof(indices[0]), MiniVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
    ibuffer.StageBufferData(dynamicPipeline.graphicsQueue, commandPool.GetPool(), indices.data(), triangle.size() * sizeof(MiniVkVertex), 0, 0);
    
    dynamicRenderer.onRenderEvents += std::callback<VkCommandBuffer>([&vbuffer, &ibuffer, &memAlloc, &swapChain, &dynamicRenderer, &dynamicPipeline](VkCommandBuffer commandBuffer) {
        dynamicRenderer.BeginRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());

        VkBuffer vertexBuffers[] = { vbuffer.buffer };
        VkDeviceSize offsets[] = { 0 };

        glm::mat4 projection = MiniVkMath::Project2D(1920.0, 1080.0, -1.0, 1.0);
        vkCmdPushConstants(commandBuffer, dynamicPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &projection);
        
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuffer.size)/sizeof(uint32_t), 1, 0, 0, 0);
        
        dynamicRenderer.EndRecordCommandBuffer(commandBuffer, { 0.0, 0.0, 0.0, 1.0 }, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
    }); // onRenderEvents are render functions that get called within RenderFrame() before the swapChain image is presented to the screen.
    
    window.SetCallbackPointer(&mvkInstance);
    std::thread mythread([&mvkInstance, &window, &dynamicRenderer]() { while (!window.ShouldClose()) { dynamicRenderer.RenderFrame(); } });
    while (!window.ShouldClosePollEvents()) {}
    mythread.join();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    /// ORDER-FORCED DISPOSE ORDER //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    mvkInstance.WaitIdleLogicalDevice();
    vbuffer.Dispose();
    ibuffer.Dispose();
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