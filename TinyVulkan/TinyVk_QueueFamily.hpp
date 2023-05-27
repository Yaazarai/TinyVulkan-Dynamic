#pragma once
#ifndef TINYVK_TINYVKQUEUEFAMILY
#define TINYVK_TINYVKQUEUEFAMILY
	#include "./TinyVK.hpp"
	
	namespace TINYVULKAN_NAMESPACE {
		class TinyVkQueueFamily {
		public:
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			~TinyVkQueueFamily() {}
			TinyVkQueueFamily() {}

			/// <summary>Copy constructor.</summary>
			TinyVkQueueFamily(const TinyVkQueueFamily& qf) {
				graphicsFamily = qf.graphicsFamily;
				presentFamily = qf.presentFamily;
			}

			bool IsComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }

			/// <summary>Returns info about the VkPhysicalDevice graphics/present queue families. If no surface provided, auto checks for Win32 surface support.</summary>
			inline static TinyVkQueueFamily FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface = nullptr) {
				TinyVkQueueFamily indices;

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

					if (presentSupport)
						indices.presentFamily = i;

					if (indices.IsComplete()) break;
					i++;
				}

				return indices;
			}
		};
	}
#endif