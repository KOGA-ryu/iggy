#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "config/RuntimeConfig.hpp"
#include "runtime/camera/CameraState.hpp"

namespace iggy3d {

enum class AppMode : std::uint8_t {
  HeadlessDemo,
  ValidatePackage,
  Replay,
  Help,
  Version,
};

enum class AppConfigStatus : std::uint8_t {
  Ok,
  MissingFixturePath,
  InvalidPackagePath,
  InvalidSavePath,
  InvalidLoadPath,
  InvalidReplayPath,
  InvalidExpectedSummaryPath,
  InvalidCameraMode,
  ConflictingMode,
  UnknownOption,
  MissingOptionValue,
};

struct AppConfig {
  AppMode mode = AppMode::HeadlessDemo;
  std::filesystem::path packagePath = "fixtures/demos/first_room/package.iggy3d.toml";
  std::filesystem::path savePath;
  std::filesystem::path loadPath;
  std::filesystem::path replayPath;
  std::filesystem::path expectedSummaryPath;
  RuntimeConfig runtimeConfig;
  CameraMode requestedRealtimeCamera = CameraMode::ThirdPerson;
  bool strict = false;
  bool verbose = false;
  bool printHelp = false;
  bool printVersion = false;
};

AppConfig makeDefaultAppConfig();
AppConfigStatus validateAppConfig(const AppConfig& config);
const char* appConfigStatusCode(AppConfigStatus status);

}  // namespace iggy3d
