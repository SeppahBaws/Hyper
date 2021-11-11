#pragma once
#include <memory>
#include <vector>
#include <ranges>

#include "Subsystem.h"

namespace Hyper
{
	class Context final
	{
	public:
		Context() = default;

		template<typename T>
		void AddSubsystem()
		{
			m_Subsystems.emplace_back(std::make_shared<T>());
		}

		template<typename T>
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

		void OnTick()
		{
			for (auto& subsystem : m_Subsystems)
			{
				subsystem->OnTick();
			}
		}

		void OnShutdown()
		{
			for (auto& subsystem : m_Subsystems | std::views::reverse)
			{
				subsystem->OnShutdown();
			}
		}

	private:
		std::vector<std::shared_ptr<Subsystem>> m_Subsystems;
	};
}
