#pragma once

#include <cstddef>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "projection/scene/SceneItem.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

struct RoomAsset;

struct SceneProjectionConfig {
  bool includeInactive = false;
  bool includeObjectiveMarkers = true;
  bool includeTacticalMarkers = true;
  bool includeDebugOnly = false;
  // Emit NPC gaze-blade meshes (a debug visualization of each NPC's vision
  // direction and range). Off by default so normal frames are unaffected.
  bool includeNpcVisionDebug = false;
};

struct SceneProjectionResult {
  std::vector<SceneItem> items;
  std::size_t playerCount = 0;
  std::size_t pickupCount = 0;
  std::size_t interactableCount = 0;
  std::size_t markerCount = 0;
  std::size_t debugOnlyCount = 0;
  StateHashValue sourceStateHash = 0;
  CommandTick sourceTick = kInvalidCommandTick;
  CameraMode cameraMode = CameraMode::ThirdPerson;
  CameraMode previousRealtimeCamera = CameraMode::ThirdPerson;
  EntityId cameraTargetEntity;
  Vec3 cameraTargetPoint;
  bool cameraTargetHasPoint = false;
  SceneRoomProjection room;
  std::vector<SceneProjectileItem> projectiles;
  std::size_t projectileCount = 0;
};

SceneProjectionResult buildSceneProjection(const SessionState& state,
                                           const SceneProjectionConfig& config = {});
SceneProjectionResult buildSceneProjection(const SessionState& state,
                                           const RoomAsset* room,
                                           const SceneProjectionConfig& config = {});

}  // namespace iggy3d
