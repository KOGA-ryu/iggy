#include "app/PackageRuntimeLookup.hpp"

#include "app/platform/ExecutablePath.hpp"

#include <cstdlib>
#include <system_error>
#include <utility>
#include <vector>

namespace iggy3d {
namespace {

struct PathCandidate {
  std::filesystem::path path;
  std::string source;
};

RenderReason packageLookupReason(std::string_view code) {
  if (code == "package_lookup_ok") {
    return {code, "package lookup ok"};
  }
  if (code == "package_lookup_executable_missing") {
    return {code, "package lookup executable missing"};
  }
  if (code == "package_lookup_resource_root_missing") {
    return {code, "package lookup resource root missing"};
  }
  if (code == "package_lookup_shader_root_missing") {
    return {code, "package lookup shader root missing"};
  }
  if (code == "package_lookup_diagnostics_dir_unwritable") {
    return {code, "package lookup diagnostics dir unwritable"};
  }
  if (code == "package_lookup_cwd_forbidden") {
    return {code, "package lookup current working directory forbidden"};
  }
  return {"package_lookup_executable_missing", "package lookup executable missing"};
}

PackageLookupResult makeFailure(PackageRuntimeLookup lookup,
                                RenderOutcome outcome,
                                std::string_view code) {
  PackageLookupResult result;
  result.lookup = std::move(lookup);
  result.outcome = outcome;
  result.reason = packageLookupReason(code);
  return result;
}

std::filesystem::path normalizedAbsolute(const std::filesystem::path& path) {
  std::error_code error;
  std::filesystem::path absolute = std::filesystem::absolute(path, error);
  if (error) {
    return {};
  }
  if (std::filesystem::exists(absolute, error)) {
    const std::filesystem::path canonical = std::filesystem::weakly_canonical(absolute, error);
    if (!error && !canonical.empty()) {
      return canonical;
    }
  }
  return absolute.lexically_normal();
}

bool isDirectory(const std::filesystem::path& path) {
  std::error_code error;
  return std::filesystem::is_directory(path, error);
}

bool isRegularFile(const std::filesystem::path& path) {
  std::error_code error;
  return std::filesystem::is_regular_file(path, error);
}

bool isCurrentWorkingDirectory(const std::filesystem::path& path) {
  if (path.empty()) {
    return false;
  }
  std::error_code error;
  const std::filesystem::path cwd = std::filesystem::weakly_canonical(
      std::filesystem::current_path(error), error);
  if (error || cwd.empty()) {
    return false;
  }
  const std::filesystem::path normalized = normalizedAbsolute(path);
  return !normalized.empty() && normalized == cwd;
}

std::filesystem::path envPath(const char* name) {
  const char* value = std::getenv(name);
  if (value == nullptr || value[0] == '\0') {
    return {};
  }
  return std::filesystem::path{value};
}

void addCandidate(std::vector<PathCandidate>& candidates,
                  std::filesystem::path path,
                  std::string source) {
  if (!path.empty()) {
    candidates.push_back({std::move(path), std::move(source)});
  }
}

bool chooseExistingDirectory(const std::vector<PathCandidate>& candidates,
                             std::filesystem::path& path,
                             std::string& source) {
  for (const PathCandidate& candidate : candidates) {
    if (isCurrentWorkingDirectory(candidate.path)) {
      path = normalizedAbsolute(candidate.path);
      source = candidate.source;
      return false;
    }
    const std::filesystem::path normalized = normalizedAbsolute(candidate.path);
    if (!normalized.empty() && isDirectory(normalized)) {
      path = normalized;
      source = candidate.source;
      return true;
    }
  }
  return false;
}

std::vector<PathCandidate> resourceCandidates(const PackageLookupConfig& config,
                                              const PackageRuntimeLookup& lookup) {
  std::vector<PathCandidate> candidates;
  addCandidate(candidates, config.resourceRootOverride, "override");
  addCandidate(candidates, envPath("IGGY3D_RESOURCE_ROOT"), "environment");
  addCandidate(candidates, lookup.executableDir / "resources", "executable_relative");
  addCandidate(candidates, lookup.executableDir.parent_path() / "share" / "iggy3d",
               "install_prefix");
  if (config.packageMode == PackageMode::BuildTreeProduct) {
    addCandidate(candidates, lookup.executableDir / "fixtures", "build_tree");
    addCandidate(candidates, lookup.executableDir.parent_path() / "fixtures", "build_tree");
    addCandidate(candidates, lookup.executableDir.parent_path().parent_path() / "fixtures",
                 "build_tree");
  }
  if (config.packageMode == PackageMode::InstalledProduct) {
    addCandidate(candidates, lookup.executableDir.parent_path() / "Resources", "bundle");
    addCandidate(candidates, lookup.executableDir.parent_path().parent_path() / "Resources",
                 "bundle");
  }
  return candidates;
}

std::vector<PathCandidate> shaderCandidates(const PackageLookupConfig& config,
                                            const PackageRuntimeLookup& lookup) {
  std::vector<PathCandidate> candidates;
  addCandidate(candidates, config.shaderRootOverride, "override");
  addCandidate(candidates, envPath("IGGY3D_SHADER_ROOT"), "environment");
  if (config.packageMode == PackageMode::Headless && !config.requireShaderRoot &&
      !config.requireGraphicsRuntime && config.shaderRootOverride.empty()) {
    return candidates;
  }
  addCandidate(candidates, lookup.executableDir / "shaders" / "vulkan", "executable_relative");
  if (!lookup.resourceRoot.empty()) {
    addCandidate(candidates, lookup.resourceRoot / "shaders" / "vulkan", "resource_root");
  }
  if (config.packageMode == PackageMode::BuildTreeProduct) {
    addCandidate(candidates, lookup.executableDir / "generated" / "shaders" / "vulkan",
                 "build_tree");
    addCandidate(candidates, lookup.executableDir.parent_path() / "generated" / "shaders" /
                                 "vulkan",
                 "build_tree");
  }
  return candidates;
}

std::vector<PathCandidate> diagnosticsCandidates(const PackageLookupConfig& config,
                                                 const PackageRuntimeLookup& lookup) {
  std::vector<PathCandidate> candidates;
  addCandidate(candidates, config.diagnosticsDirOverride, "override");
  addCandidate(candidates, envPath("IGGY3D_DIAGNOSTICS_DIR"), "environment");
  if (config.packageMode == PackageMode::BuildTreeProduct) {
    addCandidate(candidates, lookup.executableDir / "artifacts" / "render_diagnostics",
                 "build_tree");
  }
  std::error_code error;
  const std::filesystem::path temp = std::filesystem::temp_directory_path(error);
  if (!error && !temp.empty()) {
    addCandidate(candidates, temp / "iggy3d" / "render_diagnostics", "temporary");
  }
  return candidates;
}

bool ensureDiagnosticsDir(const std::filesystem::path& path, bool create) {
  if (path.empty()) {
    return false;
  }
  if (isDirectory(path)) {
    return true;
  }
  if (!create) {
    return false;
  }
  std::error_code error;
  std::filesystem::create_directories(path, error);
  return !error && isDirectory(path);
}

bool chooseDiagnosticsDirectory(const std::vector<PathCandidate>& candidates,
                                bool create,
                                std::filesystem::path& path,
                                std::string& source) {
  for (const PathCandidate& candidate : candidates) {
    if (isCurrentWorkingDirectory(candidate.path)) {
      path = normalizedAbsolute(candidate.path);
      source = candidate.source;
      return false;
    }
    const std::filesystem::path normalized = normalizedAbsolute(candidate.path);
    if (!normalized.empty() && ensureDiagnosticsDir(normalized, create)) {
      path = normalized;
      source = candidate.source;
      return true;
    }
    if (candidate.source == "override") {
      path = normalized;
      source = candidate.source;
      return false;
    }
  }
  return false;
}

}  // namespace

std::string_view packageModeName(PackageMode mode) {
  switch (mode) {
    case PackageMode::Headless:
      return "headless";
    case PackageMode::BuildTreeProduct:
      return "build_tree_product";
    case PackageMode::InstalledProduct:
      return "installed_product";
  }
  return "headless";
}

PackageLookupResult resolvePackageRuntimeLookup(const PackageLookupConfig& config) {
  PackageRuntimeLookup lookup;
  lookup.packageMode = config.packageMode;

  if (!config.executablePathOverride.empty()) {
    if (!isRegularFile(config.executablePathOverride)) {
      return makeFailure(lookup, RenderOutcome::Unsupported,
                         "package_lookup_executable_missing");
    }
    lookup.executablePath = normalizedAbsolute(config.executablePathOverride);
    lookup.executableDir = lookup.executablePath.parent_path();
  } else {
    const ExecutablePathResult executable = resolveExecutablePath();
    if (!executable.resolved) {
      return makeFailure(lookup, RenderOutcome::Unsupported,
                         "package_lookup_executable_missing");
    }
    lookup.executablePath = executable.executablePath;
    lookup.executableDir = executable.executableDir;
  }

  if (lookup.executablePath.empty() || lookup.executableDir.empty()) {
    return makeFailure(lookup, RenderOutcome::Unsupported,
                       "package_lookup_executable_missing");
  }
  lookup.packageRoot = lookup.executableDir.parent_path();

  const bool resourceFound =
      chooseExistingDirectory(resourceCandidates(config, lookup), lookup.resourceRoot,
                              lookup.resourceRootSource);
  lookup.resourceRootExists = resourceFound;
  if (!resourceFound) {
    if (isCurrentWorkingDirectory(lookup.resourceRoot)) {
      return makeFailure(lookup, RenderOutcome::Unsupported, "package_lookup_cwd_forbidden");
    }
    if (config.requireGraphicsRuntime || config.packageMode == PackageMode::InstalledProduct ||
        !config.resourceRootOverride.empty()) {
      return makeFailure(lookup, RenderOutcome::Unsupported,
                         "package_lookup_resource_root_missing");
    }
  }

  const bool shaderFound = chooseExistingDirectory(shaderCandidates(config, lookup),
                                                  lookup.shaderRoot, lookup.shaderRootSource);
  lookup.shaderRootExists = shaderFound;
  if (!shaderFound) {
    if (isCurrentWorkingDirectory(lookup.shaderRoot)) {
      return makeFailure(lookup, RenderOutcome::Unsupported, "package_lookup_cwd_forbidden");
    }
    if (config.requireShaderRoot || !config.shaderRootOverride.empty()) {
      return makeFailure(lookup, RenderOutcome::Unsupported,
                         "package_lookup_shader_root_missing");
    }
  }

  const bool diagnosticsFound =
      chooseDiagnosticsDirectory(diagnosticsCandidates(config, lookup), config.createDiagnosticsDir,
                                 lookup.diagnosticsDir, lookup.diagnosticsDirSource);
  lookup.diagnosticsDirWritable = diagnosticsFound;
  if (!diagnosticsFound) {
    if (isCurrentWorkingDirectory(lookup.diagnosticsDir)) {
      return makeFailure(lookup, RenderOutcome::Unsupported, "package_lookup_cwd_forbidden");
    }
    return makeFailure(lookup, RenderOutcome::Unsupported,
                       "package_lookup_diagnostics_dir_unwritable");
  }

  PackageLookupResult result;
  result.lookup = std::move(lookup);
  result.outcome = RenderOutcome::Ok;
  result.reason = packageLookupReason("package_lookup_ok");
  return result;
}

}  // namespace iggy3d
