#pragma once
#ifndef MINIVK_MINIVKINSTANCE
#define MINIVK_MINIVKINSTANCE
	#include "./MiniVulkan.hpp"

	namespace MINIVULKAN_NS {
		class MvkInstance : public MvkObject {
		private:
			bool isInitialized = false;
			const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
			const std::vector<const char*> instanceExtensions = { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };
			const VkPhysicalDeviceFeatures deviceFeatures { .multiDrawIndirect = VK_TRUE, .multiViewport = VK_TRUE };
			const std::vector<const char*> deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				/// Swapchain support for buffering frame images with the device driver to reduce tearing.
				
				VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, // Used to enable high memory priority for VMA.

				VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, // Dynamic Rendering Dependency
				VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, // Dynamic Rendering Dependency
				//VK_NV_INHERITED_VIEWPORT_SCISSOR_EXTENSION_NAME, // Dynamic Rendering Dependency
				
				VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
				/// Allows for rendering without framebuffers and render passes to simplify the graphics pipeline.
				/// Downside is performance might worsen on tiling GPUs (mobile platforms), which is not relevant here.
				
				VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
				/// Allows for writing descriptors directly into a command buffer rather than allocating from sets/pools.
				/// This should be faster in some cases than actual descriptor sets/pools.
				
				//VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
				VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
				VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME
				// Used for changing pipeline dynamic state such as blending operations.
			};
			
			/// DEBUG_UTILITIES ///
			VkDebugUtilsMessengerEXT debugMessenger;

			/// VKINSTANCE INFO ///
			VkApplicationInfo appInfo{};
		public:
			/// PHYSICAL_LOGICAL_DEVICES ///
			std::vector<VkPhysicalDeviceType> physicalDeviceTypes;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkDevice logicalDevice = VK_NULL_HANDLE;

			/// WINDOW RENDER SURFACE ///
			VkSurfaceKHR presentationSurface = nullptr;
			std::vector<const char*> presentationRequiredExtensions;
			
			/// VKINSTANCE_INITIATION_DESTRUCTION ///
			VkInstance instance;

			void Disposable() {
				vkDeviceWaitIdle(logicalDevice);
				vkDestroyDevice(logicalDevice, nullptr);

				#ifdef MVK_ENABLE_VALIDATION_LAYERS
					DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
				#endif
				
				vkDestroySurfaceKHR(instance, presentationSurface, nullptr);
				vkDestroyInstance(instance, nullptr);
			}

			MvkInstance(const std::vector<const char*> presentationExtensions, const std::string title, const std::vector<VkPhysicalDeviceType> pTypes)
			: presentationRequiredExtensions(presentationExtensions) {
				onDispose += std::callback<>(this, &MvkInstance::Disposable);
				physicalDeviceTypes.assign(pTypes.begin(), pTypes.end());
				CreateVkInstance(title);
			}

			MvkInstance operator=(const MvkInstance& inst) = delete;

			/// <summary>Initialize Vulkan and create VkInstance</summary>
			void CreateVkInstance(const std::string& title) {
				VkApplicationInfo appInfo{};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = title.c_str();
				appInfo.applicationVersion = MVK_RENDERER_VERSION;
				appInfo.pEngineName = "MiniVulkan";
				appInfo.engineVersion = MVK_ENGINE_VERSION;
				appInfo.apiVersion = MVK_API_VERSION;
				
				VkInstanceCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				createInfo.pApplicationInfo = &appInfo;
				//////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////// Validation Layer Support Handling ////////////////////////////
				VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
				#ifdef MVK_ENABLE_VALIDATION_LAYERS
					if (!QueryValidationLayerSupport())
						throw std::runtime_error("MiniVulkan: Failed to initialize validation layers!");

					PopulateDebugMessengerCreateInfo(debugCreateInfo);
					createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					createInfo.ppEnabledLayerNames = validationLayers.data();
					createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
				#else
					createInfo.enabledLayerCount = 0;
					createInfo.pNext = nullptr;
				#endif
				/////////////////////////// Validation Layer Support Handling ////////////////////////////
				//////////////////////////////////////////////////////////////////////////////////////////

				auto extensions = presentationRequiredExtensions;
				// Add required instance extensions to required window extensions
				for (const auto& extension : instanceExtensions) extensions.push_back(extension);

				createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
				createInfo.ppEnabledExtensionNames = extensions.data();

				VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
				if (result != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create Vulkan instance! " + result);

				#ifdef _DEBUG
				for (const auto& extension : extensions)
					std::cout << '\t' << extension << '\n';

				std::cout << "MiniVulkan: " << extensions.size() << " extensions supported\n";
				#endif
			}

			void Initialize(VkSurfaceKHR presentationSurface) {
				if (isInitialized) return;
				this->presentationSurface = presentationSurface;
				SetupDebugMessenger();
				QueryPhysicalDevice();
				CreateLogicalDevice();
				isInitialized = true;
			}

			#pragma region PHYSICAL_LOGICAL_DEVICES

			/// <summary>Creates the logical devices for the graphics/present queue families.</summary>
			void CreateLogicalDevice() {
				MvkQueueFamily indices = MvkQueueFamily::FindQueueFamilies(physicalDevice, presentationSurface);

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

				#ifdef MVK_ENABLE_VALIDATION_LAYERS
					createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					createInfo.ppEnabledLayerNames = validationLayers.data();
				#else
					createInfo.enabledLayerCount = 0;
				#endif

				if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create logical device!");
			}

			/// <summary>Wait for attached GPU device tro finish transfer/rendering commands.</summary>
			void WaitIdleLogicalDevice() {
				vkDeviceWaitIdle(logicalDevice);
			}

			/// <summary>Returns the first usable GPU.</summary>
			void QueryPhysicalDevice() {
				uint32_t deviceCount = 0;
				vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

				if (deviceCount == 0)
					throw std::runtime_error("MiniVulkan: Failed to find GPUs with Vulkan support!");

				auto suitableDevices = QuerySuitableDevices();
				if (suitableDevices.size() > 0)
					physicalDevice = suitableDevices.front();

				if (physicalDevice == VK_NULL_HANDLE)
					throw std::runtime_error("MiniVulkan: Failed to find a suitable GPU!");
			}

			/// <summary>Returns a Vector of suitable VkPhysicalDevices (GPU/iGPU).</summary>
			std::vector<VkPhysicalDevice> QuerySuitableDevices() {
				uint32_t deviceCount = 0;
				vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

				if (deviceCount == 0)
					throw std::runtime_error("MiniVulkan: Failed to find GPUs with Vulkan support!");

				std::vector<VkPhysicalDevice> suitableDevices(deviceCount);
				vkEnumeratePhysicalDevices(instance, &deviceCount, suitableDevices.data());
				std::erase_if(suitableDevices, [this](VkPhysicalDevice device) { return !QueryDeviceSuitability(device); });
				return suitableDevices;
			}

			/// <summary>Checks the VkPhysicalDevice for swap-chain availability.</summary>
			MvkSwapChainSupporter QuerySwapChainSupport(VkPhysicalDevice device) {
				MvkSwapChainSupporter details;

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
				
				MvkQueueFamily indices = MvkQueueFamily::FindQueueFamilies(device, presentationSurface);
				bool supportsExtensions = QueryDeviceExtensionSupport(device);

				bool swapChainAdequate = false;
				if (supportsExtensions) {
					MvkSwapChainSupporter swapChainSupport = QuerySwapChainSupport(device);
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

				#ifdef _DEBUG
				for (const auto& extension : requiredExtensions)
					std::cout << "UNAVAILABLE EXTENSIONS: " << extension << std::endl;
				#endif

				return requiredExtensions.empty();
			}

			#pragma endregion
			#pragma region VALIDATION_LAYERS

			bool QueryValidationLayerSupport() {
				uint32_t layerCount;
				vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
				std::vector<VkLayerProperties> availableLayers(layerCount);
				vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

				for (const auto& layerProperties : availableLayers)
					std::cout << layerProperties.layerName << std::endl;

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
				#ifndef MVK_ENABLE_VALIDATION_LAYERS
					return;
				#endif

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
		};
	}
#endif