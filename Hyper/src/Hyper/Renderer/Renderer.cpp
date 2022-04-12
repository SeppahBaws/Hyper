#include "HyperPCH.h"
#include "Renderer.h"

#include "Hyper/Core/Context.h"
#include "Hyper/Core/Window.h"
#include "Vulkan/VulkanCommands.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanShader.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanTemp.h"

namespace Hyper
{
	class Window;

	Renderer::Renderer(Context* pContext) : Subsystem(pContext)
	{
	}

	bool Renderer::OnInitialize()
	{
		m_pRenderContext = std::make_unique<RenderContext>();
		m_pDevice = std::make_unique<VulkanDevice>(m_pRenderContext.get());
		m_pCommandPool = std::make_unique<VulkanCommandPool>(m_pRenderContext.get());

		Window* pWindow = m_pContext->GetSubsystem<Window>();
		m_pSwapChain = std::make_unique<VulkanSwapChain>(pWindow, m_pRenderContext.get(), pWindow->GetWidth(), pWindow->GetHeight());


		// Shader test
		VulkanShader shader{ m_pRenderContext.get() };
		shader.AddStage(ShaderStageType::Vertex, "res/shaders/test.vert");
		shader.AddStage(ShaderStageType::Fragment, "res/shaders/test.frag");

		PipelineBuilder builder{ m_pRenderContext.get() };
		builder.SetShader(&shader);
		builder.SetInputAssembly(vk::PrimitiveTopology::eTriangleList, false);
		// builder.SetViewport(0.0f, 0.0f, static_cast<f32>(m_pRenderContext->imageExtent.width), static_cast<f32>(m_pRenderContext->imageExtent.height), 0.0f, 1.0f);
		// builder.SetScissor(vk::Offset2D(0, 0), m_pRenderContext->imageExtent);
		builder.SetRasterizer(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack);
		builder.SetMultisampling();
		builder.SetDepthStencil(true, true, vk::CompareOp::eLess);
		builder.SetColorBlend(true, vk::BlendOp::eAdd, vk::BlendOp::eAdd, false, vk::LogicOp::eCopy);
		builder.SetDescriptorSetLayout({}, {});
		builder.SetDynamicStates({ vk::DynamicState::eViewport, vk::DynamicState::eScissor });

		m_pPipeline = std::make_unique<VulkanPipeline>(builder.BuildGraphics());

		// Get command buffers. 1 for each frame in flight.
		{
			m_CommandBuffers = m_pCommandPool->GetCommandBuffers(m_pSwapChain->GetNumFrames());
		}

		// Create sync objects
		{
			const u32 numFrames = m_pSwapChain->GetNumFrames();
			m_RenderFinishedSemaphores.resize(numFrames);
			m_InFlightFences.resize(numFrames);

			const vk::SemaphoreCreateInfo semaphoreInfo{};
			const vk::FenceCreateInfo fenceInfo{ vk::FenceCreateFlagBits::eSignaled };

			try
			{
				for (u32 i = 0; i < numFrames; i++)
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

	void Renderer::OnTick()
	{
		vk::Result result = m_pRenderContext->device.waitForFences(m_InFlightFences[m_FrameIdx], true, UINT64_MAX);
		if (result != vk::Result::eSuccess)
		{
			throw std::runtime_error("Failed to wait for fence");
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

		m_FrameIdx = (m_FrameIdx + 1) % MAX_FRAMES_IN_FLIGHT;

		vk::CommandBuffer cmd = m_CommandBuffers[m_FrameIdx];
		VulkanCommandBuffer::Begin(cmd);

		// Set viewport and scissor
		const f32 imageWidth = static_cast<f32>(m_pRenderContext->imageExtent.width);
		const f32 imageHeight = static_cast<f32>(m_pRenderContext->imageExtent.height);
		cmd.setViewport(0, vk::Viewport{ 0, 0, imageWidth, imageHeight });
		cmd.setScissor(0, vk::Rect2D{ vk::Offset2D{ 0, 0 }, m_pRenderContext->imageExtent });

		InsertImageMemoryBarrier(
			cmd,
			m_pSwapChain->GetImage(),
			{}, // srcAccessMask
			vk::AccessFlagBits::eColorAttachmentWrite, // dstAccessMask
			vk::ImageLayout::eUndefined, // oldLayout
			vk::ImageLayout::eColorAttachmentOptimal, // newLayout
			vk::PipelineStageFlagBits::eTopOfPipe, // srcStageMask
			vk::PipelineStageFlagBits::eColorAttachmentOutput, // dstStageMask
			vk::ImageSubresourceRange({
				vk::ImageAspectFlagBits::eColor,
				0, 1,
				0, 1 })
				);


		vk::RenderingAttachmentInfo attachmentInfo = {};
		attachmentInfo.imageView = m_pSwapChain->GetImageView();
		attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		attachmentInfo.resolveMode = vk::ResolveModeFlagBits::eNone;
		attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		attachmentInfo.clearValue = vk::ClearColorValue(std::array{ 0.1f, 0.1f, 0.1f, 1.0f });
		
		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea = vk::Rect2D(vk::Offset2D(), m_pRenderContext->imageExtent);
		renderingInfo.layerCount = 1;
		renderingInfo.viewMask = 0;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.setPColorAttachments(&attachmentInfo);

		cmd.beginRendering(renderingInfo);
		
		// draw stuff here


		// Draw test triangle with basic shader
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pPipeline->GetPipeline());
		cmd.draw(3, 1, 0, 0);


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

		m_pPipeline.reset();
		m_pSwapChain.reset();

		// TODO: automatically keep track of allocated command buffers and destroy them all.
		m_pCommandPool->FreeCommandBuffers(m_CommandBuffers);
		m_pCommandPool.reset();
		m_pDevice.reset();
	}
}
