#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
#include "runtime/inventory/InventoryState.hpp"
#include "runtime/objective/ObjectiveState.hpp"
#include "runtime/player/PlayerSlot.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/session/SessionState.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {

inline constexpr std::uint32_t kSaveSchemaVersion = 1;
inline constexpr std::uint32_t kMinimumReadableSaveSchemaVersion = 1;
inline constexpr std::uint32_t kRuntimeSaveVersion = 1;

struct SaveEnvelopeMetadata {
  std::uint32_t schemaVersion = kSaveSchemaVersion;
  std::uint32_t minimumReadableSchemaVersion = kMinimumReadableSaveSchemaVersion;
  std::uint32_t runtimeSaveVersion = kRuntimeSaveVersion;
  std::string packageId;
  std::string scenarioId;
  std::string createdByToolId;
  std::string saveId;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
  std::uint64_t savedStateHash = 0;
  std::string savedStateHashHex;
};

struct SaveSessionSection {
  SessionLifecycle lifecycle = SessionLifecycle::Loading;
  SessionOutcome outcome = SessionOutcome::None;
  std::uint64_t currentTick = 0;
  CommandId nextCommandId = 1;
  std::uint64_t sessionSeed = 0;
  std::uint32_t sessionSchemaVersion = 1;
  std::uint32_t fixedTickRateHz = 20;
  float interactionRangeMeters = 1.500F;
  float movementDistanceMeters = 3.000F;
  float slowTimeScale = 0.250F;
  std::string packageId;
  std::string scenarioId;
};

struct SaveEntityRecord {
  EntityId id;
  std::string stableName;
  EntityKind kind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 localBounds;
  bool active = true;
  bool persistent = true;
  bool targetable = false;
  std::vector<TargetAction> targetActions;
  InteractionKind interactionKind = InteractionKind::None;
  InteractionEffectKind interactionPrimaryEffect = InteractionEffectKind::None;
  std::string interactionItemId;
  std::uint32_t interactionItemCount = 0;
  std::string interactionObjectiveId;
  std::string interactionRequiredItemId;
  std::uint32_t interactionRequiredItemCount = 0;
  bool interactionRepeatable = false;
  bool interactionDeactivateTargetOnSuccess = false;
};

struct SaveWorldSection {
  EntityId nextEntityId;
  std::vector<SaveEntityRecord> entities;
};

struct SaveAuthoredRoomSemanticsRecord {
  std::string materialId;
  std::vector<std::string> traversalTags;
  std::vector<std::string> gameplayTags;
  bool walkable = false;
  bool blocksActor = false;
  bool blocksProjectile = false;
};

struct SaveAuthoredRoomFloorRecord {
  std::string id;
  std::int32_t storyIndex = 0;
  Vec3 centerMeters;
  Vec3 sizeMeters = {1.0F, 0.10F, 1.0F};
  SaveAuthoredRoomSemanticsRecord semantics;
  bool locked = false;
  bool hidden = false;
};

struct SaveAuthoredRoomWallRecord {
  std::string id;
  std::int32_t storyIndex = 0;
  Vec3 startMeters;
  Vec3 endMeters;
  float bottomY = 0.0F;
  float heightMeters = 2.0F;
  float thicknessMeters = 0.20F;
  SaveAuthoredRoomSemanticsRecord semantics;
  bool locked = false;
  bool hidden = false;
};

struct SaveAuthoredRoomMarkerRecord {
  std::string id;
  std::string tag;
  std::string glyph;
  std::uint32_t row = 0;
  std::uint32_t column = 0;
  Vec3 positionMeters;
  std::uint32_t sourceLine = 1;
  std::uint32_t sourceColumn = 1;
};

struct SaveAuthoredRoomSection {
  bool present = false;
  std::string id = "editable_room";
  std::uint32_t version = 1;
  std::string source = "iggy3d.editor";
  std::string sourceFile = "editable_room";
  std::string sourceSubset = "authoring";
  std::vector<SaveAuthoredRoomFloorRecord> floors;
  std::vector<SaveAuthoredRoomWallRecord> walls;
  std::vector<SaveAuthoredRoomMarkerRecord> markers;
};

struct SavePlayerSlotRecord {
  PlayerSlotId slotId = kInvalidPlayerSlotId;
  PlayerSlotKind kind = PlayerSlotKind::Unknown;
  EntityId controlledActor;
  std::string stableName;
};

struct SavePlayerSection {
  std::vector<SavePlayerSlotRecord> slots;
};

struct SaveClockSection {
  ClockMode mode = ClockMode::Normal;
  ClockMode previousUnpausedMode = ClockMode::Normal;
  float previousUnpausedTimeScale = 1.0F;
  std::uint64_t tickIndex = 0;
  std::uint32_t fixedTickRateHz = 20;
  float timeScale = 1.0F;
};

struct SaveCameraSection {
  CameraMode activeMode = CameraMode::ThirdPerson;
  CameraMode previousRealtimeMode = CameraMode::ThirdPerson;
  EntityId targetEntity;
  Vec3 targetPoint;
  bool targetHasPoint = false;
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
  float orbitDistance = 8.0F;
};

struct SaveCommandRecord {
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
  CommandKind kind = CommandKind::None;
  CommandSource source = CommandSource::Unknown;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  EntityId actor;
  bool hasTargetEntity = false;
  EntityId targetEntity;
  bool hasTargetPoint = false;
  Vec3 targetPoint;
  CommandId retrySourceCommandId = kInvalidCommandId;
  std::int32_t attackDamage = 0;
  CommandAbilityKind ability = CommandAbilityKind::None;
  Vec3 abilityDirection;
  CommandTick issuedTick = kInvalidCommandTick;
  CommandTick scheduledTick = kInvalidCommandTick;
  CommandAdmissionStatus admission = CommandAdmissionStatus::Pending;
  CommandRejectionReason rejection = CommandRejectionReason::None;
};

struct SaveCommandLogSection {
  CommandLogResetPolicy resetPolicy = CommandLogResetPolicy::Clear;
  CommandSequence nextSequence = 1;
  std::uint64_t epoch = 0;
  std::vector<SaveCommandRecord> records;
};

struct SaveAbilityActorRecord {
  EntityId actor;
  std::uint32_t arcaneFocus = 0;
  CommandTick arcaneBoltReadyTick = 0;
  CommandTick arcaneFocusNextRechargeTick = 0;
};

struct SaveAbilitySection {
  std::vector<SaveAbilityActorRecord> actors;
};

struct SaveInventoryStackRecord {
  std::string itemId;
  std::uint32_t count = 0;
};

struct SavePlayerInventoryRecord {
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::vector<SaveInventoryStackRecord> stacks;
};

struct SaveInventorySection {
  std::vector<SavePlayerInventoryRecord> players;
};

struct SaveCombatantRecord {
  EntityId entity;
  std::uint32_t factionId = 0;
  std::int32_t hitPoints = 0;
  std::int32_t maxHitPoints = 0;
  bool defeated = false;
};

struct SaveCombatSection {
  std::vector<SaveCombatantRecord> combatants;
};

struct SaveAiActorRecord {
  EntityId actor;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  bool enabled = true;
  std::string behaviorProfileId = "default";
  EntityId target;
  AiBehaviorKind behavior = AiBehaviorKind::Idle;
  AiIntentKind lastIntent = AiIntentKind::None;
  std::uint32_t cooldownTicksRemaining = 0;
};

struct SaveAiSection {
  std::vector<SaveAiActorRecord> actors;
};

struct SaveObjectiveRecord {
  std::string objectiveId;
  ObjectiveStatus status = ObjectiveStatus::Inactive;
  ObjectiveConditionKind conditionKind = ObjectiveConditionKind::None;
  PlayerSlotId conditionPlayerSlot = kInvalidPlayerSlotId;
  std::string conditionItemId;
  std::uint32_t conditionItemCount = 0;
};

struct SaveObjectiveSection {
  std::vector<SaveObjectiveRecord> objectives;
};

struct SaveEnvelope {
  SaveEnvelopeMetadata metadata;
  SaveSessionSection session;
  SaveWorldSection world;
  SaveAuthoredRoomSection authoredRoom;
  SavePlayerSection players;
  SaveClockSection clock;
  SaveCameraSection camera;
  SaveAbilitySection abilities;
  SaveCommandLogSection commandLog;
  SaveInventorySection inventory;
  SaveCombatSection combat;
  SaveAiSection ai;
  SaveObjectiveSection objectives;
};

}  // namespace iggy3d
