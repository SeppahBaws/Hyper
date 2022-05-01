#pragma once
#include <glm/mat4x4.hpp>

#include "Hyper/Input/Input.h"

namespace Hyper
{
	class Context;

	class EditorCamera
	{
	public:
		EditorCamera(Context* pContext);
		// Camera(f32 fovDegrees, f32 width, f32 height, f32 zNear, f32 zFar, const glm::vec3& position, const glm::vec3& forward);

		void Setup();
		void Update(f32 dt);

		// void ComputeView();
		void ComputeProjection();
		void ComputeViewProjection();

		[[nodiscard]] glm::mat4 GetViewProjection() const
		{
			return m_ViewProjection;
		}

	private:
		Context* m_pContext;

		// Pointers to subsystems for easy access
		Input* m_pInput;
		Window* m_pWindow;

		bool m_IsSetup{ false };

		// Perspective properties
		f32 m_FovDegrees;
		u32 m_Width, m_Height;
		f32 m_ZNear, m_ZFar;

		glm::vec3 m_Position{ 0.0f, 0.0f, -2.0f };
		glm::vec3 m_Forward{ 0.0f, 0.0f, 1.0f };
		f32 m_Yaw{}, m_Pitch{};

		glm::mat4 m_View{};
		glm::mat4 m_Projection{};
		glm::mat4 m_ViewProjection{};
		f32 m_MoveSpeed{ 10.0f };
		f32 m_LookSpeed{ 10.0f };
	};
}
