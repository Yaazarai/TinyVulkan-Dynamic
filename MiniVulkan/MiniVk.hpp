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
    #include <glm/vec4.hpp>
    #include <glm/mat4x4.hpp>

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
    /// 
    /// Define MINIVULKAN_
    /// 
    #ifndef MINIVULKAN_NS
    #define MINIVULKAN_NS minivulkan
    #endif
    namespace MINIVULKAN_NS { /* Define Library Namespace */ }

    /// You can define MINIVULKCAN_SHORTREF for a shorter namespace reference. (:
    #ifdef MINIVULKAN_SHORTREF
    namespace minivk = minivulkan;
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
    /////////// DEPENDENCIES ///////////
    ////////////////////////////////////

    ////////////////////////////////////
    //// MINIVULKAN HEADER INCLUDES ////
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MiniVulkan follows a dependency chain where each next header include depends on includes before it.
    /// This helps to isolate the different Vulkan stages into individual sets for easy changes or study.
    /// Due to this includes do nto have access to other includes further ahead in the dependency chain.
    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    /// Allows for easy function callbacks and event handling.
    /* 00 */ #include "./Invokable.hpp"
    
    /// MiniVulkan object for memory disposal interface.
    /* 01 */ #include "./MiniVkObject.hpp"
    
    /// Basic Thread Pool (C++ is stupid--not provided in STL).
    /* 02 */ #include "./MiniVkThreadPool.hpp"
    
    /// Default (overridable) Window class implementation using GLFW.
    /* 03 */ #include "./MiniVkWindow.hpp"

    /// Support classes for passing Vulkan information.
    /* 04 */ #include "./MiniVkSupportDetails.hpp"

    /// Graphics/Presentation queues (used for sending draw commands to GPUs).
    /// This can be modified for compute shaders (VK_QUEUE_COMPUTE_BIT). *NOT_SUPPORTED
    /* 05 */ #include "./MiniVkQueueFamily.hpp"

    /// The Vulkan instance which handles the DEVICE-HOST driver connection.
    /* 12 */ #include "./MiniVkInstance.hpp"

    /// Swap Chains handle queuing framebuffers/vkimages(render images) for drawing and rendering to the screen.
    /// These images are borrowed from the device driver and are returned at the end of the rendering frame.
    /// You must create your own VkImages for offscreen rendering.
    /* 06 */ #include "./MiniVkSwapChain.hpp"

    /// Array of buffers for writing render/transfer commands to.
    /// Command Buffers (in the command pool) are used for all rendering and memory operations with the GPU.
    /// This includes both rendering to images using shaders and copying data from the HOST(PC/CPU) to the DEVICE(GPU).
    /* 07 */ #include "./MiniVkCommandPool.hpp"

    /// Buffer, Vertex and Uniform Buffers for shaders.
    /// MiniVkBuffer is the base-derivable buffer class for handling memory operations with the GPU.
    ///     *** All MiniVkBuffer operations are STAGED (see header file for documentation info).
    /// Vertex buffers provide an interface for passing vertex model information to shaders.
    /// Uniform buffers provide an interface for passing shader constant information to shaders.
    /* 08 */ #include "./MiniVkShaderBuffers.hpp"

    /// Shader Info and Ordering for passing to the graphics pipeline for the final render.
    /* 09 */ #include "./MiniVkShaderStage.hpp"
    
    /// Dynamic Drawing/Rendering Pipeline (avoids [framebuffers/renderpasses/subpasses] which may be slower on mobile hardware).
    /* 10 */ #include "./MiniVkDynamicPipeline.hpp"
    
    /// Dynamic Renderer for actually drawing to the screen using a SwapChain or custom VkImage render targets.
    /* 11 */ #include "./MiniVkDynamicRenderer.hpp"

    //// MINIVULKAN HEADER INCLUDES ////
    ////////////////////////////////////
#endif