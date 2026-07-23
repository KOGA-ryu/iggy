#pragma once

#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "runtime/movement/MovementDefaults.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d::creative {

inline constexpr std::string_view kCreativeDefaultPlayerProfileId = "default";
inline constexpr std::string_view kCreativeDefaultPlayerSpawnGroup = "default";
inline constexpr float kCreativeDefaultPlayerEyeHeightMeters = 1.7F;

enum class CreativePlayerSpawnStatus : std::uint8_t {
  NotRequested,
  MissingDocument,
  InvalidDocument,
  MissingRoomBake,
  InvalidObject,
  InvalidSettings,
  UnsupportedProfile,
  OutsideWorldBounds,
  UnsupportedFloor,
  Obstructed,
  Unreachable,
  GroupUnavailable,
  Ready,
};

struct CreativePlayerSpawnPlanRequest {
  const CreativeDocument* document = nullptr;
  const CreativeRoomBakeResult* roomBake = nullptr;
  const CreativeObject* spawnObject = nullptr;
  float reachabilityCellSizeMeters = 1.0F;
};

// Exact spawn geometry shared by validation, previews, and activation. The
// authored point remains durable; groundedPosition is the physical foot point
// resolved from the same baked room activation will consume.
struct CreativePlayerSpawnPlan {
  bool requested = false;
  bool accepted = false;
  CreativePlayerSpawnStatus status = CreativePlayerSpawnStatus::NotRequested;
  std::string_view reasonCode = "creative_player_spawn_not_requested";
  CreativeObjectId objectId = kInvalidObjectId;
  CreativePlayerSpawnSettings settings{};
  RoomAnchorAsset anchor;
  Vec3 authoredPositionMeters;
  Vec3 groundedPositionMeters;
  Vec3 cameraPositionMeters;
  Vec3 facingDirection;
  float yawRadians = 0.0F;
  float clearanceRadiusMeters = 0.0F;
  float bodyHeightMeters = static_cast<float>(kDefaultPlayerStandingHeightMeters);
  std::string obstructionSurfaceId;
  CreativeRoomBakeReachabilityReceipt reachability;
};

struct CreativePlayerSpawnResolveRequest {
  const CreativeDocument* document = nullptr;
  const CreativeRoomBakeResult* roomBake = nullptr;
  std::string spawnGroup = std::string(kCreativeDefaultPlayerSpawnGroup);
  float reachabilityCellSizeMeters = 1.0F;
  bool includeHidden = false;
};

struct CreativePlayerSpawnResolveResult {
  bool requested = false;
  bool accepted = false;
  CreativePlayerSpawnStatus status = CreativePlayerSpawnStatus::NotRequested;
  std::string_view reasonCode = "creative_player_spawn_not_requested";
  std::size_t groupCandidateCount = 0U;
  std::size_t rejectedCandidateCount = 0U;
  CreativePlayerSpawnPlan selected;
};

[[nodiscard]] std::string_view toString(
    CreativePlayerSpawnStatus status) noexcept;
[[nodiscard]] bool isSupportedCreativePlayerProfileId(
    std::string_view profileId) noexcept;
[[nodiscard]] CreativePlayerSpawnPlan planCreativePlayerSpawn(
    const CreativePlayerSpawnPlanRequest& request);
[[nodiscard]] CreativePlayerSpawnResolveResult resolveCreativePlayerSpawn(
    const CreativePlayerSpawnResolveRequest& request);

}  // namespace iggy3d::creative
