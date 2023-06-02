:: Change the "1.3.211.0" to your Vulkan version.
:: Change "sample_shader" to the shader you want to compile.
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader.vert -o sample_vert.spv
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader.frag -o sample_frag.spv
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader_imageRenderer.vert -o sample_vert_imageRenderer.spv
C:/VulkanSDK/1.3.211.0/Bin/glslc.exe sample_shader_imageRenderer.frag -o sample_frag_imageRenderer.spv
pause