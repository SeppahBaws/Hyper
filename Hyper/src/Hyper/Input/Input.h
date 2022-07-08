#pragma once
#include <glm/vec2.hpp>

#include "Hyper/Core/Subsystem.h"
#include "Hyper/Input/InputCodes.h"

namespace Hyper
{
	class Window;

	class Input final : public Subsystem
	{
	public:
		Input(Context* pContext);
		virtual ~Input() override = default;

		bool OnInitialize() override;
		void OnTick(f32) override;

		[[nodiscard]] bool GetKey(Key key) const;
		[[nodiscard]] bool GetMouseButton(MouseButton button) const;
		[[nodiscard]] glm::vec2 GetMousePos() const;
		[[nodiscard]] glm::vec2 GetMouseMovement();
		[[nodiscard]] glm::vec2 GetScroll();

		void SetCursorMode(CursorMode mode) const;

	private:
		Window* m_pWindow{};

		glm::vec2 m_LastMousePos{};

		// Scroll in the current frame
		glm::vec2 m_Scroll{};
		bool m_ScrollDirty = false;
	};
}
