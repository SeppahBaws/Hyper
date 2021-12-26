#pragma once
#include "Subsystem.h"

struct GLFWwindow;

namespace Hyper
{
	class Window final : public Subsystem
	{
	public:
		Window(Context* pContext);
		virtual ~Window() override = default;

		bool OnInitialize() override;
		void OnTick() override;
		void OnShutdown() override;

		bool ShouldClose() const;

	private:
		GLFWwindow* m_pWindow = nullptr;
	};
}
