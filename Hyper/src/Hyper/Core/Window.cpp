#include "HyperPCH.h"
#include "Window.h"

#include <iostream>
#include <GLFW/glfw3.h>

#include "Context.h"
#include "Hyper/Debug/Profiler.h"

namespace Hyper
{
	Window::Window(Context* pContext)
		: Subsystem(pContext)
		, m_Width(1600)
		, m_Height(900)
		, m_Title("Hyper Engine")
	{
	}

	bool Window::OnInitialize()
	{
		if (!glfwInit())
		{
			HPR_CORE_LOG_CRITICAL("Failed to initialize GLFW!");
			return false;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_pWindow = glfwCreateWindow(static_cast<i32>(m_Width), static_cast<i32>(m_Height), m_Title.c_str(), nullptr, nullptr);
		if (!m_pWindow)
		{
			HPR_CORE_LOG_CRITICAL("Failed to create GLFW window!");
			glfwTerminate();
			return false;
		}
		glfwSetWindowUserPointer(m_pWindow, this);

		glfwSetFramebufferSizeCallback(m_pWindow, [](GLFWwindow* glfwWindow, int width, int height)
		{
			Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));
			window->m_Width = static_cast<u32>(width);
			window->m_Height = static_cast<u32>(height);
		});

		glfwMakeContextCurrent(m_pWindow);
		glfwSwapInterval(1);

		return true;
	}

	void Window::OnTick(f32 dt)
	{
		HPR_PROFILE_SCOPE();
		
		glfwSwapBuffers(m_pWindow);
		glfwPollEvents();
	}

	void Window::OnShutdown()
	{
		glfwDestroyWindow(m_pWindow);
		glfwTerminate();
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(m_pWindow);
	}
}
