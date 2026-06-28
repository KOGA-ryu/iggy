#include "app/iggy3d/debug/PositionHud.hpp"

#include <iostream>

#include "projection/scene/SceneProjection.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

iggy3d::SceneProjectionResult sceneWithPlayer(iggy3d::Vec3 position) {
  iggy3d::SceneProjectionResult scene;
  iggy3d::SceneItem player;
  player.kind = iggy3d::SceneItemKind::Player;
  player.stableName = "player";
  player.active = true;
  player.visible = true;
  player.transform.position = position;
  scene.items.push_back(player);
  return scene;
}

bool hasLine(const iggy3d::PositionHud& hud, const char* text) {
  for (const iggy3d::PositionHudLine& line : hud.lines) {
    if (line.visible && line.text == text) {
      return true;
    }
  }
  return false;
}

bool hiddenUntilRequested() {
  const iggy3d::SceneProjectionResult scene =
      sceneWithPlayer({12.25F, 0.0F, -4.5F});
  const iggy3d::PositionHud inactive = iggy3d::buildPositionHud(
      iggy3d::PositionHudRequest{&scene, false, false, true, true, 0.0F, 0.0F});
  const iggy3d::PositionHud overlayDisabled = iggy3d::buildPositionHud(
      iggy3d::PositionHudRequest{&scene, true, false, true, false, 0.0F, 0.0F});

  return expect(!inactive.visible, "inactive hidden") &&
         expect(inactive.status == "not_requested", "inactive status") &&
         expect(!overlayDisabled.visible, "overlay disabled hidden") &&
         expect(overlayDisabled.debugAvailable, "hidden keeps debug availability") &&
         expect(overlayDisabled.lines.empty(), "hidden has no lines");
}

bool visibleWithDebugOverlay() {
  const iggy3d::SceneProjectionResult scene =
      sceneWithPlayer({12.25F, 8.05F, -4.5F});
  const iggy3d::PositionHud hud = iggy3d::buildPositionHud(
      iggy3d::PositionHudRequest{&scene, true, false, true, true, 92.0F, -7.0F});

  return expect(hud.visible, "debug overlay visible") &&
         expect(hud.status == "position_hud_ready", "ready status") &&
         expect(hud.playerPositionAvailable, "position available") &&
         expect(hud.gridX == 12, "grid x") &&
         expect(hud.gridY == 8, "grid y") &&
         expect(hud.gridZ == -5, "grid z floors negative") &&
         expect(hud.layerIndex == 2, "layer index") &&
         expect(hud.facing == "east", "east facing") &&
         expect(hud.lines.size() == 3U, "line count") &&
         expect(hasLine(hud, "XYZ 12.250 8.050 -4.500"), "xyz line") &&
         expect(hasLine(hud, "GRID 12 8 -5 L2"), "grid line") &&
         expect(hasLine(hud, "FACE east yaw 92 pitch -7"), "facing line");
}

bool roomEditorShowsWithoutDebugOverlay() {
  const iggy3d::SceneProjectionResult scene =
      sceneWithPlayer({0.0F, 4.0F, 0.0F});
  const iggy3d::PositionHud hud = iggy3d::buildPositionHud(
      iggy3d::PositionHudRequest{&scene, true, true, false, false, 181.0F, 3.0F});

  return expect(hud.visible, "room editor visible") &&
         expect(hud.facing == "south", "south facing") &&
         expect(hud.layerIndex == 1, "room editor layer") &&
         expect(hasLine(hud, "GRID 0 4 0 L1"), "room editor grid line");
}

bool rejectsMissingProjectionOrPlayer() {
  iggy3d::SceneProjectionResult emptyScene;
  const iggy3d::PositionHud missingProjection = iggy3d::buildPositionHud(
      iggy3d::PositionHudRequest{nullptr, true, false, true, true, 0.0F, 0.0F});
  const iggy3d::PositionHud missingPlayer = iggy3d::buildPositionHud(
      iggy3d::PositionHudRequest{&emptyScene, true, false, true, true, 0.0F, 0.0F});

  return expect(!missingProjection.visible, "missing projection hidden") &&
         expect(missingProjection.status == "projection_missing",
                "missing projection status") &&
         expect(!missingPlayer.visible, "missing player hidden") &&
         expect(missingPlayer.status == "position_hud_player_missing",
                "missing player status");
}

}  // namespace

int main() {
  const bool ok = hiddenUntilRequested() && visibleWithDebugOverlay() &&
                  roomEditorShowsWithoutDebugOverlay() &&
                  rejectsMissingProjectionOrPlayer();
  if (!ok) {
    return 1;
  }
  std::cout << "product_position_hud_tests=pass\n";
  return 0;
}
