#include "HyperPCH.h"
#include "FlyCamera.h"

#include <glm/matrix.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <imgui.h>

#include "Hyper/Core/Context.h"
#include "Hyper/Core/Window.h"

namespace Hyper
{
	FlyCamera::FlyCamera(Context* pContext)
		: m_pContext(pContext)
	{
	}

	void FlyCamera::Setup()
	{
		if (m_IsSetup)
			return;

		m_pInput = m_pContext->GetSubsystem<Input>();

		m_FovDegrees = 60.0f;
		m_pWindow = m_pContext->GetSubsystem<Window>();
		m_Width = m_pWindow->GetWidth();
		m_Height = m_pWindow->GetHeight();
		m_ZNear = 0.01f;
		m_ZFar = 100.0f;

		ComputeProjection();

		m_IsSetup = true;
	}

	void FlyCamera::Update(f32 dt)
	{
		if (m_Width != m_pWindow->GetWidth() || m_Height != m_pWindow->GetHeight())
		{
			m_Width = m_pWindow->GetWidth();
			m_Height = m_pWindow->GetHeight();
			ComputeProjection();
		}

		glm::vec3 movement{};
		glm::vec2 mouseMov = m_pInput->GetMouseMovement();
		if (m_pInput->GetMouseButton(MouseButton::Right))
		{
			if (m_pInput->GetKey(Key::A))
				movement.x -= 1;
			if (m_pInput->GetKey(Key::D))
				movement.x += 1;

			if (m_pInput->GetKey(Key::W))
				movement.y += 1;
			if (m_pInput->GetKey(Key::S))
				movement.y -= 1;

			if (m_pInput->GetKey(Key::Q))
				movement.z -= 1;
			if (m_pInput->GetKey(Key::E))
				movement.z += 1;

			m_Yaw += mouseMov.x * dt * m_LookSpeed;
			m_Pitch += mouseMov.y * dt * m_LookSpeed;

			m_MoveSpeed += m_pInput->GetScroll().y;
			m_MoveSpeed = std::clamp(m_MoveSpeed, 1.0f, 50.0f);

			m_pInput->SetCursorMode(CursorMode::Disabled);
		}
		else
		{
			m_pInput->SetCursorMode(CursorMode::Normal);
		}

		// Constrain pitch
		m_Pitch = std::clamp(m_Pitch, -89.9f, 89.9f);

		glm::vec3 front;
		front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.y = sin(glm::radians(m_Yaw)) * cos(glm::radians(-m_Pitch));
		front.z = sin(glm::radians(m_Pitch));
		front = glm::normalize(front);
		m_Forward = front;

		if (glm::length(movement) > 0)
		{
			const glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
			const glm::vec3 forward = glm::normalize(m_Forward);
			const glm::vec3 right = glm::normalize(glm::cross(m_Forward, up));

			movement = glm::normalize(movement);
			glm::vec3 mov{};
			mov += movement.x * right;
			mov += movement.y * forward;
			mov += movement.z * up;

			m_Position += mov * dt * m_MoveSpeed;
		}

		ComputeViewProjection();
	}

	void FlyCamera::DrawImGui()
	{
		if (ImGui::Begin("Camera"))
		{
			ImGui::InputFloat3("position", (float*)&m_Position);
			ImGui::InputFloat("Yaw", &m_Yaw);
			ImGui::InputFloat("Pitch", &m_Pitch);
			ImGui::InputFloat3("Forward", (float*)&m_Forward, "%.3f", ImGuiInputTextFlags_ReadOnly);
			ImGui::SliderFloat("Move speed", &m_MoveSpeed, 1.0f, 50.0f);
		}
		ImGui::End();
	}

	void FlyCamera::ComputeViewProjection()
	{
		m_View = glm::lookAtRH(m_Position, m_Position + m_Forward, glm::vec3(0.0f, 0.0f, 1.0f));
		m_ViewProjection = m_Projection * m_View;

		m_ViewI = glm::inverse(m_View);
		m_ViewProjectionI = glm::inverse(m_ViewProjection);
	}

	void FlyCamera::ComputeProjection()
	{
		m_Projection = glm::perspective(glm::radians(m_FovDegrees), static_cast<f32>(m_Width) / static_cast<f32>(m_Height), m_ZNear, m_ZFar);
		m_Projection[1][1] *= -1;

		m_ProjectionI = glm::inverse(m_Projection);
	}
}
