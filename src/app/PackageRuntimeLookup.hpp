#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

enum class PackageMode : std::uint8_t {
  Headless,
  BuildTreeVisual,
  InstalledVisual,
};

struct PackageLookupConfig {
  PackageMode packageMode = PackageMode::Headless;
  std::filesystem::path executablePathOverride;
  std::filesystem::path resourceRootOverride;
  std::filesystem::path shaderRootOverride;
  std::filesystem::path diagnosticsDirOverride;
  bool requireShaderRoot = false;
  bool requireGraphicsRuntime = false;
  bool createDiagnosticsDir = true;
};

struct PackageRuntimeLookup {
  PackageMode packageMode = PackageMode::Headless;
  std::filesystem::path executablePath;
  std::filesystem::path executableDir;
  std::filesystem::path packageRoot;
  std::filesystem::path resourceRoot;
  std::filesystem::path shaderRoot;
  std::filesystem::path diagnosticsDir;
  bool resourceRootExists = false;
  bool shaderRootExists = false;
  bool diagnosticsDirWritable = false;
  std::string resourceRootSource = "unavailable";
  std::string shaderRootSource = "unavailable";
  std::string diagnosticsDirSource = "unavailable";
};

struct PackageLookupResult {
  PackageRuntimeLookup lookup;
  RenderOutcome outcome = RenderOutcome::Unsupported;
  RenderReason reason{"package_lookup_executable_missing", "package lookup executable missing"};
};

PackageLookupResult resolvePackageRuntimeLookup(const PackageLookupConfig& config);
std::string_view packageModeName(PackageMode mode);

}  // namespace iggy3d
