#include "app/CliParser.hpp"

#include <array>

namespace iggy3d {

namespace {

Diagnostic appDiagnostic(std::string code, std::string message) {
  return makeDiagnostic(DiagnosticDomain::App, DiagnosticSeverity::Error, std::move(code),
                        std::move(message));
}

CliParseResult fail(CliParseResult result, AppConfigStatus status, std::string code,
                    std::string message) {
  result.status = status;
  result.diagnostics.push_back(appDiagnostic(std::move(code), std::move(message)));
  return result;
}

bool isOption(std::string_view value) {
  return value.starts_with("-");
}

bool needValue(const std::vector<std::string_view>& args, std::size_t index) {
  return index + 1U >= args.size() || isOption(args[index + 1U]);
}

bool setPrimaryMode(CliParseResult& result, bool& hasPrimaryMode, AppMode mode) {
  if (hasPrimaryMode) {
    return false;
  }
  hasPrimaryMode = true;
  result.config.mode = mode;
  return true;
}

}  // namespace

CliParseResult parseCommandLine(int argc, const char* const* argv) {
  std::vector<std::string_view> args;
  args.reserve(static_cast<std::size_t>(argc));
  for (int index = 0; index < argc; ++index) {
    args.push_back(argv[index]);
  }
  return parseCommandLine(args);
}

CliParseResult parseCommandLine(std::vector<std::string_view> args) {
  CliParseResult result;
  result.config = makeDefaultAppConfig();
  bool hasPrimaryMode = false;
  std::size_t index = 0;
  if (!args.empty() && !isOption(args[0])) {
    index = 1;
  }
  for (; index < args.size(); ++index) {
    const std::string_view arg = args[index];
    if (arg == "--demo") {
      if (!setPrimaryMode(result, hasPrimaryMode, AppMode::HeadlessDemo)) {
        return fail(result, AppConfigStatus::ConflictingMode, "cli.conflicting_mode",
                    "multiple primary modes");
      }
    } else if (arg == "--validate-package") {
      if (!setPrimaryMode(result, hasPrimaryMode, AppMode::ValidatePackage)) {
        return fail(result, AppConfigStatus::ConflictingMode, "cli.conflicting_mode",
                    "multiple primary modes");
      }
    } else if (arg == "--replay") {
      if (!setPrimaryMode(result, hasPrimaryMode, AppMode::Replay)) {
        return fail(result, AppConfigStatus::ConflictingMode, "cli.conflicting_mode",
                    "multiple primary modes");
      }
    } else if (arg == "--help" || arg == "-h") {
      if (!setPrimaryMode(result, hasPrimaryMode, AppMode::Help)) {
        return fail(result, AppConfigStatus::ConflictingMode, "cli.conflicting_mode",
                    "multiple primary modes");
      }
      result.config.printHelp = true;
    } else if (arg == "--version") {
      if (!setPrimaryMode(result, hasPrimaryMode, AppMode::Version)) {
        return fail(result, AppConfigStatus::ConflictingMode, "cli.conflicting_mode",
                    "multiple primary modes");
      }
      result.config.printVersion = true;
    } else if (arg == "--package" || arg == "--fixture") {
      if (needValue(args, index)) {
        return fail(result, AppConfigStatus::MissingOptionValue, "cli.missing_option_value",
                    "missing package path");
      }
      ++index;
      result.config.packagePath = std::string(args[index]);
    } else if (arg == "--save") {
      if (needValue(args, index)) {
        return fail(result, AppConfigStatus::MissingOptionValue, "cli.missing_option_value",
                    "missing save path");
      }
      ++index;
      result.config.savePath = std::string(args[index]);
    } else if (arg == "--load") {
      if (needValue(args, index)) {
        return fail(result, AppConfigStatus::MissingOptionValue, "cli.missing_option_value",
                    "missing load path");
      }
      ++index;
      result.config.loadPath = std::string(args[index]);
    } else if (arg == "--replay-path") {
      if (needValue(args, index)) {
        return fail(result, AppConfigStatus::MissingOptionValue, "cli.missing_option_value",
                    "missing replay path");
      }
      ++index;
      result.config.replayPath = std::string(args[index]);
    } else if (arg == "--summary") {
      if (needValue(args, index)) {
        return fail(result, AppConfigStatus::MissingOptionValue, "cli.missing_option_value",
                    "missing summary path");
      }
      ++index;
      result.config.expectedSummaryPath = std::string(args[index]);
    } else if (arg == "--camera") {
      if (needValue(args, index)) {
        return fail(result, AppConfigStatus::MissingOptionValue, "cli.missing_option_value",
                    "missing camera mode");
      }
      ++index;
      if (args[index] == "FirstPerson") {
        result.config.requestedRealtimeCamera = CameraMode::FirstPerson;
      } else if (args[index] == "ThirdPerson") {
        result.config.requestedRealtimeCamera = CameraMode::ThirdPerson;
      } else {
        return fail(result, AppConfigStatus::InvalidCameraMode, "cli.invalid_camera_mode",
                    "invalid camera mode");
      }
    } else if (arg == "--strict") {
      result.config.strict = true;
    } else if (arg == "--verbose" || arg == "-v") {
      result.config.verbose = true;
    } else {
      return fail(result, AppConfigStatus::UnknownOption, "cli.unknown_option", "unknown option");
    }
  }

  const AppConfigStatus validationStatus = validateAppConfig(result.config);
  if (validationStatus != AppConfigStatus::Ok) {
    return fail(result, validationStatus, "cli.invalid_app_config",
                appConfigStatusCode(validationStatus));
  }
  return result;
}

std::string_view cliHelpText() {
  return "iggy3d options: --demo --validate-package --replay --help -h --version --package "
         "--fixture --save --load --replay-path --summary --camera --strict --verbose -v";
}

std::string_view cliVersionText() {
  return "iggy3d 0.1.0 schema=1 runtime_schema=1";
}

}  // namespace iggy3d
