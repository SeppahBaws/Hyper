﻿#include "HyperPCH.h"
#include "Renderer.h"

#include "Hyper/Core/Context.h"
#include "Hyper/Core/Window.h"
#include "Hyper/Debug/Profiler.h"
#include "Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDescriptors.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanShader.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanTemp.h"

#include <glm/gtx/transform.hpp>

#include "Vulkan/VulkanDebug.h"

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

		m_pRenderContext = std::make_unique<RenderContext>();
		m_pDevice = std::make_unique<VulkanDevice>(m_pRenderContext.get());
		m_pCommandPool = std::make_unique<VulkanCommandPool>(m_pRenderContext.get());

		Window* pWindow = m_pContext->GetSubsystem<Window>();
		m_pSwapChain = std::make_unique<VulkanSwapChain>(pWindow, m_pRenderContext.get(), pWindow->GetWidth(), pWindow->GetHeight());

		// Create the frame data
		{
			m_pGlobalSetLayout = std::make_unique<vk::DescriptorSetLayout>(
				DescriptorSetLayoutBuilder(m_pRenderContext->device)
				.AddBinding(vk::DescriptorType::eUniformBuffer, 0, 1, vk::ShaderStageFlagBits::eVertex)
				.Build());
			// TODO: this pool should be centralized somewhere.
			m_pDescriptorPool = std::make_unique<DescriptorPool>(
				DescriptorPool::Builder(m_pRenderContext->device)
				.AddSize(vk::DescriptorType::eUniformBuffer, 10 * m_pSwapChain->GetNumFrames())
				.AddSize(vk::DescriptorType::eCombinedImageSampler, 1000)
				.SetMaxSets(10 * m_pSwapChain->GetNumFrames())
				.Build());

			try
			{
				for (u32 i = 0; i < m_pSwapChain->GetNumFrames(); i++)
				{
					std::vector<vk::DescriptorSet> descriptorSets = m_pDescriptorPool->Allocate({ *m_pGlobalSetLayout });
					m_FrameDatas.emplace_back<FrameData>(FrameData{
						VulkanBuffer(m_pRenderContext.get(), sizeof(CameraData), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU,
							fmt::format("Camera buffer {}", i)),
						descriptorSets[0]
					});
				}
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to create descriptor sets: "s + e.what());
			}

			try
			{
				for (u32 i = 0; i < m_pSwapChain->GetNumFrames(); i++)
				{
					DescriptorWriter writer{ m_pRenderContext->device, m_FrameDatas[i].descriptor };

					vk::DescriptorBufferInfo bufferInfo = {};
					bufferInfo.buffer = m_FrameDatas[i].cameraBuffer.GetBuffer();
					bufferInfo.offset = 0;
					bufferInfo.range = sizeof(CameraData);

					writer.WriteBuffer(bufferInfo, 0, vk::DescriptorType::eUniformBuffer);
					writer.Write();
				}
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to update descriptor sets: "s + e.what());
			}
		}

		// Create the pipeline
		{
			m_pShader = std::make_unique<VulkanShader>(
				m_pRenderContext.get(),
				std::unordered_map<ShaderStageType, std::filesystem::path>{
					{ ShaderStageType::Vertex, "res/shaders/StaticGeometry.vert" },
					{ ShaderStageType::Fragment, "res/shaders/StaticGeometry.frag" }
				});

			const auto descriptorSetLayouts = m_pShader->GetAllDescriptorSetLayouts();
			const auto pushConstants = m_pShader->GetAllPushConstantRanges();

			PipelineBuilder builder{ m_pRenderContext.get() };
			builder.SetShader(m_pShader.get());
			builder.SetInputAssembly(vk::PrimitiveTopology::eTriangleList, false);
			// builder.SetViewport(0.0f, 0.0f, static_cast<f32>(m_pRenderContext->imageExtent.width), static_cast<f32>(m_pRenderContext->imageExtent.height), 0.0f, 1.0f);
			// builder.SetScissor(vk::Offset2D(0, 0), m_pRenderContext->imageExtent);
			builder.SetRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone);
			builder.SetMultisampling();
			builder.SetDepthStencil(true, true, vk::CompareOp::eLess);
			builder.SetColorBlend(true, vk::BlendOp::eAdd, vk::BlendOp::eAdd, false, vk::LogicOp::eCopy);
			builder.SetDescriptorSetLayout(descriptorSetLayouts, pushConstants);
			builder.SetDynamicStates({ vk::DynamicState::eViewport, vk::DynamicState::eScissor });

			m_pPipeline = std::make_unique<VulkanPipeline>(builder.BuildGraphics());
		}

		// Setup model and camera (Temp code)
		{
			m_pScene = std::make_unique<Scene>(m_pRenderContext.get());

			// m_pScene->AddModel("res/models/Cuub.fbx");
			m_pScene->AddModel("res/sponza/sponza.obj", glm::vec3{ 0.0f }, glm::vec3{ 90.0f, 0.0f, 0.0f }, glm::vec3{ 0.01f });
			m_pScene->BuildAccelerationStructure();
			m_pCamera->Setup();
		}

		// Get command buffers. 1 for each frame in flight.
		{
			m_CommandBuffers = m_pCommandPool->GetCommandBuffers(m_pSwapChain->GetNumFrames());
		}

		// Create sync objects
		{
			m_RenderFinishedSemaphores.resize(m_pRenderContext->imagesInFlight);
			m_InFlightFences.resize(m_pRenderContext->imagesInFlight);

			const vk::SemaphoreCreateInfo semaphoreInfo{};
			const vk::FenceCreateInfo fenceInfo{ vk::FenceCreateFlagBits::eSignaled };

			try
			{
				for (u32 i = 0; i < m_pRenderContext->imagesInFlight; i++)
				{
					m_RenderFinishedSemaphores[i] = m_pRenderContext->device.createSemaphore(semaphoreInfo);
					m_InFlightFences[i] = m_pRenderContext->device.createFence(fenceInfo);
				}
			}
			catch (vk::SystemError& e)
			{
				throw std::runtime_error("Failed to create sync objects: "s + e.what());
			}
		}

		return true;
	}

	void Renderer::OnTick(f32 dt)
	{
		HPR_PROFILE_SCOPE();

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
				m_pRenderContext->device.waitIdle();
				m_pSwapChain->Resize(width, height);
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

		HPR_PROFILE_GPU_CONTEXT(cmd);

		// Set viewport and scissor
		const f32 imageWidth = static_cast<f32>(m_pRenderContext->imageExtent.width);
		const f32 imageHeight = static_cast<f32>(m_pRenderContext->imageExtent.height);
		cmd.setViewport(0, vk::Viewport{ 0, 0, imageWidth, imageHeight, 0.0f, 1.0f });
		cmd.setScissor(0, vk::Rect2D{ vk::Offset2D{ 0, 0 }, m_pRenderContext->imageExtent });

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
		
		// Geometry pass.
		{
			VkDebug::BeginRegion(cmd, "Geometry pass", { 0.8f, 0.6f, 0.1f, 1.0f });

			// Begin rendering
			const std::array<vk::RenderingAttachmentInfo, 2> attachments = m_pSwapChain->GetRenderingAttachments();

			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea = vk::Rect2D(vk::Offset2D(), m_pRenderContext->imageExtent);
			renderingInfo.layerCount = 1;
			renderingInfo.viewMask = 0;
			renderingInfo.colorAttachmentCount = 1;
			renderingInfo.setPColorAttachments(&attachments[0]);
			renderingInfo.setPDepthAttachment(&attachments[1]);

			cmd.beginRendering(renderingInfo);

			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pPipeline->GetPipeline());

			// draw stuff here

			FrameData& currentFrameData = m_FrameDatas[m_FrameIdx];

			// Update global uniform buffers
			{
				CameraData camData{};
				camData.view = m_pCamera->GetView();
				camData.proj = m_pCamera->GetProjection();
				camData.viewProj = m_pCamera->GetViewProjection();
				currentFrameData.cameraBuffer.SetData(&camData, sizeof(CameraData));

				DescriptorWriter writer{ m_pRenderContext->device, m_FrameDatas[m_FrameIdx].descriptor };
				vk::DescriptorBufferInfo cameraBufferInfo = {};
				cameraBufferInfo.buffer = currentFrameData.cameraBuffer.GetBuffer();
				cameraBufferInfo.offset = 0;
				cameraBufferInfo.range = sizeof(CameraData);
				writer.WriteBuffer(cameraBufferInfo, 0, vk::DescriptorType::eUniformBuffer);

				cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pPipeline->GetLayout(), 0, { currentFrameData.descriptor }, {});
			}

			// Draw the model to the screen
			m_pScene->Draw(cmd, m_pPipeline->GetLayout());

			// End rendering
			cmd.endRendering();

			VkDebug::EndRegion(cmd);
		}  

		// RT pass.
		{
			VkDebug::BeginRegion(cmd, "RT Pass", { 0.3f, 0.3f, 0.8f, 1.0f });

			// Trace rays (for the love of god please just work kthx)
			m_pScene->RayTrace(cmd);

			VkDebug::EndRegion(cmd);
		}

		InsertImageMemoryBarrier(
			cmd,
			m_pSwapChain->GetImage(),
			vk::AccessFlagBits::eColorAttachmentWrite,
			{},
			vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::ImageSubresourceRange({
				vk::ImageAspectFlagBits::eColor,
				0, 1,
				0, 1
			})
		);

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
		m_FrameNumber++;
	}

	void Renderer::OnShutdown()
	{
		// Wait till everything is finished.
		m_pRenderContext->device.waitIdle();

		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			m_pRenderContext->device.destroyFence(m_InFlightFences[i]);
			m_pRenderContext->device.destroySemaphore(m_RenderFinishedSemaphores[i]);
		}

		m_pDescriptorPool.reset();
		m_pRenderContext->device.destroyDescriptorSetLayout(*m_pGlobalSetLayout);
		m_pGlobalSetLayout.reset();
		m_FrameDatas.clear();

		m_pShader.reset();
		m_pPipeline.reset();
		m_pSwapChain.reset();

		m_pScene.reset();
		m_pCamera.reset();

		// TODO: automatically keep track of allocated command buffers and destroy them all.
		m_pCommandPool->FreeCommandBuffers(m_CommandBuffers);
		m_pCommandPool.reset();
		m_pDevice.reset();
	}
}
