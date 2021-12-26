#include "HyperPCH.h"
#include "VulkanRendererAPI.h"

#include "VulkanRenderDevice.h"

namespace Hyper
{
	void VulkanRendererAPI::Init()
	{
		m_Device = std::make_unique<VulkanRenderDevice>();
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
