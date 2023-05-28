# TinyVulkan-Dynamic
Vulkan Graphics render engine built using Dynamic Rendering and Push Constants/Descriptors.

VULKAN DEVICE EXTENSIONS: `VK_KHR_SWAPCHAIN_EXTENSION_NAME`, `VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME`,  `VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME`, `VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME`, `VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME`.

LIST OF FEATURES:
1. Push Descriptors & Push Constants
2. Dynamic Viewports & Scissors
3. 2D Projection Matrix (Can be easily swapped to 3D if needed).
4. Multi-Draw Indirect Rendering commands enabled for batch rendering.
5. Easy Invokable Events & Function callbacks subscription model (like C#).
   <br/> https://github.com/FatalSleep/Event-Callback
6. VMA implementation for memory management & allocation.
   <br/> https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
7. Implementation for Image and Buffer staging on GPU (for passing CPU bound images, verftex buffers, etc. to the GPU).
8. Easy creation of Vulkan stages for the entire render pipeline.
9. Strict Order of Dependency Design (abstracted for each stage of Vulkan to be as independent as possible from other stages).
10. Multiple Screen Buffering Modes: Double, Triple, Quadruple.
11. Thread-Pool implementation (Needs to be rewritten).
12. Window transparency using GLFW `transparentFramebuffer`.
13. Headless Rendering via `TinyVkImageRenderer` for rendering outside the swapchain (`TinyVkSwapChainRenderer`).
14. Window Input processing using GLFW for Keyboard, Mouse and Gamepads (gamepad mappings default to Xbox).
15. Triangulation of ****valid**** meshes using Earcut:
    <br/> https://github.com/mapbox/earcut.hpp

The `source.cpp` example loads QOI images (an example image is provided). Please see the official QOI repo for the full source code and any provided examples or commandline tools for dealing with QOI images (alternative to .png): https://qoiformat.org


### ABOUT TINYVULKAN-DYNAMIC

TinyVulkan-Dynamic (TV-D) is a simple to understand and use Vulkan render engine that relies on dynamic rendering instead of purpose built render-passes. This allows for rapid development with low-maintenance overhead. On top of this TV-D relies on utilizing Push Constants and Push Descriptors for all render data being passed too the GPU, this simplifies and removes maintenance overhead for managing descriptor sets and pools. The core philosophy behind TV-D ordered-tinymal-dependency between systems. Meaning that each subsytem implementation (see below or `TinyVK.hpp` for include order) contains only the required subsytems before it in order to operate. With that in mind this helps to keep the TV-D library clean, understandable and easy to modify.

The entire TV-D library relies on two very important support headers `invokable.hpp` (see link above) and `disposable.hpp`. **Invokable** allows you to create multiple event-style functions (`callbacks<A...>`) which can all be hooked into an event (`invokable<A..>`) with the same arguments and executed/invoked all at once, this can be done using lambda functions (see github page for more info). **Disposable** (requires invokable) is a parent-class with an invokable event `onDispose<bool>` for hooking in events for cleaning up dynamic memory resources when either the disposable object goes out of scope and its destructor is called or when manually called with `Dispose(default = TRUE)`--note that the event `onDispose` allows for passing in a `BOOL` for checking if extra work should be done--in the case of TV-D if Vulkan's Render Device `WaitIdle` should be called to wait for the GPU to be finished with resources before disposing.

Finally TinyVulkan-Dynamic implements GLFW for its window back-end. All of the GLFW required function calls are ONLY within this header library and as such should allow you to overwrite or ignore the implementation with ease if you want to use something like SDL.

### TINYVULKAN-DYNAMIC API:

`TinyVkWindow` creates a defaulted GLFW window which exposes its window surface for rendering and allows GLFW input events (keyboard, mouse and gamepads). This is optional, only for GUI based applications, you can avoid creating a window entirely if you only plan to do offscreen rendering (optional, GUI apps only).

`TinyVkInstance` creates the Vulkan instance context to interface with the Vulkan API and is required to use TinyVulkan (required).

`TinyVkRenderDevice` creates a logical/physical device which represents the required GPU/iGPU, GPUs are prioritized over iGPUs if your system has both). This is required to do any sort of rendering operations (as it is the interface to the GPU, required).

`TinyVkVMAllocator` creates the AMD Vulkan Memory Allocator for allocating and handling GPU memory automatically (required), instead of manually handling GPU memory allocations (required).

`TinyVkSwapChain` creates a swapchain for rendering to your `TinyVkWindow`. The SwapChain exposes the buffering images of the window to the graphics renderer and is required if creating a GUI based application (optional, GUI apps only).

`TinyVkCommandPool` creates a `VkCommandPool` which is required for GPU operations such as transfer or rendering commands which are written to VkCommandBuffers allocated by the VkCommandPool (required). The `TinyVkCommandPool` uses a lease/return model for `VkCommandBuffers`, where you lease(rent) out a `VkCommandBuffer` for whatever your use case is, then return the VkCommandBuffer by its index to the TinyVkCommandPool for reuse within the pool. Some classes such as `TinyVkImage and TinyVkBuffer` will take in the `TinyVkCommandPool` and allocate their own command buffers for use rather than renting--may be changed in the future.

`TinyVkShaderStages` loads the specified compiled shaders into memory with their intended graphics pipeline usecases (VkShaderStageFlagBits).

`TinyVkDynamicPipeline` creates a graphics pipeline that defines the properties required for your graphics renderer and shader handling (required).

`TinyVkBuffer` is for creating and copying memory from the CPU to GPU for use with shaders. Such buffers include `vertex`, `index`, `uniform`, `staging`, etc.

`TinyVkImage` represents an image ofr rendering operations. You can either stage an image from the CPU into GPU memory, such as loading a QOI/PNG/BMP image file and calling `image.StageImageData(...)` or you can render directly to the image as render target using the TinyVkImageRenderer/

`TinyVkImageRenderer` is used for offscreen image rendering, e.g. performing rendering operations directly onto an image that may not be presenting onto the screen.

`TinyVkSwapChainRenderer` is used for onscreen image rendering, e.g. rendering directly to the Window surface for GUI apps or games and is required if you have a TinyVkWindow for your application.

`TinyVkQuad` and `TinyVkPolygon` are used for creating vectors of vertexes which represent some geometry or mesh to be rendered to the screen. TinyVkPolygon does provide a function `TinyVkPolygon::TriangulatePointList()` which uses `Earcut.hpp` to generate a valid delauney triangulation of the point mesh. This may be useful for CAD applications or the like.

`invokable<...>` and `callback<...>` are the event API for creating event calls for similar functions that all need to be executed when an event happens. You can hook a `callback<...>` to an `invokable<...>` with the same template parameters and call `myinvokable.invoke()` to execute any hooked callbacks and pass through any required arguments for those callbacks. The `callback<...>` is a single object which represents a function or lambda function to execute when invoked.

So how do you use the TinyVulkan API? The API is still rather vebrose much like Vulkan, however much more digestible and limited without further extensions/changes. There are a few methods for using TinyVulkan, all of which are very similar: Create a `TinyVkWindow` if developing a GUI app. Then create your `TinyVkInstance`, `TinyVkRenderDevice`, `TinyVkVMAllocator`, `TinyVkCommandPool` (for GUI) or `TinyVkCmdPoolQueue` (for headless/offscreen), `TinyVkShaderStages`, `TinyVkDynamicPipeline` or multiple if using different pipelines--such as for multiiple renderers, either `TinyVkSwapChainRenderer` (GUI) or `TinyVkImageRenderer` (headless/offscreen). You may need multiple graphics pipelines or renderers depending on your shader or application requirements.

TinyVulkan uses and is limited to using `Push Constants` and `Push Descriptors` or UBOs which can be sent to shaders via push descriptors as descriptor pool/set handling is not implemented. `TinyVkBuffer` and `TinyVkImage` both have their own descriptor functions for getting their object data as descriptors. TinyVkDynamicPipeline provides functionality for turning those descriptors into `VkWriteDescriptorSet` which is used to upload images/buffers/etc. into GPU memory using your swapchain or image renderers `PushDescriptorSet` functionaliy for rendering. Both renderers have `.RenderExecute()` functions for executing your rendering events. Rendering events can be hooked as `callback<...>` using either lambdas or functions that record rendering operations to a received command buffer. Once your renderer is setup you can run your `window.WhileMain()` default main functionality provided (if using GUI) and execute your renderer's `.RenderExecute()` function either on the same thread or a secondary thread as needed (for non-blocking rendering operations).

Finally you call `disposable::DisposeOrdered({ obj1, obj2, obj3, ... }, descenbding?)` to dispose of your TinyVulkan objects in the order they were created and set the descending flag too true (dispose from last to first). This is required because one resource may be dependent upon a prior used resource, so its always good practice to cleanup objects in the reverse order they were created.
