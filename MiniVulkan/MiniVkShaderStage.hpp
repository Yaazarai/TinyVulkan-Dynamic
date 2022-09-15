#pragma once
#ifndef MINIVK_MINIVKSHADERSTAGE
#define MINIVK_MINIVKSHADERSTAGE
#include "./MiniVk.hpp"

namespace MINIVULKAN_NS {
	class MiniVkShaderStages : public MiniVkObject {
	private:
		MiniVkInstanceSupportDetails mvkLayer;

	public:
		std::vector<std::string> shaderPaths;
		std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfo;
		std::vector<VkShaderModule> shaderModules;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		void Disposable() {
			vkDeviceWaitIdle(mvkLayer.logicalDevice);
			for(auto shaderModule : shaderModules)
				vkDestroyShaderModule(mvkLayer.logicalDevice, shaderModule, nullptr);
		}

		MiniVkShaderStages(MiniVkInstanceSupportDetails mvkLayer, const std::vector<std::string>& shaderPaths, const std::vector<VkShaderStageFlagBits>& shaderFlagCreateBits) : mvkLayer(mvkLayer), shaderPaths(shaderPaths) {
			onDispose += std::callback<>(this, &MiniVkShaderStages::Disposable);

			for (size_t i = 0; i < shaderPaths.size(); i++) {
				std::cout << shaderPaths[i] << std::endl;
				this->shaderPaths.push_back(shaderPaths[i]);
				auto shaderCode = ReadFile(shaderPaths[i]);
				auto shaderModule = CreateShaderModule(shaderCode);
				shaderModules.push_back(shaderModule);
				shaderCreateInfo.push_back(CreateShaderInfo(shaderPaths[i], shaderModule, shaderFlagCreateBits[i]));
			}

			for (auto stage : shaderCreateInfo)
				shaderStages.push_back(stage);
		}

		MiniVkShaderStages operator=(const MiniVkShaderStages& shader) {
			mvkLayer = shader.mvkLayer;
			shaderPaths.assign(shader.shaderPaths.begin(), shader.shaderPaths.end());
			shaderCreateInfo.assign(shader.shaderCreateInfo.begin(), shader.shaderCreateInfo.end());
			shaderModules.assign(shader.shaderModules.begin(), shader.shaderModules.end());
			shaderStages.assign(shader.shaderStages.begin(), shader.shaderStages.end());
		}

		VkShaderModule CreateShaderModule(std::vector<char> shaderCode) {
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.codeSize = shaderCode.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(mvkLayer.logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
				throw std::runtime_error("MiniVulkan: Failed to create shader module!");

			return shaderModule;
		}

		VkPipelineShaderStageCreateInfo CreateShaderInfo(const std::string& path, VkShaderModule shaderModule, VkShaderStageFlagBits stageFlagBits) {
			/*
			ptrdiff_t pos = std::find(shaderPaths.begin(), shaderPaths.end(), path) - shaderPaths.begin();
			if (pos >= static_cast<long long>(shaderPaths.size()))
				throw std::runtime_error("MiniVulkan: Failed to create ShaderStageCreateInfo: " + path);
			
			VkShaderModule shaderModule = shaderModules[pos];
			*/
			VkPipelineShaderStageCreateInfo shaderStageInfo{};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.stage = stageFlagBits;
			shaderStageInfo.module = shaderModule;
			shaderStageInfo.pName = "main";

			#ifdef _DEBUG
			std::cout << "MiniVulkan: Loading Shader @ " << path << std::endl;
			#endif

			return shaderStageInfo;
		}

		std::vector<char> ReadFile(const std::string& path) {
			std::ifstream file(path, std::ios::ate | std::ios::binary);

			if (!file.is_open())
				throw std::runtime_error("MiniVulkan: Failed to Read File: " + path);

			size_t fsize = static_cast<size_t>(file.tellg());
			std::vector<char> buffer(fsize);
			file.seekg(0);
			file.read(buffer.data(), fsize);
			file.close();
			return buffer;
		}
	};
}
#endif