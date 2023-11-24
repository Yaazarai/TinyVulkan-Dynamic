#pragma once
#ifndef TINYVK_TINYVKSWAPCHAIN
#define TINYVK_TINYVKSWAPCHAIN
	#include "./TinyVK.hpp"

	namespace TINYVULKAN_NAMESPACE {
		/// <summary>SwapChain (screen buffering) description and layout for rendering to the window via TinyVkSwapChainRenderer.</summary>
		class TinyVkSwapChain : public disposable {
		private:
			/// <summary>Create the Vulkan surface swap-chain images and imageviews.</summary>
			void CreateSwapChainImages(uint32_t width = 0, uint32_t height = 0) {
				TinyVkSwapChainSupporter swapChainSupport = QuerySwapChainSupport(renderDevice.physicalDevice);
				VkSurfaceFormatKHR surfaceFormat = QuerySwapSurfaceFormat(swapChainSupport.formats);
				VkPresentModeKHR presentMode = QuerySwapPresentMode(swapChainSupport.presentModes);
				VkExtent2D extent = QuerySwapExtent(swapChainSupport.capabilities);
				uint32_t imageCount = std::min(swapChainSupport.capabilities.maxImageCount, std::max(swapChainSupport.capabilities.minImageCount, static_cast<uint32_t>(bufferingMode)));

				if (width != 0 && height != 0) {
					extent = {
						std::min(std::max((uint32_t)width, swapChainSupport.capabilities.minImageExtent.width), swapChainSupport.capabilities.maxImageExtent.width),
						std::min(std::max((uint32_t)height, swapChainSupport.capabilities.minImageExtent.height), swapChainSupport.capabilities.maxImageExtent.height)
					};
				} else {
					extent = QuerySwapExtent(swapChainSupport.capabilities);
				}

				if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
					imageCount = swapChainSupport.capabilities.maxImageCount;

				VkSwapchainCreateInfoKHR createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				createInfo.surface = renderDevice.presentationSurface;
				createInfo.minImageCount = imageCount;
				createInfo.imageFormat = surfaceFormat.format;
				createInfo.imageColorSpace = surfaceFormat.colorSpace;
				createInfo.imageExtent = extent;
				createInfo.imageArrayLayers = 1; // Change when developing VR or other 3D stereoscopic applications
				createInfo.imageUsage = imageUsage;

				TinyVkQueueFamily indices = TinyVkQueueFamily::FindQueueFamilies(renderDevice.physicalDevice, renderDevice.presentationSurface);
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
				createInfo.oldSwapchain = swapChain;

				if (vkCreateSwapchainKHR(renderDevice.logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to create swap chain!");

				vkGetSwapchainImagesKHR(renderDevice.logicalDevice, swapChain, &imageCount, nullptr);
				images.resize(imageCount);
				vkGetSwapchainImagesKHR(renderDevice.logicalDevice, swapChain, &imageCount, images.data());

				imageFormat = surfaceFormat.format;
				imageExtent = extent;
			}

			/// <summary>Create the image views for rendering to images (including those in the swap-chain).</summary>
			void CreateSwapChainImageViews(VkImageViewCreateInfo* createInfoEx = nullptr) {
				imageViews.resize(images.size());

				for (size_t i = 0; i < images.size(); i++) {
					VkImageViewCreateInfo createInfo{};

					if (createInfoEx == nullptr) {
						createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
						createInfo.image = images[i];
						createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
						createInfo.format = imageFormat;

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

					if (vkCreateImageView(renderDevice.logicalDevice, &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS)
						throw std::runtime_error("TinyVulkan: Failed to create swap chain image views!");
				}
			}

			/// <summary>Create the Vulkan surface swapchain.</summary>
			void CreateSwapChain(uint32_t width = 0, uint32_t height = 0) {
				CreateSwapChainImages(width, height);
				CreateSwapChainImageViews();
			}

			/// <summary>Checks the VkPhysicalDevice for swap-chain availability.</summary>
			TinyVkSwapChainSupporter QuerySwapChainSupport(VkPhysicalDevice device) {
				TinyVkSwapChainSupporter details;

				uint32_t formatCount;
				VkSurfaceKHR windowSurface = renderDevice.presentationSurface;
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
				renderWindow.OnFrameBufferReSizeCallback(width, height);

				VkExtent2D extent = {
					std::min(std::max((uint32_t)width, capabilities.minImageExtent.width), capabilities.maxImageExtent.width),
					std::min(std::max((uint32_t)height, capabilities.minImageExtent.height), capabilities.maxImageExtent.height)
				};

				extent.width = std::max(1u, extent.width);
				extent.height = std::max(1u, extent.height);
				return extent;
			}
			
		public:
			TinyVkRenderDevice& renderDevice;
			TinyVkWindow& renderWindow;

			std::timed_mutex swapChainMutex;
			TinyVkSurfaceSupporter presentDetails;
			VkSwapchainKHR swapChain = nullptr;
			VkFormat imageFormat;
			VkExtent2D imageExtent;
			VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			const TinyVkBufferingMode bufferingMode;
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;

			inline static invokable<int, int> onResizeFrameBuffer;
			std::atomic<bool> presentable;

			~TinyVkSwapChain() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				for (auto imageView : imageViews)
					vkDestroyImageView(renderDevice.logicalDevice, imageView, nullptr);

				vkDestroySwapchainKHR(renderDevice.logicalDevice, swapChain, nullptr);
			}

			/// <summary>Creates the swapchain and buffering images for presenting to the screen.</summary>
			TinyVkSwapChain(TinyVkRenderDevice& renderDevice, TinyVkWindow& renderWindow, const TinyVkBufferingMode bufferingMode, TinyVkSurfaceSupporter presentDetails = TinyVkSurfaceSupporter(), VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			: renderDevice(renderDevice), renderWindow(renderWindow), bufferingMode(bufferingMode), presentDetails(presentDetails), imageUsage(imageUsage), presentable(true) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				renderWindow.onResizeFrameBuffer.hook(callback<GLFWwindow*, int, int>([this](GLFWwindow* w, int x, int y) { this->OnFrameBufferResizeCallback(w, x, y); }));

				CreateSwapChain();
			}

			TinyVkSwapChain operator=(const TinyVkSwapChain& swapChain) = delete;

			/// <summary>Acquires the next image from the swap chain and returns out that image index.</summary>
			VkResult AcquireNextImage(VkSemaphore semaphore, VkFence fence, uint32_t& imageIndex) {
				return vkAcquireNextImageKHR(renderDevice.logicalDevice, swapChain, UINT64_MAX, semaphore, fence, &imageIndex);
			}

			/// <summary>[overridable] Notify the render engine that the window's frame buffer has been resized.</summary>
			void OnFrameBufferResizeCallback(GLFWwindow* hwndWindow, int width, int height) {
				if (hwndWindow != renderWindow.GetHandle()) return;

				if (width > 0 && height > 0) {
					timed_guard<false> swapChainLock(swapChainMutex);
					if (!swapChainLock.Acquired()) return;

					vkDeviceWaitIdle(renderDevice.logicalDevice);

					for (auto imageView : imageViews)
						vkDestroyImageView(renderDevice.logicalDevice, imageView, nullptr);
					
					VkSwapchainKHR oldSwapChain = swapChain;
					CreateSwapChain(width, height);
					vkDestroySwapchainKHR(renderDevice.logicalDevice, oldSwapChain, nullptr);

					presentable = true;
					onResizeFrameBuffer.invoke(imageExtent.width, imageExtent.height);
				}
			}
		};
	}
#endif