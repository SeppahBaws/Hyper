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

		[[nodiscard]] GLFWwindow* GetHandle() const { return m_pWindow; }
		[[nodiscard]] u32 GetWidth() const { return m_Width; }
		[[nodiscard]] u32 GetHeight() const { return m_Height; }

	private:
		GLFWwindow* m_pWindow = nullptr;
		u32 m_Width, m_Height;
		std::string m_Title;
	};
}
