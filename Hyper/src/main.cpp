#include "HyperPCH.h"
#ifdef HYPER_USE_VLD
// ReSharper Disable Once CppUnusedIncludeDirective
#include <vld.h>
#endif

#include "Hyper/Core/Application.h"

int main()
{
	Hyper::Application app{};
	app.Run();
	return 0;
}
