#pragma once
#ifndef TINYVK_TINYVKSUPPORTERS
#define TINYVK_TINYVKSUPPORTERS
	#include "./TinyVK.hpp"

	namespace TINYVULKAN_NAMESPACE {
		#pragma region VULKAN_DEBUG_UTILITIES

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
			std::cerr << "TinyVulkan: Validation Layer: " << pCallbackData->pMessage << std::endl;
			return VK_FALSE;
		}

		#pragma endregion

		/// <summary>List of valid Buffering Mode sizes.</summary>
		enum class TinyVkBufferingMode {
			DOUBLE = 2,
			TRIPLE = 3,
			QUADRUPLE = 4
		};

		/// <summary>Description of the SwapChain Rendering format.</summary>
		struct TinyVkSwapChainSupporter {
		public:
			VkSurfaceCapabilitiesKHR capabilities = {};
			std::vector<VkSurfaceFormatKHR> formats = {};
			std::vector<VkPresentModeKHR> presentModes = {};
		};

		/// <summary>Description of the Rendering Surface format.</summary>
		struct TinyVkSurfaceSupporter {
		public:
			VkFormat dataFormat = VK_FORMAT_B8G8R8A8_UNORM;
			VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			VkPresentModeKHR idealPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		};
	}
#endif