#pragma once
#include "Hyper/Core/Subsystem.h"

namespace Hyper
{
	class RendererAPI;

	enum class RendererAPIType
	{
		None,
		Vulkan
	};

	class Renderer final : public Subsystem
	{
	public:
		Renderer(Context* pContext);
		~Renderer() override = default;

		bool OnInitialize() override;
		void OnTick() override;
		void OnShutdown() override;

	private:
		RendererAPIType m_API = RendererAPIType::Vulkan;
		RendererAPI* m_pCurrentAPI = nullptr;
	};
}
