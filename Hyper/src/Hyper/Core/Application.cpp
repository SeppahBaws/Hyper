#include "HyperPCH.h"
#include "Application.h"

#include "Context.h"
#include "Logger.h"
#include "Window.h"

namespace Hyper
{
	Application::Application()
	{
		Logger::Init();

		m_pContext = std::make_shared<Context>();

		// Add subsystems. Order matters: first in, first initialized.
		m_pContext->AddSubsystem<Window>();


		if (!m_pContext->OnInitialize())
		{
			HPR_CORE_LOG_ERROR("Failed to initialize subsystems!");
		}
		else
		{
			HPR_CORE_LOG_INFO("Initialized all subsystems!");
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

		while (!pWindow->ShouldClose())
		{
			m_pContext->OnTick();
		}
	}
}
