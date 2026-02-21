#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <filesystem>

#define WL_HAS_CONSOLE !WL_DIST

namespace Walnut {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	std::filesystem::path GetLogsDirectory(const std::string& appName)
	{
		std::filesystem::path logsPath;

#ifdef _WIN32
		const char* appdata = std::getenv("LOCALAPPDATA");
		if (!appdata)
			appdata = std::getenv("APPDATA");

		if (appdata)
			logsPath = std::filesystem::path(appdata) / appName / "logs";
		else
			logsPath = "logs";

#elif __APPLE__
		const char* home = std::getenv("HOME");
		if (home)
			logsPath = std::filesystem::path(home) / "Library" / "Logs" / appName;
		else
			logsPath = "logs";

#elif __linux__
		const char* xdg_data = std::getenv("XDG_DATA_HOME");
		if (xdg_data)
			logsPath = std::filesystem::path(xdg_data) / appName / "logs";
		else {
			const char* home = std::getenv("HOME");
			if (home)
				logsPath = std::filesystem::path(home) / ".local" / "share" / appName / "logs";
			else
				logsPath = "logs";
		}
#else
		logsPath = "logs";
#endif

		return logsPath;
	}

	void Log::Init(const std::string& appName)
	{
		// Create "logs" directory if doesn't exist
		std::filesystem::path logsDirectory = GetLogsDirectory(appName);
		if (!std::filesystem::exists(logsDirectory)) {
			std::error_code ec;
			std::filesystem::create_directories(logsDirectory, ec);
		}

		std::vector<spdlog::sink_ptr> hazelSinks =
		{
			std::make_shared<spdlog::sinks::basic_file_sink_mt>(logsDirectory / "HAZEL.log", true),
#if WL_HAS_CONSOLE
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
#endif
		};

		std::vector<spdlog::sink_ptr> appSinks =
		{
			std::make_shared<spdlog::sinks::basic_file_sink_mt>(logsDirectory / "APP.log", true),
#if WL_HAS_CONSOLE
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
#endif
		};

		hazelSinks[0]->set_pattern("[%T] [%l] %n: %v");
		appSinks[0]->set_pattern("[%T] [%l] %n: %v");

#if WL_HAS_CONSOLE
		hazelSinks[1]->set_pattern("%^[%T] %n: %v%$");
		appSinks[1]->set_pattern("%^[%T] %n: %v%$");
#endif

		s_CoreLogger = std::make_shared<spdlog::logger>("WALNUT", hazelSinks.begin(), hazelSinks.end());
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", appSinks.begin(), appSinks.end());
		s_ClientLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		s_ClientLogger.reset();
		s_CoreLogger.reset();
		spdlog::drop_all();
	}

}
