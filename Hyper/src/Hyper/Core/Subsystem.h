#pragma once

namespace Hyper
{
	class Context;

	class Subsystem
	{
	public:
		Subsystem(Context* pContext) { m_pContext = pContext; }
		virtual ~Subsystem() = default;

		virtual bool OnInitialize() { return true; }
		virtual bool OnPostInitialize() { return true; }
		virtual void OnTick(f32 /*dt*/) {}
		virtual void OnShutdown() {}

	protected:
		Context* m_pContext;
	};

	template<class T>
	concept EngineSystem = requires
	{
		std::derived_from<T, Subsystem>;
	};
}
