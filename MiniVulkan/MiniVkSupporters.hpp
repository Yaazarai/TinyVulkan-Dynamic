#pragma once
#ifndef MINIVK_MINIVKSUPPORTERS
#define MINIVK_MINIVKSUPPORTERS
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
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

		enum class MiniVkBufferingMode { DOUBLE = 2, TRIPLE = 3, QUADRUPLE = 4 };

		struct MiniVkSwapChainSupporter {
		public:
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		struct MiniVkSurfaceSupporter {
		public:
			VkFormat dataFormat = VK_FORMAT_B8G8R8A8_SRGB;
			VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			VkPresentModeKHR idealPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		};
	}
#endif