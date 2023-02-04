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
#include "MiniVulkan.hpp"
using namespace mvk;

/// SHADER FILE REFERENCE PATHS
#define DEFAULT_VERTEX_SHADER "./Shader Files/sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./Shader Files/sample_frag.spv"

int MINIVULKAN_MAIN {
    MvkInstance mvkInstance(MvkWindow::QueryRequiredExtensions(MVK_ENABLE_VALIDATION), "MINIVK", {VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU});
    MvkWindow window(1920, 1080, true, "MINIVK WINDOW");
    mvkInstance.Initialize(window.CreateWindowSurface(mvkInstance.instance));

    MvkSwapChain swapChain(mvkInstance, MvkSurfaceSupporter(), MvkBufferingMode::TRIPLE);
    window.onResizeFrameBuffer += std::callback<int, int>(&swapChain, &MvkSwapChain::OnFrameBufferResizeCallback);
    swapChain.onGetFrameBufferSize += std::callback<int&, int&>(&window, &MvkWindow::OnFrameBufferReSizeCallback);
    swapChain.onReCreateSwapChain += std::callback<int&, int&>(&window, &MvkWindow::OnFrameBufferReSizeCallback);
    
    MvkMemAlloc memAlloc(mvkInstance);
    MvkCommandPool cmdPool(mvkInstance, (size_t)swapChain.bufferingMode);
    MvkShaderStages shaders(mvkInstance, {
        {VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, DEFAULT_VERTEX_SHADER},
        {VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, DEFAULT_FRAGMENT_SHADER}
    });

    MvkDyPipeline dyPipe(mvkInstance, swapChain.swapChainImageFormat, shaders, MvkVertex::GetVertexDescription(),
        { MvkDyPipeline::SelectPushDescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT) },
        { MvkDyPipeline::SelectPushConstantRange(sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT) }
    );
    
    MvkDyRenderer dyRender(mvkInstance, memAlloc, cmdPool, swapChain, dyPipe);

    std::vector<MvkVertex> triangle {
        {{0.0,0.0}, {480.0,270.0, 0.5}, {1.0,1.0,1.0,1.0}},
        {{1.0,0.0}, {1440.0,270.0, 0.5}, {1.0,1.0,1.0,1.0}},
        {{1.0,1.0}, {1440.0,810.0, 0.5}, {1.0,1.0,1.0,1.0}},
        {{0.0,1.0}, {480.0,810.0, 0.5}, {1.0,1.0,1.0,1.0}},
        {{0.0,0.0}, {480.0 - 128.0,270.0 - 128.0, 1.0}, {1.0,1.0,1.0,0.1}},
        {{0.5,0.0}, {1440.0 - 128.0,270.0 - 128.0, 1.0}, {1.0,1.0,1.0,0.1}},
        {{0.5,0.5}, {1440.0 - 128.0,810.0 - 128.0, 1.0}, {1.0,1.0,1.0,0.1}},
        {{0.0,0.5}, {480.0 - 128.0,810.0 - 128.0, 1.0}, {1.0,1.0,1.0,0.1}}
    };
    std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 };

    MvkBuffer vbuffer(mvkInstance, memAlloc, triangle.size() * sizeof(MvkVertex), MvkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
    vbuffer.StageBufferData(dyPipe.graphicsQueue, cmdPool.GetPool(), triangle.data(), triangle.size() * sizeof(MvkVertex), 0, 0);
    MvkBuffer ibuffer(mvkInstance, memAlloc, indices.size() * sizeof(indices[0]), MvkBufferType::VKVMA_BUFFER_TYPE_INDEX);
    ibuffer.StageBufferData(dyPipe.graphicsQueue, cmdPool.GetPool(), indices.data(), triangle.size() * sizeof(MvkVertex), 0, 0);
    
    int channels = 4;
    qoi_desc qoidesc;
    
    FILE* test = fopen("./Screeny.qoi", "rb");
    if (test == NULL) { std::cout << "NO QOI IMAGE" << std::endl; } else { fclose(test); }
    void* qoiPixels = qoi_read("./Screeny.qoi", &qoidesc, channels);
    VkDeviceSize dataSize = qoidesc.width * qoidesc.height * qoidesc.channels;
    
    MvkImage image = MvkImage(mvkInstance, memAlloc, qoidesc.width, qoidesc.height, false, VK_FORMAT_R8G8B8A8_SRGB);
    image.StageImageData(dyPipe.graphicsQueue, cmdPool.GetPool(), qoiPixels, dataSize);
    QOI_FREE(qoiPixels);

    dyRender.onRenderEvents += std::callback<VkCommandBuffer>([&mvkInstance, &image, &window, &vbuffer, &ibuffer, &memAlloc, &swapChain, &dyRender, &dyPipe](VkCommandBuffer commandBuffer) {
        VkClearValue clearColor{};
        VkClearValue depthStencil{};
        clearColor.color = { 0.0, 0.0, 0.0, 0.0 };
        depthStencil.depthStencil = { 1.0f, 0 };

        dyRender.BeginRecordCommandBuffer(commandBuffer, clearColor, depthStencil, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
        
        VkBuffer vertexBuffers[] = { vbuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        glm::mat4 projection = MvkMath::Project2D(window.GetWidth(), window.GetHeight(), 1.0, 0.0);

        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = image.layout;
        imageInfo.imageView = image.imageView;
        imageInfo.sampler = image.imageSampler;

        VkWriteDescriptorSet writeDescriptorSets = MvkDyPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo);
        vkCmdPushDescriptorSetEKHR(mvkInstance.instance, commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dyPipe.pipelineLayout, 0, 1, &writeDescriptorSets);
        vkCmdPushConstants(commandBuffer, dyPipe.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &projection);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        
        //    DEPTH BUFFER INFO:
        //        The depth buffer is an image where the depth of each fragment of every draw call is
        //        layered on top of eachother as each draw call is performed. This means that you have
        //        to sort your draw calls by depth before rendering for "depth-testing," to work.
        //
        //        Depth-Testing tests the current pixel/fragment's depth against the depth buffer. If
        //        the fragment's depth is lower than the depth buffer fragment, then the fragment is
        //        discarded.
        //
        //        THIS IS NOT DEPTH-SORTING, JUST DEPTH-TESTING.
        
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t) / 2.0, 1, 6, 0, 0);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t) / 2.0, 1, 0, 0, 0);
        
        dyRender.EndRecordCommandBuffer(commandBuffer, clearColor, depthStencil, swapChain.CurrentImageView(), swapChain.CurrentImage(), swapChain.CurrentExtent2D());
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