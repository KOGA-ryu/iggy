#include "content/PackageLoader.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace iggy3d {

namespace {

enum class PackageTable {
  None,
  Package,
  Asset,
};

std::string_view trim(std::string_view value) {
  while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) {
    value.remove_prefix(1);
  }
  while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
    value.remove_suffix(1);
  }
  return value;
}

std::string stripComment(std::string_view value) {
  bool inString = false;
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '"') {
      inString = !inString;
    } else if (value[index] == '#' && !inString) {
      return std::string(trim(value.substr(0, index)));
    }
  }
  return std::string(trim(value));
}

bool parseString(std::string_view value, std::string& out) {
  value = trim(value);
  if (value.size() < 2U || value.front() != '"' || value.back() != '"') {
    return false;
  }
  out = std::string(value.substr(1U, value.size() - 2U));
  return true;
}

bool parseU32(std::string_view value, std::uint32_t& out) {
  value = trim(value);
  if (value.empty() || value.front() == '-') {
    return false;
  }
  std::uint64_t parsed = 0;
  for (char c : value) {
    if (c < '0' || c > '9') {
      return false;
    }
    parsed = parsed * 10U + static_cast<std::uint64_t>(c - '0');
    if (parsed > UINT32_MAX) {
      return false;
    }
  }
  out = static_cast<std::uint32_t>(parsed);
  return true;
}

std::string forbiddenLegacyPrefix() {
  return std::string{"/Users/kogaryu/"} + "iggy";
}

bool containsParentComponent(const std::filesystem::path& path) {
  for (const auto& component : path) {
    if (component == "..") {
      return true;
    }
  }
  return false;
}

bool invalidRelativePath(const std::string& path, std::string_view requiredFilename) {
  const std::filesystem::path candidate(path);
  return candidate.is_absolute() || containsParentComponent(candidate) ||
         candidate.filename() != requiredFilename || path.find(forbiddenLegacyPrefix()) != std::string::npos;
}

bool findScenarioPathForRead(const std::string& packageText, std::string& scenarioPath) {
  std::istringstream input(packageText);
  std::string rawLine;
  while (std::getline(input, rawLine)) {
    const std::string line = stripComment(rawLine);
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      continue;
    }
    const std::string key(trim(std::string_view(line).substr(0, equals)));
    if (key == "scenario") {
      return parseString(trim(std::string_view(line).substr(equals + 1U)), scenarioPath);
    }
  }
  return false;
}

Diagnostic packageDiag(std::string code, std::string message, std::string file, std::uint32_t line,
                       std::uint32_t column) {
  return makeDiagnostic(DiagnosticDomain::Content, DiagnosticSeverity::Error, std::move(code),
                        std::move(message), DiagnosticLocation{std::move(file), line, column});
}

PackageLoadResult fail(PackageLoadStatus status, std::string code, std::string message,
                       std::string file = {}, std::uint32_t line = 0, std::uint32_t column = 0) {
  PackageLoadResult result;
  result.status = status;
  result.diagnostics.push_back(packageDiag(std::move(code), std::move(message), std::move(file), line, column));
  return result;
}

bool readTextFile(const std::filesystem::path& path, std::string& text) {
  std::ifstream file(path);
  if (!file) {
    return false;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  text = buffer.str();
  return true;
}

PackageLoadStatus mapScenarioStatus(ScenarioLoadStatus status) {
  switch (status) {
    case ScenarioLoadStatus::Ok:
      return PackageLoadStatus::Ok;
    case ScenarioLoadStatus::ParseError:
      return PackageLoadStatus::ParseError;
    case ScenarioLoadStatus::UnsupportedKey:
      return PackageLoadStatus::UnsupportedKey;
    case ScenarioLoadStatus::MissingRequiredKey:
      return PackageLoadStatus::MissingRequiredKey;
    case ScenarioLoadStatus::MissingScenarioId:
      return PackageLoadStatus::MissingScenarioId;
    case ScenarioLoadStatus::InvalidNumber:
      return PackageLoadStatus::InvalidNumber;
    case ScenarioLoadStatus::InvalidEnum:
      return PackageLoadStatus::InvalidEnum;
    case ScenarioLoadStatus::InvalidPath:
      return PackageLoadStatus::InvalidPath;
  }
  return PackageLoadStatus::ParseError;
}

std::string scenarioDisplayPath(const std::string& packageDirectory, const std::string& scenarioPath) {
  if (packageDirectory.ends_with(".toml")) {
    return packageDirectory;
  }
  if (packageDirectory.empty()) {
    return scenarioPath;
  }
  return (std::filesystem::path(packageDirectory) / scenarioPath).generic_string();
}

}  // namespace

PackageLoadResult parsePackageText(
    const std::string& packageText,
    const std::string& scenarioText,
    const std::string& packageDirectory) {
  PackageManifest manifest;
  PackageTable table = PackageTable::None;
  bool packageId = false;
  bool schemaVersion = false;
  bool requiredRuntimeSchema = false;
  bool scenario = false;
  bool assetId = false;
  bool assetPath = false;

  std::istringstream input(packageText);
  std::string rawLine;
  std::uint32_t lineNumber = 0;
  while (std::getline(input, rawLine)) {
    ++lineNumber;
    const std::string line = stripComment(rawLine);
    if (line.empty()) {
      continue;
    }
    if (line == "[package]") {
      table = PackageTable::Package;
      continue;
    }
    if (line == "[[assets]]") {
      table = PackageTable::Asset;
      manifest.assets.push_back({});
      assetId = false;
      assetPath = false;
      continue;
    }
    if (line.starts_with("[")) {
      return fail(PackageLoadStatus::UnsupportedKey, "package.unsupported_key", "unsupported table", {},
                  lineNumber, 1);
    }
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      return fail(PackageLoadStatus::ParseError, "package.parse_error", "expected key", {}, lineNumber, 1);
    }
    const std::string key(trim(std::string_view(line).substr(0, equals)));
    const std::string_view value = trim(std::string_view(line).substr(equals + 1U));
    const std::uint32_t column = static_cast<std::uint32_t>(rawLine.find(key) + 1U);

    if (table == PackageTable::Package) {
      if (key == "id") {
        packageId = parseString(value, manifest.packageId);
      } else if (key == "schema_version") {
        schemaVersion = parseU32(value, manifest.schemaVersion);
      } else if (key == "required_runtime_schema") {
        requiredRuntimeSchema = parseU32(value, manifest.requiredRuntimeSchema);
      } else if (key == "scenario") {
        scenario = parseString(value, manifest.scenarioPath);
      } else {
        return fail(PackageLoadStatus::UnsupportedKey, "package.unsupported_key", "unsupported package key", {},
                    lineNumber, column);
      }
      const bool ok = (key == "id" && packageId) || (key == "schema_version" && schemaVersion) ||
                      (key == "required_runtime_schema" && requiredRuntimeSchema) ||
                      (key == "scenario" && scenario);
      if (!ok) {
        return fail(PackageLoadStatus::ParseError, "package.parse_error", "invalid package value", {},
                    lineNumber, column);
      }
    } else if (table == PackageTable::Asset && !manifest.assets.empty()) {
      PackageAssetRef& asset = manifest.assets.back();
      if (key == "id") {
        assetId = parseString(value, asset.id);
      } else if (key == "path") {
        assetPath = parseString(value, asset.path);
      } else {
        return fail(PackageLoadStatus::UnsupportedKey, "package.unsupported_key", "unsupported asset key", {},
                    lineNumber, column);
      }
      if ((key == "id" && !assetId) || (key == "path" && !assetPath)) {
        return fail(PackageLoadStatus::ParseError, "package.parse_error", "invalid asset value", {}, lineNumber,
                    column);
      }
    } else {
      return fail(PackageLoadStatus::ParseError, "package.parse_error", "key outside supported table", {},
                  lineNumber, column);
    }
  }

  if (!packageId || !schemaVersion || !requiredRuntimeSchema || !scenario) {
    return fail(PackageLoadStatus::MissingRequiredKey, "package.missing_required_key",
                "missing package key");
  }
  if (invalidRelativePath(manifest.scenarioPath, "scenario.iggy3d.toml")) {
    return fail(PackageLoadStatus::InvalidPath, "package.invalid_path", "invalid scenario path");
  }
  for (const PackageAssetRef& asset : manifest.assets) {
    if (asset.id.empty() || asset.path.empty() || invalidRelativePath(asset.path, std::filesystem::path(asset.path).filename().generic_string())) {
      return fail(PackageLoadStatus::InvalidPath, "package.invalid_path", "invalid asset path");
    }
  }

  ScenarioLoadResult scenarioResult = parseScenarioText(scenarioText);
  if (scenarioResult.status != ScenarioLoadStatus::Ok) {
    PackageLoadResult result;
    result.status = mapScenarioStatus(scenarioResult.status);
    result.diagnostics = std::move(scenarioResult.diagnostics);
    const std::string displayPath = scenarioDisplayPath(packageDirectory, manifest.scenarioPath);
    for (Diagnostic& diagnostic : result.diagnostics) {
      if (diagnostic.location.file.empty()) {
        diagnostic.location.file = displayPath;
      }
    }
    return result;
  }

  PackageLoadResult result;
  result.manifest = std::move(manifest);
  result.scenario = std::move(scenarioResult.seed);
  return result;
}

PackageLoadResult loadPackage(const PackageLoadRequest& request) {
  const std::filesystem::path packagePath(request.packagePath);
  if (!std::filesystem::exists(packagePath)) {
    return fail(PackageLoadStatus::MissingPackageFile, "package.missing_file", "missing package file",
                packagePath.generic_string());
  }
  std::ifstream packageFile(packagePath);
  if (!packageFile) {
    return fail(PackageLoadStatus::PackageReadFailed, "package.read_failed", "package read failed",
                packagePath.generic_string());
  }
  std::ostringstream packageBuffer;
  packageBuffer << packageFile.rdbuf();

  const std::filesystem::path packageDirectory = packagePath.parent_path();
  std::string scenarioRelativePath;
  if (!findScenarioPathForRead(packageBuffer.str(), scenarioRelativePath)) {
    return parsePackageText(packageBuffer.str(), "", packageDirectory.generic_string());
  }
  if (invalidRelativePath(scenarioRelativePath, "scenario.iggy3d.toml")) {
    return fail(PackageLoadStatus::InvalidPath, "package.invalid_path", "invalid scenario path",
                packagePath.generic_string());
  }
  const std::filesystem::path scenarioPath = packageDirectory / scenarioRelativePath;
  std::ifstream scenarioFile(scenarioPath);
  if (!scenarioFile) {
    return fail(PackageLoadStatus::ScenarioReadFailed, "scenario.read_failed", "scenario read failed",
                scenarioPath.generic_string());
  }
  std::ostringstream scenarioBuffer;
  scenarioBuffer << scenarioFile.rdbuf();
  PackageLoadResult result =
      parsePackageText(packageBuffer.str(), scenarioBuffer.str(), packageDirectory.generic_string());
  if (result.status != PackageLoadStatus::Ok) {
    return result;
  }
  for (const PackageAssetRef& asset : result.manifest.assets) {
    const std::filesystem::path assetPath = packageDirectory / asset.path;
    std::string assetText;
    if (!readTextFile(assetPath, assetText)) {
      return fail(PackageLoadStatus::PackageReadFailed, "package.asset_read_failed",
                  "asset read failed", assetPath.generic_string());
    }
    const std::string genericPath = assetPath.generic_string();
    if (genericPath.find(".room.") != std::string::npos) {
      RoomAssetParseResult parsed = parseRoomAssetText(assetText);
      if (!parsed.ok) {
        return fail(PackageLoadStatus::ParseError, parsed.reason, "room asset parse failed",
                    assetPath.generic_string());
      }
      result.rooms.push_back(std::move(parsed.room));
    } else if (genericPath.find(".meshes.") != std::string::npos) {
      MeshAssetParseResult parsed = parseMeshAssetText(assetText);
      if (!parsed.ok) {
        return fail(PackageLoadStatus::ParseError, parsed.reason, "mesh asset parse failed",
                    assetPath.generic_string());
      }
      result.meshes.primitives.insert(result.meshes.primitives.end(),
                                      parsed.library.primitives.begin(),
                                      parsed.library.primitives.end());
    } else if (genericPath.find(".materials.") != std::string::npos) {
      MaterialAssetParseResult parsed = parseMaterialAssetText(assetText);
      if (!parsed.ok) {
        return fail(PackageLoadStatus::ParseError, parsed.reason,
                    "material asset parse failed", assetPath.generic_string());
      }
      result.materials.materials.insert(result.materials.materials.end(),
                                       parsed.library.materials.begin(),
                                       parsed.library.materials.end());
    }
  }
  return result;
}

}  // namespace iggy3d
