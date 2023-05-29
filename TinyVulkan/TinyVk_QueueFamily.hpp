#pragma once
#ifndef TINYVK_TINYVKQUEUEFAMILY
#define TINYVK_TINYVKQUEUEFAMILY
	#include "./TinyVK.hpp"
	
	namespace TINYVULKAN_NAMESPACE {
		/// <summary>Represents a valid QueueFamily graphics/present pair on the VkDevice.</summary>
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

			/// <summary>Returns true/false if this is a complete graphics and present queue family pair.</summary>
			bool IsComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }

			/// <summary>Returns info about the VkPhysicalDevice graphics/present queue families. If no surface provided, auto checks for Win32 surface support.</summary>
			inline static TinyVkQueueFamily FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface = nullptr) {
				TinyVkQueueFamily indices;

				uint32_t queueFamilyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

				std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

				for (int i = 0; i < queueFamilies.size(); i++) {
					const auto& queueFamily = queueFamilies[i];
					if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
						indices.graphicsFamily = i;

					VkBool32 presentSupport = false;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

					if (presentSupport)
						indices.presentFamily = i;

					if (indices.IsComplete()) break;
				}

				return indices;
			}
		};
	}
#endif