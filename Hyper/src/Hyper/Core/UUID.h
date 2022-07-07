#pragma once

#include "Hyper/Defines.h"

namespace Hyper
{
	class UUID
	{
	public:
		UUID();
		UUID(u64 uuid);
		UUID(const UUID& other) = default;
		UUID(UUID&& other) = default;
		UUID& operator=(const UUID& other) = default;
		UUID& operator=(UUID&& other) = default;

		operator const u64() const { return m_UUID; }

		bool operator==(const UUID& other) const
		{
			return m_UUID == other.m_UUID;
		}

	private:
		u64 m_UUID;
	};
}

namespace std {

	template <>
	struct hash<Hyper::UUID>
	{
		std::size_t operator()(const Hyper::UUID& id) const noexcept
		{
			// Implicit cast: UUID -> u64 -> std::size_t
			return id;
		}
	};

}
