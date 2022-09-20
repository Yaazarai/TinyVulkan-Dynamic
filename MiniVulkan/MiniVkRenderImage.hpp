#ifndef MINIVK_MINIVKRENDERIMAGE
#define MINIVK_MINIVKRENDERIMAGE
	#include "./MiniVk.hpp"

	namespace MINIVULKAN_NS {
		#pragma region DYNAMIC RENDERING FUNCTIONS

		VkResult vkCmdBeginRenderingEKHR(VkInstance instance, VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo) {
			auto func = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
			if (func == VK_NULL_HANDLE) throw std::runtime_error("MiniVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdBeginRenderingKHR");
			func(commandBuffer, pRenderingInfo);
			return VK_SUCCESS;
		}

		VkResult vkCmdEndRenderingEKHR(VkInstance instance, VkCommandBuffer commandBuffer) {
			auto func = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");
			if (func == VK_NULL_HANDLE) throw std::runtime_error("MiniVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdEndRenderingKHR");
			func(commandBuffer);
			return VK_SUCCESS;
		}

		#pragma endregion

		class MiniVkRenderImage : public MiniVkObject {
		private:
			MiniVkInstance& mvkLayer;

			#pragma region VKIMAGE CREATION

			uint32_t QueryMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
				VkPhysicalDeviceMemoryProperties memProperties;
				vkGetPhysicalDeviceMemoryProperties(mvkLayer.physicalDevice, &memProperties);

				for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
					if (typeFilter & (1 << i)) return i;

				throw std::runtime_error("MiniVulkan: Failed to find suitable memory type for vertex buffer!");
			}

			void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageTiling tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL) {
				VkImageCreateInfo imageInfo{};
				imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				imageInfo.imageType = VK_IMAGE_TYPE_2D;
				imageInfo.extent.width = width;
				imageInfo.extent.height = height;
				imageInfo.extent.depth = 1;
				imageInfo.mipLevels = 1;
				imageInfo.arrayLayers = 1;
				imageInfo.format = format;
				imageInfo.tiling = tiling;
				imageInfo.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//VK_IMAGE_LAYOUT_UNDEFINED;
				imageInfo.usage = usage;
				imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if (vkCreateImage(mvkLayer.logicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create vkimage!");

				VkMemoryRequirements memRequirements;
				vkGetImageMemoryRequirements(mvkLayer.logicalDevice, image, &memRequirements);

				VkMemoryAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = QueryMemoryType(memRequirements.memoryTypeBits, properties);

				if (vkAllocateMemory(mvkLayer.logicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to allocate vkimage memory!");

				vkBindImageMemory(mvkLayer.logicalDevice, image, imageMemory, 0);
			}

			void CreateSyncObjects() {
				VkSemaphoreCreateInfo semaphoreInfo{};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VkFenceCreateInfo fenceInfo{};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

				if (vkCreateSemaphore(mvkLayer.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
					vkCreateSemaphore(mvkLayer.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
					vkCreateFence(mvkLayer.logicalDevice, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
						throw std::runtime_error("MiniVulkan: Failed to create synchronization objects for a frame!");
			}

			#pragma endregion
		public:
			VkImage image;
			VkImageView imageView;
			VkDeviceMemory memory;
			VkSemaphore imageAvailableSemaphore;
			VkSemaphore renderFinishedSemaphore;
			VkFence inFlightFence;

			void Disposable() {
				vkDestroySemaphore(mvkLayer.logicalDevice, imageAvailableSemaphore, nullptr);
				vkDestroySemaphore(mvkLayer.logicalDevice, renderFinishedSemaphore, nullptr);
				vkDestroyFence(mvkLayer.logicalDevice, inFlightFence, nullptr);

				vkDestroyImage(mvkLayer.logicalDevice, image, nullptr);
				vkDestroyImageView(mvkLayer.logicalDevice, imageView, nullptr);
				vkFreeMemory(mvkLayer.logicalDevice, memory, nullptr);
			}

			MiniVkRenderImage(MiniVkInstance& mvkLayer, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage,
				VkMemoryPropertyFlags properties, VkImageTiling tiling = VkImageTiling::VK_IMAGE_TILING_OPTIMAL) : mvkLayer(mvkLayer) {
				onDispose += std::callback<>(this, &MiniVkRenderImage::Disposable);

				CreateImage(width, height, format, usage, properties, image, memory, tiling);
				MiniVkSurfaceSupportDetails imageDetails;
				VkImageViewCreateInfo imageViewCreateInfo{};
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfo.image = image;
				imageViewCreateInfo.format = imageDetails.dataFormat;
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfo.pNext = VK_NULL_HANDLE;
				imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				vkCreateImageView(mvkLayer.logicalDevice, &imageViewCreateInfo, VK_NULL_HANDLE, &imageView);
				CreateSyncObjects();
			}
		};
	}
#endif