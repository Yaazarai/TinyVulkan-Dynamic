#pragma once
#ifndef MINIVK_MINIVKDYNAMICRENDERER
#define MINIVK_MINIVKDYNAMICRENDERER
	#include "./MiniVk.hpp"

	namespace MINIVULKAN_NS {
		#pragma region DYNAMIC RENDERING FUNCTIONS

		VkResult vkCmdBeginRenderingEKHR(VkInstance instance, VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo) {
			auto func = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
			if (func == VK_NULL_HANDLE) throw std::runtime_error("MiniVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdBeginRenderingKHR");
			func(commandBuffer, pRenderingInfo);
			return VK_SUCCESS;
		}

		VkResult vkCmdEndRenderingEKHR(VkInstance instance, VkCommandBuffer commandBuffer) {
			auto func = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");
			if (func == VK_NULL_HANDLE) throw std::runtime_error("MiniVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdEndRenderingKHR");
			func(commandBuffer);
			return VK_SUCCESS;
		}

		#pragma endregion

		class MiniVkDynamicSwapChainRenderer : public MiniVkObject {
		private:
			MiniVkInstance& mvkLayer;
		public:
			MiniVkSwapChain& swapChain;
			MiniVkDynamicPipeline& graphicsPipeline;

			/// SWAPCHAIN SYNCHRONIZATION_OBJECTS ///
			std::vector<VkSemaphore> swapChain_imageAvailableSemaphores;
			std::vector<VkSemaphore> swapChain_renderFinishedSemaphores;
			std::vector<VkFence> swapChain_inFlightFences;

			/// INVOKABLE RENDER EVENTS: (executed in MiniVkDynamicRenderer::RenderFrame()
			std::invokable<VkCommandBuffer> onRenderEvents;

			/// COMMAND POOL FOR RENDER COMMANDS
			MiniVkCommandPool& commandPool;

			void Disposable() {
				vkDeviceWaitIdle(mvkLayer.logicalDevice);

				for (size_t i = 0; i < swapChain_inFlightFences.size(); i++) {
					vkDestroySemaphore(mvkLayer.logicalDevice, swapChain_imageAvailableSemaphores[i], nullptr);
					vkDestroySemaphore(mvkLayer.logicalDevice, swapChain_renderFinishedSemaphores[i], nullptr);
					vkDestroyFence(mvkLayer.logicalDevice, swapChain_inFlightFences[i], nullptr);
				}
			}

			MiniVkDynamicSwapChainRenderer(MiniVkInstance& mvkLayer, MiniVkCommandPool& commandPool, MiniVkSwapChain& swapChain, MiniVkDynamicPipeline& graphicsPipeline) :
			mvkLayer(mvkLayer), commandPool(commandPool), swapChain(swapChain), graphicsPipeline(graphicsPipeline) {
				onDispose += std::callback<>(this, &MiniVkDynamicSwapChainRenderer::Disposable);

				CreateSwapChainSyncObjects();
			}

			void CreateSwapChainSyncObjects() {
				swapChain_imageAvailableSemaphores.resize(static_cast<size_t>(swapChain.swapChainImages.size()));
				swapChain_renderFinishedSemaphores.resize(static_cast<size_t>(swapChain.swapChainImages.size()));
				swapChain_inFlightFences.resize(static_cast<size_t>(swapChain.swapChainImages.size()));

				VkSemaphoreCreateInfo semaphoreInfo{};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VkFenceCreateInfo fenceInfo{};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

				for (size_t i = 0; i < swapChain.swapChainImages.size(); i++) {
					if (vkCreateSemaphore(mvkLayer.logicalDevice, &semaphoreInfo, nullptr, &swapChain_imageAvailableSemaphores[i]) != VK_SUCCESS ||
						vkCreateSemaphore(mvkLayer.logicalDevice, &semaphoreInfo, nullptr, &swapChain_renderFinishedSemaphores[i]) != VK_SUCCESS ||
						vkCreateFence(mvkLayer.logicalDevice, &fenceInfo, nullptr, &swapChain_inFlightFences[i]) != VK_SUCCESS)
						throw std::runtime_error("MiniVulkan: Failed to create synchronization objects for a frame!");
				}
			}

			/// <summary>Begins recording commands into a command buffers.</summary>
			void BeginRecordCommandBuffer(VkCommandBuffer commandBuffer, const VkClearValue clearColor, VkImageView& renderTarget, VkImage& renderImageTarget, VkExtent2D renderArea) {
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0; // Optional
				beginInfo.pInheritanceInfo = nullptr; // Optional

				if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [begin] to command buffer!");

				const VkImageMemoryBarrier image_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.image = renderImageTarget,
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					}
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &image_memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = renderTarget;
				colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
				colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentInfo.clearValue = clearColor;

				VkRenderingInfoKHR dynamicRenderInfo{};
				dynamicRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

				VkRect2D renderAreaKHR{};
				renderAreaKHR.extent = renderArea;
				renderAreaKHR.offset = { 0,0 };
				dynamicRenderInfo.renderArea = renderAreaKHR;
				dynamicRenderInfo.layerCount = 1;
				dynamicRenderInfo.colorAttachmentCount = 1;
				dynamicRenderInfo.pColorAttachments = &colorAttachmentInfo;

				VkViewport dynamicViewportKHR{};
				dynamicViewportKHR.x = 0;
				dynamicViewportKHR.y = 0;
				dynamicViewportKHR.width = static_cast<float>(renderArea.width);
				dynamicViewportKHR.height = static_cast<float>(renderArea.height);
				dynamicViewportKHR.minDepth = 0.0f;
				dynamicViewportKHR.maxDepth = 1.0f;
				vkCmdSetViewport(commandBuffer, 0, 1, &dynamicViewportKHR);
				vkCmdSetScissor(commandBuffer, 0, 1, &renderAreaKHR);

				vkCmdBeginRenderingEKHR(mvkLayer.instance, commandBuffer, &dynamicRenderInfo);
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.graphicsPipeline);
			}

			/// <summary>Ends recording to the command buffer.</summary>
			void EndRecordCommandBuffer(VkCommandBuffer commandBuffer, const VkClearValue clearColor, VkImageView& renderTarget, VkImage& renderImageTarget, VkExtent2D renderArea) {
				vkCmdEndRenderingEKHR(mvkLayer.instance, commandBuffer);

				const VkImageMemoryBarrier image_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.image = renderImageTarget,
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					}
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &image_memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = renderTarget;
				colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
				colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentInfo.clearValue = clearColor;

				VkRenderingInfoKHR dynamicRenderInfo{};
				dynamicRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

				VkRect2D renderAreaKHR{};
				renderAreaKHR.extent = renderArea;
				renderAreaKHR.offset = { 0,0 };
				dynamicRenderInfo.renderArea = renderAreaKHR;
				dynamicRenderInfo.layerCount = 1;
				dynamicRenderInfo.colorAttachmentCount = 1;
				dynamicRenderInfo.pColorAttachments = &colorAttachmentInfo;

				VkViewport dynamicViewportKHR{};
				dynamicViewportKHR.x = 0;
				dynamicViewportKHR.y = 0;
				dynamicViewportKHR.width = static_cast<float>(renderArea.width);
				dynamicViewportKHR.height = static_cast<float>(renderArea.height);
				dynamicViewportKHR.minDepth = 0.0f;
				dynamicViewportKHR.maxDepth = 1.0f;
				vkCmdSetViewport(commandBuffer, 0, 1, &dynamicViewportKHR);
				vkCmdSetScissor(commandBuffer, 0, 1, &renderAreaKHR);

				if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [end] to command buffer!");
			}

			/// <summary>Resets the command buffer for new input.</summary>
			void ResetCommandBuffer(VkCommandBuffer commandBuffer) { vkResetCommandBuffer(commandBuffer, 0); }

			void RenderFrame() {
				std::vector<VkCommandBuffer>& commandBuffers = commandPool.GetBuffers();
				vkWaitForFences(mvkLayer.logicalDevice, 1, &swapChain_inFlightFences[swapChain.currentFrame], VK_TRUE, UINT64_MAX);

				uint32_t imageIndex;
				VkResult result = vkAcquireNextImageKHR(mvkLayer.logicalDevice, swapChain.swapChain, UINT64_MAX, swapChain_imageAvailableSemaphores[swapChain.currentFrame], VK_NULL_HANDLE, &imageIndex);
				VkCommandBuffer cmdBuffer = commandBuffers[imageIndex];

				if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || swapChain.framebufferResized) {
					swapChain.SetFrameBufferResized(false);
					swapChain.ReCreateSwapChain();
					return;
				} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
					throw std::runtime_error("MiniVulkan: Failed to acquire swap chain image!");

				vkResetFences(mvkLayer.logicalDevice, 1, &swapChain_inFlightFences[swapChain.currentFrame]);
				vkResetCommandBuffer(cmdBuffer, 0); //VkCommandBufferResetFlagBits

				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				onRenderEvents.invoke(cmdBuffer);
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				VkSemaphore waitSemaphores[] = { swapChain_imageAvailableSemaphores[swapChain.currentFrame] };
				VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = waitSemaphores;
				submitInfo.pWaitDstStageMask = waitStages;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &cmdBuffer;

				VkSemaphore signalSemaphores[] = { swapChain_renderFinishedSemaphores[swapChain.currentFrame] };
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = signalSemaphores;

				if (vkQueueSubmit(graphicsPipeline.graphicsQueue, 1, &submitInfo, swapChain_inFlightFences[swapChain.currentFrame]) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to submit draw command buffer!");

				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				presentInfo.waitSemaphoreCount = 1;
				presentInfo.pWaitSemaphores = signalSemaphores;
			
				VkSwapchainKHR swapChainList[] { swapChain.swapChain };
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = swapChainList;
				presentInfo.pImageIndices = &imageIndex;

				result = vkQueuePresentKHR(graphicsPipeline.presentQueue, &presentInfo);

				swapChain.currentFrame = (swapChain.currentFrame + 1) % swapChain.swapChainImages.size();

				if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || swapChain.framebufferResized) {
					swapChain.SetFrameBufferResized(false);
					swapChain.ReCreateSwapChain();
				} else if (result != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to present swap chain image!");
			}
		};

		/*
		class MiniVkDynamicImageRenderer : public MiniVkObject {
		private:
			MiniVkInstance& mvkLayer;
		public:
			MiniVkDynamicPipeline& graphicsPipeline;
			std::invokable<VkCommandBuffer> onRenderEvents;
			MiniVkCommandPool& commandPool;

			std::vector<MiniVkImage&> renderImages;
			std::vector<VkSemaphore> availableSemaphores;
			std::vector<VkSemaphore> finishedSemaphores;
			std::vector<VkFence> inFlightFences;
			uint32_t currentRenderImage;

			void Disposable() {
				vkDeviceWaitIdle(mvkLayer.logicalDevice);

				for (size_t i = 0; i < renderImages.size(); i++) {
					vkDestroySemaphore(mvkLayer.logicalDevice, availableSemaphores[i], nullptr);
					vkDestroySemaphore(mvkLayer.logicalDevice, finishedSemaphores[i], nullptr);
					vkDestroyFence(mvkLayer.logicalDevice, inFlightFences[i], nullptr);
				}
			}

			MiniVkDynamicImageRenderer(MiniVkInstance& mvkLayer, MiniVkCommandPool& commandPool, MiniVkDynamicPipeline& graphicsPipeline, std::vector<MiniVkImage&> images)
				: mvkLayer(mvkLayer), commandPool(commandPool), graphicsPipeline(graphicsPipeline), renderImages(images) {
				onDispose += std::callback<>(this, &MiniVkDynamicImageRenderer::Disposable);
			}

			void CreateSyncObjects() {
				availableSemaphores.resize(renderImages.size());
				finishedSemaphores.resize(renderImages.size());
				inFlightFences.resize(renderImages.size());

				VkSemaphoreCreateInfo semaphoreInfo{};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VkFenceCreateInfo fenceInfo{};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

				for (size_t i = 0; i < renderImages.size(); i++) {
					if (vkCreateSemaphore(mvkLayer.logicalDevice, &semaphoreInfo, nullptr, &availableSemaphores[i]) != VK_SUCCESS ||
						vkCreateSemaphore(mvkLayer.logicalDevice, &semaphoreInfo, nullptr, &finishedSemaphores[i]) != VK_SUCCESS ||
						vkCreateFence(mvkLayer.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
						throw std::runtime_error("MiniVulkan: Failed to create synchronization objects for a image!");
				}
			}

			/// <summary>Begins recording commands into a command buffers.</summary>
			void BeginRecordCommandBuffer(VkCommandBuffer commandBuffer, const VkClearValue clearColor, VkImageView& renderTarget, VkImage& renderImageTarget, VkExtent2D renderArea) {
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = 0; // Optional
				beginInfo.pInheritanceInfo = nullptr; // Optional

				if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [begin] to command buffer!");

				const VkImageMemoryBarrier image_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.image = renderImageTarget,
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					}
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &image_memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = renderTarget;
				colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
				colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentInfo.clearValue = clearColor;

				VkRenderingInfoKHR dynamicRenderInfo{};
				dynamicRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

				VkRect2D renderAreaKHR{};
				renderAreaKHR.extent = renderArea;
				renderAreaKHR.offset = { 0,0 };
				dynamicRenderInfo.renderArea = renderAreaKHR;
				dynamicRenderInfo.layerCount = 1;
				dynamicRenderInfo.colorAttachmentCount = 1;
				dynamicRenderInfo.pColorAttachments = &colorAttachmentInfo;

				VkViewport dynamicViewportKHR{};
				dynamicViewportKHR.x = 0;
				dynamicViewportKHR.y = 0;
				dynamicViewportKHR.width = static_cast<float>(renderArea.width);
				dynamicViewportKHR.height = static_cast<float>(renderArea.height);
				dynamicViewportKHR.minDepth = 0.0f;
				dynamicViewportKHR.maxDepth = 1.0f;
				vkCmdSetViewport(commandBuffer, 0, 1, &dynamicViewportKHR);
				vkCmdSetScissor(commandBuffer, 0, 1, &renderAreaKHR);

				vkCmdBeginRenderingEKHR(mvkLayer.instance, commandBuffer, &dynamicRenderInfo);
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.graphicsPipeline);
			}

			/// <summary>Ends recording to the command buffer.</summary>
			void EndRecordCommandBuffer(VkCommandBuffer commandBuffer, const VkClearValue clearColor, VkImageView& renderTarget, VkImage& renderImageTarget, VkExtent2D renderArea) {
				vkCmdEndRenderingEKHR(mvkLayer.instance, commandBuffer);

				const VkImageMemoryBarrier image_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.image = renderImageTarget,
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					}
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &image_memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = renderTarget;
				colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
				colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachmentInfo.clearValue = clearColor;

				VkRenderingInfoKHR dynamicRenderInfo{};
				dynamicRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;

				VkRect2D renderAreaKHR{};
				renderAreaKHR.extent = renderArea;
				renderAreaKHR.offset = { 0,0 };
				dynamicRenderInfo.renderArea = renderAreaKHR;
				dynamicRenderInfo.layerCount = 1;
				dynamicRenderInfo.colorAttachmentCount = 1;
				dynamicRenderInfo.pColorAttachments = &colorAttachmentInfo;

				VkViewport dynamicViewportKHR{};
				dynamicViewportKHR.x = 0;
				dynamicViewportKHR.y = 0;
				dynamicViewportKHR.width = static_cast<float>(renderArea.width);
				dynamicViewportKHR.height = static_cast<float>(renderArea.height);
				dynamicViewportKHR.minDepth = 0.0f;
				dynamicViewportKHR.maxDepth = 1.0f;
				vkCmdSetViewport(commandBuffer, 0, 1, &dynamicViewportKHR);
				vkCmdSetScissor(commandBuffer, 0, 1, &renderAreaKHR);

				if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [end] to command buffer!");
			}

			/// <summary>Resets the command buffer for new input.</summary>
			void ResetCommandBuffer(VkCommandBuffer commandBuffer) { vkResetCommandBuffer(commandBuffer, 0); }

			void RenderToImage(uint32_t imageIndex) {
				std::vector<VkCommandBuffer>& commandBuffers = commandPool.GetBuffers();
				vkWaitForFences(mvkLayer.logicalDevice, 1, &inFlightFences[currentRenderImage], VK_TRUE, UINT64_MAX);

				VkCommandBuffer cmdBuffer = commandBuffers[imageIndex];
				currentRenderImage = imageIndex;

				vkResetFences(mvkLayer.logicalDevice, 1, &inFlightFences[currentRenderImage]);
				vkResetCommandBuffer(cmdBuffer, 0);

				onRenderEvents.invoke(cmdBuffer);

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				VkSemaphore waitSemaphores[] = { availableSemaphores[currentRenderImage] };
				VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = waitSemaphores;
				submitInfo.pWaitDstStageMask = waitStages;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &cmdBuffer;

				VkSemaphore signalSemaphores[] = { finishedSemaphores[currentRenderImage] };
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = signalSemaphores;

				if (vkQueueSubmit(graphicsPipeline.graphicsQueue, 1, &submitInfo, inFlightFences[currentRenderImage]) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to submit draw command buffer!");
			}
		};
		*/
	}
#endif