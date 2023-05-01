#pragma once
#ifndef MINIVULKAN_MINIVKDYNAMICRENDERER
#define MINIVULKAN_MINIVKDYNAMICRENDERER
	#include "./MiniVK.hpp"

	namespace MINIVULKAN_NAMESPACE {
		#pragma region DYNAMIC_RENDERING_FUNCTIONS

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

		VkResult vkCmdSetColorBlendEnableEKHR(VkInstance instance, VkCommandBuffer commandBuffer, uint32_t first, uint32_t attachmentCount, const std::vector<VkBool32> blendTesting) {
			auto func = (PFN_vkCmdSetColorBlendEnableEXT) vkGetInstanceProcAddr(instance, "vkCmdSetColorBlendEnableEXT");
			if (func == VK_NULL_HANDLE) throw std::runtime_error("MiniVulkan: Failed to load VK_KHR_dynamic_rendering EXT function: PFN_vkCmdSetColorBlendEnableEXT");
			func(commandBuffer, first, attachmentCount, blendTesting.data());
			return VK_SUCCESS;
		}

		#pragma endregion

		/// 
		/// RENDERING PARADIGM:
		///		MiniVkImageRenderer is for rendering directly to a VkImage render target for offscreen rendering.
		///		Call RenderExecute(mutex[optional], preRecordedCommandBuffer[optiona]) to render to the image.
		///			You may pass a pre-recorded command buffer ready to go, or retrieve a command buffer from
		///			the underlying command pool queue and build your command buffer with a render event (onRenderEvent).
		///				onRenderEvent += callback<VkCommandBuffer>(...);
		///				
		///			If you use a render event, the command buffer will be returned to the command pool queue after execution.
		///		
		///		MiniVkSwapChainRenderer is for rendering directly to the SwapChain for onscreen rendering.
		///		Call RenderExecute(mutex[optional]) to render to the swap chain image.
		///			All swap chain rendering is done via render events and does not accept pre-recorded command buffers.
		///			The command buffer will be returned to the command pool queue after execution.
		///	
		///	Why? This rendering paradigm allows the swap chain to effectively manage and synchronize its own resources
		///	minimizing screen rendering errors or validation layer errors.
		/// 
		/// Both the MiniVkImageRenderer and MiniVkSwapChainRenderer contain their own managed depthImages which will
		/// be optionally created and utilized if their underlying graphics pipeline supports depth testing.
		/// 

		/// <summary>Offscreen Rendering: Render to VkImage.</summary>
		class MiniVkImageRenderer : public disposable {
		private:
			MiniVkRenderDevice& renderDevice;
			MiniVkVMAllocator& vmAlloc;
			MiniVkDynamicPipeline& graphicsPipeline;
			MiniVkCmdPoolQueue& cmdPoolQueue;
			MiniVkImage* optionalDepthImage;

			VkFormat QueryDepthFormat(VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				const std::vector<VkFormat>& candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
				for (VkFormat format : candidates) {
					VkFormatProperties props;
					vkGetPhysicalDeviceFormatProperties(renderDevice.physicalDevice, format, &props);

					if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
						return format;
					} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
						return format;
					}
				}

				throw std::runtime_error("MiniVulkan: Failed to find supported format!");
			}
		public:
			MiniVkImage* renderTarget;

			invokable<VkCommandBuffer> onRenderEvent;

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					optionalDepthImage->Dispose();
					delete optionalDepthImage;
				}
			}

			MiniVkImageRenderer(MiniVkRenderDevice& renderDevice, MiniVkVMAllocator& vmAlloc, MiniVkCmdPoolQueue& cmdPoolQueue, MiniVkImage* renderTarget, MiniVkDynamicPipeline& graphicsPipeline)
			: renderDevice(renderDevice), vmAlloc(vmAlloc), cmdPoolQueue(cmdPoolQueue), graphicsPipeline(graphicsPipeline), renderTarget(renderTarget) {
				//onDispose.hook(callback<bool>(this, &MiniVkImageRenderer::Disposable));
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				optionalDepthImage = nullptr;
				if (graphicsPipeline.DepthTestingIsEnabled())
					optionalDepthImage = new MiniVkImage(renderDevice, vmAlloc, renderTarget->width, renderTarget->height, true, QueryDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_ASPECT_DEPTH_BIT);
			}

			void SetRenderTarget(MiniVkImage* renderTarget, bool waitOldTarget = true) {
				if (this->renderTarget != nullptr && waitOldTarget) {
					vkWaitForFences(renderDevice.logicalDevice, 1, &renderTarget->imageWaitable, VK_TRUE, UINT64_MAX);
					vkResetFences(renderDevice.logicalDevice, 1, &renderTarget->imageWaitable);
				}

				this->renderTarget = renderTarget;
			}

			void BeginRecordCmdBuffer(VkCommandBuffer commandBuffer, VkExtent2D renderArea, const VkClearValue clearColor, const VkClearValue depthStencil, bool enableColorBlending = true) {
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				if (renderTarget == nullptr)
					throw new std::runtime_error("MiniVulkan: RenderTarget for MiniVkImageRenderer is not set [nullptr]!");

				if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [begin] to command buffer!");

				const VkImageMemoryBarrier swapchain_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.image = renderTarget->image,
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					},
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &swapchain_memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = renderTarget->imageView;
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

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					const VkImageMemoryBarrier depth_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
						.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.image = optionalDepthImage->image,
						.subresourceRange = {
						  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						  .baseMipLevel = 0,
						  .levelCount = 1,
						  .baseArrayLayer = 0,
						  .layerCount = 1,
						},
					};
					
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &depth_memory_barrier);

					VkRenderingAttachmentInfoKHR depthStencilAttachmentInfo{};
					depthStencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
					depthStencilAttachmentInfo.imageView = optionalDepthImage->imageView;
					depthStencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
					depthStencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					depthStencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					depthStencilAttachmentInfo.clearValue = depthStencil;
					depthStencilAttachmentInfo.imageLayout = optionalDepthImage->layout;
					dynamicRenderInfo.pDepthAttachment = &depthStencilAttachmentInfo;
				}

				VkViewport dynamicViewportKHR{};
				dynamicViewportKHR.x = 0;
				dynamicViewportKHR.y = 0;
				dynamicViewportKHR.width = static_cast<float>(renderArea.width);
				dynamicViewportKHR.height = static_cast<float>(renderArea.height);
				dynamicViewportKHR.minDepth = 0.0f;
				dynamicViewportKHR.maxDepth = 1.0f;

				vkCmdSetViewport(commandBuffer, 0, 1, &dynamicViewportKHR);
				vkCmdSetScissor(commandBuffer, 0, 1, &renderAreaKHR);

				if (vkCmdBeginRenderingEKHR(renderDevice.instance.instance, commandBuffer, &dynamicRenderInfo) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [begin] to rendering!");
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.graphicsPipeline);

				//vkCmdSetColorBlendEnableEKHR(renderDevice.instance.instance, commandBuffer, 0U, 1U, { (VkBool32)enableColorBlending });
			}

			void EndRecordCmdBuffer(VkCommandBuffer commandBuffer, VkExtent2D renderArea, const VkClearValue clearColor, const VkClearValue depthStencil) {
				if (renderTarget == nullptr)
					throw new std::runtime_error("MiniVulkan: RenderTarget for MiniVkImageRenderer is not set [nullptr]!");

				if (vkCmdEndRenderingEKHR(renderDevice.instance.instance, commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [end] to rendering!");

				const VkImageMemoryBarrier image_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					.image = renderTarget->image,
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
				colorAttachmentInfo.imageView = renderTarget->imageView;
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

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					const VkImageMemoryBarrier depth_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
						.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.image = optionalDepthImage->image,
						.subresourceRange = {
						  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						  .baseMipLevel = 0,
						  .levelCount = 1,
						  .baseArrayLayer = 0,
						  .layerCount = 1,
						},
					};

					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &depth_memory_barrier);

					VkRenderingAttachmentInfoKHR depthStencilAttachmentInfo{};
					depthStencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
					depthStencilAttachmentInfo.imageView = optionalDepthImage->imageView;
					depthStencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
					depthStencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					depthStencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					depthStencilAttachmentInfo.clearValue = depthStencil;
					depthStencilAttachmentInfo.imageLayout = optionalDepthImage->layout;
					dynamicRenderInfo.pDepthAttachment = &depthStencilAttachmentInfo;
				}

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

			VkResult PushDescriptorSet(VkCommandBuffer cmdBuffer, std::vector<VkWriteDescriptorSet> writeDescriptorSets) {
				return vkCmdPushDescriptorSetEKHR(renderDevice.instance.instance, cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
					0, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data());
			}

			void PushConstants(VkCommandBuffer cmdBuffer, VkShaderStageFlagBits shaderFlags, uint32_t byteSize, const void* pValues) {
				vkCmdPushConstants(cmdBuffer, graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, byteSize, pValues);
			}
			
			void RenderExecute(VkCommandBuffer preRecordedCmdBuffer = nullptr) {
				atomic_lock swapChainLock(renderTarget->image_lock);
				if (!swapChainLock.AcquiredLock()) return;
				
				if (renderTarget == nullptr)
					throw new std::runtime_error("MiniVulkan: RenderTarget for MiniVkImageRenderer is not set [nullptr]!");

				vkWaitForFences(renderDevice.logicalDevice, 1, &renderTarget->imageWaitable, VK_TRUE, UINT64_MAX);
				vkResetFences(renderDevice.logicalDevice, 1, &renderTarget->imageWaitable);
				
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (graphicsPipeline.DepthTestingIsEnabled()) {
					MiniVkImage* depthImage = optionalDepthImage;
					if (depthImage->width < renderTarget->width || depthImage->height < renderTarget->height) {
						depthImage->Disposable(false);
						depthImage->ReCreateImage(renderTarget->width, renderTarget->height, depthImage->isDepthImage, QueryDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_ASPECT_DEPTH_BIT);
					}
				}
				
				VkCommandBuffer renderBuffer = preRecordedCmdBuffer;
				if (renderBuffer == nullptr) {
					int32_t cmdBufferIndex;
					renderBuffer = cmdPoolQueue.RentBuffer(cmdBufferIndex);
					vkResetCommandBuffer(renderBuffer, 0);
					
					onRenderEvent.invoke(renderBuffer);
					
					cmdPoolQueue.ReturnBuffer(cmdBufferIndex);
				}
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &renderBuffer;

				if (vkQueueSubmit(graphicsPipeline.graphicsQueue, 1, &submitInfo, renderTarget->imageWaitable) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to submit draw command buffer!");
			}
		};

		/// <summary>Onscreen Rendering: Render to SwapChain.</summary>
		class MiniVkSwapChainRenderer : public disposable {
		private:
			MiniVkRenderDevice& renderDevice;
			MiniVkVMAllocator& memAlloc;

			VkFormat QueryDepthFormat(VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				const std::vector<VkFormat>& candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
				for (VkFormat format : candidates) {
					VkFormatProperties props;
					vkGetPhysicalDeviceFormatProperties(renderDevice.physicalDevice, format, &props);

					if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
						return format;
					} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
						return format;
					}
				}

				throw std::runtime_error("MiniVulkan: Failed to find supported format!");
			}
		
		public:
			MiniVkSwapChain& swapChain;
			MiniVkDynamicPipeline& graphicsPipeline;

			/// SWAPCHAIN SYNCHRONIZATION_OBJECTS ///
			std::vector<VkSemaphore> imageAvailableSemaphores;
			std::vector<VkSemaphore> renderFinishedSemaphores;
			std::vector<VkFence> inFlightFences;

			/// COMMAND POOL FOR RENDER COMMANDS ///
			/// RENDERING DEPTH IMAGE ///
			MiniVkCommandPool& commandPool;
			std::vector<MiniVkImage*> optionalDepthImages;

			/// SWAPCHAIN FRAME MANAGEMENT ///
			size_t currentSyncFrame = 0;
			size_t currentSwapFrame = 0;

			/// INVOKABLE RENDER EVENTS: (executed in MiniVkDynamicRenderer::RenderFrame() ///
			invokable<VkCommandBuffer> onRenderEvents;

			void Disposable(bool waitIdle) {
				if (waitIdle) vkDeviceWaitIdle(renderDevice.logicalDevice);

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					for (MiniVkImage* depthImage : optionalDepthImages) {
						depthImage->Dispose();
						delete depthImage;
					}
				}

				for (size_t i = 0; i < inFlightFences.size(); i++) {
					vkDestroySemaphore(renderDevice.logicalDevice, imageAvailableSemaphores[i], nullptr);
					vkDestroySemaphore(renderDevice.logicalDevice, renderFinishedSemaphores[i], nullptr);
					vkDestroyFence(renderDevice.logicalDevice, inFlightFences[i], nullptr);
				}
			}

			MiniVkSwapChainRenderer(MiniVkRenderDevice& renderDevice, MiniVkVMAllocator& memAlloc, MiniVkCommandPool& commandPool, MiniVkSwapChain& swapChain, MiniVkDynamicPipeline& graphicsPipeline)
				: renderDevice(renderDevice), memAlloc(memAlloc), commandPool(commandPool), swapChain(swapChain), graphicsPipeline(graphicsPipeline) {
				onDispose.hook(callback<bool>([this](bool forceDispose) {this->Disposable(forceDispose); }));

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					for (int32_t i = 0; i < swapChain.images.size(); i++)
						optionalDepthImages.push_back(new MiniVkImage(renderDevice, memAlloc, swapChain.imageExtent.width * 4, swapChain.imageExtent.height * 4, true, QueryDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_ASPECT_DEPTH_BIT));
				}

				CreateImageSyncObjects();
			}

			void CreateImageSyncObjects() {
				size_t count = static_cast<size_t>(swapChain.bufferingMode);
				imageAvailableSemaphores.resize(count);
				renderFinishedSemaphores.resize(count);
				inFlightFences.resize(count);

				VkSemaphoreCreateInfo semaphoreInfo{};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VkFenceCreateInfo fenceInfo{};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

				for (size_t i = 0; i < swapChain.images.size(); i++) {
					if (vkCreateSemaphore(renderDevice.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
						vkCreateSemaphore(renderDevice.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
						vkCreateFence(renderDevice.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
						throw std::runtime_error("MiniVulkan: Failed to create synchronization objects for a frame!");
				}
			}

			void BeginRecordCmdBuffer(VkCommandBuffer commandBuffer, VkExtent2D renderArea, const VkClearValue clearColor, const VkClearValue depthStencil, bool enableColorBlending = true) {
				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [begin] to command buffer!");

				const VkImageMemoryBarrier swapchain_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.image = swapChain.images[currentSwapFrame],
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					},
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &swapchain_memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = swapChain.imageViews[currentSwapFrame];
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
				
				if (graphicsPipeline.DepthTestingIsEnabled()) {
					const VkImageMemoryBarrier depth_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
						.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.image = optionalDepthImages[currentSyncFrame]->image,
						.subresourceRange = {
						  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						  .baseMipLevel = 0,
						  .levelCount = 1,
						  .baseArrayLayer = 0,
						  .layerCount = 1,
						},
					};

					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &depth_memory_barrier);

					VkRenderingAttachmentInfoKHR depthStencilAttachmentInfo{};
					depthStencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
					depthStencilAttachmentInfo.imageView = optionalDepthImages[currentSyncFrame]->imageView;
					depthStencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
					depthStencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					depthStencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					depthStencilAttachmentInfo.clearValue = depthStencil;
					depthStencilAttachmentInfo.imageLayout = optionalDepthImages[currentSyncFrame]->layout;
					dynamicRenderInfo.pDepthAttachment = &depthStencilAttachmentInfo;

					std::cout << "USE DEPTH TESTING" << std::endl;
				}

				VkViewport dynamicViewportKHR{};
				dynamicViewportKHR.x = 0;
				dynamicViewportKHR.y = 0;
				dynamicViewportKHR.width = static_cast<float>(renderArea.width);
				dynamicViewportKHR.height = static_cast<float>(renderArea.height);
				dynamicViewportKHR.minDepth = 0.0f;
				dynamicViewportKHR.maxDepth = 1.0f;

				vkCmdSetViewport(commandBuffer, 0, 1, &dynamicViewportKHR);
				vkCmdSetScissor(commandBuffer, 0, 1, &renderAreaKHR);

				if (vkCmdBeginRenderingEKHR(renderDevice.instance.instance, commandBuffer, &dynamicRenderInfo) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [begin] to rendering!");
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.graphicsPipeline);

				//vkCmdSetColorBlendEnableEKHR(renderDevice.instance.instance, commandBuffer, 0U, 1U, { (VkBool32)enableColorBlending });
			}

			void EndRecordCmdBuffer(VkCommandBuffer commandBuffer, VkExtent2D renderArea, const VkClearValue clearColor, const VkClearValue depthStencil) {
				if (vkCmdEndRenderingEKHR(renderDevice.instance.instance, commandBuffer) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to record [end] to rendering!");

				const VkImageMemoryBarrier swapchain_memory_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.image = swapChain.images[currentSwapFrame],
					.subresourceRange = {
					  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					  .baseMipLevel = 0,
					  .levelCount = 1,
					  .baseArrayLayer = 0,
					  .layerCount = 1,
					}
				};

				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &swapchain_memory_barrier);

				VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
				colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
				colorAttachmentInfo.imageView = swapChain.imageViews[currentSwapFrame];
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

				if (graphicsPipeline.DepthTestingIsEnabled()) {
					const VkImageMemoryBarrier depth_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
						.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						.image = optionalDepthImages[currentSyncFrame]->image,
						.subresourceRange = {
						  .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						  .baseMipLevel = 0,
						  .levelCount = 1,
						  .baseArrayLayer = 0,
						  .layerCount = 1,
						},
					};

					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &depth_memory_barrier);

					VkRenderingAttachmentInfoKHR depthStencilAttachmentInfo{};
					depthStencilAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
					depthStencilAttachmentInfo.imageView = optionalDepthImages[currentSyncFrame]->imageView;
					depthStencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
					depthStencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					depthStencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					depthStencilAttachmentInfo.clearValue = depthStencil;
					depthStencilAttachmentInfo.imageLayout = optionalDepthImages[currentSyncFrame]->layout;
					dynamicRenderInfo.pDepthAttachment = &depthStencilAttachmentInfo;
				}

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

			VkResult PushDescriptorSet(VkCommandBuffer cmdBuffer, std::vector<VkWriteDescriptorSet> writeDescriptorSets) {
				return vkCmdPushDescriptorSetEKHR(renderDevice.instance.instance, cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout,
					0, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data());
			}

			void PushConstants(VkCommandBuffer cmdBuffer, VkShaderStageFlagBits shaderFlags, uint32_t byteSize, const void* pValues) {
				vkCmdPushConstants(cmdBuffer, graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, byteSize, pValues);
			}

			void RenderExecute() {
				atomic_lock swapChainLock(swapChain.swapChain_lock);
				if (!swapChainLock.AcquiredLock()) return;

				if (!swapChain.presentable) return;

				vkWaitForFences(renderDevice.logicalDevice, 1, &inFlightFences[currentSyncFrame], VK_TRUE, UINT64_MAX);

				uint32_t imageIndex;
				VkResult result = vkAcquireNextImageKHR(renderDevice.logicalDevice, swapChain.swapChain, UINT64_MAX, imageAvailableSemaphores[currentSyncFrame], VK_NULL_HANDLE, &imageIndex);
				currentSwapFrame = imageIndex;

				vkResetFences(renderDevice.logicalDevice, 1, &inFlightFences[currentSyncFrame]);

				if (result == VK_ERROR_OUT_OF_DATE_KHR) {
					swapChain.presentable = false;
					currentSyncFrame = 0;
				} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
					throw std::runtime_error("MiniVulkan: Failed to acquire swap chain image!");

				if (!swapChain.presentable) return;

				VkCommandBuffer cmdBuffer = commandPool.GetBuffers()[currentSyncFrame];
				vkResetCommandBuffer(cmdBuffer, 0);

				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (graphicsPipeline.DepthTestingIsEnabled()) {
					MiniVkImage* depthImage = optionalDepthImages[currentSyncFrame];
					if (depthImage->width < swapChain.imageExtent.width || depthImage->height < swapChain.imageExtent.height) {
						depthImage->Disposable(false);
						depthImage->ReCreateImage(swapChain.imageExtent.width, swapChain.imageExtent.height, depthImage->isDepthImage, QueryDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_ASPECT_DEPTH_BIT);
					}
				}

				onRenderEvents.invoke(cmdBuffer);
				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentSyncFrame] };
				VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = waitSemaphores;
				submitInfo.pWaitDstStageMask = waitStages;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &cmdBuffer;

				VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentSyncFrame] };
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = signalSemaphores;

				if (vkQueueSubmit(graphicsPipeline.graphicsQueue, 1, &submitInfo, inFlightFences[currentSyncFrame]) != VK_SUCCESS)
					throw std::runtime_error("MiniVulkan: Failed to submit draw command buffer!");

				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				presentInfo.waitSemaphoreCount = 1;
				presentInfo.pWaitSemaphores = signalSemaphores;

				VkSwapchainKHR swapChainList[]{ swapChain.swapChain };
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = swapChainList;
				presentInfo.pImageIndices = &imageIndex;

				result = vkQueuePresentKHR(graphicsPipeline.presentQueue, &presentInfo);
				currentSyncFrame = (currentSyncFrame + 1) % static_cast<size_t>(swapChain.images.size());

				if (result == VK_ERROR_OUT_OF_DATE_KHR) {
					swapChain.presentable = false;
					currentSyncFrame = 0;
				} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
					throw std::runtime_error("MiniVulkan: Failed to acquire swap chain image!");
			}
		};
	}
#endif