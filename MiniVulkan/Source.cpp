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
    MiniVkInstance mvkInstance(MiniVkWindow::QueryRequiredExtensions(MVK_ENABLE_VALIDATION_LAYERS), "MINIVK", {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU});
    MiniVkWindow window(1920, 1080, true, "MINIVK WINDOW");
    mvkInstance.Initialize(window.CreateWindowSurface(mvkInstance.instance));

    MiniVkSwapChain swapChain(mvkInstance, MiniVkSurfaceSupportDetails(), MiniVkBufferingMode::TRIPLE);
    MiniVkMemAlloc memAlloc(mvkInstance);
    MiniVkCommandPool cmdPool(mvkInstance, (size_t)swapChain.bufferingMode);
    MiniVkShaderStages shaders(mvkInstance, { DEFAULT_VERTEX_SHADER, DEFAULT_FRAGMENT_SHADER }, { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT });
    
    MiniVkDynamicPipeline dyPipe(mvkInstance, swapChain.swapChainImageFormat, shaders, MiniVkVertex::GetBindingDescription(), MiniVkVertex::GetAttributeDescriptions(),
    { MiniVkDynamicPipeline::SelectPushDescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT)},
    { MiniVkDynamicPipeline::SelectPushConstantRange(sizeof(glm::mat4),VK_SHADER_STAGE_VERTEX_BIT) });
    MiniVkDynamicRenderer dyRender(mvkInstance, memAlloc, cmdPool, swapChain, dyPipe);

    window.onResizeFrameBuffer += std::callback<int, int>(&swapChain, &MiniVkSwapChain::OnFrameBufferResizeCallback);
    swapChain.onGetFrameBufferSize += std::callback<int&, int&>(&window, &MiniVkWindow::OnFrameBufferReSizeCallback);
    swapChain.onReCreateSwapChain += std::callback<int&, int&>(&window, &MiniVkWindow::OnFrameBufferReSizeCallback);

    std::vector<MiniVkVertex> triangle {
        {{0.0,0.0}, {480.0,270.0}, {0.0,0.0,0.0,0.5}},
        {{1.0,0.0}, {1440.0,270.0}, {0.0,0.0,0.0,0.5}},
        {{1.0,1.0}, {1440.0,810.0}, {0.0,0.0,0.0,0.5}},
        {{0.0,1.0}, {480.0,810.0}, {0.0,0.0,0.0,0.5}}
    };
    std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

    MiniVkBuffer vbuffer(mvkInstance, memAlloc, triangle.size() * sizeof(MiniVkVertex), MiniVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
    vbuffer.StageBufferData(dyPipe.graphicsQueue, cmdPool.GetPool(), triangle.data(), triangle.size() * sizeof(MiniVkVertex), 0, 0);
    MiniVkBuffer ibuffer(mvkInstance, memAlloc, indices.size() * sizeof(indices[0]), MiniVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
    ibuffer.StageBufferData(dyPipe.graphicsQueue, cmdPool.GetPool(), indices.data(), triangle.size() * sizeof(MiniVkVertex), 0, 0);
    
    int channels = 4;
    qoi_desc qoidesc;
    FILE* test = fopen("./Screeny.qoi", "rb");
    if (test == NULL) { std::cout << "NO QOI IMAGE" << std::endl; } else { fclose(test); }
    void* qoiPixels = qoi_read("./Screeny.qoi", &qoidesc, channels);
    VkDeviceSize dataSize = qoidesc.width * qoidesc.height * qoidesc.channels;
    
    MiniVkImage image = MiniVkImage(mvkInstance, memAlloc, qoidesc.width, qoidesc.height, VK_FORMAT_R8G8B8A8_SRGB);
    image.StageImageData(dyPipe.graphicsQueue, cmdPool.GetPool(), qoiPixels, dataSize);
    QOI_FREE(qoiPixels);

    dyRender.onRenderEvents += std::callback<VkCommandBuffer>([&mvkInstance, &image, &window, &vbuffer, &ibuffer, &memAlloc, &swapChain, &dyRender, &dyPipe](VkCommandBuffer commandBuffer) {
        VkClearValue clearColor{};
        clearColor.color = { 0.0, 0.0, 0.0, 0.0 };

        dyRender.BeginRecordCommandBuffer(commandBuffer, clearColor, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
        
        VkBuffer vertexBuffers[] = { vbuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        glm::mat4 projection = MiniVkMath::Project2D(window.GetWidth(), window.GetHeight(), -1.0, 1.0);

        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = image.layout;
        imageInfo.imageView = image.imageView;
        imageInfo.sampler = image.imageSampler;
        
        VkWriteDescriptorSet writeDescriptorSets = MiniVkDynamicPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo);
        vkCmdPushDescriptorSetEKHR(mvkInstance.instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dyPipe.pipelineLayout, 0, 1, &writeDescriptorSets);
        vkCmdPushConstants(commandBuffer, dyPipe.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &projection);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);
        
        dyRender.EndRecordCommandBuffer(commandBuffer, clearColor, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
    });

    std::thread mythread([&mvkInstance, &window, &dyRender]() { while (!window.ShouldClose()) { dyRender.RenderFrame(); } });
    while (!window.ShouldClosePollEvents()) {}
    mythread.join();
    
    mvkInstance.WaitIdleLogicalDevice();
    vbuffer.Dispose();
    ibuffer.Dispose();
    
    image.Dispose();

    swapChain.Dispose();
    dyRender.Dispose();
    dyPipe.Dispose();
    shaders.Dispose();
    cmdPool.Dispose();
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