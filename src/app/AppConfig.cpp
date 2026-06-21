#include "app/AppConfig.hpp"

#include <string_view>

namespace iggy3d {

namespace {

bool containsOldIggyPath(const std::filesystem::path& path) {
  std::filesystem::path forbidden;
  forbidden /= "/";
  forbidden /= "Users";
  forbidden /= "kogaryu";
  forbidden /= "iggy";

  const std::filesystem::path normalized = path.lexically_normal();
  auto candidate = normalized.begin();
  auto legacy = forbidden.begin();
  for (; candidate != normalized.end() && legacy != forbidden.end(); ++candidate, ++legacy) {
    if (*candidate != *legacy) {
      return false;
    }
  }
  return legacy == forbidden.end();
}

bool containsParentComponent(const std::filesystem::path& path) {
  for (const auto& component : path) {
    if (component == "..") {
      return true;
    }
  }
  return false;
}

bool hasFilename(const std::filesystem::path& path) {
  return !path.filename().empty();
}

bool hasReplayExtension(const std::filesystem::path& path) {
  return path.filename().generic_string().ends_with(".iggy3d.replay");
}

bool primaryFlagConflict(const AppConfig& config) {
  if (config.printHelp && config.mode != AppMode::Help) {
    return true;
  }
  if (config.printVersion && config.mode != AppMode::Version) {
    return true;
  }
  return config.printHelp && config.printVersion;
}

}  // namespace

AppConfig makeDefaultAppConfig() {
  AppConfig config;
  config.mode = AppMode::HeadlessDemo;
  config.packagePath = "fixtures/demos/first_room/package.iggy3d.toml";
  config.requestedRealtimeCamera = CameraMode::ThirdPerson;
  return config;
}

AppConfigStatus validateAppConfig(const AppConfig& config) {
  if (config.printHelp) {
    return config.mode == AppMode::Help ? AppConfigStatus::Ok : AppConfigStatus::ConflictingMode;
  }
  if (config.printVersion) {
    return config.mode == AppMode::Version ? AppConfigStatus::Ok
                                           : AppConfigStatus::ConflictingMode;
  }
  if (primaryFlagConflict(config)) {
    return AppConfigStatus::ConflictingMode;
  }
  if (config.requestedRealtimeCamera != CameraMode::FirstPerson &&
      config.requestedRealtimeCamera != CameraMode::ThirdPerson) {
    return AppConfigStatus::InvalidCameraMode;
  }
  if ((config.mode == AppMode::HeadlessDemo || config.mode == AppMode::ValidatePackage) &&
      config.packagePath.empty()) {
    return AppConfigStatus::MissingFixturePath;
  }
  if (!config.packagePath.empty()) {
    if (containsOldIggyPath(config.packagePath) || containsParentComponent(config.packagePath) ||
        config.packagePath.filename() != "package.iggy3d.toml") {
      return AppConfigStatus::InvalidPackagePath;
    }
  }
  if (!config.savePath.empty() && (!hasFilename(config.savePath) || containsOldIggyPath(config.savePath))) {
    return AppConfigStatus::InvalidSavePath;
  }
  if (!config.loadPath.empty() && (!hasFilename(config.loadPath) || containsOldIggyPath(config.loadPath))) {
    return AppConfigStatus::InvalidLoadPath;
  }
  if (config.mode == AppMode::Replay && config.replayPath.empty()) {
    return AppConfigStatus::InvalidReplayPath;
  }
  if (!config.replayPath.empty() &&
      (!hasReplayExtension(config.replayPath) || containsOldIggyPath(config.replayPath))) {
    return AppConfigStatus::InvalidReplayPath;
  }
  if (!config.expectedSummaryPath.empty() &&
      (!hasFilename(config.expectedSummaryPath) || containsOldIggyPath(config.expectedSummaryPath))) {
    return AppConfigStatus::InvalidExpectedSummaryPath;
  }
  return AppConfigStatus::Ok;
}

const char* appConfigStatusCode(AppConfigStatus status) {
  switch (status) {
    case AppConfigStatus::Ok:
      return "app.ok";
    case AppConfigStatus::MissingFixturePath:
      return "app.missing_fixture_path";
    case AppConfigStatus::InvalidPackagePath:
      return "app.invalid_package_path";
    case AppConfigStatus::InvalidSavePath:
      return "app.invalid_save_path";
    case AppConfigStatus::InvalidLoadPath:
      return "app.invalid_load_path";
    case AppConfigStatus::InvalidReplayPath:
      return "app.invalid_replay_path";
    case AppConfigStatus::InvalidExpectedSummaryPath:
      return "app.invalid_expected_summary_path";
    case AppConfigStatus::InvalidCameraMode:
      return "app.invalid_camera_mode";
    case AppConfigStatus::ConflictingMode:
      return "app.conflicting_mode";
    case AppConfigStatus::UnknownOption:
      return "app.unknown_option";
    case AppConfigStatus::MissingOptionValue:
      return "app.missing_option_value";
  }
  return "app.internal_error";
}

}  // namespace iggy3d
