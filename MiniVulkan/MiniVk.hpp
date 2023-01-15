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

    #include <vulkan/vulkan.hpp>
    
    // GLM (OpenGL math) properties.
    #define GLM_FORCE_RADIANS
    #define GLM_FORCE_LEFT_HANDED
    #define GLM_FORCE_DEPTH_ZERO_TO_ONE
    #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
    
    #include <glm/glm.hpp>
    #include <glm/vec2.hpp>
    #include <glm/vec4.hpp>
    #include <glm/mat2x2.hpp>

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
    /* 01 */ #include "./Invokable.hpp"
    
    /// MiniVulkan object for memory disposal interface.
    /* 02 */ #include "./MiniVkObject.hpp"
    
    /// Basic Thread Pool (C++ is stupid--not provided in STL).
    /* 03 */ #include "./MiniVkThreadPool.hpp"
    
    /// Default (overridable) Window class implementation using GLFW.
    /* 04 */ #include "./MiniVkWindow.hpp"

    /// Support classes for passing Vulkan information.
    /* 05 */ #include "./MiniVkSupportDetails.hpp"

    /// Graphics/Presentation queues (used for sending draw commands to GPUs).
    /// This can be modified for compute shaders (VK_QUEUE_COMPUTE_BIT). *NOT_SUPPORTED
    /* 06 */ #include "./MiniVkQueueFamily.hpp"

    /// The Vulkan instance which handles the DEVICE-HOST driver connection.
    /* 07 */ #include "./MiniVkInstance.hpp"

    /// Swap Chains handle queuing framebuffers/vkimages(render images) for drawing and rendering to the screen.
    /// These images are borrowed from the device driver and are returned at the end of the rendering frame.
    /// You must create your own VkImages for offscreen rendering.
    /* 09 */ #include "./MiniVkSwapChain.hpp"

    /// Array of buffers for writing render/transfer commands to.
    /// Command Buffers (in the command pool) are used for all rendering and memory operations with the GPU.
    /// This includes both rendering to images using shaders and copying data from the HOST(PC/CPU) to the DEVICE(GPU).
    /* 10 */ #include "./MiniVkCommandPool.hpp"

    /// Vulkan Memory Allocator (Buffer / Image memory allocation on host/device)
    /// Buffer, Vertex and Uniform Buffers for shaders.
    /// MiniVkBuffer is the base-derivable buffer class for handling memory operations with the GPU.
    ///     *** All MiniVkBuffer operations are STAGED (see header file for documentation info).
    /// Vertex buffers provide an interface for passing vertex model information to shaders.
    /// Uniform buffers provide an interface for passing shader constant information to shaders.
    /* 08 */ #include "./MiniVkMemAlloc.hpp"

    /// Shader Info and Ordering for passing to the graphics pipeline for the final render.
    /* 12 */ #include "./MiniVkShaderStage.hpp"
    
    /// Dynamic Drawing/Rendering Pipeline (avoids [framebuffers/renderpasses/subpasses] which may be slower on mobile hardware).
    /* 13 */ #include "./MiniVkDynamicPipeline.hpp"

    /// Dynamic Renderer for actually drawing to the screen using a SwapChain or custom VkImage render targets.
    /* 14 */ #include "./MiniVkDynamicRenderer.hpp"

    /// Represents a renderable image(drawing surface) in Vulkan with attached semaphores/fences (optional usage).
    /* 15 */ #include "./MiniVkImage.hpp"

    //// MINIVULKAN HEADER INCLUDES ////
    ////////////////////////////////////
#endif