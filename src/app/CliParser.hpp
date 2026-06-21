#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/AppConfig.hpp"
#include "core/diagnostics/Diagnostic.hpp"

namespace iggy3d {

struct CliParseResult {
  AppConfigStatus status = AppConfigStatus::Ok;
  AppConfig config;
  std::vector<Diagnostic> diagnostics;
};

CliParseResult parseCommandLine(int argc, const char* const* argv);
CliParseResult parseCommandLine(std::vector<std::string_view> args);
std::string_view cliHelpText();
std::string_view cliVersionText();

}  // namespace iggy3d
