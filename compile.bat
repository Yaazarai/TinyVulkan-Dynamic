:: Change the "1.3.211.0" to your Vulkan version.
:: Change "sample_shader" to the shader you want to compile.
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader.vert -o ./TinyVulkan/x64/Debug/sample_vert.spv
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader.frag -o ./TinyVulkan/x64/Debug/sample_frag.spv
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader.vert -o ./TinyVulkan/x64/Release/sample_vert.spv
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader.frag -o ./TinyVulkan/x64/Release/sample_frag.spv
pause