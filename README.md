# MiniVulkan-Dynamic
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
13. Headless Rendering via `MiniVkImageRenderer` for rendering outside the swapchain (`MiniVkSwapChainRenderer`).
14. Window Input processing using GLFW for Keyboard, Mouse and Gamepads (gamepad mappings default to Xbox).
15. Triangulation of ****valid**** meshes using Earcut:
    <br/> https://github.com/mapbox/earcut.hpp

The `source.cpp` example loads QOI images (an example image is provided). Please see the official QOI repo for the full source code and any provided examples or commandline tools for dealing with QOI images (alternative to .png): https://qoiformat.org


### ABOUT MINIVULKAN-DYNAMIC

MiniVulkan-Dynamic (MV-D) is a simple to understand and use Vulkan render engine that relies on dynamic rendering instead of purpose built render-passes. This allows for rapid development with low-maintenance overhead. On top of this MV-D relies on utilizing Push Constants and Push Descriptors for all render data being passed too the GPU, this simplifies and removes maintenance overhead for managing descriptor sets and pools. The core philosophy behind MV-D ordered-minimal-dependency between systems. Meaning that each subsytem implementation (see below or `MiniVK.hpp` for include order) contains only the required subsytems before it in order to operate. With that in mind this helps to keep the MV-D library clean, understandable and easy to modify.

The entire MV-D library relies on two very important support headers `invokable.hpp` (see link above) and `disposable.hpp`. **Invokable** allows you to create multiple event-style functions (`callbacks<A...>`) which can all be hooked into an event (`invokable<A..>`) with the same arguments and executed/invoked all at once, this can be done using lambda functions (see github page for more info). **Disposable** (requires invokable) is a parent-class with an invokable event `onDispose<bool>` for hooking in events for cleaning up dynamic memory resources when either the disposable object goes out of scope and its destructor is called or when manually called with `Dispose(default = TRUE)`--note that the event `onDispose` allows for passing in a `BOOL` for checking if extra work should be done--in the case of MV-D if Vulkan's Render Device `WaitIdle` should be called to wait for the GPU to be finished with resources before disposing.

Finally MiniVulkan-Dynamic implements GLFW for its window back-end. All of the GLFW required function calls are ONLY within this header library and as such should allow you to overwrite or ignore the implementation with ease if you want to use something like SDL.

### MINIVULKAN-DYNAMIC API:

FUTURE
