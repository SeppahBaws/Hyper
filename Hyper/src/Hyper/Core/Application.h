#pragma once
#include <memory>

struct GLFWwindow;

namespace Hyper
{
	class Context;

	class Application
	{
	public:
		Application();
		~Application();

		void Run();

	private:
		std::shared_ptr<Context> m_pContext;
	};
}
