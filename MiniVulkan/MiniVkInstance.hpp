#pragma once
#ifndef MINIVULKAN_MINIVKINSTANCE
#define MINIVULKAN_MINIVKINSTANCE
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
		#pragma region DYNAMIC_RENDERING_FUNCTIONS

		PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingEXTKHR = nullptr;
		PFN_vkCmdEndRenderingKHR vkCmdEndRenderingEXTKHR = nullptr;
		PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetEXTKHR = nullptr;

		void vkCmdRenderingGetCallbacks(VkInstance instance) {
			if (vkCmdBeginRenderingEXTKHR == nullptr)
				vkCmdBeginRenderingEXTKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");

			if (vkCmdEndRenderingEXTKHR == nullptr)
				vkCmdEndRenderingEXTKHR = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");

			if (vkCmdPushDescriptorSetEXTKHR == nullptr)
				vkCmdPushDescriptorSetEXTKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetInstanceProcAddr(instance, "vkCmdPushDescriptorSetKHR");
		}

		VkResult vkCmdBeginRenderingEKHR(VkInstance instance, VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo) {
			#if MVK_VALIDATION_LAYERS == VK_TRUE
				if (vkCmdBeginRenderingEXTKHR == VK_NULL_HANDLE)
					throw std::runtime_error("MiniVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdBeginRenderingKHR");
			#endif

			vkCmdBeginRenderingEXTKHR(commandBuffer, pRenderingInfo);
			return VK_SUCCESS;
		}

		VkResult vkCmdEndRenderingEKHR(VkInstance instance, VkCommandBuffer commandBuffer) {
			#if MVK_VALIDATION_LAYERS == VK_TRUE
				if (vkCmdEndRenderingEXTKHR == VK_NULL_HANDLE)
					throw std::runtime_error("MiniVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdEndRenderingKHR");
			#endif

			vkCmdEndRenderingEXTKHR(commandBuffer);
			return VK_SUCCESS;
		}

		VkResult vkCmdPushDescriptorSetEKHR(VkInstance instance, VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t set, uint32_t writeCount, const VkWriteDescriptorSet* pWriteSets) {
			#if MVK_VALIDATION_LAYERS == VK_TRUE
				if (vkCmdPushDescriptorSetEXTKHR == VK_NULL_HANDLE)
					throw std::runtime_error("MiniVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdPushDescriptorSetKHR");
			#endif

			vkCmdPushDescriptorSetEXTKHR(commandBuffer, bindPoint, layout, set, writeCount, pWriteSets);
			return VK_SUCCESS;
		}

		#pragma endregion

		class MiniVkInstance : public disposable {
		private:
			const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
			const std::vector<const char*> instanceExtensions = { /*VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME*/ };
		public:
			std::vector<const char*> presentationExtensions;
			VkDebugUtilsMessengerEXT debugMessenger;
			VkApplicationInfo appInfo{};
			VkInstance instance;

			void Disposable(bool waitIdle) {
				#if MVK_VALIDATION_LAYERS
					DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
				#endif
				
				vkDestroyInstance(instance, nullptr);
			}

			MiniVkInstance(const std::vector<const char*> presentationExtensions, const std::string title) : presentationExtensions(presentationExtensions) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				
				CreateVkInstance(title);
				SetupDebugMessenger();
				vkCmdRenderingGetCallbacks(instance);
			}

			MiniVkInstance operator=(const MiniVkInstance& inst) = delete;

			void CreateVkInstance(const std::string& title) {
				VkApplicationInfo appInfo{};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = title.c_str();
				appInfo.applicationVersion = MVK_RENDERER_VERSION;
				appInfo.pEngineName = MVK_RENDERER_NAME;
				appInfo.engineVersion = MVK_RENDERER_VERSION;
				appInfo.apiVersion = MVK_RENDERER_VERSION;
				
				VkInstanceCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				createInfo.pApplicationInfo = &appInfo;
				//////////////////////////////////////////////////////////////////////////////////////////
				/////////////////////////// Validation Layer Support Handling ////////////////////////////
				#if MVK_VALIDATION_LAYERS
					if (!QueryValidationLayerSupport())
						throw std::runtime_error("MiniVulkan: Failed to initialize validation layers!");

					VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
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

				std::vector<const char*> extensions(presentationExtensions);
				// Add required instance extensions to required window extensions
				for (const auto& extension : instanceExtensions) extensions.push_back(extension);

				createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
				createInfo.ppEnabledExtensionNames = extensions.data();

				VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
				if (result != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to create Vulkan instance! " + result);

				#if MVK_VALIDATION_LAYERS
				for (const auto& extension : extensions)
					std::cout << '\t' << extension << '\n';

				std::cout << "MiniVulkan: " << extensions.size() << " extensions supported\n";
				#endif
			}

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
				#if !MVK_VALIDATION_LAYERS
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