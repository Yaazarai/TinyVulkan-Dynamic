# MiniVulkan-Dynamic
Vulkan Graphics render engine using extension `VK_KHR_DYNAMIC_RENDERING`, `VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME`, `VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME`.

LIST OF FEATURES: (\** means future, not yet implemented)

1. Push Descriptors & Push Constants
2. Dynamic VIewports & Scissors
3. Runtime Enable/Disable of Color/Alpha Blending
4. 2D Projection Matrix (Can be easily swapped to 3D if needed).
5. Multi-Draw Indirect Rendering commands enabled for batch rendering.
6. Easy Invokable Events & Function callbacks subscription model (like C#).
   https://github.com/FatalSleep/Event-Callback
7. VMA implementation for memory management & allocation.
   https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
9. Implementation for Image and Buffer staging on GPU (for passing CPU bound images, verftex buffers, etc. to the GPU).
10. Easy creation of Vulkan stages for the entire render pipeline.
11. Strict Order of Dependency Design (abstracted for each stage of Vulkan to be as independent as possible from other stages).
12. Multiple Screen Buffering Modes: Double, Triple, Quadruple.
13. Thread-Pool implementation.
14. Window transparency using GLFW `transparentFramebuffer`.
15. \** Headless Rendering (NOT-IMPLEMENTED) for rendering outside the swapchain (surfaces).
