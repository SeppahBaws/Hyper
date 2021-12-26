#include "HyperPCH.h"
#include "Renderer.h"

#include "Base/RendererAPI.h"
#include "Vulkan/VulkanRendererAPI.h"

namespace Hyper
{
	Renderer::Renderer(Context* pContext) : Subsystem(pContext)
	{
	}

	bool Renderer::OnInitialize()
	{
		switch (m_API)
		{
		case RendererAPIType::Vulkan:
			m_pCurrentAPI = new VulkanRendererAPI();
			break;
		default:
			return false;
		}

		m_pCurrentAPI->Init();
		return true;
	}

	void Renderer::OnTick()
	{
	}

	void Renderer::OnShutdown()
	{
		delete m_pCurrentAPI;
	}
}
