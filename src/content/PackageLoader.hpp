#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "content/FixtureScenarioLoader.hpp"
#include "content/PackageManifest.hpp"
#include "core/diagnostics/Diagnostic.hpp"
#include "core/result/Result.hpp"

namespace iggy3d {

enum class PackageLoadStatus : std::uint8_t {
  Ok,
  MissingPackageFile,
  PackageReadFailed,
  ScenarioReadFailed,
  ParseError,
  UnsupportedKey,
  MissingRequiredKey,
  MissingScenarioId,
  InvalidNumber,
  InvalidEnum,
  InvalidPath,
};

struct PackageLoadRequest {
  std::string packagePath;
};

struct PackageLoadResult {
  PackageLoadStatus status = PackageLoadStatus::Ok;
  PackageManifest manifest;
  FixtureScenarioSeed scenario;
  std::vector<Diagnostic> diagnostics;
};

PackageLoadResult loadPackage(const PackageLoadRequest& request);
PackageLoadResult parsePackageText(
    const std::string& packageText,
    const std::string& scenarioText,
    const std::string& packageDirectory);

}  // namespace iggy3d
