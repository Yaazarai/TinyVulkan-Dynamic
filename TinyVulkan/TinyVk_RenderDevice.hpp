#pragma once
#ifndef TINYVK_TINYVKRENDERDEVICE
#define TINYVK_TINYVKRENDERDEVICE
	#include "./TinyVK.hpp"

	namespace TINYVULKAN_NAMESPACE {
		/// <summary>Initialized Vulkan GPU render device.</summary>
		class TinyVkRenderDevice : public disposable {
		private:
			const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
			const VkPhysicalDeviceFeatures deviceFeatures { .multiDrawIndirect = VK_TRUE, .multiViewport = VK_TRUE };
			const std::vector<const char*> deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME, /// Swapchain support for buffering frame images with the device driver to reduce tearing.
				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, // Dynamic Rendering Dependency.
				VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, // Dynamic Rendering Dependency.
				VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, /// Allows for rendering without framebuffers and render passes.
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, /// Allows for writing descriptors directly into a command buffer rather than allocating from sets/pools.
				//VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME // Used for extended line drawing support (March 2023, OK device support: 57%).
			};
		public:
			TinyVkInstance& instance;
			std::vector<VkPhysicalDeviceType> physicalDeviceTypes;
			VkPhysicalDevice physicalDevice = nullptr;
			VkDevice logicalDevice = nullptr;
			VkSurfaceKHR presentationSurface = nullptr;

			~TinyVkRenderDevice() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(logicalDevice);

				vkDestroyDevice(logicalDevice, nullptr);
				vkDestroySurfaceKHR(instance.instance, presentationSurface, nullptr);
			}

			TinyVkRenderDevice(TinyVkInstance& instance, VkSurfaceKHR presentSurface, const std::vector<VkPhysicalDeviceType> pTypes) : instance(instance), presentationSurface(presentSurface), physicalDeviceTypes(pTypes) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				QueryPhysicalDevice();
				CreateLogicalDevice();
			}

			TinyVkRenderDevice operator=(const TinyVkRenderDevice& rdevice) = delete;

			#pragma region PHYSICAL_LOGICAL_DEVICES

			/// <summary>Creates the logical devices for the graphics/present queue families.</summary>
			void CreateLogicalDevice() {
				TinyVkQueueFamily indices = TinyVkQueueFamily::FindQueueFamilies(physicalDevice, presentationSurface);

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
				
				VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingCreateInfo{};
				dynamicRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
				dynamicRenderingCreateInfo.dynamicRendering = VK_TRUE;

				VkDeviceCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				createInfo.pNext = &dynamicRenderingCreateInfo;
				createInfo.pQueueCreateInfos = queueCreateInfos.data();
				createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
				createInfo.pEnabledFeatures = &deviceFeatures;
				createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
				createInfo.ppEnabledExtensionNames = deviceExtensions.data();

				#if TVK_VALIDATION_LAYERS
					createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					createInfo.ppEnabledLayerNames = validationLayers.data();
				#else
					createInfo.enabledLayerCount = 0;
					createInfo.ppEnabledLayerNames = nullptr;
				#endif

				if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to create logical device!");
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
					throw std::runtime_error("TinyVulkan: Failed to find GPUs with Vulkan support!");

				auto suitableDevices = QuerySuitableDevices();
				if (suitableDevices.size() > 0)
					physicalDevice = suitableDevices.front();

				if (physicalDevice == nullptr)
					throw std::runtime_error("TinyVulkan: Failed to find a suitable GPU!");
				
				#if TVK_VALIDATION_LAYERS
					VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps{};
					pushDescriptorProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;

					VkPhysicalDeviceProperties2 deviceProperties{};
					deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
					deviceProperties.pNext = &pushDescriptorProps;

					vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);

					std::cout << "GPU Device Name: " << deviceProperties.properties.deviceName << std::endl;
					std::cout << "Push Constant Memory Space: " << deviceProperties.properties.limits.maxPushConstantsSize << std::endl;
					std::cout << "Push Descriptor Memory Space: " << pushDescriptorProps.maxPushDescriptors << std::endl;
				#endif
			}

			/// <summary>Returns a Vector of suitable VkPhysicalDevices (GPU/iGPU).</summary>
			std::vector<VkPhysicalDevice> QuerySuitableDevices() {
				uint32_t deviceCount = 0;
				vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr);

				if (deviceCount == 0)
					throw std::runtime_error("TinyVulkan: Failed to find GPUs with Vulkan support!");

				std::vector<VkPhysicalDevice> suitableDevices(deviceCount);
				vkEnumeratePhysicalDevices(instance.instance, &deviceCount, suitableDevices.data());
				std::erase_if(suitableDevices, [this](VkPhysicalDevice device) { return !QueryDeviceSuitability(device); });
				return suitableDevices;
			}

			/// <summary>Checks the VkPhysicalDevice for swap-chain availability.</summary>
			TinyVkSwapChainSupporter QuerySwapChainSupport(VkPhysicalDevice device) {
				TinyVkSwapChainSupporter details;

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
				
				VkPhysicalDeviceFeatures2 deviceFeatures {};
				deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);
				
				TinyVkQueueFamily indices = TinyVkQueueFamily::FindQueueFamilies(device, presentationSurface);
				bool supportsExtensions = QueryDeviceExtensionSupport(device);

				bool swapChainAdequate = false;
				if (supportsExtensions) {
					TinyVkSwapChainSupporter swapChainSupport = QuerySwapChainSupport(device);
					swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
				}

				bool hasType = false;
				for (auto type : physicalDeviceTypes)
					if (deviceProperties.properties.deviceType == type) {
						hasType = true;
						break;
					}

				return indices.IsComplete() && hasType && supportsExtensions && swapChainAdequate
					&& deviceFeatures.features.multiViewport && deviceFeatures.features.multiDrawIndirect;
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

				#if TVK_VALIDATION_LAYERS
					for (const auto& extension : requiredExtensions)
						std::cout << "UNAVAILABLE EXTENSIONS: " << extension << std::endl;
				#endif

				return requiredExtensions.empty();
			}

			#pragma endregion
		};
	}
#endif