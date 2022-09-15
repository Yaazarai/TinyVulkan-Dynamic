/*
* About MiniVkLayer
*	Most functions are commented [overridable] and marked as virtual so as to be overridable for personalized implementations.
*		* except debug/validation layer support.
*	
*	debugCallback can be used when compiling for _DEBUG to check for errors.
*/
#pragma once
#ifndef MINIVULKAN_MINIVKLAYER
#define MINIVULKAN_MINIVKLAYER
	#include "./MiniVulkan.hpp"
	#include "./Window.hpp"
	#include "./Invokable.hpp"
	
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

	namespace MINIVULKAN_NS {
		#pragma region DEBUG_UTILITIES
		
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) {
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else { return VK_ERROR_EXTENSION_NOT_PRESENT; }
		}

		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) func(instance, debugMessenger, pAllocator);
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
			std::cerr << "MiniVulkan: Validation Layer: " << pCallbackData->pMessage << std::endl;
			return VK_FALSE;
		}
		
		#pragma endregion

		class SwapChainSupportDetails {
		public:
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		class QueueFamilyIndices {
		public:
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool IsComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
		};
		
		class MiniVkLayer {
		private:
			#ifdef _DEBUG
			const bool enableValidationLayers = true;
			#else
			const bool enableValidationLayers = false;
			#endif

			const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
			const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

			/// VKINSTANCE_INITIATION_DESTRUCTION ///
			bool isMemoryDestroyed = false;
			VkApplicationInfo appInfo{};

			/// DEBUG_UTILITIES ///
			VkDebugUtilsMessengerEXT debugMessenger;
			
			/// PHYSICAL_LOGICAL_DEVICES ///
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkDevice logicalDevice;

			/// QUEUE_FAMILIES ///
			VkQueue graphicsQueue;
			VkQueue presentQueue;
			
			/// SWAP_CHAINS ///
			VkSwapchainKHR swapChain;
			std::vector<VkImage> swapChainImages;
			VkFormat swapChainImageFormat;
			VkExtent2D swapChainExtent;
			std::vector<VkImageView> swapChainImageViews;

			/// GRAPHICS_PIPELINE ///
			std::vector<std::string> shaderPaths;
			VkRenderPass renderPass;
			VkPipelineLayout pipelineLayout;
			VkPipeline graphicsPipeline;

			/// FRAME_BUFFERS ///
			std::vector<VkFramebuffer> swapChainFramebuffers;

			/// COMMAND_POOLS_BUFFERS ///
			VkCommandPool commandPool;
			std::vector<VkCommandBuffer> commandBuffers;

			/// SYNCHRONIZATION_OBJECTS ///
			std::vector<VkSemaphore> imageAvailableSemaphores;
			std::vector<VkSemaphore> renderFinishedSemaphores;
			std::vector<VkFence> inFlightFences;
			uint32_t currentFrame = 0;
			const size_t MAX_FRAMES_IN_FLIGHT = 3;
			bool framebufferResized = false;
		public:
			/// VKINSTANCE_INITIATION_DESTRUCTION ///
			VkInstance instance;
			
			/// SWAP_CHAINS ///
			VkSurfaceKHR surface;
			inline static VkFormat surfaceFormat = VK_FORMAT_B8G8R8A8_SRGB;
			inline static VkColorSpaceKHR surfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			inline static VkPresentModeKHR idealPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

			/// INCLUDED WINDOW LIB ///
			Window* window;

			/// INVOKABLE EVENTS ///
			std::invokable<void*, int, int> onReCreateSwapChain;

			#pragma region VKINSTANCE_INITIATION_DESTRUCTION
			~MiniVkLayer() {
				if (!isMemoryDestroyed) DestroyMiniVulkan();
			}

			/// <summary>[overridable] (called in destructor) Cleans up MiniVulkan subsystem.</summary>
			virtual void DestroyMiniVulkan() {
				if (isMemoryDestroyed) return;

				DestroyFrameBuffers();
				DestroySwapChain();

				for (size_t i = 0; i < std::min(inFlightFences.size(), MAX_FRAMES_IN_FLIGHT); i++) {
					vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
					vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
					vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
				}

				vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
				vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
				vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

				vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
				vkDestroyDevice(logicalDevice, nullptr);
				vkDestroySurfaceKHR(instance, surface, nullptr);

				if (enableValidationLayers)
					DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

				vkDestroyInstance(instance, nullptr);

				isMemoryDestroyed = true;
			}

			MiniVkLayer(Window* hwndWindow, const std::string& title, const std::vector<std::string>& shaderPaths) {
				this->shaderPaths = shaderPaths;
				window = hwndWindow;

				Initiate(title);
				SetupDebugMessenger();
				CreateSurface();
				SelectPhysicalDevice();
				CreateLogicalDevice();
				CreateSwapChain();
				CreateSwapImageViews();
				CreateRenderPass();
				CreateGraphicsPipeline();
				CreateFrameBuffers();
				CreateCommandPool();
				CreateCommandBuffers();
				CreateSyncObjects();
			}

			/// <summary>[overridable] </summary>
			virtual void Initiate(const std::string& title) {
				VkApplicationInfo appInfo{};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = title.c_str();
				appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
				appInfo.pEngineName = "MiniVulkan";
				appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
				appInfo.apiVersion = VK_API_VERSION_1_0;

				VkInstanceCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				createInfo.pApplicationInfo = &appInfo;

				//////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////// Validation Layer Support Handling ////////////////////////////
				VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
				if (enableValidationLayers) {
					if (!QueryValidationLayerSupport())
						throw std::runtime_error("MiniVulkan: Failed to initialize validation layers!");

					PopulateDebugMessengerCreateInfo(debugCreateInfo);
					createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					createInfo.ppEnabledLayerNames = validationLayers.data();
					createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
				}
				else {
					createInfo.enabledLayerCount = 0;
					createInfo.pNext = nullptr;
				}
				/////////////////////////// Validation Layer Support Handling ////////////////////////////
				//////////////////////////////////////////////////////////////////////////////////////////

				auto extensions = SelectRequiredExtensions();
				createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
				createInfo.ppEnabledExtensionNames = extensions.data();

				VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
				if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create Vulkan instance!");

				uint32_t extensionCount = 0;
				vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
				std::vector<VkExtensionProperties> extensionProperties(extensionCount);
				vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());

				#ifdef _DEBUG
				for (const auto& extension : extensions)
					std::cout << '\t' << extension << '\n';

				std::cout << "MiniVulkan: " << extensionCount << " extensions supported\n";
				#endif
			}

			#pragma endregion
			#pragma region QUEUE_FAMILIES

			/// <summary>[overridable] Returns info about the VkPhysicalDevice graphics/present queue families.</summary>
			virtual QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
				QueueFamilyIndices indices;

				uint32_t queueFamilyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

				std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

				int i = 0;
				for (const auto& queueFamily : queueFamilies) {
					if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
						indices.graphicsFamily = i;

					VkBool32 presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

					if (presentSupport) {
						indices.presentFamily = i;
					}

					if (indices.IsComplete()) break;

					i++;
				}

				return indices;
			}

			#pragma endregion
			#pragma region PHYSICAL_LOGICAL_DEVICES

			/// <summary>[overridable] </summary>
			virtual std::vector<const char*> SelectRequiredExtensions() {
				return window->GetRequiredExtensions(enableValidationLayers);
			}

			/// <summary>[overridable] Returns the first usable GPU.</summary>
			virtual void SelectPhysicalDevice() {
				uint32_t deviceCount = 0;
				vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

				if (deviceCount == 0)
					throw std::runtime_error("MiniVulkan: Failed to find GPUs with Vulkan support!");

				std::vector<VkPhysicalDevice> devices(deviceCount);
				vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

				auto suitableDevices = SelectSuitableDevices();
				if (suitableDevices.size() > 0)
					physicalDevice = suitableDevices.front();

				if (physicalDevice == VK_NULL_HANDLE)
					throw std::runtime_error("MiniVulkan: Failed to find a suitable GPU!");
			}

			/// <summary>[overridable] Returns a Vector of suitable VkPhysicalDevices (GPU/iGPU).</summary>
			virtual std::vector<VkPhysicalDevice> SelectSuitableDevices() {
				uint32_t deviceCount = 0;
				vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

				if (deviceCount == 0)
					throw std::runtime_error("MiniVulkan: Failed to find GPUs with Vulkan support!");

				std::vector<VkPhysicalDevice> suitableDevices(deviceCount);
				vkEnumeratePhysicalDevices(instance, &deviceCount, suitableDevices.data());
				std::erase_if(suitableDevices, [this](VkPhysicalDevice device) { return !QueryDeviceSuitability(device); });
				return suitableDevices;
			}

			/// <summary>[overridable] Returns BOOL(true/false) if a VkPhysicalDevice (GPU/iGPU) is suitable for use.</summary>
			virtual bool QueryDeviceSuitability(VkPhysicalDevice device) {
				VkPhysicalDeviceProperties deviceProperties;
				vkGetPhysicalDeviceProperties(device, &deviceProperties);
				
				// You can uncomment this to add check for specific device features.
				//VkPhysicalDeviceFeatures deviceFeatures;
				//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

				QueueFamilyIndices indices = FindQueueFamilies(device);
				bool supportsExtensions = QueryDeviceExtensionSupport(device);
				
				bool swapChainAdequate = false;
				if (supportsExtensions) {
					SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
					swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
				}

				return indices.IsComplete() && supportsExtensions && swapChainAdequate
					&& (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
					|| deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
					|| deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU);
			}

			/// <summary>[overridabl] Returns BOOL(true/false) if the VkPhysicalDevice (GPU/iGPU) supports extensions.</summary>
			bool QueryDeviceExtensionSupport(VkPhysicalDevice device) {
				uint32_t extensionCount;
				vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

				std::vector<VkExtensionProperties> availableExtensions(extensionCount);
				vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
				
				std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
				for (const auto& extension : availableExtensions)
					requiredExtensions.erase(extension.extensionName);

				return requiredExtensions.empty();
			}

			/// <summary>[overridable] Crfeates the logical devices for the graphics/present queue families.</summary>
			virtual void CreateLogicalDevice() {
				QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

				std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
				std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

				float queuePriority = 1.0f;
				for (uint32_t queueFamily : uniqueQueueFamilies) {
					VkDeviceQueueCreateInfo queueCreateInfo{};
					queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queueCreateInfo.queueFamilyIndex = queueFamily;
					queueCreateInfo.queueCount = 1;
					queueCreateInfo.pQueuePriorities = &queuePriority;
					queueCreateInfos.push_back(queueCreateInfo);
				}

				VkPhysicalDeviceFeatures deviceFeatures{};
				VkDeviceCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				createInfo.pQueueCreateInfos = queueCreateInfos.data();
				createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
				createInfo.pEnabledFeatures = &deviceFeatures;
				
				createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
				createInfo.ppEnabledExtensionNames = deviceExtensions.data();

				if (enableValidationLayers) {
					createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					createInfo.ppEnabledLayerNames = validationLayers.data();
				} else { createInfo.enabledLayerCount = 0; }

				if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
					throw std::runtime_error("failed to create logical device!");

				vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
				vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
			}

			/// <summary>[overridable] Wait for logical device after main() loop close.</summary>
			virtual void LogicalDeviceWaitIdle() {
				vkDeviceWaitIdle(logicalDevice);
			}

			#pragma endregion
			#pragma region SWAP_CHAINS
			
			/// <summary>[overridable] Creates the Vulkan surface. </summary>
			virtual void CreateSurface() { window->CreateWindowSurface(instance, surface); }

			/// <summary>[overridable] Create the Vulkan surface swap-chain.</summary>
			virtual void CreateSwapChain() {
				SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);
				VkSurfaceFormatKHR surfaceFormat = SelectSwapSurfaceFormat(swapChainSupport.formats);
				VkPresentModeKHR presentMode = SelectSwapPresentMode(swapChainSupport.presentModes);
				VkExtent2D extent = SelectSwapExtent(swapChainSupport.capabilities);

				/// The "+1" is to request an additional swap-chain image for rendering from the device driver.
				uint32_t imageCount = MAX_FRAMES_IN_FLIGHT;//swapChainSupport.capabilities.minImageCount + 1;
				if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
					imageCount = swapChainSupport.capabilities.maxImageCount;

				VkSwapchainCreateInfoKHR createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				createInfo.surface = surface;
				createInfo.minImageCount = imageCount;
				createInfo.imageFormat = surfaceFormat.format;
				createInfo.imageColorSpace = surfaceFormat.colorSpace;
				createInfo.imageExtent = extent;
				createInfo.imageArrayLayers = 1;
				// Using the TRANSFER DST BIT so we can render outside of the swap-chain if needed.
				createInfo.imageUsage =
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
					| VK_IMAGE_USAGE_TRANSFER_DST_BIT;

				QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
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

				if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create swap chain!");

				vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
				swapChainImages.resize(imageCount);
				vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

				swapChainImageFormat = surfaceFormat.format;
				swapChainExtent = extent;
			}

			/// <summary>[overridable] Re-creates the Vulkan surface swap-chain.</summary>
			void ReCreateSwapChain() {
				int width = 0, height = 0; // Optional.
				onReCreateSwapChain.invoke(window->GetHandle(), width, height);
				vkDeviceWaitIdle(logicalDevice);

				DestroyFrameBuffers();

				for (auto imageView : swapChainImageViews)
					vkDestroyImageView(logicalDevice, imageView, nullptr);

				vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);

				CreateSwapChain();
				CreateSwapImageViews();
				CreateFrameBuffers();
			}

			/// <summary>[overridable] Create the image views for rendering to images (including those in the swap-chain).</summary>
			virtual void CreateSwapImageViews() {
				swapChainImageViews.resize(swapChainImages.size());

				for (size_t i = 0; i < swapChainImages.size(); i++) {
					VkImageViewCreateInfo createInfo{};
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

					if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
						throw std::runtime_error("failed to create image views!");
				}
			}

			/// <summary>[overridable] Destroy the active surface swap-chain.</summary>
			virtual void DestroySwapChain() {
				for (auto imageView : swapChainImageViews)
					vkDestroyImageView(logicalDevice, imageView, nullptr);

				vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
			}

			/// <summary>Change the swap-chain surface format.</summary>
			inline static void SelectSwapSurfaceFormats(VkFormat surfFormat, VkColorSpaceKHR surfColorSpace) {
				surfaceFormat = surfFormat;
				surfaceColorSpace = surfColorSpace;
			}

			/// <summary>[overridable] Checks the VkPhysicalDevice for swap-chain availability.</summary>
			virtual SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
				SwapChainSupportDetails details;

				uint32_t formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

				if (formatCount > 0) {
					details.formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
				}

				uint32_t presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

				if (presentModeCount != 0) {
					details.presentModes.resize(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
				}

				return details;
			}

			/// <summary>[overridable] Select swap-chain surfasce format.</summary>
			virtual VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
				for (const auto& availableFormat : availableFormats)
					if (availableFormat.format == surfaceFormat && availableFormat.colorSpace == surfaceColorSpace)
						return availableFormat;

				return availableFormats[0];
			}

			/// <summary>Change the swap-chains preffered present mode.</summary>
			inline static void SelectIdealSwapPresentMode(VkPresentModeKHR idealPresent) {
				idealPresentMode = idealPresent;
			}

			/// <summary>[overridable] Select the swap-chains active present mode.</summary>
			virtual VkPresentModeKHR SelectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
				for (const auto& availablePresentMode : availablePresentModes) {
					if (availablePresentMode == idealPresentMode) {
						return availablePresentMode;
					}
				}

				return VK_PRESENT_MODE_FIFO_KHR;
			}

			/// <summary>[overridable] Select swap-chaion extent (swap-chain surface resolution).</summary>
			virtual VkExtent2D SelectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
				if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
					return capabilities.currentExtent;
				} else {
					int width, height;
					window->GetFrameBufferSize(width, height);

					VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
					actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
					actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
					return actualExtent;
				}
			}

			#pragma endregion

			#pragma region GRAPHICS_PIPELINE

			/// <summary>[overridable] Creates the graphics pipeline (default shader modules).</summary>
			virtual void CreateGraphicsPipeline() {
				for (size_t i = 0; i < shaderPaths.size() / 2; i++) {
					auto vertexShaderCode = ReadFile(shaderPaths[0]); //ReadFile("Shader Files/vert.spv");
					auto fragmentShaderCode = ReadFile(shaderPaths[1]); //ReadFile("Shader Files/frag.spv");
					VkShaderModule vertShaderModule = CreateShaderModule(vertexShaderCode);
					VkShaderModule fragShaderModule = CreateShaderModule(fragmentShaderCode);

					VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
					vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
					vertShaderStageInfo.module = vertShaderModule;
					vertShaderStageInfo.pName = "main";

					VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
					fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
					fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
					fragShaderStageInfo.module = fragShaderModule;
					fragShaderStageInfo.pName = "main";

					#ifdef _DEBUG
					std::cout << "MiniVulkan: Loading Shader @ " << shaderPaths[0] << std::endl;
					std::cout << "MiniVulkan: Loading Shader @ " << shaderPaths[1] << std::endl;
					#endif

					VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
					
					VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
					vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
					vertexInputInfo.vertexBindingDescriptionCount = 0;
					vertexInputInfo.vertexAttributeDescriptionCount = 0;

					VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
					inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
					inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
					inputAssembly.primitiveRestartEnable = VK_FALSE;

					VkViewport viewport{};
					viewport.x = 0.0f;
					viewport.y = 0.0f;
					viewport.width = (float)swapChainExtent.width;
					viewport.height = (float)swapChainExtent.height;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;

					VkRect2D scissor{};
					scissor.offset = { 0, 0 };
					scissor.extent = swapChainExtent;

					VkPipelineViewportStateCreateInfo viewportState{};
					viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
					viewportState.viewportCount = 1;
					viewportState.pViewports = &viewport;
					viewportState.scissorCount = 1;
					viewportState.pScissors = &scissor;

					VkPipelineRasterizationStateCreateInfo rasterizer{};
					rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
					rasterizer.depthClampEnable = VK_FALSE;
					rasterizer.rasterizerDiscardEnable = VK_FALSE;
					rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
					rasterizer.lineWidth = 1.0f;
					rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
					rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
					rasterizer.depthBiasEnable = VK_FALSE;

					VkPipelineMultisampleStateCreateInfo multisampling{};
					multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
					multisampling.sampleShadingEnable = VK_FALSE;
					multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

					VkPipelineColorBlendAttachmentState colorBlendAttachment{};
					colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
					colorBlendAttachment.blendEnable = VK_FALSE;

					VkPipelineColorBlendStateCreateInfo colorBlending{};
					colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
					colorBlending.logicOpEnable = VK_FALSE;
					colorBlending.logicOp = VK_LOGIC_OP_COPY;
					colorBlending.attachmentCount = 1;
					colorBlending.pAttachments = &colorBlendAttachment;
					colorBlending.blendConstants[0] = 0.0f;
					colorBlending.blendConstants[1] = 0.0f;
					colorBlending.blendConstants[2] = 0.0f;
					colorBlending.blendConstants[3] = 0.0f;

					VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
					pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutInfo.setLayoutCount = 0;
					pipelineLayoutInfo.pushConstantRangeCount = 0;

					if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
						throw std::runtime_error("failed to create pipeline layout!");
					}

					VkGraphicsPipelineCreateInfo pipelineInfo{};
					pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					pipelineInfo.stageCount = 2;
					pipelineInfo.pStages = shaderStages;
					pipelineInfo.pVertexInputState = &vertexInputInfo;
					pipelineInfo.pInputAssemblyState = &inputAssembly;
					pipelineInfo.pViewportState = &viewportState;
					pipelineInfo.pRasterizationState = &rasterizer;
					pipelineInfo.pMultisampleState = &multisampling;
					pipelineInfo.pColorBlendState = &colorBlending;
					pipelineInfo.layout = pipelineLayout;
					pipelineInfo.renderPass = renderPass;
					pipelineInfo.subpass = 0;
					pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
					pipelineInfo.basePipelineIndex = -1; // Optional

					if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
						throw std::runtime_error("MiniVulkan: Failed to create graphics pipeline!");
					}

					vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
					vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
				}
			}

			/// <summary>[overridable] Creates the specified shader's shader module.</summary>
			virtual VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode) {
				VkShaderModuleCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.codeSize = shaderCode.size();
				createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

				VkShaderModule shaderModule;
				if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create shader module!");

				return shaderModule;
			}

			/// <summary>[overridable] Create the render pass layout/dependency info.</summary>
			virtual void CreateRenderPass() {
				VkAttachmentDescription colorAttachment{};
				colorAttachment.format = swapChainImageFormat;
				colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

				VkAttachmentReference colorAttachmentRef{};
				colorAttachmentRef.attachment = 0;
				colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				VkSubpassDescription subpass{};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.colorAttachmentCount = 1;
				subpass.pColorAttachments = &colorAttachmentRef;

				VkSubpassDependency dependency{};
				dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
				dependency.dstSubpass = 0;
				dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.srcAccessMask = 0;
				dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

				VkRenderPassCreateInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				renderPassInfo.attachmentCount = 1;
				renderPassInfo.pAttachments = &colorAttachment;
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pSubpasses = &subpass;
				renderPassInfo.dependencyCount = 1;
				renderPassInfo.pDependencies = &dependency;

				if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
					throw std::runtime_error("failed to create render pass!");
				}
			}

			#pragma endregion
			#pragma region FRAME_BUFFERS

			/// <summary>[overridable] </summary>
			virtual void CreateFrameBuffers() {
				swapChainFramebuffers.resize(swapChainImageViews.size());

				for (size_t i = 0; i < swapChainImageViews.size(); i++) {
					VkImageView attachments[] = { swapChainImageViews[i] };

					VkFramebufferCreateInfo framebufferInfo{};
					framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					framebufferInfo.renderPass = renderPass;
					framebufferInfo.attachmentCount = 1;
					framebufferInfo.pAttachments = attachments;
					framebufferInfo.width = swapChainExtent.width;
					framebufferInfo.height = swapChainExtent.height;
					framebufferInfo.layers = 1;

					if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
						throw std::runtime_error("failed to create framebuffer!");
					}
				}
			}

			/// <summary>[overridable] Destroy the swap chain's frame buffers.</summary>
			virtual void DestroyFrameBuffers() {
				for (auto framebuffer : swapChainFramebuffers)
					vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
			}

			#pragma endregion

			#pragma region COMMAND_POOLS_BUFFERS

			/// <summary>[overridable] Create command pools for drawing/memory transfer operations to the GPU.</summary>
			virtual void CreateCommandPool() {
				QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

				VkCommandPoolCreateInfo poolInfo{};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

				if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create command pool!");
			}

			/// <summary>[overridable] Creates command buffers for the command pools.</summary>
			virtual void CreateCommandBuffers() {
				commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = commandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

				if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
					throw std::runtime_error("failed to allocate command buffers!");
			}

			/// <summary>[overridable] Records commands into a command buffers.</summary>
			virtual void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0; // Optional
				beginInfo.pInheritanceInfo = nullptr; // Optional

				if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [begin] to command buffer!");

				VkRenderPassBeginInfo renderPassInfo{};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = renderPass;
				renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
				renderPassInfo.renderArea.offset = { 0, 0 };
				renderPassInfo.renderArea.extent = swapChainExtent;

				VkClearValue clearColor = { {1.0f, 1.0f, 1.0f, 1.0f} };
				renderPassInfo.clearValueCount = 1;
				renderPassInfo.pClearValues = &clearColor;

				vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
				vkCmdDraw(commandBuffer, 3, 1, 0, 0);
				vkCmdEndRenderPass(commandBuffer);

				if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [end] to command buffer!");
			}

			#pragma endregion

			#pragma region DRAWING_FRAMES

			/// <summary>[overridable] Notify the render engine that the window's frame buffer has been resized.</summary>
			virtual void OnFrameBufferNotifyResizeCallback() {
				framebufferResized = true;
			}

			/// <summary>[overridable] </summary>
			virtual void DrawFrame() {
				/*
					Outline of a frame
					At a high level, rendering a frame in Vulkan consists of a common set of steps:

					Wait for the previous frame to finish
					Acquire an image from the swap chain
					Record a command buffer which draws the scene onto that image
					Submit the recorded command buffer
					Present the swap chain image
					While we will expand the drawing function in later chapters, for now this is the core of our render loop.
				*/
				vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

				uint32_t imageIndex;
				VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

				if (result == VK_ERROR_OUT_OF_DATE_KHR) {
					ReCreateSwapChain();
					return;
				} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
					throw std::runtime_error("MiniVulkan: Failed to acquire swap chain image!");

				vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

				vkResetCommandBuffer(commandBuffers[currentFrame], 0); //VkCommandBufferResetFlagBits
				RecordCommandBuffer(commandBuffers[currentFrame], imageIndex);

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
				VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = waitSemaphores;
				submitInfo.pWaitDstStageMask = waitStages;

				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

				VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = signalSemaphores;

				if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to submit draw command buffer!");

				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

				presentInfo.waitSemaphoreCount = 1;
				presentInfo.pWaitSemaphores = signalSemaphores;

				VkSwapchainKHR swapChains[] = { swapChain };
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = swapChains;

				presentInfo.pImageIndices = &imageIndex;

				result = vkQueuePresentKHR(presentQueue, &presentInfo);

				if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
					framebufferResized = false;
					ReCreateSwapChain();
				} else if (result != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to present swap chain image!");

				currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
			}

			#pragma endregion

			#pragma region SYNCHRONIZATION_OBJECTS

			/// <summary>[overridable] </summary>
			virtual void CreateSyncObjects() {
				imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
				renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
				inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

				VkSemaphoreCreateInfo semaphoreInfo{};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VkFenceCreateInfo fenceInfo{};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

				for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
					if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
						vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
						vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
						throw std::runtime_error("MiniVulkan: Failed to create synchronization objects for a frame!");
				}
			}

			#pragma endregion
			#pragma region VALIDATION_LAYERS

			/// <summary>Returns BOOL(true/false) if Vulkan ValidationLayers are supported.</summary>
			bool QueryValidationLayerSupport() {
				uint32_t layerCount;
				vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
				std::vector<VkLayerProperties> availableLayers(layerCount);
				vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

				for (const std::string layerName : validationLayers) {
					bool layerFound = false;

					for (const auto& layerProperties : availableLayers)
						if (layerName.compare(layerProperties.layerName)) {
							layerFound = true;
							break;
						}

					if (!layerFound) return false;
				}

				return true;
			}

			void SetupDebugMessenger() {
				if (!enableValidationLayers) return;

				VkDebugUtilsMessengerCreateInfoEXT createInfo;
				PopulateDebugMessengerCreateInfo(createInfo);

				if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to set up debug messenger!");
			}

			void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
				createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
				createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				createInfo.pfnUserCallback = DebugCallback;
				createInfo.pUserData = nullptr; // Optional
			}

			#pragma endregion
			#pragma region FILE_SYSTEM

			std::vector<char> ReadFile(const std::string& path) {
				std::ifstream file(path, std::ios::ate | std::ios::binary);

				if (!file.is_open())
					throw std::runtime_error("MiniVulkan: Failed to Read File: " + path);

				size_t fsize = static_cast<size_t>(file.tellg());
				std::vector<char> buffer(fsize);
				file.seekg(file.beg);
				file.read(buffer.data(), fsize);
				file.close();
				return buffer;
			}

			#pragma endregion
		};
	}
#endif