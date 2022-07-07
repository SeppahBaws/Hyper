#include "HyperPCH.h"
#include "UUID.h"

#include <random>

namespace Hyper
{
	static std::random_device g_RandomDevice;
	static std::mt19937_64 g_Engine(g_RandomDevice());
	static std::uniform_int_distribution<u64> g_UniformDistribution;

	UUID::UUID()
		: m_UUID(g_UniformDistribution(g_Engine))
	{
	}

	UUID::UUID(u64 uuid)
		: m_UUID(uuid)
	{
	}
}
