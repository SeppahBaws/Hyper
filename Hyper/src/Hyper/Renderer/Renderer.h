#pragma once
#include "Hyper/Core/Subsystem.h"

#include "Hyper/Renderer/Vulkan/VulkanRenderDevice.h"

namespace Hyper
{
	class Renderer final : public Subsystem
	{
	public:
		Renderer(Context* pContext);
		~Renderer() override = default;

		bool OnInitialize() override;
		void OnTick() override;
		void OnShutdown() override;

	private:
		std::unique_ptr<VulkanRenderDevice> m_Device;
	};
}
