#pragma once
#ifndef TINYVK_TINYVKINSTANCE
#define TINYVK_TINYVKINSTANCE
	#include "./TinyVK.hpp"

	namespace TINYVULKAN_NAMESPACE {
		#pragma region DYNAMIC_RENDERING_FUNCTIONS

		PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingEXTKHR = nullptr;
		PFN_vkCmdEndRenderingKHR vkCmdEndRenderingEXTKHR = nullptr;
		PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetEXTKHR = nullptr;

		void vkCmdRenderingGetCallbacks(VkInstance instance) {
			vkCmdBeginRenderingEXTKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
			vkCmdEndRenderingEXTKHR = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");
			vkCmdPushDescriptorSetEXTKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetInstanceProcAddr(instance, "vkCmdPushDescriptorSetKHR");
		}

		VkResult vkCmdBeginRenderingEKHR(VkInstance instance, VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo) {
			#if TVK_VALIDATION_LAYERS
				if (vkCmdBeginRenderingEXTKHR == VK_NULL_HANDLE)
					throw std::runtime_error("TinyVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdBeginRenderingKHR");
			#endif

			vkCmdBeginRenderingEXTKHR(commandBuffer, pRenderingInfo);
			return VK_SUCCESS;
		}

		VkResult vkCmdEndRenderingEKHR(VkInstance instance, VkCommandBuffer commandBuffer) {
			#if TVK_VALIDATION_LAYERS
				if (vkCmdEndRenderingEXTKHR == VK_NULL_HANDLE)
					throw std::runtime_error("TinyVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdEndRenderingKHR");
			#endif

			vkCmdEndRenderingEXTKHR(commandBuffer);
			return VK_SUCCESS;
		}

		VkResult vkCmdPushDescriptorSetEKHR(VkInstance instance, VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint32_t set, uint32_t writeCount, const VkWriteDescriptorSet* pWriteSets) {
			#if TVK_VALIDATION_LAYERS
				if (vkCmdPushDescriptorSetEXTKHR == VK_NULL_HANDLE)
					throw std::runtime_error("TinyVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdPushDescriptorSetKHR");
			#endif

			vkCmdPushDescriptorSetEXTKHR(commandBuffer, bindPoint, layout, set, writeCount, pWriteSets);
			return VK_SUCCESS;
		}

		#pragma endregion

		// 
		// USER MACROS: (Must be defined before you call #include "TinyVK.hpp")
		//		Forces TinyVkInstance to automatically populate GUI window extensions: TinyVkWindow::QueryRequiredExtensions(TVK_VALIDATION_LAYERS)
		//		#define TINYVK_AUTO_PRESENT_EXTENSIONS
		//		
		// TINYVK API MACROS: (Handled automatically via DEBUG/RELEASE build preprocessors defines in TinyVK.hpp)
		//      Forces Vulkan to output validation layer errors
		//		#define TINYVK_VALIDATION_LAYERS
		// 

		/// <summary>Initializes Vulkan and optionally Vulkan Validation Layers using the TinyVK interface.</summary>
		class TinyVkInstance : public disposable {
		private:
			const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
			const std::vector<const char*> instanceExtensions = {  };

			bool QueryValidationLayerSupport() {
				uint32_t layerCount;
				vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
				std::vector<VkLayerProperties> availableLayers(layerCount);
				vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

				#if TVK_VALIDATION_LAYERS
				for (const auto& layerProperties : availableLayers)
					std::cout << layerProperties.layerName << std::endl;
				#endif

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
			
			void CreateVkInstance(const std::string& title) {
				VkApplicationInfo appInfo{};
				appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
				appInfo.pApplicationName = title.c_str();
				appInfo.applicationVersion = TVK_RENDERER_VERSION;
				appInfo.pEngineName = TVK_RENDERER_NAME;
				appInfo.engineVersion = TVK_RENDERER_VERSION;
				appInfo.apiVersion = TVK_RENDERER_VERSION;

				VkInstanceCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
				createInfo.pApplicationInfo = &appInfo;

				#if TVK_VALIDATION_LAYERS
					if (!QueryValidationLayerSupport())
						throw std::runtime_error("TinyVulkan: Failed to initialize validation layers!");

					VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
					debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
					debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
					debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
					debugCreateInfo.pfnUserCallback = DebugCallback;
					debugCreateInfo.pUserData = nullptr;

					createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					createInfo.ppEnabledLayerNames = validationLayers.data();
					createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
				#endif

				std::vector<const char*> extensions(presentationExtensions);
				for (const auto& extension : instanceExtensions) extensions.push_back(extension);

				createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
				createInfo.ppEnabledExtensionNames = extensions.data();

				VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
				if (result != VK_SUCCESS)
					throw std::runtime_error("TinyVulkan: Failed to create Vulkan instance! " + result);

				#if TVK_VALIDATION_LAYERS
					// CreateDebugUtilsMessenger requires the VkInstance, so it goes at the end.
					if (CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
						throw std::runtime_error("TinyVulkan: Failed to set up debug messenger!");

					// Gimme all the extensions used by TinyVK.
					for (const auto& extension : extensions)
						std::cout << '\t' << extension << '\n';

					std::cout << "TinyVulkan: " << extensions.size() << " extensions supported\n";
				#endif
			}

		public:
			std::vector<const char*> presentationExtensions;
			VkDebugUtilsMessengerEXT debugMessenger;
			VkApplicationInfo appInfo{};
			VkInstance instance;

			~TinyVkInstance() { this->Dispose(); }

			void Disposable(bool waitIdle) {
				#if TVK_VALIDATION_LAYERS
					DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
				#endif
				
				vkDestroyInstance(instance, nullptr);
			}

			/// <summary>Initiates the Vulkan subsystem with the required instance/presentation extensions.</summary>
			TinyVkInstance(const std::vector<const char*> presentationExtensions, const std::string title) : presentationExtensions(presentationExtensions) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));
				
				#ifdef TINYVK_AUTO_PRESENT_EXTENSIONS
					for (auto str : TinyVkWindow::QueryRequiredExtensions(TVK_VALIDATION_LAYERS))
						this->presentationExtensions.push_back(str);
				#endif

				CreateVkInstance(title);
				vkCmdRenderingGetCallbacks(instance);
			}

			TinyVkInstance operator=(const TinyVkInstance& inst) = delete;

			/// <summary>Returns the initialized VkInstance struct.</summary>
			VkInstance GetInstance() { return instance; }
		};
	}
#endif