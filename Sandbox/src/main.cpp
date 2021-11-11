#ifdef SANDBOX_USE_VLD
// ReSharper Disable Once CppUnusedIncludeDirective
#include <vld.h>
#endif

#include <Hyper.h>

int main()
{
	Hyper::Application app{};
	app.Run();
	return 0;
}
