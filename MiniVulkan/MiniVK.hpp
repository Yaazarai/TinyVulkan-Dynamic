#pragma once
#ifndef MINIVULKAN_LIBRARY
#define MINIVULKAN_LIBRARY

    /*
       MINIVULKAN LIBRARY DEPENDENCIES: VULKAN and GLFW compiled lbirary binaries.

        C/C++ | Code Configuration | Runtime Libraries:
            RELEASE: /MD
            DEBUG  : /mtD
        C/C++ | General | Additional Include Directories: (DEBUG & RELEASE)
            ..\VulkanSDK\1.3.239.0\Include;...\glfw-3.3.7.bin.WIN64\glfw-3.3.7.bin.WIN64\include;
        Linker | General | Additional Library Directories: (DEBUG & RELEASE)
            ...\glfw-3.3.7.bin.WIN64\lib-vc2022;$(VULKAN_SDK)\Lib; // C:\VulkanSDK\1.3.239.0\Lib
        Linker | Input | Additional Dependencies:
            RELEASE: vulkan-1.lib;glfw3.lib;
            DEBUG  : vulkan-1.lib;glfw3_mt.lib;
        Linker | System | SubSystem
            RELEASE: Windows (/SUBSYSTEM:WINDOWS)
            DEBUG  : Console (/SUBSYSTEM:CONSOLE)

        ** If Debug mode is enabled the console window will be visible, but not on Release.
        **** Make sure you've installed the Vulkan SDK binaries.
        **** Make sure you've installed the GLFW SDK binaries.
        
        DEVICE EXTENSIONS:
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
                VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
    */

    #define GLFW_INCLUDE_VULKAN
    #if defined (_WIN32)
        #define GLFW_EXPOSE_NATIVE_WIN32
        #define VK_USE_PLATFORM_WIN32_KHR
    #endif
    #include <GLFW/glfw3.h>
    #include <GLFW/glfw3native.h>
    #include <vulkan/vulkan.h>

    #ifdef _DEBUG
        #define MINIVULKAN_WINDOWMAIN main(int argc, char* argv[])
        #define MVK_VALIDATION_LAYERS VK_TRUE
    #else
        #ifdef _WIN32
            #define MINIVULKAN_WINDOWMAIN __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
        #else
            #define MINIVULKAN_WINDOWMAIN main(int argc, char* argv[])
        #endif
        #define MVK_VALIDATION_LAYERS VK_FALSE
    #endif
    #ifndef MINIVULKAN_NAMESPACE
        #define MINIVULKAN_NAMESPACE minivulkan
        namespace MINIVULKAN_NAMESPACE {}
        
        #ifdef MINIVULKAN_SHORTREF
            namespace mvk = MINIVULKAN_NAMESPACE;
        #endif
    #endif

    #define MVK_MAKE_VERSION(variant, major, minor, patch) ((((uint32_t)variant)<<29)|(((uint32_t)major)<<22)|(((uint32_t)minor)<<12)|((uint32_t)patch))
    #define MVK_RENDERER_VERSION MVK_MAKE_VERSION(0, 1, 1, 0)
    #define MVK_RENDERER_NAME "MINIVULKAN_LIBRARY"

    #define VMA_IMPLEMENTATION
    #define VMA_DEBUG_GLOBAL_MUTEX VK_TRUE
    #define VMA_USE_STL_CONTAINERS VK_TRUE
    #define VMA_RECORDING_ENABLED MVK_VALIDATION_LAYERS
    #include <vma/vk_mem_alloc.h>

    #define GLM_FORCE_RADIANS
    #define GLM_FORCE_LEFT_HANDED
    #define GLM_FORCE_DEPTH_ZERO_TO_ONE
    #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    #include <glm/glm.hpp>
    #include <glm/ext.hpp>

    #include <fstream>
    #include <iostream>
    #include <array>
    #include <queue>
    #include <set>
    #include <string>
    #include <vector>
    #include <algorithm>
    #include <chrono>

    #include "./atomic_lock.hpp"
    #include "./disposable.hpp"
    
    /// You can define this macro to throw a new std::runtime error when a duplicate hook is made.
    #define INVOKABLE_ERRORON_DUPLICATEHOOKS
    #include "./invokable.hpp"

    #include "./MiniVkThreadPool.hpp"
    #include "./MiniVkWindow.hpp"

    #include "./MiniVkSupporters.hpp"
    #include "./MiniVkQueueFamily.hpp"
    #include "./MiniVkInstance.hpp"
    #include "./MiniVkRenderDevice.hpp"
    #include "./MiniVkSwapChain.hpp"
    #include "./MiniVkCommandPool.hpp"
    #include "./MiniVkVMAllocator.hpp"
    #include "./MiniVkBuffer.hpp"
    #include "./MiniVkImage.hpp"
    #include "./MiniVkShaderStages.hpp"
    #include "./MiniVkDynamicPipeline.hpp"
    #include "./MiniVkDynamicRenderer.hpp"
    #include "./MiniVkVertexMath.hpp"
    
#endif

/// 
/// Rendering Pipeline Order:
///     0. Determine MiniVkBufferingMode.
///     1. Create MiniVkWindow
///     2. Create MiniVkInstance
///     3. Create MiniVkRenderDevice with window surface.
///     4. Create MiniVkVMAllocator
///     5. Create MiniVkCommandPool (size is swapchain buffering mode).
///     6. Create MiniVkSwapChain with MiniVkSurfaceSupporter() and buffering mode.
///     7. Create MiniVkShaderStages for each graphics pipeline.
///     8. Create MiniVkDynamicPipeline for each graphics pipeline w/ their repsective shader stages.
///     9. Create MiniVkSwapChainRenderer for rendering to the screen.
///     10. Create MiniVkImageRenderer if rendering to a specific MiniVkImage(VkImage) target.
///         a. This is optional if you plan on rendering directly to a specific MiniVkImage instead of the swap chain.
///             If so create a new MiniVkImage as your render target.
///         b. If using a MiniVkImageRenderer you will need a separate MiniVkCommandPool and MiniVkCommandPoolQueue
///             for any and all render operations separate from the swapchain command pool.
///         c. Either get a command buffer from your command pool queue and pre-record it
///             or pass a render event to myMiniVkImageRenderer.onRenderEvents and record on execute.
///     11. Add a callback to yoru swapchain renderer myMiniVkSwapChainRenderer.onRenderEvents.
///         You may use a lambda, static or instance method for the callback--lambdas are superior.
///         This event on the swap chain renderer is critical for rendering to the screen.
///     12. If necessary create a new thread or retrieve one from a thread pool and execute your render functions.
///         myMiniVkSwapChainRenderer.RenderExecute(...) or myMiniVkImageRenderer.RenderExecute(...).
///         Make sure to check if the window should close to stop any executing threads when the program fails.
///         Make sure to join threads as needed after program closes (window should close).
///     13. Or add these render functions to your window's onWhileMain render event for single-threaded rendering.
/// 