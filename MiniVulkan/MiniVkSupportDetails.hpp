#pragma once
#ifndef MINIVK_MINIVKDETAILS
#define MINIVK_MINIVKDETAILS
	#include "./MiniVk.hpp"
	
	namespace MINIVULKAN_NS {
		#pragma region DEBUG_UTILITIES

		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			return VK_ERROR_EXTENSION_NOT_PRESENT;
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

		enum class MiniVkBufferingMode { SINGLE = 1, DOUBLE = 2, TRIPLE = 3 };

		struct MiniVkInstanceSupportDetails {
		public:
			VkInstance& instance;
			VkDevice& logicalDevice;
			VkPhysicalDevice& physicalDevice;
			MiniVkWindow* window = nullptr;
			VkSurfaceKHR renderSurface = nullptr;

			MiniVkInstanceSupportDetails(VkInstance& inst, VkDevice& logDevice, VkPhysicalDevice& physDevice, MiniVkWindow* window, VkSurfaceKHR rendSurface = nullptr)
				: instance(inst), logicalDevice(logDevice), physicalDevice(physDevice), window(window), renderSurface(rendSurface) {}

			MiniVkInstanceSupportDetails operator=(const MiniVkInstanceSupportDetails& vkI) {
				instance = vkI.instance;
				logicalDevice = vkI.logicalDevice;
				physicalDevice = vkI.physicalDevice;
				window = vkI.window;
				renderSurface = vkI.renderSurface;
			}
		};

		struct MiniVkSwapChainSupportDetails {
		public:
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		struct MiniVkSurfaceSupportDetails {
		public:
			VkFormat dataFormat = VK_FORMAT_B8G8R8A8_SRGB;
			VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			VkPresentModeKHR idealPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		};
	}
#endif