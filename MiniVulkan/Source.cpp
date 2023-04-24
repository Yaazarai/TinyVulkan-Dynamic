#define MINIVULKAN_SHORTREF
#define QOI_IMPLEMENTATION
#include "./MiniVK.hpp"
#include "./QuiteOkImageFormat.h"
using namespace mvk;

#define DEFAULT_VERTEX_SHADER "./sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./sample_frag.spv"

int MINIVULKAN_WINDOWMAIN {
    try {
        const std::vector<VkPhysicalDeviceType> renderDeviceTypes = { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU };
        const MiniVkBufferingMode bufferingMode = MiniVkBufferingMode::TRIPLE;
        const std::tuple<VkShaderStageFlagBits, std::string> vertexShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, DEFAULT_VERTEX_SHADER };
        const std::tuple<VkShaderStageFlagBits, std::string> fragmentShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, DEFAULT_FRAGMENT_SHADER };
        const MiniVkVertexDescription vertexDescription = MiniVkVertex::GetVertexDescription();
        const std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings = { MiniVkDynamicPipeline::SelectPushDescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT) };
        const std::vector<VkPushConstantRange>& pushConstantRanges = { MiniVkDynamicPipeline::SelectPushConstantRange(sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT) };

        MiniVkWindow window("MINIVK WINDOW", 1920, 1080, true);
        MiniVkInstance instance(MiniVkWindow::QueryRequiredExtensions(MVK_VALIDATION_LAYERS), "MINIVK");
        MiniVkRenderDevice renderDevice(instance, window.CreateWindowSurface(instance.instance), renderDeviceTypes);
        MiniVkVMAllocator vmAlloc(instance, renderDevice);

        MiniVkCommandPool cmdSwapPool(renderDevice, static_cast<size_t>(bufferingMode));
        MiniVkSwapChain swapChain(renderDevice, MiniVkSurfaceSupporter(), bufferingMode);
        MiniVkShaderStages shaders(renderDevice, { vertexShader, fragmentShader });

        MiniVkDynamicPipeline dySwapChainPipe(renderDevice, swapChain.imageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges, true, false, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        MiniVkSwapChainRenderer dyRender(renderDevice, vmAlloc, cmdSwapPool, swapChain, dySwapChainPipe);

        MiniVkDynamicPipeline dyImagePipe(renderDevice, swapChain.imageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges, true, false, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        MiniVkCommandPool cmdRenderPool(renderDevice, static_cast<size_t>(bufferingMode));
        MiniVkCmdPoolQueue cmdRenderQueue(cmdRenderPool);
        MiniVkImage renderSurface(renderDevice, vmAlloc, window.GetWidth(), window.GetHeight(), false, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        MiniVkImageRenderer imageRenderer(renderDevice, vmAlloc, cmdRenderQueue, &renderSurface, dyImagePipe);

        window.onResizeFrameBuffer.hook(callback<int, int>(&swapChain, &MiniVkSwapChain::OnFrameBufferResizeCallback));
        swapChain.onResizeFrameBuffer.hook(callback<int&, int&>(&window, &MiniVkWindow::OnFrameBufferReSizeCallback));

        VkClearValue clearColor{ .color = { 1.0, 0.0, 0.0, 1.0 } };
        VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };





        std::vector<MiniVkVertex> quad1 = MiniVkQuad::CreateWithOffset({480.0,270.0}, {960.0,540.0,0.5}, {1.0,1.0,1.0,0.75});
        std::vector<MiniVkVertex> quad2 = MiniVkQuad::CreateWithOffset({128.0,128.0}, {960.0,540.0,1.0}, {1.0,1.0,1.0,0.5});
        quad2.insert(quad2.end() - 1, MiniVkVertex({ 0.5, 1.0 }, { 620.0,640.0,1.0 }, { 1.0,1.0,1.0,0.5 }));

        constexpr glm::float32 angle = 45.0f * (glm::pi<glm::float32>() / 180.0f);
        constexpr glm::float32 scale = 0.5f;
        glm::vec3 origin = quad2[0].position;
        MiniVkQuad::RotateFromOrigin(quad2, origin, angle);
        MiniVkQuad::ScaleFromOrigin(quad2, origin, scale);
        MiniVkQuad::RotateScaleFromOrigin(quad2, origin, -angle * 1.5, 2.0f);

        std::vector<MiniVkVertex> triangle;
        triangle.insert(triangle.end(), quad2.begin(), quad2.end());
        
        std::vector<uint32_t> indices = MiniVkPolygon::TriangulatePointList(quad2);
        
        std::cout << "HELLO WORLD" << std::endl;
        for (size_t i = 0; i < indices.size(); i++) {
            std::cout << (size_t)indices[i] << ", ";
        }
        std::cout << std::endl;
        std::cout << "GOODBYE WORLD" << std::endl;

        MiniVkBuffer vbuffer(renderDevice, vmAlloc, triangle.size() * sizeof(MiniVkVertex), MiniVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
        vbuffer.StageBufferData(dyImagePipe.graphicsQueue, cmdRenderPool.GetPool(), triangle.data(), triangle.size() * sizeof(MiniVkVertex), 0, 0);
        MiniVkBuffer ibuffer(renderDevice, vmAlloc, indices.size() * sizeof(indices[0]), MiniVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
        ibuffer.StageBufferData(dyImagePipe.graphicsQueue, cmdRenderPool.GetPool(), indices.data(), indices.size() * sizeof(indices[0]), 0, 0);
        
        FILE* test = fopen("./Screeny.qoi", "rb");
        if (test == NULL) { std::cout << "NO QOI IMAGE" << std::endl; } else { fclose(test); }
        qoi_desc qoidesc;
        void* qoiPixels = qoi_read("./Screeny.qoi", &qoidesc, 4);
        VkDeviceSize dataSize = qoidesc.width * qoidesc.height * qoidesc.channels;
        MiniVkImage image = MiniVkImage(renderDevice, vmAlloc, qoidesc.width, qoidesc.height, false, VK_FORMAT_R8G8B8A8_SRGB);
        image.StageImageData(dyImagePipe.graphicsQueue, cmdSwapPool.GetPool(), qoiPixels, dataSize);
        QOI_FREE(qoiPixels);

        int32_t rentBufferIndex;
        VkCommandBuffer renderTargetBuffer = cmdRenderQueue.RentBuffer(rentBufferIndex);
        imageRenderer.BeginRecordCmdBuffer(renderTargetBuffer, VkExtent2D {.width = (uint32_t)renderSurface.width, .height = (uint32_t)renderSurface.height}, clearColor, depthStencil, true);
            glm::mat4 projection = MiniVkMath::Project2D(window.GetWidth(), window.GetHeight(), 0.0, 0.0, 1.0, 0.0);
            imageRenderer.PushConstants(renderTargetBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &projection);
            VkDescriptorImageInfo imageInfo = image.GetImageDescriptor();
            VkWriteDescriptorSet writeDescriptorSets = MiniVkDynamicPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo);
            imageRenderer.PushDescriptorSet(renderTargetBuffer, writeDescriptorSets);
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(renderTargetBuffer, 0, 1, &vbuffer.buffer, offsets);
            vkCmdBindIndexBuffer(renderTargetBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(renderTargetBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);
        imageRenderer.EndRecordCmdBuffer(renderTargetBuffer, VkExtent2D{ .width = (uint32_t)renderSurface.width, .height = (uint32_t)renderSurface.height }, clearColor, depthStencil);
        imageRenderer.RenderExecute(renderTargetBuffer);
        
        renderDevice.WaitIdle();
        cmdRenderQueue.ReturnBuffer(rentBufferIndex);


        


        std::vector<MiniVkVertex> sw_triangles = MiniVkQuad::Create(glm::vec3(1920.0, 1080.0,0.0));
        std::vector<uint32_t> sw_indices = { 0, 1, 2, 2, 3, 0 };
        MiniVkBuffer sw_vbuffer(renderDevice, vmAlloc, sw_triangles.size() * sizeof(MiniVkVertex), MiniVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
        sw_vbuffer.StageBufferData(dyImagePipe.graphicsQueue, cmdSwapPool.GetPool(), sw_triangles.data(), sw_triangles.size() * sizeof(MiniVkVertex), 0, 0);
        MiniVkBuffer sw_ibuffer(renderDevice, vmAlloc, sw_indices.size() * sizeof(sw_indices[0]), MiniVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
        sw_ibuffer.StageBufferData(dyImagePipe.graphicsQueue, cmdSwapPool.GetPool(), sw_indices.data(), sw_indices.size() * sizeof(sw_indices[0]), 0, 0);

        size_t frame = 0;
        bool swap = false;
        dyRender.onRenderEvents.hook(callback<VkCommandBuffer>([&swap, &frame, &instance, &window, &swapChain, &dyRender, &dySwapChainPipe, &renderSurface, &sw_ibuffer, &sw_vbuffer](VkCommandBuffer commandBuffer) {
            VkClearValue clearColor{ .color = { 0.25, 0.25, 0.25, 1.0 } };
            VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };
        
            dyRender.BeginRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil, true);
            
            glm::mat4 projection = MiniVkMath::Project2D(window.GetWidth(), window.GetHeight(), 120 * (size_t) swap, 0.0, 1.0, 0.0);
            dyRender.PushConstants(commandBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &projection);
            
            VkDescriptorImageInfo imageInfo = renderSurface.GetImageDescriptor();
            VkWriteDescriptorSet writeDescriptorSets = MiniVkDynamicPipeline::SelectWriteImageDescriptor(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo);
            dyRender.PushDescriptorSet(commandBuffer, writeDescriptorSets);
            
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sw_vbuffer.buffer, offsets);
            vkCmdBindIndexBuffer(commandBuffer, sw_ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sw_ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);
            
            dyRender.EndRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);

            frame ++;

            if (frame > 120) {
                frame = 0;
                swap = (swap == true)? false : true;
            }
        }));

        std::thread mythread([&window, &dyRender]() { while (!window.ShouldClose()) { dyRender.RenderExecute(); } });
        window.WhileMain();
        mythread.join();

        disposable::DisposeOrdered({ &instance, &window, &renderDevice, &vmAlloc, &swapChain, &cmdSwapPool, &shaders, &dySwapChainPipe, &dyRender,
            &dyImagePipe, &cmdRenderPool, &cmdRenderQueue, &renderSurface, &imageRenderer, &vbuffer, &ibuffer, &image, &sw_vbuffer, &sw_ibuffer }, true);
        
    } catch (std::runtime_error err) { std::cout << err.what() << std::endl; }
    return VK_SUCCESS;
}