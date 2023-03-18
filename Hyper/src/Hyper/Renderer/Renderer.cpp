#include "HyperPCH.h"
#include "Renderer.h"

#include <imgui.h>

#include "ShaderLibrary.h"
#include "MaterialLibrary.h"
#include "Hyper/Core/Context.h"
#include "Hyper/Core/Window.h"
#include "Hyper/Debug/Profiler.h"
#include "Hyper/Scene/Scene.h"
#include "Vulkan/VulkanAccelerationStructure.h"
#include "Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanShader.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanTemp.h"
#include "Vulkan/VulkanUtility.h"

namespace Hyper
{
	class Window;

	Renderer::Renderer(Context* pContext)
		: Subsystem(pContext)
		, m_pCamera(std::make_unique<FlyCamera>(pContext))
	{
	}

	bool Renderer::OnInitialize()
	{
		HPR_PROFILE_SCOPE();

		m_pScene = m_pContext->GetSubsystem<Scene>();
		if (!m_pScene)
		{
			HPR_CORE_LOG_ERROR("Failed to get the scene");
			throw std::runtime_error("Failed to get the scene");
		}

		m_pRenderContext = std::make_unique<RenderContext>();
		m_pDevice = std::make_unique<VulkanDevice>(m_pRenderContext.get());
		m_pCommandPool = std::make_unique<VulkanCommandPool>(m_pRenderContext.get());
		m_pShaderLibrary = std::make_unique<ShaderLibrary>(m_pRenderContext.get());
		m_pMaterialLibrary = std::make_unique<MaterialLibrary>(m_pRenderContext.get());
		m_pRenderContext->pShaderLibrary = m_pShaderLibrary.get();
		m_pRenderContext->pMaterialLibrary = m_pMaterialLibrary.get();

		// Create the default sampler
		{
			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.minFilter = vk::Filter::eLinear;
			samplerInfo.magFilter = vk::Filter::eLinear;
			samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

			m_pRenderContext->defaultSampler = VulkanUtils::Check(m_pRenderContext->device.createSampler(samplerInfo));
		}

		Window* pWindow = m_pContext->GetSubsystem<Window>();
		m_pSwapChain = std::make_unique<VulkanSwapChain>(pWindow, m_pRenderContext.get(), pWindow->GetWidth(), pWindow->GetHeight());

		// Setup camera
		m_pCamera->Setup();

		// Setup Dear ImGui
		m_pImGuiWrapper = std::make_unique<ImGuiWrapper>(m_pRenderContext.get(), pWindow->GetHandle(), m_pRenderContext->imageColorFormat, m_pRenderContext->imageDepthFormat);

		// Get command buffers. 1 for each frame in flight.
		m_CommandBuffers = m_pCommandPool->GetCommandBuffers(m_pSwapChain->GetNumFrames());

		// Create sync objects
		{
			m_RenderFinishedSemaphores.resize(m_pRenderContext->imagesInFlight);
			m_InFlightFences.resize(m_pRenderContext->imagesInFlight);

			const vk::SemaphoreCreateInfo semaphoreInfo{};
			const vk::FenceCreateInfo fenceInfo{ vk::FenceCreateFlagBits::eSignaled };

			for (u32 i = 0; i < m_pRenderContext->imagesInFlight; i++)
			{
				m_RenderFinishedSemaphores[i] = VulkanUtils::Check(m_pRenderContext->device.createSemaphore(semaphoreInfo));
				m_InFlightFences[i] = VulkanUtils::Check(m_pRenderContext->device.createFence(fenceInfo));
			}
		}

		return true;
	}

	bool Renderer::OnPostInitialize()
	{
		Window* pWindow = m_pContext->GetSubsystem<Window>();

		m_pRayTracer = std::make_unique<VulkanRaytracer>(m_pRenderContext.get(), m_pScene->GetAccelerationStructure(), m_pSwapChain->GetNumFrames(), pWindow->GetWidth(), pWindow->GetHeight());

		// Initialize the geometry pass
		{
			m_pGeometryShader = m_pShaderLibrary->GetShader("StaticGeometry");

			// Render target
			m_pGeometryRenderTarget = std::make_unique<RenderTarget>(m_pRenderContext.get(), vk::Format::eR8G8B8A8Unorm, "Geometry pass render target", m_pRenderContext->imageExtent.width, m_pRenderContext->imageExtent.height);

			// Create the frame data
			{
				// TODO: this pool should be centralized somewhere.
				m_pGeometryDescriptorPool = std::make_unique<DescriptorPool>(
					DescriptorPool::Builder(m_pRenderContext->device)
					.AddSize(vk::DescriptorType::eUniformBuffer, 10 * m_pSwapChain->GetNumFrames())
					.AddSize(vk::DescriptorType::eCombinedImageSampler, 1000)
					.SetMaxSets(10 * m_pSwapChain->GetNumFrames())
					.Build());

				for (u32 i = 0; i < m_pSwapChain->GetNumFrames(); i++)
				{
					std::vector<vk::DescriptorSet> descriptorSets = m_pGeometryDescriptorPool->Allocate(m_pGeometryShader->GetAllDescriptorSetLayouts());
					m_GeometryFrameDatas.emplace_back<FrameData>(FrameData{
						VulkanBuffer(m_pRenderContext.get(), sizeof(CameraData), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU,
							fmt::format("Camera buffer {}", i)),
						descriptorSets[0]
						});
				}

				for (u32 i = 0; i < m_pSwapChain->GetNumFrames(); i++)
				{
					DescriptorWriter writer{ m_pRenderContext->device, m_GeometryFrameDatas[i].descriptor };

					vk::DescriptorBufferInfo bufferInfo = {};
					bufferInfo.buffer = m_GeometryFrameDatas[i].cameraBuffer.GetBuffer();
					bufferInfo.offset = 0;
					bufferInfo.range = sizeof(CameraData);

					writer.WriteBuffer(bufferInfo, 0, vk::DescriptorType::eUniformBuffer);
					writer.Write();
				}
			}

			// Create the pipeline
			GraphicsPipelineSpecification geometryPipelineSpec{
				.debugName = "Geometry pipeline",
				.pShader = m_pGeometryShader,
				.polygonMode = vk::PolygonMode::eFill,
				.cullMode = vk::CullModeFlagBits::eNone,
				.frontFace = vk::FrontFace::eCounterClockwise,
				.depthTest = true,
				.depthWrite = true,
				.compareOp = vk::CompareOp::eLess,
				.viewport = {},
				.scissor = {},
				.blendEnable = true,
				.colorFormats = { m_pGeometryRenderTarget->GetColorImage()->GetFormat() },
				.depthStencilFormat = m_pGeometryRenderTarget->GetDepthImage()->GetFormat(),
				.dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor },
				.flags = {}
			};
			m_pGeometryPipeline = std::make_unique<VulkanGraphicsPipeline>(m_pRenderContext.get(), geometryPipelineSpec);
		}

		// Initialize the composite pass
		{
			m_pCompositeShader = m_pShaderLibrary->GetShader("Composite");

			// Create the composite descriptors
			{
				// TODO: this pool should be centralized somewhere.
				m_pCompositeDescriptorPool = std::make_unique<DescriptorPool>(
					DescriptorPool::Builder(m_pRenderContext->device)
					.AddSize(vk::DescriptorType::eCombinedImageSampler, 2 * m_pSwapChain->GetNumFrames())
					.SetMaxSets(10 * m_pSwapChain->GetNumFrames())
					.Build());

				m_CompositeDescriptorSet = m_pCompositeDescriptorPool->Allocate(m_pCompositeShader->GetAllDescriptorSetLayouts())[0];

				DescriptorWriter writer{ m_pRenderContext->device, m_CompositeDescriptorSet };

				vk::DescriptorImageInfo geometryInfo = {};
				geometryInfo.imageLayout = m_pGeometryRenderTarget->GetColorImage()->GetImageLayout();
				geometryInfo.imageView = m_pGeometryRenderTarget->GetColorImage()->GetImageView();
				geometryInfo.sampler = m_pGeometryRenderTarget->GetColorSampler();

				vk::DescriptorImageInfo rtInfo = {};
				const auto* rtOutput = m_pRayTracer->GetOutputImage();
				rtInfo.imageLayout = vk::ImageLayout::eGeneral;
				rtInfo.imageView = rtOutput->GetColorImage()->GetImageView();
				rtInfo.sampler = rtOutput->GetColorSampler();

				writer.WriteImage(geometryInfo, 0, vk::DescriptorType::eCombinedImageSampler);
				writer.WriteImage(rtInfo, 1, vk::DescriptorType::eCombinedImageSampler);
				writer.Write();
			}

			// Create the composite pipeline
			GraphicsPipelineSpecification compositePipelineSpecification{
				.debugName = "Composite pipeline",
				.pShader = m_pCompositeShader,
				.polygonMode = vk::PolygonMode::eFill,
				.cullMode = vk::CullModeFlagBits::eNone,
				.frontFace = vk::FrontFace::eCounterClockwise,
				.depthTest = false,
				.depthWrite = false,
				.compareOp = vk::CompareOp::eNever,
				.viewport = {},
				.scissor = {},
				.blendEnable = true,
				.colorFormats = { m_pRenderContext->imageColorFormat },
				.depthStencilFormat = vk::Format::eD24UnormS8Uint, // Temp.
				.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor},
				.flags = {}
			};
			m_pCompositePipeline = std::make_unique<VulkanGraphicsPipeline>(m_pRenderContext.get(), compositePipelineSpecification);
		}

		return true;
	}

	bool isKeyDown{false};
	void Renderer::OnTick(f32 dt)
	{
		HPR_PROFILE_SCOPE();

		{
			Input* input = m_pContext->GetSubsystem<Input>();
			if (input->GetKey(Key::F1))
			{
				if (!isKeyDown)
				{
					m_pRenderContext->drawImGui = !m_pRenderContext->drawImGui;
				}
				isKeyDown = true;
			}
			else
			{
				isKeyDown = false;
			}
		} 

		vk::Result result;

		{
			result = m_pRenderContext->device.waitForFences(m_InFlightFences[m_FrameIdx], true, UINT64_MAX);
			if (result != vk::Result::eSuccess)
			{
				throw std::runtime_error("Failed to wait for fence");
			}
		}

		// Resize the swap chain if necessary.
		{
			const Window* window = m_pContext->GetSubsystem<Window>();
			const u32 width = window->GetWidth();
			const u32 height = window->GetHeight();

			if (width != m_pSwapChain->GetWidth() || height != m_pSwapChain->GetHeight() || !m_pSwapChain->IsPresentEnabled())
			{
				VulkanUtils::Check(m_pRenderContext->device.waitIdle());
				m_pSwapChain->Resize(width, height);
				m_pRayTracer->Resize(width, height);
				m_pGeometryRenderTarget->Resize(width, height);

				// test: Update the composite pass descriptors
				// TODO: this shouldn't have to be done manually like this.
				{
					DescriptorWriter writer{ m_pRenderContext->device, m_CompositeDescriptorSet };

					vk::DescriptorImageInfo geometryInfo = {};
					geometryInfo.imageLayout = vk::ImageLayout::eGeneral;
					geometryInfo.imageView = m_pGeometryRenderTarget->GetColorImage()->GetImageView();
					geometryInfo.sampler = m_pGeometryRenderTarget->GetColorSampler();

					vk::DescriptorImageInfo rtInfo = {};
					const auto* rtOutput = m_pRayTracer->GetOutputImage();
					rtInfo.imageLayout = vk::ImageLayout::eGeneral;
					rtInfo.imageView = rtOutput->GetColorImage()->GetImageView();
					rtInfo.sampler = rtOutput->GetColorSampler();

					writer.WriteImage(geometryInfo, 0, vk::DescriptorType::eCombinedImageSampler);
					writer.WriteImage(rtInfo, 1, vk::DescriptorType::eCombinedImageSampler);
					writer.Write();
				}
			}
		}

		if (!m_pSwapChain->IsPresentEnabled())
		{
			return;
		}

		// Only reset fences if we're submitting any work.
		m_pRenderContext->device.resetFences(m_InFlightFences[m_FrameIdx]);

		// Update camera just for test
		m_pCamera->Update(dt);

		vk::CommandBuffer cmd = m_CommandBuffers[m_FrameIdx];
		VulkanCommandBuffer::Begin(cmd);

		// Begin Dear ImGui frame
		m_pImGuiWrapper->NewFrame();

		HPR_PROFILE_GPU_CONTEXT(cmd);

		// Set viewport and scissor
		const f32 imageWidth = static_cast<f32>(m_pRenderContext->imageExtent.width);
		const f32 imageHeight = static_cast<f32>(m_pRenderContext->imageExtent.height);
		cmd.setViewport(0, vk::Viewport{ 0, 0, imageWidth, imageHeight, 0.0f, 1.0f });
		cmd.setScissor(0, vk::Rect2D{ vk::Offset2D{ 0, 0 }, m_pRenderContext->imageExtent });


		m_pCamera->DrawImGui();

		// VMA memory stats
		if (m_pRenderContext->drawImGui)
		{
			if (ImGui::Begin("GPU memory stats"))
			{

				VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
				vmaGetHeapBudgets(m_pRenderContext->allocator, budgets);

				const char* items[VK_MAX_MEMORY_HEAPS] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16" };

				static i32 selectedHeap = 0;
				ImGui::Combo("Selected memory heap", &selectedHeap, items, VK_MAX_MEMORY_HEAPS);

				// Convert bytes to megabytes
				const f32 usageMB = budgets[selectedHeap].usage / 1000000.0f;
				const f32 budgetMB = budgets[selectedHeap].budget / 1000000.0f;
				ImGui::Text("Current memory usage:");
				ImGui::Text("total usage: %.2f MB", usageMB);
				ImGui::Text("total available: %.2f MB", budgetMB);
				ImGui::ProgressBar(usageMB / budgetMB);
				ImGui::Separator();
				ImGui::Text("VmaAllocation objects: %d - occupying %.2f MB", budgets[selectedHeap].statistics.allocationCount, budgets[selectedHeap].statistics.allocationBytes / 1000000.0f);
				ImGui::Text("VkDeviceMemory objects: %d - occupying %.2f MB", budgets[selectedHeap].statistics.blockCount, budgets[selectedHeap].statistics.blockBytes / 1000000.0f);

				// ImGui::Text("Current memory usage:");
				// ImGui::Text("%u allocations (%llu B)", budgets[0].statistics.allocationCount, budgets[0].statistics.allocationBytes);
				// ImGui::Text("Total memory blocks available")
			}
			ImGui::End();
		}

		m_pShaderLibrary->DrawImGui();


		// Geometry pass.
		{
			VkDebug::BeginRegion(cmd, "Geometry pass", { 0.8f, 0.6f, 0.1f, 1.0f });
			m_pGeometryRenderTarget->GetColorImage()->TransitionLayout(
				cmd,
				vk::AccessFlagBits::eColorAttachmentWrite,
				vk::ImageLayout::eGeneral,
				vk::PipelineStageFlagBits::eColorAttachmentOutput
			);

			// Begin rendering
			const auto attachments = m_pGeometryRenderTarget->GetRenderingAttachments();

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea = vk::Rect2D(vk::Offset2D(), m_pRenderContext->imageExtent);
			renderingInfo.layerCount = 1;
			renderingInfo.viewMask = 0;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.setPColorAttachments(&attachments[0]);
			renderingInfo.setPDepthAttachment(&attachments[1]);

			cmd.beginRendering(renderingInfo);

			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pGeometryPipeline->GetPipeline());

			// draw stuff here

			FrameData& currentFrameData = m_GeometryFrameDatas[m_FrameIdx];

			// Update global uniform buffers
			{
				CameraData camData{};
				camData.view = m_pCamera->GetView();
				camData.proj = m_pCamera->GetProjection();
				camData.viewProj = m_pCamera->GetViewProjection();
				currentFrameData.cameraBuffer.SetData(&camData, sizeof(CameraData));

				DescriptorWriter writer{ m_pRenderContext->device, m_GeometryFrameDatas[m_FrameIdx].descriptor };
				vk::DescriptorBufferInfo cameraBufferInfo = {};
				cameraBufferInfo.buffer = currentFrameData.cameraBuffer.GetBuffer();
				cameraBufferInfo.offset = 0;
				cameraBufferInfo.range = sizeof(CameraData);
				writer.WriteBuffer(cameraBufferInfo, 0, vk::DescriptorType::eUniformBuffer);

				cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pGeometryPipeline->GetLayout(), 0, { currentFrameData.descriptor }, {});
			}

			// TEMP pushconst garbage
			const u32 offset = m_pGeometryShader->GetAllPushConstantRanges()[1].offset;
			cmd.pushConstants<LightingSettings>(m_pGeometryPipeline->GetLayout(), vk::ShaderStageFlagBits::eFragment, offset, m_pScene->GetLightingSettings());

			// Draw the model to the screen
			m_pScene->Draw(cmd, m_pGeometryPipeline->GetLayout());

			// End rendering
			cmd.endRendering();

			VkDebug::EndRegion(cmd);
		}

		// RT pass.
		{
			VkDebug::BeginRegion(cmd, "RT Pass", { 0.3f, 0.3f, 0.8f, 1.0f });

			// Trace them rays
			m_pRayTracer->RayTrace(cmd, m_pCamera.get(), m_FrameIdx, m_pScene->GetLightingSettings());

			VkDebug::EndRegion(cmd);
		}

		// Composite pass.
		{
			VkDebug::BeginRegion(cmd, "Composite pass.", { 0.2f, 0.9f, 0.4f, 1.0f });

			// Transition swapchain image
			InsertImageMemoryBarrier(
				cmd,
				m_pSwapChain->GetImage(),
				{},
				vk::AccessFlagBits::eColorAttachmentWrite,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eColorAttachmentOptimal,
				vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::ImageSubresourceRange({
					vk::ImageAspectFlagBits::eColor,
					0, 1,
					0, 1
					})
			);

			// Begin rendering
			const std::array<vk::RenderingAttachmentInfo, 2> attachments = m_pSwapChain->GetRenderingAttachments();

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea = vk::Rect2D(vk::Offset2D(), m_pRenderContext->imageExtent);
			renderingInfo.layerCount = 1;
			renderingInfo.viewMask = 0;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.setPColorAttachments(&attachments[0]);

			cmd.beginRendering(renderingInfo);

			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pCompositePipeline->GetPipeline());

			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pCompositePipeline->GetLayout(), 0, { m_CompositeDescriptorSet }, {}, {});
			cmd.draw(3, 1, 0, 0);

			// Render Dear ImGui frame data
			m_pImGuiWrapper->Draw(cmd);

			// End rendering
			cmd.endRendering();

			// Transition swapchain image
			InsertImageMemoryBarrier(
				cmd,
				m_pSwapChain->GetImage(),
				vk::AccessFlagBits::eColorAttachmentWrite,
				{},
				vk::ImageLayout::eColorAttachmentOptimal,
				vk::ImageLayout::ePresentSrcKHR,
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor,
					0, 1,
					0, 1
				)
			);

			VkDebug::EndRegion(cmd);
		}

		VulkanCommandBuffer::End(cmd);

		// Submit
		m_pRenderContext->device.resetFences(m_InFlightFences[m_FrameIdx]);

		m_pRenderContext->graphicsQueue.Submit(
			{ vk::PipelineStageFlagBits::eColorAttachmentOutput },
			{ m_pSwapChain->GetSemaphore() },
			{ m_RenderFinishedSemaphores[m_FrameIdx] },
			cmd,
			m_InFlightFences[m_FrameIdx]);


		// Present
		m_pSwapChain->Present(m_RenderFinishedSemaphores[m_FrameIdx]);

		m_FrameIdx = (m_FrameIdx + 1) % m_pRenderContext->imagesInFlight;
		m_pRenderContext->frameNumber++;
	}

	void Renderer::OnShutdown()
	{
		// Destroy Dear ImGui data
		m_pImGuiWrapper.reset();

		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			m_pRenderContext->device.destroyFence(m_InFlightFences[i]);
			m_pRenderContext->device.destroySemaphore(m_RenderFinishedSemaphores[i]);
		}

		m_pRayTracer.reset();

		m_pGeometryDescriptorPool.reset();
		m_GeometryFrameDatas.clear();
		m_pGeometryPipeline.reset();
		m_pGeometryRenderTarget.reset();

		m_pCompositeDescriptorPool.reset();
		m_pCompositePipeline.reset();

		m_pSwapChain.reset();

		m_pCamera.reset();

		m_pRenderContext->device.destroySampler(m_pRenderContext->defaultSampler);

		m_pMaterialLibrary.reset();
		m_pShaderLibrary.reset();
		// TODO: automatically keep track of allocated command buffers and destroy them all.
		m_pCommandPool->FreeCommandBuffers(m_CommandBuffers);
		m_pCommandPool.reset();
		m_pDevice.reset();
	}

	void Renderer::WaitIdle()
	{
		VulkanUtils::Check(m_pRenderContext->device.waitIdle());
	}
}
