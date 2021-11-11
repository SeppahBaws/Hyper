#pragma once

namespace Hyper
{
	class Subsystem
	{
	public:
		Subsystem() = default;
		virtual ~Subsystem() = default;

		virtual bool OnInitialize() { return true; }
		virtual void OnTick() {}
		virtual void OnShutdown() {}
	};
}
