#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/debug/RuntimeDebugSnapshot.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

struct NpcBehaviorDebugSnapshot;
struct PhysicsAabbCollisionBatchResult;
struct PhysicsDebugSnapshot;

enum class DebugProjectionKind : std::uint8_t {
  TargetCandidate,
  ReachRadius,
  CommandRejected,
  ClockMode,
  CameraMode,
  ObjectiveState,
  StateHash,
  ReplayDivergence,
  RuntimeTelemetry,
  NpcBehavior,
  PhysicsAabb,
  PhysicsContactNormal,
  PhysicsBroadphasePair,
};

struct DebugProjectionConfig {
  bool includeTargetCandidates = true;
  bool includeReach = true;
  bool includeCommandRejections = true;
  bool includeSessionFacts = true;
};

struct PhysicsDebugGeometryProjectionConfig {
  bool includeAabbs = true;
  bool includeContacts = true;
  bool includeBroadphasePairs = true;
  std::size_t maxAabbs = 128U;
  std::size_t maxContacts = 128U;
  std::size_t maxPairs = 128U;
};

struct DebugProjectionItem {
  DebugProjectionKind kind = DebugProjectionKind::StateHash;
  CommandTick sourceTick = kInvalidCommandTick;
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  EntityId actor;
  EntityId target;
  CommandRejectionReason rejection = CommandRejectionReason::None;
  bool hasWorldPoint = false;
  Vec3 worldPoint;
  bool hasBounds = false;
  Aabb3 worldBounds;
  bool hasScalar = false;
  float radiusMeters = 0.0F;
  float scalarValue = 0.0F;
  std::string objectiveId;
  std::string labelCode;
  std::string valueCode;
};

struct DebugProjectionResult {
  std::vector<DebugProjectionItem> items;
  std::vector<std::string> runtimeDebugHudLines;
  std::vector<std::string> npcBehaviorDebugHudLines;
  std::vector<std::string> physicsDebugHudLines;
  StateHashValue sourceStateHash = 0;
  CommandTick sourceTick = kInvalidCommandTick;
};

DebugProjectionResult buildDebugProjection(const SessionState& state,
                                           const DebugProjectionConfig& config = {});
void appendRuntimeDebugSnapshot(DebugProjectionResult& result,
                                const RuntimeDebugSnapshot& snapshot);
void appendNpcBehaviorDebugSnapshot(DebugProjectionResult& result,
                                    const NpcBehaviorDebugSnapshot& snapshot);
void appendPhysicsDebugSnapshot(DebugProjectionResult& result,
                                const PhysicsDebugSnapshot& snapshot);
void appendPhysicsCollisionBatchDebugProjection(
    DebugProjectionResult& result,
    const PhysicsAabbCollisionBatchResult& batch,
    const PhysicsDebugGeometryProjectionConfig& config = {});

}  // namespace iggy3d
