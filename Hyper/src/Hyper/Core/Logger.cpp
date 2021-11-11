#include "HyperPCH.h"
#include "Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Hyper
{
	std::shared_ptr<spdlog::logger> Logger::s_pEngineLogger;
	std::shared_ptr<spdlog::logger> Logger::s_pVulkanLogger;

	void Logger::Init()
	{
		std::string pattern = "[%T.%e] [%n] [%^%-8l%$] %v";
		spdlog::flush_every(std::chrono::seconds(5));

		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console_sink->set_level(spdlog::level::trace);
		
		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Output.log", true);
		file_sink->set_level(spdlog::level::trace);

		s_pEngineLogger = std::make_shared<spdlog::logger>(spdlog::logger("Hyper", { console_sink, file_sink }));
		s_pEngineLogger->set_pattern(pattern);
		s_pEngineLogger->set_level(spdlog::level::trace);

		s_pVulkanLogger = std::make_shared<spdlog::logger>(spdlog::logger("Vulkan", { console_sink, file_sink }));
		s_pVulkanLogger->set_pattern(pattern);
		s_pVulkanLogger->set_level(spdlog::level::trace);

	}
}
