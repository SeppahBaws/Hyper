#include "HyperPCH.h"
#include "Window.h"

#include <iostream>
#include <GLFW/glfw3.h>

namespace Hyper
{
	bool Window::OnInitialize()
	{
		if (!glfwInit())
		{
			HPR_CORE_LOG_CRITICAL("Failed to initialize GLFW!");
			return false;
		}

		m_pWindow = glfwCreateWindow(1600, 900, "Hyper Engine", nullptr, nullptr);
		if (!m_pWindow)
		{
			HPR_CORE_LOG_CRITICAL("Failed to create GLFW window!");
			glfwTerminate();
			return false;
		}

		glfwMakeContextCurrent(m_pWindow);

		return true;
	}

	void Window::OnTick()
	{
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
