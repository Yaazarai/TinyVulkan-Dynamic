:: Change the "1.3.211.0" to your Vulkan version.
:: Change "sample_shader" to the shader you want to compile.
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader.vert -o vert.spv
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader.frag -o frag.spv
pause