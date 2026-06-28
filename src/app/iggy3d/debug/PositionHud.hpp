#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/math/Vec3.hpp"

namespace iggy3d {

struct SceneProjectionResult;

struct PositionHudLine {
  std::string text;
  bool visible = false;
};

struct PositionHud {
  bool visible = false;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
  bool debugAvailable = false;
  bool roomEditingReady = false;
  bool playerPositionAvailable = false;
  std::size_t lineCount = 0U;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  float worldX = 0.0F;
  float worldY = 0.0F;
  float worldZ = 0.0F;
  std::int64_t gridX = 0;
  std::int64_t gridY = 0;
  std::int64_t gridZ = 0;
  std::int64_t layerIndex = 0;
  std::string facing = "north";
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
  std::vector<PositionHudLine> lines;
};

struct PositionHudRequest {
  const SceneProjectionResult* scene = nullptr;
  bool gameplayActive = false;
  bool roomEditingReady = false;
  bool developerToolsEnabled = false;
  bool debugOverlayEnabled = false;
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
};

PositionHud buildPositionHud(const PositionHudRequest& request);

}  // namespace iggy3d
