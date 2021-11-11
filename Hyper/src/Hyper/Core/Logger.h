#pragma once
#include <spdlog/spdlog.h>

namespace Hyper
{
	class Logger
	{
	public:
		Logger() = default;

		static void Init();

		static std::shared_ptr<spdlog::logger>& GetEngineLogger() { return s_pEngineLogger; }
		static std::shared_ptr<spdlog::logger>& GetVulkanLogger() { return s_pVulkanLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_pEngineLogger;
		static std::shared_ptr<spdlog::logger> s_pVulkanLogger;
	};
}

#define HPR_CORE_LOG_DEBUG(...)			::Hyper::Logger::GetEngineLogger()->debug(__VA_ARGS__)
#define HPR_CORE_LOG_TRACE(...)			::Hyper::Logger::GetEngineLogger()->trace(__VA_ARGS__)
#define HPR_CORE_LOG_INFO(...)			::Hyper::Logger::GetEngineLogger()->info(__VA_ARGS__)
#define HPR_CORE_LOG_WARN(...)			::Hyper::Logger::GetEngineLogger()->warn(__VA_ARGS__)
#define HPR_CORE_LOG_ERROR(...)			::Hyper::Logger::GetEngineLogger()->error(__VA_ARGS__)
#define HPR_CORE_LOG_CRITICAL(...)		::Hyper::Logger::GetEngineLogger()->critical(__VA_ARGS__)

#define HPR_VKLOG_TRACE(...)			::Hyper::Logger::GetVulkanLogger()->trace(__VA_ARGS__)
#define HPR_VKLOG_INFO(...)				::Hyper::Logger::GetVulkanLogger()->info(__VA_ARGS__)
#define HPR_VKLOG_WARN(...)				::Hyper::Logger::GetVulkanLogger()->warn(__VA_ARGS__)
#define HPR_VKLOG_ERROR(...)			::Hyper::Logger::GetVulkanLogger()->error(__VA_ARGS__)

