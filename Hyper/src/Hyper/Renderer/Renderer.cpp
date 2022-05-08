#include "HyperPCH.h"
#include "Renderer.h"

#include "Hyper/Core/Context.h"
#include "Hyper/Core/Window.h"
#include "Hyper/Debug/Profiler.h"
#include "Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanShader.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanTemp.h"

#include <glm/gtx/transform.hpp>

namespace Hyper
{
	class Window;

	Renderer::Renderer(Context* pContext)
		: Subsystem(pContext)
		  , m_pCamera(std::make_unique<EditorCamera>(pContext))
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


		// Shader test
		VulkanShader shader{ m_pRenderContext.get() };
		shader.AddStage(ShaderStageType::Vertex, "res/shaders/test.vert");
		shader.AddStage(ShaderStageType::Fragment, "res/shaders/test.frag");

		vk::PushConstantRange pushConst{};
		pushConst.offset = 0;
		pushConst.size = sizeof(RenderMatrixPushConst);
		pushConst.stageFlags = vk::ShaderStageFlagBits::eVertex;

		const std::vector pushConsts = { pushConst };

		PipelineBuilder builder{ m_pRenderContext.get() };
		builder.SetShader(&shader);
		builder.SetInputAssembly(vk::PrimitiveTopology::eTriangleList, false);
		// builder.SetViewport(0.0f, 0.0f, static_cast<f32>(m_pRenderContext->imageExtent.width), static_cast<f32>(m_pRenderContext->imageExtent.height), 0.0f, 1.0f);
		// builder.SetScissor(vk::Offset2D(0, 0), m_pRenderContext->imageExtent);
		builder.SetRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone);
		builder.SetMultisampling();
		builder.SetDepthStencil(true, true, vk::CompareOp::eLess);
		builder.SetColorBlend(true, vk::BlendOp::eAdd, vk::BlendOp::eAdd, false, vk::LogicOp::eCopy);
		builder.SetDescriptorSetLayout({}, pushConsts);
		builder.SetDynamicStates({ vk::DynamicState::eViewport, vk::DynamicState::eScissor });

		m_pPipeline = std::make_unique<VulkanPipeline>(builder.BuildGraphics());

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

		// Create test mesh
		{
			m_pMesh = std::make_unique<TestMesh>(m_pRenderContext.get());
		}

		// Setup test camera (temp code)
		{
			m_pCamera->Setup();
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

		// draw stuff here


		// Upload push const
		RenderMatrixPushConst pushConst{};
		// m_Rot += 40 * dt;
		// const glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rot), glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rot), glm::vec3(0.0f, 1.0f, 0.0f));
		
		pushConst.renderMatrix = m_pCamera->GetViewProjection() * model;

		cmd.pushConstants<RenderMatrixPushConst>(m_pPipeline->GetLayout(), vk::ShaderStageFlagBits::eVertex, 0, pushConst);

		// Draw test triangle with basic shader
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pPipeline->GetPipeline());
		// m_pMesh->Draw(cmd);
		m_pMesh->Draw(cmd);


		// End rendering
		cmd.endRendering();

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

		m_pCamera.reset();
		m_pMesh.reset();

		for (size_t i = 0; i < m_CommandBuffers.size(); i++)
		{
			m_pRenderContext->device.destroyFence(m_InFlightFences[i]);
			m_pRenderContext->device.destroySemaphore(m_RenderFinishedSemaphores[i]);
		}

		m_pPipeline.reset();
		m_pSwapChain.reset();

		// TODO: automatically keep track of allocated command buffers and destroy them all.
		m_pCommandPool->FreeCommandBuffers(m_CommandBuffers);
		m_pCommandPool.reset();
		m_pDevice.reset();
	}
}
