#include <QApplication>

#include <filesystem>
#include <iostream>
#include <string>

#include "IggyQtShellWindow.hpp"

namespace {

struct ParsedArgs {
	bool ok = true;
	std::string error;
	iggy::qt_shell::IggyQtShellLaunchOptions launch;
};

void PrintUsage(const char *program)
{
	std::cerr << "usage: " << program << " [--preview PATH] [--preview-mode run|trace|check|lint] [--play PATH]\n";
}

bool ApplyPreviewMode(
	const std::string &value,
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig &config)
{
	if (value == "run") {
		config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Run;
		config.captureTraceFrames = false;
		config.lintOnly = false;
		return true;
	}
	if (value == "trace") {
		config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
		config.captureTraceFrames = true;
		config.lintOnly = false;
		return true;
	}
	if (value == "check") {
		config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Check;
		config.captureTraceFrames = false;
		config.lintOnly = false;
		return true;
	}
	if (value == "lint") {
		config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Lint;
		config.captureTraceFrames = false;
		config.lintOnly = true;
		return true;
	}
	return false;
}

ParsedArgs ParseArgs(int argc, char **argv)
{
	ParsedArgs parsed;
	for (int index = 1; index < argc; ++index) {
		const std::string arg = argv[index];
		if (arg == "--preview") {
			if (index + 1 >= argc) {
				parsed.ok = false;
				parsed.error = "--preview requires a path";
				return parsed;
			}
			parsed.launch.preview.enabled = true;
			parsed.launch.preview.path = std::filesystem::path(argv[++index]);
			continue;
		}
		if (arg == "--preview-mode") {
			if (index + 1 >= argc) {
				parsed.ok = false;
				parsed.error = "--preview-mode requires run, trace, check, or lint";
				return parsed;
			}
			if (!ApplyPreviewMode(argv[++index], parsed.launch.preview.config)) {
				parsed.ok = false;
				parsed.error = "unsupported --preview-mode";
				return parsed;
			}
			continue;
		}
		if (arg == "--play") {
			if (index + 1 >= argc) {
				parsed.ok = false;
				parsed.error = "--play requires a path";
				return parsed;
			}
			parsed.launch.play.enabled = true;
			parsed.launch.play.path = std::filesystem::path(argv[++index]);
			continue;
		}
		if (arg == "--help" || arg == "-h") {
			parsed.ok = false;
			parsed.error.clear();
			return parsed;
		}
	}
	if (parsed.launch.play.enabled && parsed.launch.preview.enabled) {
		parsed.ok = false;
		parsed.error = "--play cannot be combined with --preview";
		return parsed;
	}
	return parsed;
}

} // namespace

int main(int argc, char **argv)
{
	const ParsedArgs parsed = ParseArgs(argc, argv);
	if (!parsed.ok) {
		if (!parsed.error.empty())
			std::cerr << parsed.error << '\n';
		PrintUsage(argv[0]);
		return parsed.error.empty() ? 0 : 2;
	}

	QApplication app(argc, argv);
	iggy::qt_shell::IggyQtShellWindow window(parsed.launch);
	window.show();
	return app.exec();
}
