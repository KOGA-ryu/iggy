#include "app/iggy3d/debug/PositionHud.hpp"

#include <charconv>
#include <cmath>
#include <string_view>
#include <utility>

#include "projection/scene/SceneProjection.hpp"

namespace iggy3d {
namespace {

constexpr float kLayerSpacingMeters = 4.0F;

std::string fixed3(float value) {
  char buffer[32]{};
  const auto [ptr, error] =
      std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::fixed, 3);
  // branch-gate: BG-1193
  if (error != std::errc{}) {
    return "unavailable";
  }
  return std::string(buffer, static_cast<std::size_t>(ptr - buffer));
}

std::string intText(std::int64_t value) {
  char buffer[32]{};
  const auto [ptr, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
  // branch-gate: BG-1193
  if (error != std::errc{}) {
    return "0";
  }
  return std::string(buffer, static_cast<std::size_t>(ptr - buffer));
}

std::int64_t gridCoordinate(float value) {
  // branch-gate: BG-1193
  if (!std::isfinite(value)) {
    return 0;
  }
  return static_cast<std::int64_t>(std::floor(value));
}

std::int64_t layerIndexFor(float yMeters) {
  // branch-gate: BG-1193
  if (!std::isfinite(yMeters)) {
    return 0;
  }
  return static_cast<std::int64_t>(std::floor((yMeters + 0.001F) / kLayerSpacingMeters));
}

float normalizedYaw(float yawDegrees) {
  // branch-gate: BG-1193
  if (!std::isfinite(yawDegrees)) {
    return 0.0F;
  }
  float yaw = std::fmod(yawDegrees, 360.0F);
  // branch-gate: BG-1193
  if (yaw < 0.0F) {
    yaw += 360.0F;
  }
  return yaw;
}

std::string facingForYaw(float yawDegrees) {
  const float yaw = normalizedYaw(yawDegrees);
  // branch-gate: BG-1193
  if (yaw >= 45.0F && yaw < 135.0F) {
    return "east";
  }
  // branch-gate: BG-1193
  if (yaw >= 135.0F && yaw < 225.0F) {
    return "south";
  }
  // branch-gate: BG-1193
  if (yaw >= 225.0F && yaw < 315.0F) {
    return "west";
  }
  return "north";
}

std::string roundedDegrees(float value) {
  // branch-gate: BG-1193
  if (!std::isfinite(value)) {
    return "0";
  }
  return intText(static_cast<std::int64_t>(std::lround(value)));
}

const SceneItem* playerItem(const SceneProjectionResult& scene) {
  for (const SceneItem& item : scene.items) {
    // branch-gate: BG-1193
    if (item.kind == SceneItemKind::Player || item.stableName == "player") {
      return &item;
    }
  }
  return nullptr;
}

void addLine(PositionHud& hud, std::string text) {
  hud.lines.push_back(PositionHudLine{std::move(text), hud.visible});
  hud.lineCount = hud.lines.size();
}

}  // namespace

PositionHud buildPositionHud(const PositionHudRequest& request) {
  PositionHud hud;
  hud.developerToolsEnabled = request.developerToolsEnabled;
  hud.debugOverlayEnabled = request.debugOverlayEnabled;
  hud.debugAvailable = request.scene != nullptr;
  hud.roomEditingReady = request.roomEditingReady;
  hud.yawDegrees = request.yawDegrees;
  hud.pitchDegrees = request.pitchDegrees;
  hud.facing = facingForYaw(request.yawDegrees);

  // branch-gate: BG-1193
  if (!request.gameplayActive) {
    return hud;
  }

  const bool requested =
      (request.developerToolsEnabled && request.debugOverlayEnabled) ||
      request.roomEditingReady;
  // branch-gate: BG-1193
  if (!requested) {
    return hud;
  }

  // branch-gate: BG-1193
  if (request.scene == nullptr) {
    hud.status = "projection_missing";
    hud.reasonCode = hud.status;
    return hud;
  }

  const SceneItem* player = playerItem(*request.scene);
  // branch-gate: BG-1193
  if (player == nullptr) {
    hud.status = "position_hud_player_missing";
    hud.reasonCode = hud.status;
    return hud;
  }

  hud.visible = true;
  hud.status = "position_hud_ready";
  hud.reasonCode = hud.status;
  hud.playerPositionAvailable = true;
  hud.worldX = player->transform.position.x;
  hud.worldY = player->transform.position.y;
  hud.worldZ = player->transform.position.z;
  hud.gridX = gridCoordinate(hud.worldX);
  hud.gridY = gridCoordinate(hud.worldY);
  hud.gridZ = gridCoordinate(hud.worldZ);
  hud.layerIndex = layerIndexFor(hud.worldY);

  addLine(hud,
          "XYZ " + fixed3(hud.worldX) + " " + fixed3(hud.worldY) + " " +
              fixed3(hud.worldZ));
  addLine(hud,
          "GRID " + intText(hud.gridX) + " " + intText(hud.gridY) + " " +
              intText(hud.gridZ) + " L" + intText(hud.layerIndex));
  addLine(hud,
          "FACE " + hud.facing + " yaw " + roundedDegrees(hud.yawDegrees) +
              " pitch " + roundedDegrees(hud.pitchDegrees));
  return hud;
}

}  // namespace iggy3d
