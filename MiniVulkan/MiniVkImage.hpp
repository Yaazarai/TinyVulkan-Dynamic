#ifndef MINIVULKAN_MINIVKIMAGE
#define MINIVULKAN_MINIVKIMAGE
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
		/*
			ABVOUT BUFFERS & IMAGES:
				When Creating Buffers:
					Buffers must be initialized with a VkDviceSize, which is the size of the data in BYTES, not the
					size of the data container (number of items). This same principle applies to stagging buffer data.

				There are 3 types of GPU dedicated memory buffers:
					Vertex:		Allows you to send mesh triangle data to the GPU.
					Index:		Allws you to send mapped indices for vertex buffers to the GPU.
					Uniform:	Allows you to send data to shaders using uniforms.
						* Push Constants are an alternative that do not require buffers, simply use: vkCmdPushConstants(...).

				The last buffer type is a CPU memory buffer for transfering data from the CPU to the GPU:
					Staging:	Staging CPU data for transfer to the GPU.

				Render images are for rendering sprites or textures on the GPU (similar to the swap chain, but handled manually).
					The default image layout is: VK_IMAGE_LAYOUT_UNDEFINED
					To render to shaders you must change/transition the layout to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
					Once the layout is set for transfering you can write data to the image from CPU memory to GPU memory.
					Finally for use in shaders you need to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
		*/

		class MiniVkImage : public std::disposable {
		private:
			MiniVkRenderDevice& renderDevice;
			MiniVkVMAllocator& vmAlloc;
						
			VkSamplerAddressMode addressingMode;

			void CreateImageView() {
				VkImageViewCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.image = image;
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = format;
				
				createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

				createInfo.subresourceRange.aspectMask = aspectFlags;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;

				if (vkCreateImageView(renderDevice.logicalDevice, &createInfo, nullptr, &imageView) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create MiniVkImage view!");
			}

			void CreateTextureSampler() {
				VkSamplerCreateInfo samplerInfo {};
				samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerInfo.magFilter = VK_FILTER_LINEAR;
				samplerInfo.minFilter = VK_FILTER_LINEAR;
				samplerInfo.addressModeU = addressingMode;
				samplerInfo.addressModeV = addressingMode;
				samplerInfo.addressModeW = addressingMode;
				samplerInfo.anisotropyEnable = VK_FALSE;
				
				VkPhysicalDeviceProperties properties{};
				vkGetPhysicalDeviceProperties(renderDevice.physicalDevice, &properties);
				samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

				samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
				samplerInfo.unnormalizedCoordinates = VK_FALSE;
				samplerInfo.compareEnable = VK_FALSE;
				samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
				samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				samplerInfo.mipLodBias = 0.0f;
				samplerInfo.minLod = 0.0f;
				samplerInfo.maxLod = 0.0f;

				if (vkCreateSampler(renderDevice.logicalDevice, &samplerInfo, nullptr, &imageSampler) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create image texture sampler!");
			}
		public:
			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				vkDestroySampler(renderDevice.logicalDevice, imageSampler, nullptr);
				vkDestroyImageView(renderDevice.logicalDevice, imageView, nullptr);
				vmaDestroyImage(vmAlloc.GetAllocator(), image, memory);
			}

			VmaAllocation memory = VK_NULL_HANDLE;
			VkImage image = VK_NULL_HANDLE;
			VkImageView imageView = VK_NULL_HANDLE;
			VkSampler imageSampler = VK_NULL_HANDLE;
			VkImageLayout layout;
			VkImageAspectFlags aspectFlags;

			VkDeviceSize width, height;
			VkFormat format;
			bool isDepthImage = false;

			MiniVkImage(MiniVkRenderDevice& renderDevice, MiniVkVMAllocator& vmAlloc, VkDeviceSize width, VkDeviceSize height, bool isDepthImage = false, VkFormat format = VK_FORMAT_B8G8R8A8_SRGB, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED, VkSamplerAddressMode addressingMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT)
			: renderDevice(renderDevice), vmAlloc(vmAlloc), width(width), height(height), isDepthImage(isDepthImage), format(format), layout(layout), addressingMode(addressingMode), aspectFlags(aspectFlags) {
				onDispose += std::callback<bool>(this, &MiniVkImage::Disposable);

				ReCreateImage(width, height, isDepthImage, format, layout, addressingMode, aspectFlags);
			}

			void ReCreateImage(VkDeviceSize width, VkDeviceSize height, bool isDepthImage = false, VkFormat format = VK_FORMAT_B8G8R8A8_SRGB, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED, VkSamplerAddressMode addressingMode = VK_SAMPLER_ADDRESS_MODE_REPEAT, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT) {
				VkImageCreateInfo imgCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
				imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
				imgCreateInfo.extent.width = static_cast<uint32_t>(width);
				imgCreateInfo.extent.height = static_cast<uint32_t>(height);
				imgCreateInfo.extent.depth = 1;
				imgCreateInfo.mipLevels = 1;
				imgCreateInfo.arrayLayers = 1;
				imgCreateInfo.format = format;
				imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				
				if (!isDepthImage) {
					imgCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				} else {
					imgCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				}

				VmaAllocationCreateInfo allocCreateInfo = {};
				allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
				allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
				allocCreateInfo.priority = 1.0f;

				if (vmaCreateImage(vmAlloc.GetAllocator(), &imgCreateInfo, &allocCreateInfo, &image, &memory, nullptr) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Could not allocate GPU image data for MiniVkImage!");

				CreateTextureSampler();
				CreateImageView();
			}

			void TransitionLayoutCmd(VkQueue graphicsQueue, VkCommandPool commandPool, VkImageLayout newLayout) {
				VkCommandBuffer commandBuffer = BeginTransferCmd(graphicsQueue, commandPool);

				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = layout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;
				barrier.subresourceRange.aspectMask = aspectFlags;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags sourceStage;
				VkPipelineStageFlags destinationStage;
				if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				} else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				} else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				} else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

					if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				} else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				} else {
					barrier.subresourceRange.aspectMask = aspectFlags;
					destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					throw std::invalid_argument("MiniVkImage: Unsupported layout transition!");
				}

				layout = newLayout;
				vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

				EndTransferCmd(graphicsQueue, commandPool, commandBuffer);
			}

			void TransferFromBufferCmd(VkQueue graphicsQueue, VkCommandPool commandPool, MiniVkBuffer& srcBuffer) {
				VkCommandBuffer commandBuffer = BeginTransferCmd(graphicsQueue, commandPool);

				VkBufferImageCopy region{};
				region.bufferOffset = 0;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;
				region.imageSubresource.aspectMask = aspectFlags;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
				vkCmdCopyBufferToImage(commandBuffer, srcBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				EndTransferCmd(graphicsQueue, commandPool, commandBuffer);
			}

			void StageImageData(VkQueue graphicsQueue, VkCommandPool commandPool, void* data, VkDeviceSize dataSize) {
				MiniVkBuffer stagingBuffer = MiniVkBuffer(renderDevice, vmAlloc, dataSize, MiniVkBufferType::VKVMA_BUFFER_TYPE_STAGING);
				memcpy(stagingBuffer.description.pMappedData, data, (size_t)dataSize);

				TransitionLayoutCmd(graphicsQueue, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				TransferFromBufferCmd(graphicsQueue, commandPool, stagingBuffer);
				TransitionLayoutCmd(graphicsQueue, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				stagingBuffer.Dispose();
			}

			VkCommandBuffer BeginTransferCmd(VkQueue graphicsQueue, VkCommandPool commandPool) {
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandPool = commandPool;
				allocInfo.commandBufferCount = 1;

				VkCommandBuffer commandBuffer;
				vkAllocateCommandBuffers(renderDevice.logicalDevice, &allocInfo, &commandBuffer);

				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(commandBuffer, &beginInfo);
				return commandBuffer;
			}

			void EndTransferCmd(VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
				vkEndCommandBuffer(commandBuffer);

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(graphicsQueue);
				vkFreeCommandBuffers(renderDevice.logicalDevice, commandPool, 1, &commandBuffer);
			}

			VkDescriptorImageInfo GetImageDescriptor() { return { imageSampler, imageView, layout }; }
		};
	}
#endif