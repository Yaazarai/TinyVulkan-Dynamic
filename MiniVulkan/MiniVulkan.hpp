#pragma once
#ifndef MINIVK_LIBRARY
#define MINIVK_LIBRARY
    /*
        Vulkan requires the GLFW library linkage for native Window handling on Microsoft Windows, macOS, X11 (UNIX) and Wayland (UNIX Server).
        GLFWV will only compile on both Release and Debug with the following options:

        C/C++ | Code Configuration | Runtime Libraries:
            RELEASE: /MD
            DEBUG  : /mtD

        Linker | Input | Additional Dependencies:
            RELEASE:vulkan-1.lib;glfw3.lib;
            DEBUG  : vulkan-1.lib;glfw3_mt.lib;

        C/C++ | General | Additional Include Directories:
            RELEASE: ...\VulkanSDK\1.3.211.0\Include;...\glfw-3.3.7.bin.WIN64\glfw-3.3.7.bin.WIN64\include;
            DEBUG  : ...\VulkanSDK\1.3.211.0\Include;...\glfw-3.3.7.bin.WIN64\glfw-3.3.7.bin.WIN64\include;

        Linker | General | Additional Library Directories:
            RELEASE: ...\glfw-3.3.7.bin.WIN64\lib-vc2022;$(VULKAN_SDK)\Lib;
            DEBUG  : ...\glfw-3.3.7.bin.WIN64\lib-vc2022;$(VULKAN_SDK)\Lib;
    */
    // ** Make sure you've installed the Vulkan SDK binaries.
    #define GLFW_INCLUDE_VULKAN
    #define VK_USE_PLATFORM_WIN32_KHR
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3.h>
    #include <GLFW/glfw3native.h>

    #include <vulkan/vulkan.h>
    #define MVK_RENDERER_VERSION VK_MAKE_API_VERSION(0, 1, 0, 0)
    #define MVK_ENGINE_VERSION VK_MAKE_API_VERSION(0, 1, 3, 0)
    #define MVK_API_VERSION VK_API_VERSION_1_3
    
    // GLM (OpenGL math) properties.
    #define GLM_FORCE_RADIANS
    #define GLM_FORCE_LEFT_HANDED
    #define GLM_FORCE_DEPTH_ZERO_TO_ONE
    #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    
    #include <glm/glm.hpp>
    #include <glm/vec2.hpp>
    #include <glm/vec4.hpp>
    #include <glm/mat2x2.hpp>
    #include <glm/mat4x4.hpp>
    #include <glm/ext.hpp>

    #define MIN(a,b) ((a)<(b)?(a):(b))
    #define MAX(a,b) ((a)>(b)?(a):(b))
    
    // Sets a GLFW Windows' Main entry point. If DebugMode is enabled the console window will be visible.
    /*
        Linker | System | SubSystem
            RELEASE: Windows (/SUBSYSTEM:WINDOWS)
            DEBUG  : Console (/SUBSYSTEM:CONSOLE)
    */
    #ifndef _DEBUG
        #define MINIVULKAN_MAIN __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
    #else
        #define MINIVULKAN_MAIN main(int argc, char* argv[])
    #endif

    /// The namespace can be quickly renamed to fix conflcits.
    #ifndef MINIVULKAN_NS
        #define MINIVULKAN_NS minivulkan
    #endif
    namespace MINIVULKAN_NS { /* Define Library Namespace */ }

    /// You can define MINIVULKCAN_SHORTREF for a shorter namespace reference. (:
    #ifdef MINIVULKAN_SHORTREF
        namespace minivk = minivulkan;
    #endif
    #ifdef MINIVULKAN_SHORTERREF
        namespace mvk = minivulkan;
    #endif

    #ifdef _DEBUG
        #define MVK_ENABLE_VALIDATION_LAYERS
        #define MVK_ENABLE_VALIDATION true
    #elif
        #define MVK_ENABLE_VALIDATION false
    #endif

    ////////////////////////////////////
    /////////// DEPENDENCIES ///////////
    #include <atomic>
    #include <functional>
    #include <fstream>
    #include <iostream>
    #include <iterator>
    #include <array>
    #include <map>
    #include <memory>
    #include <optional>
    #include <queue>
    #include <set>
    #include <string>
    #include <tuple>
    #include <vector>
    #include <stack>
    #include <unordered_set>

    // VULKAN DEVICE MEMORY ALLOCATOR //
    #define VMA_IMPLEMENTATION
    #ifdef _DEBUG
    #define VMA_RECORDING_ENABLED VK_TRUE
    #endif
    #define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
    #define VMA_DEBUG_GLOBAL_MUTEX VK_TRUE
    #define VMA_USE_STL_CONTAINERS VK_TRUE
    #define VMA_STATIC_VULKAN_FUNCTIONS VK_TRUE
    #define VMA_DYNAMIC_VULKAN_FUNCTIONS VK_TRUE
    #include "./VkMemAlloc.h"

    /////// QuiteOK Image Format ///////
    #define QOI_IMPLEMENTATION
    #include "./QuiteOkImageFormat.h"

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    // Convert PNG source images to the QOI format for faster encoding/decoding times vs PNG/BMP/JPG.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    
    /////////// DEPENDENCIES ///////////
    ////////////////////////////////////

    ////////////////////////////////////
    //// MINIVULKAN HEADER INCLUDES ////
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MiniVulkan follows a dependency chain where each next header include depends on includes before it.
    /// This helps to isolate the different Vulkan stages into individual sets for easy changes or study.
    /// Due to this includes do not have access to other includes further ahead in the dependency chain.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Allows for easy function callbacks and event handling.
    /* 00 */ #include "./Invokable.hpp"
    
    /// MiniVulkan object for memory disposal interface.
    /* 01 */ #include "./MvkObject.hpp"
    
    /// Basic Thread Pool (C++ is stupid--not provided in STL).
    /* 02 */ #include "./MvkThreadPool.hpp"

    /// Support classes for passing Vulkan information.
    /* 03 */ #include "./MvkSupportDetails.hpp"

    /// Graphics/Presentation queues (used for sending draw commands to GPUs).
    /// This can be modified for compute shaders (VK_QUEUE_COMPUTE_BIT). *NOT_SUPPORTED
    /* 04 */ #include "./MvkQueueFamily.hpp"

    /// The Vulkan instance which handles the DEVICE-HOST driver connection.
    /* 05 */ #include "./MvkInstance.hpp"

    /// Default (overridable) Window class implementation using GLFW.
    /* 06 */ #include "./MvkWindow.hpp"

    /// Swap Chains handle queuing framebuffers/vkimages(render images) for drawing and rendering to the screen.
    /// These images are borrowed from the device driver and are returned at the end of the rendering frame.
    /// You must create your own VkImages for offscreen rendering.
    /* 07 */ #include "./MvkSwapChain.hpp"

    /// Array of buffers for writing render/transfer commands to.
    /// Command Buffers (in the command pool) are used for all rendering and memory operations with the GPU.
    /// This includes both rendering to images using shaders and copying data from the HOST(PC/CPU) to the DEVICE(GPU).
    /* 08 */ #include "./MvkCommandPool.hpp"

    /// Vulkan Memory Allocator (Buffer / Image memory allocation on host/device)
    /* 09 */ #include "./MvkMemAlloc.hpp"

    /// Shader Info and Ordering for passing to the graphics pipeline for the final render.
    /* 10 */ #include "./MvkShaderStage.hpp"
    
    /// Buffers for sending different types of data (or indirect draw commands) to the GPU during rendering.
    /* 11 */ #include "./MvkBuffer.hpp"

    /// Represents a renderable image(drawing surface) in Vulkan with attached semaphores/fences (optional usage).
    /* 12 */ #include "./MvkImage.hpp"

    /// Dynamic Drawing/Rendering Pipeline (avoids [framebuffers/renderpasses/subpasses] which may be slower on mobile hardware).
    /* 13 */ #include "./MvkDyPipeline.hpp"

    /// Dynamic Renderer for actually drawing to the screen using a SwapChain or custom VkImage render targets.
    /* 14 */ #include "./MvkDyRenderer.hpp"

    /// Simple math functions for vertices, matrix transforms, etc.
    /* 15 */ #include "./MvkMath.hpp"
    //// MINIVULKAN HEADER INCLUDES ////
    ////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HOW TO USE MINIVULKAN: (In this order, every time) ////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///      0. Setup your main function as: int MINIVULKAN_MAIN {}
    ///      1. Create MvkInstance
    ///      2. Create MvkWindow
    ///      3. Initialize MvkInstance with MvkWindow.CreateSurface()
    ///      4. Create MvkSwapChain with appropriate buffering mode.
    ///      5. Add window.onResizeFrameBuffer += std::callback<int, int>(&swapChain, &MvkSwapChain::OnFrameBufferResizeCallback);
    ///         ** This creates a callback function for notifying the swapchain that gets called when the window is resized.
    /// 
    ///      6. swapChain.onGetFrameBufferSize += std::callback<int&, int&>(&window, &MvkWindow::OnFrameBufferReSizeCallback);
    ///         ** This create a callback function for updating the swapchain with the window size when it attempts to recreate itself.
    /// 
    ///      7. Create MvkMemAlloc (for runtime buffer/image memory management using VMA).
    ///      8. Create MvkCommandPool for processing render commands.
    ///      9. Create MvkShaderStages to specify the shaders in the render pipeline you wish to use.
    ///     10. Create MvkDyPipeline to specify the hwo the render pipeline will operate (vertex descriptions, push descriptors, push constants, etc.).
    ///     11. Create MvkDyRenderer to which will perform on-screen rendering with the appropriate command pool, swapChain, pipeline.
    ///     12. Subscribe any render callback functions to be called for every frame prior to rendering to the screen.
    ///         ** All commands must be written to command buffer with BeginRecordCommandBuffer before commands and after EndRecordCommandBuffer commands.
    ///     13. Set infinite loop that calls dyRenderer.RenderFrame() to render the window/screen. Set loop to exit on !window.ShouldClosePollEvents()
    ///         ** Alternatively setup the infinite loop on a thread with an exit call on !window.ShouldClose()
    ///             and then set an infinite loop on the main thread with an exit call on !window.ShouldClosePollEvents()
    ///         ** This will setup a basic multi-threaded window which won't block rendering when the window has to process OS events.
    ///     14. After the loop called mvkInstance.WaitIdleLogicalDevice() before disposing of any in-use resources.
    ///     15. Explicitly dispose of all Mvk* objects in reverse order that they were created.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif