#include "HyperPCH.h"
#include "Renderer.h"

namespace Hyper
{
	Renderer::Renderer(Context* pContext) : Subsystem(pContext)
	{
	}

	bool Renderer::OnInitialize()
	{
		m_Device = std::make_unique<VulkanRenderDevice>();

		return true;
	}

	void Renderer::OnTick()
	{
	}

	void Renderer::OnShutdown()
	{
	}
}
