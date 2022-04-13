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

		[[nodiscard]] bool GetKey(Key key) const;
		[[nodiscard]] bool GetMouseButton(MouseButton button) const;
		[[nodiscard]] glm::vec2 GetMousePos() const;

	private:
		Window* m_pWindow;
	};
}
