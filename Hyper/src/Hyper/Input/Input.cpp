﻿#include "HyperPCH.h"
#include "Input.h"

#include <GLFW/glfw3.h>

#include "Hyper/Core/Context.h"
#include "Hyper/Core/Window.h"

namespace Hyper
{
	Input::Input(Context* pContext)
		: Subsystem(pContext)
	{
	}

	bool Input::OnInitialize()
	{
		m_pWindow = m_pContext->GetSubsystem<Window>();
		return true;
	}

	bool Input::GetKey(Key key) const
	{
		return glfwGetKey(m_pWindow->GetHandle(), static_cast<int>(key));
	}

	bool Input::GetMouseButton(MouseButton button) const
	{
		return glfwGetMouseButton(m_pWindow->GetHandle(), static_cast<int>(button));
	}

	glm::vec2 Input::GetMousePos() const
	{
		f64 x, y;
		glfwGetCursorPos(m_pWindow->GetHandle(), &x, &y);

		return { static_cast<f32>(x), static_cast<f32>(y) };
	}

	glm::vec2 Input::GetMouseMovement()
	{
		const glm::vec2 currPos = GetMousePos();
		const glm::vec2 mov = m_LastMousePos - currPos;
		m_LastMousePos = currPos;
		return mov;
	}

	void Input::SetCursorMode(CursorMode mode) const
	{
		glfwSetInputMode(m_pWindow->GetHandle(), GLFW_CURSOR, static_cast<u32>(mode));
	}
}
