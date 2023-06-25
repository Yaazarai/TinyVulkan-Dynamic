#include "./TinyVK.hpp"
using namespace tinyvk;

#define DEFAULT_VERTEX_SHADER "./sample_vert.spv"
#define DEFAULT_FRAGMENT_SHADER "./sample_frag.spv"
#define DEFAULT_COMMAND_POOLSIZE 20

const std::vector<VkPhysicalDeviceType> rdeviceTypes = { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU };
const TinyVkBufferingMode bufferingMode = TinyVkBufferingMode::TRIPLE;
const std::tuple<VkShaderStageFlagBits, std::string> vertexShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, DEFAULT_VERTEX_SHADER };
const std::tuple<VkShaderStageFlagBits, std::string> fragmentShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, DEFAULT_FRAGMENT_SHADER };
const TinyVkVertexDescription vertexDescription = TinyVkVertex::GetVertexDescription();
const std::vector<VkPushConstantRange> pushConstantRanges = { TinyVkGraphicsPipeline::SelectPushConstantRange(sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT) };
const std::vector<VkDescriptorSetLayoutBinding> pushDescriptorLayouts = { TinyVkGraphicsPipeline::SelectPushDescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1) };

int32_t TINYVULKAN_WINDOWMAIN {
    /// 
    /// Initialize TinyVK with a GLFW window/swapchain and complete render pipeline.
    ///     * Window -> Instance -> Device -> CommandPool -> SwapChain -> Shaders -> GraphicsPipeline -> GPU Memory ALlocator -> SwapChainRenderer.
    /// 
    /// Alternatively you can avoid using a window and swapchain/swapchain renderer by using the image renderer model.
    ///     * Instance -> Device -> CommandPool -> Shaders -> GraphicsPipeline -> GPU Memory Allocator -> ImageRenderer.
    /// 
    /// You can also have as many of each as you'd like if for example you needed a Window/Swapchain and two separate rendering pipelines each with their own set of shaders, you can do that too.
    /// 
    
    TinyVkWindow window("TINYVK WINDOW", 1440, 810, true, false);
	TinyVkInstance instance(TinyVkWindow::QueryRequiredExtensions(TVK_VALIDATION_LAYERS), "TINYVK");
	TinyVkRenderDevice rdevice(instance, window.CreateWindowSurface(instance.GetInstance()), rdeviceTypes);
	TinyVkCommandPool commandPool(rdevice, static_cast<size_t>(bufferingMode) + DEFAULT_COMMAND_POOLSIZE);
	TinyVkSwapChain swapChain(rdevice, window, bufferingMode);
	TinyVkShaderStages shaders(rdevice, { vertexShader, fragmentShader });
    //TinyVkGraphicsPipeline renderPipe(rdevice, swapChain.imageFormat, shaders, vertexDescription, {}, pushConstantRanges, true, TinyVkGraphicsPipeline::GetBlendDescription(), VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
    TinyVkGraphicsPipeline renderPipe(rdevice, swapChain.imageFormat, shaders, vertexDescription, pushDescriptorLayouts, {}, true, TinyVkGraphicsPipeline::GetBlendDescription(), VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
    TinyVkVMAllocator vmAlloc(instance, rdevice);
    TinyVkSwapChainRenderer swapRenderer(rdevice, vmAlloc, commandPool, swapChain, renderPipe);

    VkClearValue clearColor{ .color = { 0.0, 0.0, 0.0, 0.5f } };
    VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };
    VkDeviceSize offsets[] = { 0 };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 
    /// Create a quad (list of 4 vertices) and the index list for how it's triangles map.
    /// Then copy the quad and indices into their respect vertex/index buffers in GPU memory.
    /// 

    std::vector<TinyVkVertex> triangles = {
        TinyVkVertex({0.0f,0.0f}, {240.0f,135.0f,               1.0f}, {1.0f,0.0f,0.0f,1.0f}),
        TinyVkVertex({0.0f,0.0f}, {240.0f+960.0f,135.0f,        1.0f}, {0.0f,1.0f,0.0f,1.0f}),
        TinyVkVertex({0.0f,0.0f}, {240.0f+960.0f,135.0f+540.0f, 1.0f}, {1.0f,0.0f,1.0f,1.0f}),
        TinyVkVertex({0.0f,0.0f}, {240.0f,135.0f + 540.0f,      1.0f}, {0.0f,0.0f,1.0f,1.0f})
    }; std::vector<uint32_t> indices = {0,1,2,2,3,0};

    TinyVkBuffer vbuffer(rdevice, renderPipe, commandPool, vmAlloc, triangles.size() * sizeof(TinyVkVertex), TinyVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
    vbuffer.StageBufferData(triangles.data(), triangles.size() * sizeof(TinyVkVertex), 0, 0);
    TinyVkBuffer ibuffer(rdevice, renderPipe, commandPool, vmAlloc, indices.size() * sizeof(indices[0]), TinyVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
    ibuffer.StageBufferData(indices.data(), indices.size() * sizeof(indices[0]), 0, 0);
    TinyVkBuffer projection1(rdevice, renderPipe, commandPool, vmAlloc, sizeof(glm::mat4), TinyVkBufferType::VKVMA_BUFFER_TYPE_UNIFORM);
    TinyVkBuffer projection2(rdevice, renderPipe, commandPool, vmAlloc, sizeof(glm::mat4), TinyVkBufferType::VKVMA_BUFFER_TYPE_UNIFORM);
    TinyVkBuffer projection3(rdevice, renderPipe, commandPool, vmAlloc, sizeof(glm::mat4), TinyVkBufferType::VKVMA_BUFFER_TYPE_UNIFORM);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 
    /// Create an onRenderEvent on the swapchain (called every frame, vsync enabled).
    /// Begin recording rendering commands into the Swapchain supplied command buffer.
    /// Then set the camera projection matrix and bind the previously copied to the GPU vertex/index buffers.
    /// Finally use the draw indexed command to draw the vertex buffer (quad) using the index buffer to map the triangle layout.
    /// End recording rendering commands using the Swapchain supplied command buffer.
    /// 

    struct SwapFrame {
        TinyVkBuffer &projection, &ibuffer, &vbuffer;
        SwapFrame(TinyVkBuffer& projection, TinyVkBuffer& ibuffer, TinyVkBuffer& vbuffer) : projection(projection), ibuffer(ibuffer), vbuffer(vbuffer) {}
    };

    TinyVkResourceQueue<SwapFrame,static_cast<size_t>(bufferingMode)> queue(
        {
            SwapFrame(projection1,ibuffer,vbuffer),
            SwapFrame(projection2,ibuffer,vbuffer),
            SwapFrame(projection3,ibuffer,vbuffer)
        }, callback<size_t&>([&swapRenderer](size_t& frameIndex){ frameIndex = swapRenderer.GetSyncronizedFrameIndex(); }),
        callback<SwapFrame&>([](SwapFrame& resource){})
    );

    int angle = 0;
    swapRenderer.onRenderEvents.hook(callback<VkCommandBuffer>([&angle, &instance, &window, &swapChain, &swapRenderer, &renderPipe, &queue, &clearColor, &depthStencil, &offsets](VkCommandBuffer commandBuffer) {
        auto frame = queue.GetFrameResource();
        swapRenderer.BeginRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);
        
        int offsetx, offsety;
        offsetx = glm::sin(glm::radians(static_cast<glm::float32>(angle))) * 64;
        offsety = glm::cos(glm::radians(static_cast<glm::float32>(angle))) * 64;
        glm::mat4 camera = TinyVkMath::Project2D(window.GetWidth(), window.GetHeight(), offsetx, offsety, 1.0, 0.0);
        frame.projection.StageBufferData(&camera, sizeof(glm::mat4), 0, 0);
        VkDescriptorBufferInfo cameraDescriptorInfo = frame.projection.GetBufferDescriptor();
        VkWriteDescriptorSet cameraDescriptor = renderPipe.SelectWriteBufferDescriptor(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { &cameraDescriptorInfo });
        swapRenderer.PushDescriptorSet(commandBuffer, { cameraDescriptor });
        //swapRenderer.PushConstants(commandBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &camera);

        swapRenderer.CmdBindGeometry(commandBuffer, &frame.vbuffer.buffer, frame.ibuffer.buffer, offsets, offsets[0]);
        swapRenderer.CmdDrawGeometry(commandBuffer, true, 1, 0, static_cast<uint32_t>(frame.ibuffer.size) / sizeof(uint32_t), 0, 0);
        swapRenderer.EndRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);

        angle += 1.25;
    }));

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// 
    /// Run the SwapChainRenderer render execution function on its own thread to avoid hanging GLFW or vice versa for smooth resizing and
    ///     constant playback when moving the window--otherwise window interactions pause the main() and consequently the render function.
    ///     Note that the window is also passed to the thread to check if the window closed to close the render thread as well.
    /// Then execute the window's built in main function to handling the window loop and polling events.
    /// Finally when the render thread closes with the window, join and free the render thread.
    /// 

    std::thread mythread([&window, &swapRenderer]() { while (!window.ShouldClose()) { swapRenderer.RenderExecute(); } });
    window.onWhileMain.hook(callback<std::atomic<bool>&>([&swapRenderer](std::atomic<bool>& shouldClose){ swapRenderer.RenderExecute(); }));
    window.WhileMain(false);
    mythread.join();
    return VK_SUCCESS;
};