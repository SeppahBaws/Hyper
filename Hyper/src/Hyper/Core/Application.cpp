#include "HyperPCH.h"
#include "Application.h"

#include <iostream>

#include "Context.h"
#include "Logger.h"
#include "Window.h"
#include "Hyper/Input/Input.h"
#include "Hyper/Renderer/Renderer.h"

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

		while (!pWindow->ShouldClose())
		{
			const auto start = std::chrono::high_resolution_clock::now();

			// TODO: cap at 60fps
			m_pContext->OnTick();

			// const auto end = std::chrono::high_resolution_clock::now();
			// const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
			// const auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			//
			// std::cout << "\rEntire frame took " << std::setw(6) << durationMs << "ms (" << std::setw(6) << durationUs << "us) -- roughly " << std::setw(6) << (1'000'000 / durationUs) << " FPS" << std::flush;
			// std::cout << "Entire frame took " << std::setw(6) << durationMs << "ms (" << std::setw(6) << durationUs << "us) -- roughly " << std::setw(6) << (1'000'000 / durationUs) << " FPS" << std::endl;
		}
		std::cout << std::endl;
	}
}
