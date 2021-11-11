#include "HyperPCH.h"
#include "VulkanRendererAPI.h"

#include "VulkanRenderContext.h"

namespace Hyper
{
	void VulkanRendererAPI::Init()
	{
		m_Context = std::make_unique<VulkanRenderContext>();
	}

	void VulkanRendererAPI::BeginScene()
	{
	}

	void VulkanRendererAPI::EndScene()
	{
	}

	void VulkanRendererAPI::Clear()
	{
	}

	void VulkanRendererAPI::DrawIndexed()
	{
	}
}
