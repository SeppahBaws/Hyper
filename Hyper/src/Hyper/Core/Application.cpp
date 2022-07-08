#include "HyperPCH.h"
#include "Application.h"

#include "Context.h"
#include "Logger.h"
#include "Window.h"
#include "Hyper/Debug/Profiler.h"
#include "Hyper/Input/Input.h"
#include "Hyper/Renderer/Renderer.h"
#include "Hyper/Scene/Scene.h"

namespace Hyper
{
	Application::Application()
	{
		Logger::Init();

		m_pContext = std::make_shared<Context>();

		// Add subsystems. Order matters: first in, first initialized.
		m_pContext->AddSubsystem<Window>();
		m_pContext->AddSubsystem<Input>();
		m_pContext->AddSubsystem<Renderer>();
		m_pContext->AddSubsystem<Scene>();


		if (!m_pContext->OnInitialize())
		{
			HPR_CORE_LOG_ERROR("Failed to initialize subsystems!");
		}
		else
		{
			HPR_CORE_LOG_INFO("Successfully initialized all subsystems!");
		}
	}

	Application::~Application()
	{
		m_pContext->OnShutdown();
	}

	void Application::Run()
	{
		Window* pWindow = m_pContext->GetSubsystem<Window>();
		if (!pWindow)
		{
			return;
		}

		auto t = std::chrono::high_resolution_clock::now();
		auto lastTime = std::chrono::high_resolution_clock::now();
		while (!pWindow->ShouldClose())
		{
			HPR_PROFILE_FRAME("Application loop");

			const auto currentTime = std::chrono::high_resolution_clock::now();
			const f32 dt = std::chrono::duration<f32>(currentTime - lastTime).count();
			lastTime = currentTime;

			m_pContext->OnTick(dt);
		}

		HPR_PROFILE_SHUTDOWN();
	}
}
