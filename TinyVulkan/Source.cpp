#define TINYVK_ALLOWS_POLLING_GAMEPADS
#include "./TinyVK.hpp"
using namespace tinyvk;

#define QOI_IMPLEMENTATION
#include "./images_qoi.h"

// Relative file path locations for the vertex shader SPIR-V binaries:
#define DEFAULT_VERTEX_SHADER "./sample_vert.spv"
// Relative file path locations for the fragment shader SPIR-V binaries:
#define DEFAULT_FRAGMENT_SHADER "./sample_frag.spv"
// Used below as the default extra VkCommandBuffer allocations (for render commands not emiotted by the SwapChainRenderer):
#define DEFAULT_COMMAND_POOLSIZE 20

// GPU device types to look for, Screen Buffering Mode (Single/Double/Triple/Quadruple), ShaderStageFlags/Path pairs for loading:
const std::vector<VkPhysicalDeviceType> rdeviceTypes = { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU };
const TinyVkBufferingMode bufferingMode = TinyVkBufferingMode::TRIPLE;
const std::tuple<VkShaderStageFlagBits, std::string> vertexShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, DEFAULT_VERTEX_SHADER };
const std::tuple<VkShaderStageFlagBits, std::string> fragmentShader = { VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, DEFAULT_FRAGMENT_SHADER };

// Vertex description (as required by the binding layout in the shaders), DescriptorLayout/Push Constant bindings for descriptors we'll use in the shaders:
const TinyVkVertexDescription vertexDescription = TinyVkVertex::GetVertexDescription();
const std::vector<VkDescriptorSetLayoutBinding>& descriptorBindings = { TinyVkGraphicsPipeline::SelectPushDescriptorLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT) };
const std::vector<VkPushConstantRange>& pushConstantRanges = { TinyVkGraphicsPipeline::SelectPushConstantRange(sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT) };

// Get a QOI formatted image from disc:
void* image_get(std::string fpath, qoi_desc& qoidesc) {
    FILE* test = fopen(fpath.c_str(), "rb");
    if (test == NULL) throw std::runtime_error("TinyVulkan: Cannot load QOI image! " + fpath);
    return qoi_read("./Screeny.qoi", &qoidesc, 4);
}
// Free a QOI fomratted image from memory:
void image_free(void* imgPixels) { QOI_FREE(imgPixels); }

int32_t TINYVULKAN_WINDOWMAIN {
    /*  Section 0:
        TL;DR:
            S0: Setup the TinyVK pipeline with a Window/SwapChain SwapChainRenderer (presentation-model) and ImageRenderer (render-to-texture model).
            S1: Create a quad of TinyVkVertex (vertex buffer), then triangulate it with Earcut and vertex indices (index buffer). Load a QOI image
                into memory and then take that QOI image onto the GPU using TinyVkImage.
            S2: Using the TinyVkImageRenderer render the previous QOI image onto a TinyVkImage on the GPU (rsurface).
            S3: Create a quad for rsurface and a render event on the SwapChainRenderer to render rsurface to the screen.
            S4: Execute the SwapChainRender.onRenderEvents on a separate thread to avoid hanging/polling/reisizing conflicts with GLFW main thread.
            S5: Clean up resources in reverse order (by order of dependency).
                NOTE: TinyVK objects have destructors to clean up their own memory, but you have to force cleanup by order of dependency to avoid
                objects going out of scope and be disposed of prior to when we actually need them to be disposed.
    */
	TinyVkWindow window("TINYVK WINDOW", 1920, 1080, true);
	TinyVkInstance instance(TinyVkWindow::QueryRequiredExtensions(TVK_VALIDATION_LAYERS), "TINYVK");
	TinyVkRenderDevice rdevice(instance, window.CreateWindowSurface(instance.GetInstance()), rdeviceTypes);
	TinyVkCommandPool commandPool(rdevice, static_cast<size_t>(bufferingMode) + DEFAULT_COMMAND_POOLSIZE);
	TinyVkSwapChain swapChain(rdevice, window, bufferingMode /* 3 default optional args used*/);
	TinyVkShaderStages shaders(rdevice, { vertexShader, fragmentShader });
	TinyVkGraphicsPipeline renderPipe(rdevice, swapChain.imageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges, true, true, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
    TinyVkVMAllocator vmAlloc(instance, rdevice);
    TinyVkSwapChainRenderer swapRenderer(rdevice, vmAlloc, commandPool, swapChain, renderPipe);
	
    // You can have multiple graphics pipelines if your pipelines are different:
    //TinyVkGraphicsPipeline imagePipe(rdevice, swapChain.imageFormat, shaders, vertexDescription, descriptorBindings, pushConstantRanges, true, true, VKCOMP_RGBA, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL);
    TinyVkImage rsurface(rdevice, renderPipe, commandPool, vmAlloc, window.GetWidth(), window.GetHeight(), false, VK_FORMAT_B8G8R8A8_SRGB, TINYVK_SHADER_READONLY_OPTIMAL);
	TinyVkImageRenderer imageRenderer(rdevice, commandPool, vmAlloc, &rsurface, renderPipe);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*  Section 1:
        Create the render background properties: Clear Color and Clear Depth.
        Create an image quad (array/vector of vertices) of arbitrary size (whatever you want)
        and triangulate the quad vertices as an array of vertex mapped indices for rendering.
            * You don't have to use indices by manually defining your own triangulated quad
            * negating the use of index buffers entirely if necessary.
        Load an image in QOI format from disc into CPU memory using TinyVkImage.

        Create vertex/index buffers for the image quad data in GPU memory (TinyVkBuffer).
        Stage the QOI image, Vertex/Index buffers into GPU memory to read for rendering.
        Finally free out the QOI image that was CPU memory allocated as it is now staged to GPU.
    */

    VkClearValue clearColor{ .color = { 1.0, 0.0, 0.0, 1.0 } };
    VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };

	std::vector<TinyVkVertex> quad2 = TinyVkQuad::CreateWithOffset({ 128.0,128.0 }, { 960.0,540.0,0.0 }, { 1.0,1.0,1.0,0.75 });
    std::vector<uint32_t> indices = TinyVkPolygon::TriangulatePointList(quad2);

    TinyVkBuffer vbuffer(rdevice, renderPipe, commandPool, vmAlloc, quad2.size() * sizeof(TinyVkVertex), TinyVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
    vbuffer.StageBufferData(quad2.data(), quad2.size() * sizeof(TinyVkVertex), 0, 0);
    TinyVkBuffer ibuffer(rdevice, renderPipe, commandPool, vmAlloc, indices.size() * sizeof(indices[0]), TinyVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
    ibuffer.StageBufferData(indices.data(), indices.size() * sizeof(indices[0]), 0, 0);

    qoi_desc qoidesc;
    void* qoiPixels = image_get("./Screeny.qoi", qoidesc);
    TinyVkImage image = TinyVkImage(rdevice, renderPipe, commandPool, vmAlloc, qoidesc.width, qoidesc.height, false, VK_FORMAT_R8G8B8A8_SRGB, TINYVK_SHADER_READONLY_OPTIMAL);
    image.StageImageData(qoiPixels, qoidesc.width* qoidesc.height* qoidesc.channels);
    image_free(qoiPixels);
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*  Section 2:
        Using the Render-To-Image renderer (VkImageRenderer), render a QOI image onto a VkImage with the
        VkImageRenderer's default built-in command buffer and onRenderEvent (callback<VkCommandBuffer>).
    */

    imageRenderer.onRenderEvents.hook(callback<VkCommandBuffer>([&window, &imageRenderer, &rsurface, &clearColor, &depthStencil, &image, &vbuffer, &ibuffer](VkCommandBuffer renderBuffer){
        imageRenderer.BeginRecordCmdBuffer(VkExtent2D{ .width = (uint32_t)rsurface.width, .height = (uint32_t)rsurface.height }, clearColor, depthStencil, renderBuffer);

        glm::mat4 projection = TinyVkMath::Project2D(window.GetWidth(), window.GetHeight(), 0.0, 0.0, -1.0, 1.0);
        imageRenderer.PushConstants(renderBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &projection);

        auto image_descriptor = image.GetImageDescriptor();
        VkWriteDescriptorSet writeDescriptorSets = TinyVkGraphicsPipeline::SelectWriteImageDescriptor(0, 1, &image_descriptor);
        imageRenderer.PushDescriptorSet(renderBuffer, { writeDescriptorSets });

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(renderBuffer, 0, 1, &vbuffer.buffer, offsets);
        vkCmdBindIndexBuffer(renderBuffer, ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(renderBuffer, static_cast<uint32_t>(ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);

        imageRenderer.EndRecordCmdBuffer(VkExtent2D{ .width = (uint32_t)rsurface.width, .height = (uint32_t)rsurface.height }, clearColor, depthStencil, renderBuffer);
    }));
    imageRenderer.RenderExecute();
    
    // Manually dispose so that the objects don't exist for the life of the program.
    // TinyVK objects free themselves from memory out-of-scope to avoid memory leaks, but you
    //      still have to free them manually if you don't need them after a certain point.
    vbuffer.Dispose();
    ibuffer.Dispose();
    image.Dispose();

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*  Section 3:
        Create an image quad from the previous rendered image for rendering to the swap chain (window).
        Render that image onto the swap chain (window) with a camera projection offset based on the current frame.
        Use the swap chain renderer's built-in onRender event and command buffers for rendering.
    */

    std::vector<TinyVkVertex> sw_triangles = TinyVkQuad::Create(glm::vec3(1920.0, 1080.0, -0.5));
    std::vector<uint32_t> sw_indices = { 0, 1, 2, 2, 3, 0 };
    TinyVkBuffer sw_vbuffer(rdevice, renderPipe, commandPool, vmAlloc, sw_triangles.size() * sizeof(TinyVkVertex), TinyVkBufferType::VKVMA_BUFFER_TYPE_VERTEX);
    sw_vbuffer.StageBufferData(sw_triangles.data(), sw_triangles.size() * sizeof(TinyVkVertex), 0, 0);
    TinyVkBuffer sw_ibuffer(rdevice, renderPipe, commandPool, vmAlloc, sw_indices.size() * sizeof(sw_indices[0]), TinyVkBufferType::VKVMA_BUFFER_TYPE_INDEX);
    sw_ibuffer.StageBufferData(sw_indices.data(), sw_indices.size() * sizeof(sw_indices[0]), 0, 0);

    size_t frame = 0;
    bool swap = false;
    swapRenderer.onRenderEvents.hook(callback<VkCommandBuffer>([&swap, &frame, &instance, &window, &swapChain, &swapRenderer, &renderPipe, &rsurface, &sw_ibuffer, &sw_vbuffer](VkCommandBuffer commandBuffer) {
        VkClearValue clearColor{ .color = { 0.25, 0.25, 0.25, 1.0 } };
        VkClearValue depthStencil{ .depthStencil = { 1.0f, 0 } };

        swapRenderer.BeginRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);

        glm::mat4 projection = TinyVkMath::Project2D(window.GetWidth(), window.GetHeight(), 120.0F * (double)swap, 0.0, -1.0, 1.0);
        swapRenderer.PushConstants(commandBuffer, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), &projection);

        auto image_descriptor = rsurface.GetImageDescriptor();
        VkWriteDescriptorSet writeDescriptorSets = TinyVkGraphicsPipeline::SelectWriteImageDescriptor(0, 1, &image_descriptor);
        swapRenderer.PushDescriptorSet(commandBuffer, { writeDescriptorSets });

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sw_vbuffer.buffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, sw_ibuffer.buffer, offsets[0], VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sw_ibuffer.size) / sizeof(uint32_t), 1, 0, 0, 0);

        swapRenderer.EndRecordCmdBuffer(commandBuffer, swapChain.imageExtent, clearColor, depthStencil);

        if (frame++ > 60) {
            frame = 0;
            swap = (swap == true) ? false : true;
        }
    }));

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /* Section 4:
        Finally execute the swap chain (per-frame) render events on a secondary thread to avoid
        being blocked by the GLFW main thread--allows resizing, avoids handing on window move, etc.
        Then clean up the render thread and TinyVk remaining allocated resources.
    */

    std::thread mythread([&window, &swapRenderer]() { while (!window.ShouldClose()) { swapRenderer.RenderExecute(); } });
    window.WhileMain();
    mythread.join();

    /// 
    /// Each TinyVk object extends the disposable class and cleans up their own resources out of scope.
    /// 
    return VK_SUCCESS;
};