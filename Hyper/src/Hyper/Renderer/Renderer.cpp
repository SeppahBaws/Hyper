#include "HyperPCH.h"
#include "Renderer.h"

#include <iostream>

#include "Hyper/Core/Context.h"
#include "Hyper/Core/Window.h"

namespace Hyper
{
	class Window;

	Renderer::Renderer(Context* pContext) : Subsystem(pContext)
	{
	}

	bool Renderer::OnInitialize()
	{
		m_pRenderContext = std::make_shared<RenderContext>();

		m_Device = std::make_shared<VulkanDevice>(m_pRenderContext);

		Window* pWindow = m_pContext->GetSubsystem<Window>();
		m_SwapChain = std::make_shared<VulkanSwapChain>(pWindow, m_pRenderContext, m_Device, pWindow->GetWidth(), pWindow->GetHeight());

		return true;
	}

	void Renderer::OnTick()
	{
		m_FrameCount++;
		std::cout << "\rFrame count: " << m_FrameCount << std::flush;
	}

	void Renderer::OnShutdown()
	{
		m_SwapChain.reset();
		m_Device.reset();
	}
}
