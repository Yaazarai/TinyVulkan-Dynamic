# MiniVulkan-Dynamic
Vulkan Graphics render engine using extension VK_KHR_DYNAMIC_RENDERING.


FINISHED:
1. MiniVkObject
    * Small interface for cleaning up dynamically allocated memory.
    * Calls onDispose and adds a cleanup method `Disposable()` in the constructor using included `<invokable.hpp>`.
2. MiniVkWindow (GLFW)
    * Overridable Window class built with the default using GLFW.
    * Derive this class and overrid ALL virtual methods to change the underlying Window API.
3. MiniVkInstance (Vulkan Instance + Logical/Physical Device loading)
    * Vulkan instance, physical device and logical device creation class.
4. MiniVkSwapChain
    * Vulkan swapchain class for managing swap chain loaded device images.
5. MiniVkCommandPool
    * Vulkan command pool and buffer class for render/transfer commands.
6. MiniVkDynamicPipeline
    * Vulkan dynamic rendering pipeline creation.
7. MiniVk[Vertex/Index/Uniform]Buffer
    * Support buffers for interop with SPIR-V shaders.
8. MiniVkDynamicRenderer
    * Actual rendering pipeline that draws the final frame and provides command buffer recording methods.

UNFINISHED:
1. Test and finish MiniVkThreadPool implemention.
2. ~~Add more optional properties for configuring the MiniVkDynamicPipeline when creating the graphics pipeline.~~
3. Add in Texture/Sprite loading support.
4. ~~Setup default rendering passthrough shaders.~~
5. ~~Setup default model/view/projection matrices.~~
6. ~~Fix centered (-1,-1 to +1,+1) GLSL coordinate system to top-left (0,0, to 1,1).~~
7. Provide better solution to `FORCE DISPOSE ORDER` calls in `source.cpp`.
8. Add framerate handling and clean timestep support.
9. Add Fast Rotsprite shader 2x & 3x scale (forked on profile).
10. Add headless (without window) mode.
