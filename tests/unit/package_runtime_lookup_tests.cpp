#include "app/PackageRuntimeLookup.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

std::filesystem::path processTempRoot() {
#if defined(_WIN32)
  const int processId = _getpid();
#else
  const int processId = getpid();
#endif
  return std::filesystem::temp_directory_path() /
         ("iggy3d_package_lookup_tests_" + std::to_string(processId));
}

void touchFile(const std::filesystem::path& path) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path);
  out << "iggy3d\n";
}

iggy3d::PackageLookupConfig baseConfig(const std::filesystem::path& executable) {
  iggy3d::PackageLookupConfig config;
  config.packageMode = iggy3d::PackageMode::BuildTreeVisual;
  config.executablePathOverride = executable;
  return config;
}

bool buildTreeRootsResolve() {
  const std::filesystem::path root = processTempRoot() / "build_tree";
  std::filesystem::remove_all(root);
  const std::filesystem::path executable = root / "build" / "iggy3d_visual_demo";
  const std::filesystem::path resourceRoot = root / "fixtures";
  const std::filesystem::path shaderRoot = root / "build" / "generated" / "shaders" / "vulkan";
  touchFile(executable);
  std::filesystem::create_directories(resourceRoot);
  std::filesystem::create_directories(shaderRoot);

  iggy3d::PackageLookupConfig config = baseConfig(executable);
  config.requireShaderRoot = true;
  const iggy3d::PackageLookupResult result = iggy3d::resolvePackageRuntimeLookup(config);
  return expect(result.outcome == iggy3d::RenderOutcome::Ok, "build tree ok") &&
         expect(result.reason.code == "package_lookup_ok", "build tree reason") &&
         expect(result.lookup.resourceRootSource == "build_tree", "resource source") &&
         expect(result.lookup.shaderRootSource == "build_tree", "shader source") &&
         expect(result.lookup.resourceRootExists, "resource exists") &&
         expect(result.lookup.shaderRootExists, "shader exists") &&
         expect(result.lookup.resourceRoot.is_absolute(), "resource absolute") &&
         expect(result.lookup.shaderRoot.is_absolute(), "shader absolute");
}

bool installedLayoutsResolve() {
  const std::filesystem::path root = processTempRoot() / "installed";
  std::filesystem::remove_all(root);
  const std::filesystem::path executable = root / "bin" / "iggy3d_visual_demo";
  const std::filesystem::path shareRoot = root / "share" / "iggy3d";
  const std::filesystem::path shaderRoot = shareRoot / "shaders" / "vulkan";
  touchFile(executable);
  std::filesystem::create_directories(shaderRoot);

  iggy3d::PackageLookupConfig config;
  config.packageMode = iggy3d::PackageMode::InstalledVisual;
  config.executablePathOverride = executable;
  config.requireShaderRoot = true;
  config.requireGraphicsRuntime = true;
  const iggy3d::PackageLookupResult result = iggy3d::resolvePackageRuntimeLookup(config);
  return expect(result.outcome == iggy3d::RenderOutcome::Ok, "installed ok") &&
         expect(result.lookup.resourceRootSource == "install_prefix", "install resource") &&
         expect(result.lookup.shaderRootSource == "resource_root", "install shader");
}

bool explicitShaderRootMissingFails() {
  const std::filesystem::path root = processTempRoot() / "missing_shader";
  std::filesystem::remove_all(root);
  const std::filesystem::path executable = root / "build" / "iggy3d_visual_demo";
  touchFile(executable);
  std::filesystem::create_directories(root / "fixtures");

  iggy3d::PackageLookupConfig config = baseConfig(executable);
  config.requireShaderRoot = true;
  config.shaderRootOverride = root / "missing" / "shaders";
  const iggy3d::PackageLookupResult result = iggy3d::resolvePackageRuntimeLookup(config);
  return expect(result.outcome == iggy3d::RenderOutcome::Unsupported, "missing shader fail") &&
         expect(result.reason.code == "package_lookup_shader_root_missing",
                "missing shader reason");
}

bool diagnosticsDirectoryPolicyWorks() {
  const std::filesystem::path root = processTempRoot() / "diagnostics";
  std::filesystem::remove_all(root);
  const std::filesystem::path executable = root / "build" / "iggy3d_visual_demo";
  const std::filesystem::path diagnostics = root / "diagnostics_out";
  touchFile(executable);
  std::filesystem::create_directories(root / "fixtures");

  iggy3d::PackageLookupConfig config = baseConfig(executable);
  config.diagnosticsDirOverride = diagnostics;
  const iggy3d::PackageLookupResult result = iggy3d::resolvePackageRuntimeLookup(config);

  const std::filesystem::path blocker = root / "blocker";
  touchFile(blocker);
  iggy3d::PackageLookupConfig blocked = baseConfig(executable);
  blocked.diagnosticsDirOverride = blocker / "child";
  const iggy3d::PackageLookupResult blockedResult = iggy3d::resolvePackageRuntimeLookup(blocked);

  return expect(result.outcome == iggy3d::RenderOutcome::Ok, "diagnostics ok") &&
         expect(result.lookup.diagnosticsDirSource == "override", "diagnostics source") &&
         expect(std::filesystem::is_directory(diagnostics), "diagnostics created") &&
         expect(blockedResult.outcome == iggy3d::RenderOutcome::Unsupported,
                "blocked diagnostics fail") &&
         expect(blockedResult.reason.code == "package_lookup_diagnostics_dir_unwritable",
                "blocked diagnostics reason");
}

bool currentWorkingDirectoryIsIgnored() {
  const std::filesystem::path root = processTempRoot() / "cwd";
  std::filesystem::remove_all(root);
  const std::filesystem::path executable = root / "build" / "iggy3d_visual_demo";
  const std::filesystem::path cwdResource = root / "cwd" / "resources";
  touchFile(executable);
  std::filesystem::create_directories(cwdResource);

  std::error_code error;
  const std::filesystem::path previous = std::filesystem::current_path(error);
  std::filesystem::current_path(cwdResource, error);

  iggy3d::PackageLookupConfig config;
  config.packageMode = iggy3d::PackageMode::InstalledVisual;
  config.executablePathOverride = executable;
  config.requireGraphicsRuntime = true;
  const iggy3d::PackageLookupResult result = iggy3d::resolvePackageRuntimeLookup(config);

  std::filesystem::current_path(previous, error);
  return expect(result.outcome == iggy3d::RenderOutcome::Unsupported, "cwd ignored fail") &&
         expect(result.reason.code == "package_lookup_resource_root_missing", "cwd reason");
}

bool headlessModeAllowsMissingShaderRoot() {
  const std::filesystem::path root = processTempRoot() / "headless";
  std::filesystem::remove_all(root);
  const std::filesystem::path executable = root / "tool";
  touchFile(executable);

  iggy3d::PackageLookupConfig config;
  config.packageMode = iggy3d::PackageMode::Headless;
  config.executablePathOverride = executable;
  const iggy3d::PackageLookupResult result = iggy3d::resolvePackageRuntimeLookup(config);
  return expect(result.outcome == iggy3d::RenderOutcome::Ok, "headless ok") &&
         expect(!result.lookup.shaderRootExists, "headless shader missing allowed");
}

bool requiredGraphicsRuntimeFailsMissingResourceRoot() {
  const std::filesystem::path root = processTempRoot() / "missing_resource";
  std::filesystem::remove_all(root);
  const std::filesystem::path executable = root / "bin" / "iggy3d_visual_demo";
  touchFile(executable);

  iggy3d::PackageLookupConfig config;
  config.packageMode = iggy3d::PackageMode::BuildTreeVisual;
  config.executablePathOverride = executable;
  config.requireGraphicsRuntime = true;
  const iggy3d::PackageLookupResult result = iggy3d::resolvePackageRuntimeLookup(config);
  return expect(result.outcome == iggy3d::RenderOutcome::Unsupported, "missing resource fail") &&
         expect(result.reason.code == "package_lookup_resource_root_missing",
                "missing resource reason");
}

}  // namespace

int main() {
  bool ok = true;
  ok = buildTreeRootsResolve() && ok;
  ok = installedLayoutsResolve() && ok;
  ok = explicitShaderRootMissingFails() && ok;
  ok = diagnosticsDirectoryPolicyWorks() && ok;
  ok = currentWorkingDirectoryIsIgnored() && ok;
  ok = headlessModeAllowsMissingShaderRoot() && ok;
  ok = requiredGraphicsRuntimeFailsMissingResourceRoot() && ok;
  std::filesystem::remove_all(processTempRoot());
  return ok ? 0 : 1;
}
