#pragma once
#ifndef MINIVULKAN_MINIVKRENDERDEVICE
#define MINIVULKAN_MINIVKRENDERDEVICE
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
		class MiniVkRenderDevice : public std::disposable {
		private:
			const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
			const VkPhysicalDeviceFeatures deviceFeatures { .multiDrawIndirect = VK_TRUE, .multiViewport = VK_TRUE };
			const std::vector<const char*> deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME, /// Swapchain support for buffering frame images with the device driver to reduce tearing.
				VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, // Used to enable high memory priority for VMA.
				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, // Dynamic Rendering Dependency
				VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, // Dynamic Rendering Dependency
				VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, /// Allows for rendering without framebuffers and render passes.
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, /// Allows for writing descriptors directly into a command buffer rather than allocating from sets/pools.
				VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME, // Used for changing pipeline dynamic state such as blending/viewport/scissor operations.
				VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME // Used for changing pipeline dynamic state such as blending/viewport/scissor operations.
			};
		public:
			/// PHYSICAL_LOGICAL_DEVICES ///
			MiniVkInstance& instance;
			std::vector<VkPhysicalDeviceType> physicalDeviceTypes;
			VkPhysicalDevice physicalDevice = nullptr;
			VkDevice logicalDevice = nullptr;
			VkSurfaceKHR presentationSurface = nullptr;

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(logicalDevice);

				vkDestroyDevice(logicalDevice, nullptr);
				vkDestroySurfaceKHR(instance.instance, presentationSurface, nullptr);
			}

			MiniVkRenderDevice(MiniVkInstance& instance, VkSurfaceKHR presentSurface, const std::vector<VkPhysicalDeviceType> pTypes) : instance(instance), presentationSurface(presentSurface), physicalDeviceTypes(pTypes) {
				onDispose += std::callback<bool>(this, &MiniVkRenderDevice::Disposable);
				QueryPhysicalDevice();
				CreateLogicalDevice();
			}

			MiniVkRenderDevice operator=(const MiniVkRenderDevice& inst) = delete;

			#pragma region PHYSICAL_LOGICAL_DEVICES

			/// <summary>Creates the logical devices for the graphics/present queue families.</summary>
			void CreateLogicalDevice() {
				MiniVkQueueFamily indices = MiniVkQueueFamily::FindQueueFamilies(physicalDevice, presentationSurface);

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
				
				VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicState3Features{};
				dynamicState3Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
				dynamicState3Features.extendedDynamicState3ColorBlendEnable = VK_TRUE;
				
				VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingCreateInfo{};
				dynamicRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
				dynamicRenderingCreateInfo.dynamicRendering = VK_TRUE;
				dynamicRenderingCreateInfo.pNext = &dynamicState3Features;

				VkDeviceCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				createInfo.pNext = &dynamicRenderingCreateInfo;
				createInfo.pQueueCreateInfos = queueCreateInfos.data();
				createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
				createInfo.pEnabledFeatures = &deviceFeatures;
				createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
				createInfo.ppEnabledExtensionNames = deviceExtensions.data();

				#ifdef MVK_VALIDATION_LAYERS
					createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					createInfo.ppEnabledLayerNames = validationLayers.data();
				#else
					createInfo.enabledLayerCount = 0;
					createInfo.ppEnabledLayerNames = nullptr;
				#endif

				if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create logical device!");
			}

			/// <summary>Wait for attached GPU device tro finish transfer/rendering commands.</summary>
			void WaitIdle() {
				vkDeviceWaitIdle(logicalDevice);
			}

			/// <summary>Returns the first usable GPU.</summary>
			void QueryPhysicalDevice() {
				uint32_t deviceCount = 0;
				vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr);

				if (deviceCount == 0)
					throw std::runtime_error("MiniVulkan: Failed to find GPUs with Vulkan support!");

				auto suitableDevices = QuerySuitableDevices();
				if (suitableDevices.size() > 0)
					physicalDevice = suitableDevices.front();

				if (physicalDevice == nullptr)
					throw std::runtime_error("MiniVulkan: Failed to find a suitable GPU!");
			}

			/// <summary>Returns a Vector of suitable VkPhysicalDevices (GPU/iGPU).</summary>
			std::vector<VkPhysicalDevice> QuerySuitableDevices() {
				uint32_t deviceCount = 0;
				vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr);

				if (deviceCount == 0)
					throw std::runtime_error("MiniVulkan: Failed to find GPUs with Vulkan support!");

				std::vector<VkPhysicalDevice> suitableDevices(deviceCount);
				vkEnumeratePhysicalDevices(instance.instance, &deviceCount, suitableDevices.data());
				std::erase_if(suitableDevices, [this](VkPhysicalDevice device) { return !QueryDeviceSuitability(device); });
				return suitableDevices;
			}

			/// <summary>Checks the VkPhysicalDevice for swap-chain availability.</summary>
			MiniVkSwapChainSupporter QuerySwapChainSupport(VkPhysicalDevice device) {
				MiniVkSwapChainSupporter details;

				uint32_t formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, presentationSurface, &formatCount, nullptr);
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, presentationSurface, &details.capabilities);

				if (formatCount > 0) {
					details.formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(device, presentationSurface, &formatCount, details.formats.data());
				}

				uint32_t presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, presentationSurface, &presentModeCount, nullptr);

				if (presentModeCount != 0) {
					details.presentModes.resize(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(device, presentationSurface, &presentModeCount, details.presentModes.data());
				}

				return details;
			}

			/// <summary>Returns BOOL(true/false) if a VkPhysicalDevice (GPU/iGPU) is suitable for use.</summary>
			bool QueryDeviceSuitability(VkPhysicalDevice device) {
				VkPhysicalDeviceProperties2 deviceProperties {};
				deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				vkGetPhysicalDeviceProperties2(device, &deviceProperties);

				// You can uncomment this to add check for specific device features.
				VkPhysicalDeviceFeatures2 deviceFeatures {};
				deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicState3Features{};
				dynamicState3Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
				deviceFeatures.pNext = &dynamicState3Features;
				vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);
				
				MiniVkQueueFamily indices = MiniVkQueueFamily::FindQueueFamilies(device, presentationSurface);
				bool supportsExtensions = QueryDeviceExtensionSupport(device);

				bool swapChainAdequate = false;
				if (supportsExtensions) {
					MiniVkSwapChainSupporter swapChainSupport = QuerySwapChainSupport(device);
					swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
				}

				bool hasType = false;
				for (auto type : physicalDeviceTypes)
					if (deviceProperties.properties.deviceType == type) {
						hasType = true;
						break;
					}

				return indices.IsComplete() && hasType && supportsExtensions && swapChainAdequate
					&& deviceFeatures.features.multiViewport && deviceFeatures.features.multiDrawIndirect
					&& dynamicState3Features.extendedDynamicState3ColorBlendEnable;
			}

			/// <summary>Returns BOOL(true/false) if the VkPhysicalDevice (GPU/iGPU) supports extensions.</summary>
			bool QueryDeviceExtensionSupport(VkPhysicalDevice device) {
				uint32_t extensionCount;
				vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
				
				std::vector<VkExtensionProperties> availableExtensions(extensionCount);
				vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
				
				std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
				for (const auto& extension : availableExtensions)
					requiredExtensions.erase(extension.extensionName);

				#ifdef MVK_VALIDATION_LAYERS
				for (const auto& extension : requiredExtensions)
					std::cout << "UNAVAILABLE EXTENSIONS: " << extension << std::endl;
				#endif

				return requiredExtensions.empty();
			}

			#pragma endregion
		};
	}
#endif