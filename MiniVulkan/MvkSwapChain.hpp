#pragma once
#ifndef MINIVK_MINIVKSWAPCHAIN
#define MINIVK_MINIVKSWAPCHAIN
	#include "./MiniVulkan.hpp"

	namespace MINIVULKAN_NS {
		class MvkSwapChain : public MvkObject {
		private:
			/// VKINSTANCE_INFO ///
			MvkInstance& mvkInstance;
		public:
			/// VKSURFACE FOR PRESENTATION ///
			MvkSurfaceSupporter presentDetails;

			/// INVOKABLE EVENTS ///
			std::invokable<int&, int&> onGetFrameBufferSize;
			bool framebufferResized = false;

			/// SWAP_CHAINS ///
			VkSwapchainCreateInfoKHR createInfo;
			VkSwapchainKHR swapChain = VK_NULL_HANDLE;
			VkFormat swapChainImageFormat;
			VkExtent2D swapChainExtent;
			VkImageUsageFlags swapChainImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			const MvkBufferingMode bufferingMode;
			size_t currentFrame = 0;
			std::vector<VkImage> swapChainImages;
			std::vector<VkImageView> swapChainImageViews;

			void Disposable() {
				vkDeviceWaitIdle(mvkInstance.logicalDevice);

				for (auto imageView : swapChainImageViews)
					vkDestroyImageView(mvkInstance.logicalDevice, imageView, nullptr);

				vkDestroySwapchainKHR(mvkInstance.logicalDevice, swapChain, nullptr);
			}

			MvkSwapChain(MvkInstance& mvkInstance, MvkSurfaceSupporter presentDetails, MvkBufferingMode bufferingMode = MvkBufferingMode::TRIPLE, VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			: mvkInstance(mvkInstance), bufferingMode(bufferingMode), presentDetails(presentDetails), swapChainImageUsage(imageUsage) {
				onDispose += std::callback<>(this, &MvkSwapChain::Disposable);
				CreateSwapChain();
			}

			MvkSwapChain operator=(const MvkSwapChain& swapChain) = delete;

			/// <summary>Create the Vulkan surface swapchain.</summary>
			void CreateSwapChain() {
				vkDeviceWaitIdle(mvkInstance.logicalDevice);
				CreateSwapChainImages();
				CreateSwapChainImageViews();
			}

			/// <summary>Re-creates the Vulkan surface swap-chain & resets the currentFrame to 0.</summary>
			void ReCreateSwapChain() {
				int w, h;
				onGetFrameBufferSize.invoke(w, h);

				vkDeviceWaitIdle(mvkInstance.logicalDevice);
				createInfo.oldSwapchain = swapChain;
				Disposable();
				CreateSwapChain();
				currentFrame = 0;
			}

			/// <summary>Create the Vulkan surface swap-chain images and imageviews.</summary>
			void CreateSwapChainImages() {
				MvkSwapChainSupporter swapChainSupport = QuerySwapChainSupport(mvkInstance.physicalDevice);
				VkSurfaceFormatKHR surfaceFormat = QuerySwapSurfaceFormat(swapChainSupport.formats);
				VkPresentModeKHR presentMode = QuerySwapPresentMode(swapChainSupport.presentModes);
				VkExtent2D extent = QuerySwapExtent(swapChainSupport.capabilities);
				uint32_t imageCount = MIN(swapChainSupport.capabilities.maxImageCount, MAX(swapChainSupport.capabilities.minImageCount, static_cast<uint32_t>(bufferingMode)));
				
				while(extent.width == 0u || extent.height == 0u) {
					int w, h; onGetFrameBufferSize.invoke(w, h);
					swapChainSupport = QuerySwapChainSupport(mvkInstance.physicalDevice);
					surfaceFormat = QuerySwapSurfaceFormat(swapChainSupport.formats);
					presentMode = QuerySwapPresentMode(swapChainSupport.presentModes);
					extent = QuerySwapExtent(swapChainSupport.capabilities);
				}

				if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
					imageCount = swapChainSupport.capabilities.maxImageCount;

				VkSwapchainCreateInfoKHR createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				createInfo.surface = mvkInstance.presentationSurface;
				createInfo.minImageCount = imageCount;
				createInfo.imageFormat = surfaceFormat.format;
				createInfo.imageColorSpace = surfaceFormat.colorSpace;
				createInfo.imageExtent = extent;
				createInfo.imageArrayLayers = 1; // Change when developing VR or other 3D stereoscopic applications
				createInfo.imageUsage = swapChainImageUsage;

				MvkQueueFamily indices = MvkQueueFamily::FindQueueFamilies(mvkInstance.physicalDevice, mvkInstance.presentationSurface);
				uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

				if (indices.graphicsFamily != indices.presentFamily) {
					createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
					createInfo.queueFamilyIndexCount = 2;
					createInfo.pQueueFamilyIndices = queueFamilyIndices;
				} else {
					createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
					createInfo.queueFamilyIndexCount = 0; // Optional
					createInfo.pQueueFamilyIndices = nullptr; // Optional
				}

				createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
				createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
				createInfo.presentMode = presentMode;
				createInfo.clipped = VK_TRUE;

				if (vkCreateSwapchainKHR(mvkInstance.logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create swap chain!");

				vkGetSwapchainImagesKHR(mvkInstance.logicalDevice, swapChain, &imageCount, nullptr);
				swapChainImages.resize(imageCount);
				vkGetSwapchainImagesKHR(mvkInstance.logicalDevice, swapChain, &imageCount, swapChainImages.data());

				swapChainImageFormat = surfaceFormat.format;
				swapChainExtent = extent;
			}

			/// <summary>Create the image views for rendering to images (including those in the swap-chain).</summary>
			void CreateSwapChainImageViews(VkImageViewCreateInfo* createInfoEx = nullptr) {
				swapChainImageViews.resize(swapChainImages.size());

				for (size_t i = 0; i < swapChainImages.size(); i++) {
					VkImageViewCreateInfo createInfo{};

					if (createInfoEx == nullptr) {
						createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
						createInfo.image = swapChainImages[i];
						createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
						createInfo.format = swapChainImageFormat;

						createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
						createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
						createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
						createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

						createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						createInfo.subresourceRange.baseMipLevel = 0;
						createInfo.subresourceRange.levelCount = 1;
						createInfo.subresourceRange.baseArrayLayer = 0;
						createInfo.subresourceRange.layerCount = 1;
					} else { createInfo = *createInfoEx; }

					if (vkCreateImageView(mvkInstance.logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
						throw std::runtime_error("MiniVulkan: Failed to create swap chain image views!");
				}
			}

			/// <summary>Checks the VkPhysicalDevice for swap-chain availability.</summary>
			MvkSwapChainSupporter QuerySwapChainSupport(VkPhysicalDevice device) {
				MvkSwapChainSupporter details;

				uint32_t formatCount;
				VkSurfaceKHR windowSurface = mvkInstance.presentationSurface;
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatCount, nullptr);
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, windowSurface, &details.capabilities);

				if (formatCount > 0) {
					details.formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(device, windowSurface, &formatCount, details.formats.data());
				}

				uint32_t presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount, nullptr);

				if (presentModeCount != 0) {
					details.presentModes.resize(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(device, windowSurface, &presentModeCount, details.presentModes.data());
				}

				return details;
			}

			/// <summary>Gets the swap-chain surface format.</summary>
			VkSurfaceFormatKHR QuerySwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
				for (const auto& availableFormat : availableFormats)
					if (availableFormat.format == presentDetails.dataFormat && availableFormat.colorSpace == presentDetails.colorSpace)
						return availableFormat;

				return availableFormats[0];
			}

			/// <summary>Select the swap-chains active present mode.</summary>
			VkPresentModeKHR QuerySwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
				for (const auto& availablePresentMode : availablePresentModes)
					if (availablePresentMode == presentDetails.idealPresentMode)
						return availablePresentMode;

				return VK_PRESENT_MODE_FIFO_KHR;
			}

			/// <summary>Select swap-chain extent (swap-chain surface resolution).</summary>
			VkExtent2D QuerySwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
				int width, height;
				onGetFrameBufferSize.invoke(width, height);

				VkExtent2D actualExtent {
					MIN(MAX((uint32_t)width, capabilities.minImageExtent.width), capabilities.maxImageExtent.width),
					MIN(MAX((uint32_t)height, capabilities.minImageExtent.height), capabilities.maxImageExtent.height)
				};

				return actualExtent;
			}

			/// <summary>Acquires the next image from the swap chain and returns out that image index.</summary>
			VkResult AcquireNextImage(VkSemaphore semaphore, VkFence fence, uint32_t& imageIndex) {
				return vkAcquireNextImageKHR(mvkInstance.logicalDevice, swapChain, UINT64_MAX, semaphore, fence, &imageIndex);
			}

			/// <summary>Returns the current swap chain image.</summary>
			VkImage& CurrentImage() { return swapChainImages[currentFrame]; }

			/// <summary>Returns the current swap chain image view.</summary>
			VkImageView& CurrentImageView() { return swapChainImageViews[currentFrame]; }

			VkExtent2D CurrentExtent2D() { return swapChainExtent; }

			size_t SelectCurrentIndex() { return currentFrame; }

			/// <summary>[overridable] Notify the render engine that the window's frame buffer has been resized.</summary>
			void OnFrameBufferResizeCallback(int width, int height) { SetFrameBufferResized(true); }

			/// <summary>Notifies the swap chain of the presentation framebuffer resize status.</summary>
			void SetFrameBufferResized(bool resized) { framebufferResized = resized; }
		};
	}
#endif