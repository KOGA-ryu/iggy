#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/diagnostics/Diagnostic.hpp"
#include "content/ScenarioSeed.hpp"

namespace iggy3d {

enum class ScenarioLoadStatus : std::uint8_t {
  Ok,
  ParseError,
  UnsupportedKey,
  MissingRequiredKey,
  MissingScenarioId,
  InvalidNumber,
  InvalidEnum,
  InvalidPath,
};

struct ScenarioLoadResult {
  ScenarioLoadStatus status = ScenarioLoadStatus::Ok;
  FixtureScenarioSeed seed;
  std::vector<Diagnostic> diagnostics;
};

ScenarioLoadResult parseScenarioText(const std::string& scenarioText);

}  // namespace iggy3d
