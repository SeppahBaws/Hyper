#pragma once
#include <memory>
#include <vector>
#include <ranges>

#include "Subsystem.h"
#include "Hyper/Debug/Profiler.h"

namespace Hyper
{
	class Context final
	{
	public:
		Context() = default;

		template<EngineSystem T>
		void AddSubsystem()
		{
			m_Subsystems.emplace_back(std::make_shared<T>(this));
		}

		template<EngineSystem T>
		T* GetSubsystem()
		{
			for (const auto& subsystem : m_Subsystems)
			{
				if (subsystem && typeid(T) == typeid(*subsystem))
					return static_cast<T*>(subsystem.get());
			}

			return nullptr;
		}

		bool OnInitialize()
		{
			HPR_PROFILE_SCOPE();

			bool success = true;
			for (auto& subsystem : m_Subsystems)
			{
				if (!subsystem->OnInitialize())
				{
					success = false;
					break;
				}
			}

			return success;
		}

		void OnTick(f32 dt)
		{
			HPR_PROFILE_SCOPE();

			for (auto& subsystem : m_Subsystems)
			{
				subsystem->OnTick(dt);
			}
		}

		void OnShutdown()
		{
			HPR_PROFILE_SCOPE();

			for (auto& subsystem : m_Subsystems | std::views::reverse)
			{
				subsystem->OnShutdown();
			}
		}

	private:
		std::vector<std::shared_ptr<Subsystem>> m_Subsystems;
	};
}
