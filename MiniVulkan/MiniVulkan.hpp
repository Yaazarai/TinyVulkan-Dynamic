#pragma once
#ifndef MINIVULKAN_LIBRARY
#define MINIVULKAN_LIBRARY
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
    #include <functional>
    #include <map>
    #include <memory>
    #include <iostream>
    #include <iterator>
    #include <string>
    #include <vector>
    #include <optional>
    #include <set>
    #include <tuple>
    #include <fstream>
    /////////// DEPENDENCIES ///////////
    ////////////////////////////////////
    
    ////////////////////////////////////
    //// MINIVULKAN HEADER INCLUDES ////
    #include "./Invokable.hpp"
    #include "./Window.hpp"
    #include "./MiniVkLayer.hpp"
    //// MINIVULKAN HEADER INCLUDES ////
    ////////////////////////////////////
#endif