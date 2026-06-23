#include "app/PackageRuntimeLookup.hpp"
#if defined(IGGY3D_HAS_SDL3)
#include <SDL3/SDL.h>
#include "app/platform/SdlWindow.hpp"
#endif
#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
#include "app/platform/SdlVulkanSurface.hpp"
#include "render/vulkan/VulkanBackend.hpp"
#endif
#include "content/PackageLoader.hpp"
#include "content/authoring/EditableRoomDocument.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/FrameInput.hpp"
#include "render/RendererApi.hpp"
#include "runtime/ability/AbilitySystem.hpp"
#include "runtime/collision/CollisionQuery.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/debug/RuntimeDebugSnapshot.hpp"
#include "runtime/movement/MovementSystem.hpp"
#include "runtime/movement/MovementTraversal.hpp"
#include "runtime/movement/MovementTraversalSlots.hpp"
#include "runtime/player/PlayerMotor.hpp"
#include "runtime/save/SaveLoad.hpp"
#include "runtime/session/Session.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr std::string_view kVisualDemoWindowTitle = "iggy3d visual demo";

enum class VisualInputBackend : std::uint8_t {
  Keyboard,
  Gamepad,
  Auto,
  Scripted,
};

enum class DevMechanic : std::uint8_t {
  Walk,
  Crouch,
  Jump,
  Dash,
  Spell,
  Vault,
  Clamber,
  WireWalk,
};

struct VisualOptions {
  std::filesystem::path fixturePath = "demos/first_room/package.iggy3d.toml";
  std::filesystem::path shaderRoot;
  std::filesystem::path diagnosticsDir;
  std::filesystem::path codexControlPath;
  iggy3d::RendererBackendKind backend = iggy3d::RendererBackendKind::Null;
  bool autoBackend = false;
  bool requireRenderer = false;
  bool strictVulkan = false;
  bool printReceipt = false;
  bool window = false;
  bool windowFlagSeen = false;
  bool noWindowFlagSeen = false;
  bool interactive = false;
  bool scriptedPlayableSmoke = false;
  bool scriptedKinematicInput = false;
  bool scriptedCrouchInput = false;
  bool devMenu = false;
  bool codexControlPathSet = false;
  VisualInputBackend inputBackend = VisualInputBackend::Keyboard;
  std::uint32_t holdSeconds = 0U;
  std::uint32_t frames = 1U;
  bool framesExplicit = false;
};

struct ParseResult {
  bool ok = true;
  bool packagePathSet = false;
  bool fixturePathSet = false;
  std::string reason = "visual_demo_ok";
  VisualOptions options;
};

struct WindowReceiptFields {
  bool requested = false;
  bool sdlAvailable = false;
  bool created = false;
  bool opened = false;
  bool closed = false;
  bool drawable = false;
  bool quitRequested = false;
  std::uint32_t windowWidth = 0;
  std::uint32_t windowHeight = 0;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  std::uint32_t eventPollCount = 0;
};

struct PlayableReceiptFields {
  bool playable = false;
  bool interactiveMode = false;
  VisualInputBackend inputBackend = VisualInputBackend::Keyboard;
  bool targetDiscovered = false;
  bool initialReachFailed = false;
  bool reachPassed = false;
  bool interactionExecuted = false;
  bool retryExecuted = false;
  bool attackExecuted = false;
  bool objectiveComplete = false;
  bool acceptanceDemoRequested = false;
  bool acceptanceDemoComplete = false;
  bool acceptancePreResetSavedState = false;
  bool resetExecuted = false;
  bool resetBaselineRestored = false;
  bool resetCommandLogCleared = false;
  std::uint64_t resetNextCommandId = 0;
  bool tacticalToggleExecuted = false;
  bool tacticalModeActive = false;
  bool pauseExecuted = false;
  bool pauseTickFrozen = false;
  bool stepExecuted = false;
  bool stepAdvancedOnce = false;
  bool resumeExecuted = false;
  std::uint64_t pauseTickBefore = 0;
  std::uint64_t pauseTickAfter = 0;
  std::uint64_t stepTickBefore = 0;
  std::uint64_t stepTickAfter = 0;
  std::uint64_t resumeTickAfter = 0;
  std::string runtimeClockMode = "normal";
  bool saveLoadReplayStable = true;
  bool saveLoadRequested = false;
  bool saveLoadRoundtripPassed = false;
  bool saveLoadHashMatched = false;
  bool saveLoadSessionReplaced = false;
  std::uint64_t saveLoadSavedHash = 0;
  std::uint64_t saveLoadLoadedHash = 0;
  std::uint64_t saveLoadEncodedBytes = 0;
  std::string saveLoadStatus = "not_requested";
  bool retryAvailable = true;
  bool kinematicControllerActive = false;
  bool kinematicMovementAttempted = false;
  bool kinematicMovementAccepted = false;
  bool kinematicCommandLogIntegrated = false;
  bool movementClamped = false;
  bool movementSlid = false;
  bool groundSnapApplied = false;
  bool crouchAvailable = false;
  bool crouchActive = false;
  bool crouchInputObserved = false;
  bool playerMotorActive = false;
  bool playerGrounded = true;
  bool playerLanded = false;
  bool jumpInputObserved = false;
  bool jumpAccepted = false;
  bool airMoveIntentObserved = false;
  bool airControlActive = false;
  bool airMovementClamped = false;
  bool airMovementSlid = false;
  bool dashInputObserved = false;
  bool dashAccepted = false;
  bool dashActive = false;
  bool dashMovementClamped = false;
  bool dashMovementSlid = false;
  bool spellInputObserved = false;
  bool spellProjectileSpawned = false;
  bool spellProjectileActive = false;
  bool spellProjectileImpact = false;
  bool spellProjectileVisible = false;
  bool abilityCastRequested = false;
  bool abilityCastAccepted = false;
  std::uint64_t abilityCastRequestCount = 0;
  std::uint64_t abilityCastAcceptCount = 0;
  std::uint64_t abilityCastRejectCount = 0;
  std::uint64_t abilitySlotBusyRejectCount = 0;
  std::uint64_t abilityCooldownRejectCount = 0;
  std::uint64_t abilityResourceRejectCount = 0;
  bool abilitySlotBusyRejected = false;
  bool abilityCooldownRejected = false;
  bool abilityResourceRejected = false;
  bool abilityRecastAccepted = false;
  bool abilityRuntimeOwnedProjectile = false;
  bool abilityHitEntity = false;
  bool abilityDamageApplied = false;
  bool abilityTargetDefeated = false;
  bool trainingDummyCombatTracked = false;
  bool trainingDummyDefeated = false;
  std::uint64_t trainingDummyHitPoints = 0;
  std::uint64_t abilityResourceRemaining = 0;
  std::uint64_t abilityResourceCost = 0;
  std::uint64_t abilityResourceMax = 0;
  std::uint64_t abilityResourceNextRechargeTick = 0;
  std::uint64_t abilityResourceRechargeRemainingTicks = 0;
  std::uint64_t abilityCooldownReadyTick = 0;
  std::uint64_t abilityCooldownRemainingTicks = 0;
  bool debugOverlayEnabled = false;
  bool debugOverlayOpen = false;
  bool debugOverlayToggleObserved = false;
  bool debugOverlayVisible = false;
  bool debugPlayerPositionAvailable = false;
  bool debugSpeedAvailable = false;
  bool debugDistanceAvailable = false;
  std::string stance = "standing";
  std::string eyeHeightMeters = "1.650";
  std::string actorHeightMeters = "1.800";
  std::string movementSpeedMetersPerSecond = "4.800";
  std::string playerMotorPhase = "grounded";
  std::string playerMotorReason = "not_attempted";
  std::string verticalVelocityState = "zero";
  std::string horizontalVelocityState = "zero";
  std::string dashCooldownState = "ready";
  std::string abilityId = "none";
  std::string abilityCastStatus = "not_requested";
  std::string abilityCastReason = "not_requested";
  std::string abilityResourceState = "full";
  std::string abilityCooldownState = "ready";
  std::string abilityTickStatus = "no_active_projectile";
  std::string abilityTickReason = "not_requested";
  std::string abilityImpactKind = "none";
  std::uint64_t abilityHitEntityId = 0;
  std::string abilityHitStableName = "none";
  std::uint64_t abilityDamageAmount = 0;
  std::string spellProjectileStatus = "not_started";
  std::string spellProjectileReason = "not_requested";
  std::string spellProjectileHitSurfaceId = "none";
  std::string spellProjectilePositionX = "0.000";
  std::string spellProjectilePositionY = "0.000";
  std::string spellProjectilePositionZ = "0.000";
  std::string debugOverlayReason = "debug_overlay_disabled";
  std::string debugOverlaySurface = "closed";
  bool debugTitleFallbackActive = false;
  std::string debugPlayerPhase = "grounded";
  std::string debugMovementPolicyBand = "not_attempted";
  std::string debugGroundSurfaceId = "none";
  std::string debugHitSurfaceId = "none";
  bool debugGroundSampleValid = false;
  bool debugGroundContact = false;
  bool debugGroundWalkable = false;
  bool debugCarefulFooting = false;
  std::string debugGroundDistanceMeters = "0.000";
  std::string debugGroundNormalX = "0.000";
  std::string debugGroundNormalY = "1.000";
  std::string debugGroundNormalZ = "0.000";
  std::string debugSlopeAngleDegrees = "0.000";
  std::string debugSlopeUpDot = "1.000";
  std::string debugSpeedMultiplier = "1.000";
  std::string debugStaminaCostMultiplier = "1.000";
  std::string debugStepPenaltyMultiplier = "1.000";
  std::string debugMovementHorizontalDistanceMeters = "0.000";
  std::string debugMovementVerticalDeltaMeters = "0.000";
  std::string debugMovementGradePercent = "0.000";
  std::string debugSlopeTravelDirection = "stationary";
  std::string debugMovedThisFrameMeters = "0.000";
  std::string debugHorizontalSpeedMetersPerSecond = "0.000";
  std::string debugVerticalSpeedMetersPerSecond = "0.000";
  std::string debugDistanceFromSpawnMeters = "0.000";
  std::string debugPositionX = "0.000";
  std::string debugPositionY = "0.000";
  std::string debugPositionZ = "0.000";
  std::string debugYawRadians = "0.000";
  std::string debugPitchRadians = "0.000";
  bool debugTraversalPreviewAvailable = false;
  bool debugTraversalPreviewReady = false;
  bool debugTraversalPreviewCandidateAvailable = false;
  std::string debugTraversalPreviewStatus = "traversal_preview_unavailable";
  std::string debugTraversalPreviewHudCode = "NONE";
  std::string debugTraversalPreviewMechanic = "none";
  std::string debugTraversalPreviewSlotId = "none";
  std::string debugTraversalPreviewSlotKind = "none";
  std::string debugTraversalPreviewSlotHeightBand = "none";
  std::string debugTraversalPreviewTargetId = "none";
  std::string debugTraversalPreviewLandingSurfaceId = "none";
  std::string debugTraversalPreviewSlotLedgeHeightMeters = "0.000";
  std::string debugTraversalPreviewSlotUsableWidthMeters = "0.000";
  std::string debugTraversalPreviewSlotStartRangeMeters = "0.000";
  std::string debugTraversalPreviewSlotFacingDot = "0.000";
  bool debugTraversalAvailable = false;
  bool debugTraversalIntentRequested = false;
  bool debugTraversalIntentConsumed = false;
  bool debugTraversalIntentAccepted = false;
  bool debugTraversalIntentFallbackJumpAllowed = false;
  bool debugTraversalAttempted = false;
  bool debugTraversalAccepted = false;
  std::string debugTraversalIntentTrigger = "none";
  std::string debugTraversalIntentStatus = "traversal_intent_no_intent";
  std::string debugTraversalIntentSelectedMechanic = "none";
  std::string debugTraversalMechanic = "none";
  std::string debugTraversalReason = "not_attempted";
  std::string debugTraversalSlotId = "none";
  std::string debugTraversalSlotKind = "none";
  std::string debugTraversalSlotHeightBand = "none";
  std::string debugTraversalTargetId = "none";
  std::string debugTraversalLandingSurfaceId = "none";
  std::string debugTraversalSlotLedgeHeightMeters = "0.000";
  std::string debugTraversalSlotUsableWidthMeters = "0.000";
  std::string debugTraversalSlotStartRangeMeters = "0.000";
  std::string debugTraversalSlotFacingDot = "0.000";
  bool devMenuEnabled = false;
  bool devMenuOpen = false;
  bool devMenuToggleObserved = false;
  bool devMenuExecuteRequested = false;
  std::string devMenuSelectedMechanic = "walk";
  std::string devMenuExecutionStatus = "not_requested";
  bool editorEnabled = false;
  bool editorOpen = false;
  bool editorToggleObserved = false;
  std::string editorTool = "select";
  std::string editorPreset = "solid_wall";
  bool editorApplyRequested = false;
  bool editorDeleteRequested = false;
  bool editorUndoRequested = false;
  bool editorRedoRequested = false;
  std::string editorLastCommand = "none";
  std::string editorLastStatus = "not_requested";
  std::string editorSelectedId = "none";
  std::string editorCursorX = "0.000";
  std::string editorCursorY = "0.000";
  std::string editorCursorZ = "0.000";
  bool editorProbeAvailable = false;
  bool editorProbeHit = false;
  bool editorPlacementValid = false;
  std::string editorProbeStatus = "editor_probe_disabled";
  std::string editorProbeSurfaceId = "none";
  std::string editorProbeSurfaceRole = "none";
  std::string editorProbeDistanceMeters = "0.000";
  std::string editorProbeX = "0.000";
  std::string editorProbeY = "0.000";
  std::string editorProbeZ = "0.000";
  std::string editorProbeNormalX = "0.000";
  std::string editorProbeNormalY = "1.000";
  std::string editorProbeNormalZ = "0.000";
  bool editorGhostVisible = false;
  std::string editorGhostRole = "none";
  std::string editorGhostX = "0.000";
  std::string editorGhostY = "0.000";
  std::string editorGhostZ = "0.000";
  std::string editorGhostSizeX = "0.000";
  std::string editorGhostSizeY = "0.000";
  std::string editorGhostSizeZ = "0.000";
  std::string editorSelectionSource = "none";
  std::uint64_t editorFloorCount = 0;
  std::uint64_t editorWallCount = 0;
  std::uint64_t editorRuntimeStaticMeshCount = 0;
  std::uint64_t editorRuntimeSurfaceCount = 0;
  std::uint64_t editorRuntimeTraversalSlotCount = 0;
  std::uint64_t editorApplyCount = 0;
  std::uint64_t editorDeleteCount = 0;
  std::uint64_t editorUndoCount = 0;
  std::uint64_t editorRedoCount = 0;
  bool editorBakeOk = true;
  std::string editorBakeReason = "room_bake_ok";
  bool codexControlConfigured = false;
  bool codexControlRead = false;
  bool codexControlApplied = false;
  std::string codexControlStatus = "disabled";
  std::string codexControlPath = "unavailable";
  std::string movementReason = "not_attempted";
  std::string movementPolicyBand = "not_attempted";
  std::string movementDistanceMeters = "0.000";
  std::string movementHorizontalDistanceMeters = "0.000";
  std::string movementVerticalDeltaMeters = "0.000";
  std::string movementGradePercent = "0.000";
  std::string slopeTravelDirection = "stationary";
  bool traversalAttempted = false;
  bool traversalAccepted = false;
  std::string traversalMechanic = "none";
  std::string traversalReason = "not_attempted";
  std::string traversalSlotId = "none";
  std::string traversalSlotKind = "none";
  std::string traversalSlotHeightBand = "none";
  std::string traversalTargetId = "none";
  std::string traversalLandingSurfaceId = "none";
  std::string traversalSlotLedgeHeightMeters = "0.000";
  std::string traversalSlotUsableWidthMeters = "0.000";
  std::string traversalSlotStartRangeMeters = "0.000";
  std::string traversalSlotFacingDot = "0.000";
  std::string traversalDistanceMeters = "0.000";
  std::string traversalHorizontalDistanceMeters = "0.000";
  std::string traversalVerticalDeltaMeters = "0.000";
  std::string traversalGradePercent = "0.000";
  std::string traversalDirection = "stationary";
  bool traversalIntentRequested = false;
  bool traversalIntentConsumed = false;
  bool traversalIntentAccepted = false;
  bool traversalIntentFallbackJumpAllowed = false;
  std::string traversalIntentTrigger = "none";
  std::string traversalIntentStatus = "traversal_intent_no_intent";
  std::string traversalIntentSelectedMechanic = "none";
  std::string traversalStartX = "0.000";
  std::string traversalStartY = "0.000";
  std::string traversalStartZ = "0.000";
  std::string traversalFinalX = "0.000";
  std::string traversalFinalY = "0.000";
  std::string traversalFinalZ = "0.000";
  std::string movementStartX = "0.000";
  std::string movementStartY = "0.000";
  std::string movementStartZ = "0.000";
  std::string movementDestinationX = "0.000";
  std::string movementDestinationY = "0.000";
  std::string movementDestinationZ = "0.000";
  std::string movementFinalX = "0.000";
  std::string movementFinalY = "0.000";
  std::string movementFinalZ = "0.000";
  std::string groundSurfaceId = "none";
  std::string hitSurfaceId = "none";
  bool groundSampleValid = false;
  bool groundContact = false;
  bool groundWalkable = false;
  bool carefulFooting = false;
  std::string groundDistanceMeters = "0.000";
  std::string groundNormalX = "0.000";
  std::string groundNormalY = "1.000";
  std::string groundNormalZ = "0.000";
  std::string slopeAngleDegrees = "0.000";
  std::string slopeUpDot = "1.000";
  std::string speedMultiplier = "1.000";
  std::string staminaCostMultiplier = "1.000";
  std::string stepPenaltyMultiplier = "1.000";
  bool mouseLookAvailable = false;
  bool mouseLookUsed = false;
  bool gamepadAvailable = false;
  bool gamepadLeftStickUsed = false;
  bool gamepadRightStickUsed = false;
  std::string gamepadName = "unavailable";
  std::string gamepadMapping = "unavailable";
  std::string gamepadActionButton = "unavailable";
};

std::string_view inputBackendName(VisualInputBackend backend) {
  switch (backend) {
    case VisualInputBackend::Keyboard:
      return "keyboard";
    case VisualInputBackend::Gamepad:
      return "gamepad";
    case VisualInputBackend::Auto:
      return "auto";
    case VisualInputBackend::Scripted:
      return "scripted";
  }
  return "keyboard";
}

struct DevMenuState {
  bool enabled = false;
  bool open = false;
  DevMechanic selected = DevMechanic::Walk;
  bool executeRequested = false;
};

enum class EditorTool : std::uint8_t {
  Select,
  PlaceFloor,
  PlaceWall,
  Semantics,
  Delete,
};

enum class EditorPreset : std::uint8_t {
  Floor,
  SolidWall,
  ClamberWall,
  ProjectileWall,
};

struct EditorModeState {
  bool enabled = false;
  bool open = false;
  EditorTool tool = EditorTool::Select;
  EditorPreset preset = EditorPreset::SolidWall;
  iggy3d::EditableRoomSession session;
  std::string selectedId = "none";
  iggy3d::Vec3 cursorWorldMeters;
  bool probeAvailable = false;
  bool probeHit = false;
  bool placementValid = false;
  std::string probeStatus = "editor_probe_disabled";
  std::string probeSurfaceId = "none";
  std::string probeSurfaceRole = "none";
  float probeDistanceMeters = 0.0F;
  iggy3d::Vec3 probePointMeters;
  iggy3d::Vec3 probeNormal = {0.0F, 1.0F, 0.0F};
  bool ghostVisible = false;
  std::string ghostRole = "none";
  iggy3d::Vec3 ghostPositionMeters;
  iggy3d::Vec3 ghostSizeMeters;
  std::string selectionSource = "none";
  std::string lastCommand = "none";
  std::string lastStatus = "not_requested";
  std::uint64_t nextFloorIndex = 1;
  std::uint64_t nextWallIndex = 1;
  std::uint64_t applyCount = 0;
  std::uint64_t deleteCount = 0;
  std::uint64_t undoCount = 0;
  std::uint64_t redoCount = 0;
  bool bakeOk = true;
  std::string bakeReason = "room_bake_ok";
  std::uint64_t runtimeStaticMeshCount = 0;
  std::uint64_t runtimeSurfaceCount = 0;
  std::uint64_t runtimeTraversalSlotCount = 0;
};

struct CodexControlFrame {
  bool configured = false;
  bool read = false;
  bool applied = false;
  bool parseError = false;
  std::string status = "disabled";
  std::uint64_t lineCount = 0;
  bool devMenuOpenSet = false;
  bool devMenuOpen = false;
  bool debugOverlayOpenSet = false;
  bool debugOverlayOpen = false;
  bool editorOpenSet = false;
  bool editorOpen = false;
  bool editorToolSet = false;
  EditorTool editorTool = EditorTool::Select;
  bool editorPresetSet = false;
  EditorPreset editorPreset = EditorPreset::SolidWall;
  bool editorApply = false;
  std::vector<std::uint32_t> editorApplyFrames;
  bool editorDelete = false;
  std::vector<std::uint32_t> editorDeleteFrames;
  bool editorUndo = false;
  std::vector<std::uint32_t> editorUndoFrames;
  bool editorRedo = false;
  std::vector<std::uint32_t> editorRedoFrames;
  bool editorCursorSet = false;
  iggy3d::Vec3 editorCursorMeters;
  bool editorSelectSet = false;
  std::string editorSelectId;
  bool mechanicSet = false;
  DevMechanic mechanic = DevMechanic::Walk;
  bool executeMechanic = false;
  std::vector<std::uint32_t> executeMechanicFrames;
  std::vector<std::uint32_t> tacticalToggleFrames;
  std::vector<std::uint32_t> pauseFrames;
  std::vector<std::uint32_t> stepFrames;
  std::vector<std::uint32_t> resumeFrames;
  std::vector<std::uint32_t> saveLoadFrames;
  std::vector<std::uint32_t> resetFrames;
  bool jump = false;
  bool dash = false;
  bool stanceSet = false;
  bool crouched = false;
  bool moveSet = false;
  float moveForward = 0.0F;
  float moveRight = 0.0F;
  bool playerPositionSet = false;
  iggy3d::Vec3 playerPositionMeters;
  bool yawSet = false;
  bool pitchSet = false;
  float yaw = 0.0F;
  float pitch = 0.0F;
  float yawDelta = 0.0F;
  float pitchDelta = 0.0F;
  bool acceptanceDemo = false;
  bool interact = false;
  bool attack = false;
  bool reset = false;
  bool quit = false;
};

std::string_view devMechanicName(DevMechanic mechanic) {
  switch (mechanic) {
    case DevMechanic::Walk:
      return "walk";
    case DevMechanic::Crouch:
      return "crouch";
    case DevMechanic::Jump:
      return "jump";
    case DevMechanic::Dash:
      return "dash";
    case DevMechanic::Spell:
      return "spell";
    case DevMechanic::Vault:
      return "vault";
    case DevMechanic::Clamber:
      return "clamber";
    case DevMechanic::WireWalk:
      return "wire_walk";
  }
  return "walk";
}

bool parseDevMechanic(std::string_view value, DevMechanic& out) {
  if (value == "walk") {
    out = DevMechanic::Walk;
    return true;
  }
  if (value == "crouch" || value == "crouched") {
    out = DevMechanic::Crouch;
    return true;
  }
  if (value == "jump" || value == "jump_stub") {
    out = DevMechanic::Jump;
    return true;
  }
  if (value == "dash" || value == "dash_stub") {
    out = DevMechanic::Dash;
    return true;
  }
  if (value == "spell" || value == "fire" || value == "projectile") {
    out = DevMechanic::Spell;
    return true;
  }
  if (value == "vault" || value == "vault_stub") {
    out = DevMechanic::Vault;
    return true;
  }
  if (value == "clamber" || value == "clamber_stub") {
    out = DevMechanic::Clamber;
    return true;
  }
  if (value == "wire_walk" || value == "wire_walk_stub") {
    out = DevMechanic::WireWalk;
    return true;
  }
  return false;
}

DevMechanic nextDevMechanic(DevMechanic mechanic) {
  switch (mechanic) {
    case DevMechanic::Walk:
      return DevMechanic::Crouch;
    case DevMechanic::Crouch:
      return DevMechanic::Jump;
    case DevMechanic::Jump:
      return DevMechanic::Dash;
    case DevMechanic::Dash:
      return DevMechanic::Spell;
    case DevMechanic::Spell:
      return DevMechanic::Vault;
    case DevMechanic::Vault:
      return DevMechanic::Clamber;
    case DevMechanic::Clamber:
      return DevMechanic::WireWalk;
    case DevMechanic::WireWalk:
      return DevMechanic::Walk;
  }
  return DevMechanic::Walk;
}

DevMechanic previousDevMechanic(DevMechanic mechanic) {
  switch (mechanic) {
    case DevMechanic::Walk:
      return DevMechanic::WireWalk;
    case DevMechanic::Crouch:
      return DevMechanic::Walk;
    case DevMechanic::Jump:
      return DevMechanic::Crouch;
    case DevMechanic::Dash:
      return DevMechanic::Jump;
    case DevMechanic::Spell:
      return DevMechanic::Dash;
    case DevMechanic::Vault:
      return DevMechanic::Spell;
    case DevMechanic::Clamber:
      return DevMechanic::Vault;
    case DevMechanic::WireWalk:
      return DevMechanic::Clamber;
  }
  return DevMechanic::Walk;
}

std::string_view editorToolName(EditorTool tool) {
  switch (tool) {
    case EditorTool::Select:
      return "select";
    case EditorTool::PlaceFloor:
      return "place_floor";
    case EditorTool::PlaceWall:
      return "place_wall";
    case EditorTool::Semantics:
      return "semantics";
    case EditorTool::Delete:
      return "delete";
  }
  return "select";
}

bool parseEditorTool(std::string_view value, EditorTool& out) {
  if (value == "select") {
    out = EditorTool::Select;
    return true;
  }
  if (value == "place_floor" || value == "floor") {
    out = EditorTool::PlaceFloor;
    return true;
  }
  if (value == "place_wall" || value == "wall") {
    out = EditorTool::PlaceWall;
    return true;
  }
  if (value == "semantics" || value == "meaning") {
    out = EditorTool::Semantics;
    return true;
  }
  if (value == "delete") {
    out = EditorTool::Delete;
    return true;
  }
  return false;
}

EditorTool nextEditorTool(EditorTool tool) {
  switch (tool) {
    case EditorTool::Select:
      return EditorTool::PlaceFloor;
    case EditorTool::PlaceFloor:
      return EditorTool::PlaceWall;
    case EditorTool::PlaceWall:
      return EditorTool::Semantics;
    case EditorTool::Semantics:
      return EditorTool::Delete;
    case EditorTool::Delete:
      return EditorTool::Select;
  }
  return EditorTool::Select;
}

EditorTool previousEditorTool(EditorTool tool) {
  switch (tool) {
    case EditorTool::Select:
      return EditorTool::Delete;
    case EditorTool::PlaceFloor:
      return EditorTool::Select;
    case EditorTool::PlaceWall:
      return EditorTool::PlaceFloor;
    case EditorTool::Semantics:
      return EditorTool::PlaceWall;
    case EditorTool::Delete:
      return EditorTool::Semantics;
  }
  return EditorTool::Select;
}

std::string_view editorPresetName(EditorPreset preset) {
  switch (preset) {
    case EditorPreset::Floor:
      return "floor";
    case EditorPreset::SolidWall:
      return "solid_wall";
    case EditorPreset::ClamberWall:
      return "clamber_wall";
    case EditorPreset::ProjectileWall:
      return "projectile_wall";
  }
  return "solid_wall";
}

bool parseEditorPreset(std::string_view value, EditorPreset& out) {
  if (value == "floor" || value == "walkable_floor") {
    out = EditorPreset::Floor;
    return true;
  }
  if (value == "solid_wall" || value == "wall") {
    out = EditorPreset::SolidWall;
    return true;
  }
  if (value == "clamber_wall" || value == "clamber") {
    out = EditorPreset::ClamberWall;
    return true;
  }
  if (value == "projectile_wall" || value == "projectile_blocker") {
    out = EditorPreset::ProjectileWall;
    return true;
  }
  return false;
}

EditorPreset nextEditorPreset(EditorPreset preset) {
  switch (preset) {
    case EditorPreset::Floor:
      return EditorPreset::SolidWall;
    case EditorPreset::SolidWall:
      return EditorPreset::ClamberWall;
    case EditorPreset::ClamberWall:
      return EditorPreset::ProjectileWall;
    case EditorPreset::ProjectileWall:
      return EditorPreset::Floor;
  }
  return EditorPreset::SolidWall;
}

constexpr float kStandingEyeHeightMeters = 1.65F;
constexpr float kCrouchedEyeHeightMeters = 1.05F;
constexpr float kStandingActorHeightMeters = 1.80F;
constexpr float kCrouchedActorHeightMeters = 1.20F;
constexpr float kStandingSpeedMetersPerSecond = 4.80F;
constexpr float kCrouchedSpeedMetersPerSecond = 2.35F;

void recordStance(PlayableReceiptFields& fields, bool crouched) {
  fields.crouchAvailable = true;
  fields.crouchActive = crouched;
  fields.crouchInputObserved = fields.crouchInputObserved || crouched;
  fields.stance = crouched ? "crouched" : "standing";
  fields.eyeHeightMeters = crouched ? "1.050" : "1.650";
  fields.actorHeightMeters = crouched ? "1.200" : "1.800";
  fields.movementSpeedMetersPerSecond = crouched ? "2.350" : "4.800";
}

std::string_view velocityState(float velocityMetersPerSecond) {
  if (velocityMetersPerSecond > 0.01F) {
    return "positive";
  }
  if (velocityMetersPerSecond < -0.01F) {
    return "negative";
  }
  return "zero";
}

std::string debugFloat(float value) {
  char buffer[32];
  if (!std::isfinite(value)) {
    return "nan";
  }
  (void)std::snprintf(buffer, sizeof(buffer), "%.3f", static_cast<double>(value));
  return buffer;
}

std::string debugOverlayWindowTitle(const iggy3d::RuntimeDebugSnapshot& snapshot) {
  if (!snapshot.enabled) {
    return std::string(kVisualDemoWindowTitle);
  }
  if (snapshot.status != iggy3d::RuntimeDebugSnapshotStatus::Ok) {
    const std::string reason =
        snapshot.reasonCode == nullptr ? "unavailable" : snapshot.reasonCode;
    return "iggy3d | debug unavailable | " + reason;
  }
  return "iggy3d | pos " + debugFloat(snapshot.position.x) + ", " +
         debugFloat(snapshot.position.y) + ", " + debugFloat(snapshot.position.z) +
         " | speed " + debugFloat(snapshot.horizontalSpeedMetersPerSecond) + " m/s" +
         " | up " + debugFloat(snapshot.verticalSpeedMetersPerSecond) + " m/s" +
         " | moved " + debugFloat(snapshot.movedThisFrameMeters) + " m" +
         " | dist " + debugFloat(snapshot.distanceFromSpawnMeters) + " m" +
         " | slope " + debugFloat(snapshot.slopeAngleDegrees) + "deg " +
         snapshot.movementPolicyBand +
         " | " + std::string(iggy3d::playerMotorPhaseName(snapshot.motorPhase));
}

std::string_view trimControlText(std::string_view value) {
  while (!value.empty() &&
         (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) {
    value.remove_prefix(1);
  }
  while (!value.empty() &&
         (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
    value.remove_suffix(1);
  }
  return value;
}

std::string_view stripControlComment(std::string_view value) {
  const std::size_t comment = value.find('#');
  if (comment != std::string_view::npos) {
    value = value.substr(0, comment);
  }
  return trimControlText(value);
}

bool parseControlBool(std::string_view value, bool& out) {
  value = trimControlText(value);
  if (value == "true" || value == "1" || value == "yes" || value == "on") {
    out = true;
    return true;
  }
  if (value == "false" || value == "0" || value == "no" || value == "off") {
    out = false;
    return true;
  }
  return false;
}

bool parseControlFloat(std::string_view value, float& out) {
  value = trimControlText(value);
  if (value.empty()) {
    return false;
  }
  std::string text(value);
  char* end = nullptr;
  errno = 0;
  out = std::strtof(text.c_str(), &end);
  return end != text.c_str() && *end == '\0' && errno != ERANGE && std::isfinite(out);
}

bool parseControlVec3(std::string_view value, iggy3d::Vec3& out) {
  value = trimControlText(value);
  const std::size_t firstComma = value.find(',');
  if (firstComma == std::string_view::npos) {
    return false;
  }
  const std::size_t secondComma = value.find(',', firstComma + 1U);
  if (secondComma == std::string_view::npos ||
      value.find(',', secondComma + 1U) != std::string_view::npos) {
    return false;
  }

  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
  if (!parseControlFloat(value.substr(0, firstComma), x) ||
      !parseControlFloat(value.substr(firstComma + 1U, secondComma - firstComma - 1U), y) ||
      !parseControlFloat(value.substr(secondComma + 1U), z)) {
    return false;
  }
  out = {x, y, z};
  return iggy3d::isFinite(out);
}

iggy3d::Vec3 feetToMeters(iggy3d::Vec3 value) {
  constexpr float kFeetToMeters = 0.3048F;
  return value * kFeetToMeters;
}

bool parseControlFrameList(std::string_view value, std::vector<std::uint32_t>& out) {
  out.clear();
  value = trimControlText(value);
  if (value.empty()) {
    return false;
  }

  std::size_t cursor = 0;
  while (cursor <= value.size()) {
    const std::size_t comma = value.find(',', cursor);
    const std::size_t end = comma == std::string_view::npos ? value.size() : comma;
    const std::string_view token = trimControlText(value.substr(cursor, end - cursor));
    if (token.empty()) {
      return false;
    }

    std::uint64_t parsed = 0;
    for (const char c : token) {
      if (c < '0' || c > '9') {
        return false;
      }
      parsed = parsed * 10U + static_cast<std::uint64_t>(c - '0');
      if (parsed > std::numeric_limits<std::uint32_t>::max()) {
        return false;
      }
    }
    out.push_back(static_cast<std::uint32_t>(parsed));

    if (comma == std::string_view::npos) {
      break;
    }
    cursor = comma + 1U;
  }

  return !out.empty();
}

bool controlFrameListed(const std::vector<std::uint32_t>& frames, std::uint32_t frameIndex) {
  return std::find(frames.begin(), frames.end(), frameIndex) != frames.end();
}

void setControlStatus(CodexControlFrame& frame, std::string status) {
  if (frame.status != "parse_error") {
    frame.status = std::move(status);
  }
}

CodexControlFrame readCodexControlFile(const VisualOptions& options) {
  CodexControlFrame frame;
  frame.configured = options.codexControlPathSet;
  if (!options.codexControlPathSet) {
    return frame;
  }

  std::ifstream input(options.codexControlPath);
  if (!input) {
    frame.status = "missing";
    return frame;
  }

  frame.read = true;
  frame.status = "read";
  std::string line;
  while (std::getline(input, line)) {
    const std::string_view trimmed = stripControlComment(line);
    if (trimmed.empty()) {
      continue;
    }
    ++frame.lineCount;
    const std::size_t equals = trimmed.find('=');
    if (equals == std::string_view::npos || equals == 0U) {
      frame.parseError = true;
      frame.status = "parse_error";
      continue;
    }
    const std::string_view key = trimControlText(trimmed.substr(0, equals));
    const std::string_view value = trimControlText(trimmed.substr(equals + 1U));

    bool boolValue = false;
    float floatValue = 0.0F;
    iggy3d::Vec3 vecValue;
    std::vector<std::uint32_t> frameList;
    DevMechanic mechanic = DevMechanic::Walk;
    EditorTool editorTool = EditorTool::Select;
    EditorPreset editorPreset = EditorPreset::SolidWall;
    std::string editorSelectId;
    if (key == "dev_menu.open") {
      if (parseControlBool(value, boolValue)) {
        frame.devMenuOpenSet = true;
        frame.devMenuOpen = boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "debug_overlay.open" || key == "debug.open") {
      if (parseControlBool(value, boolValue)) {
        frame.debugOverlayOpenSet = true;
        frame.debugOverlayOpen = boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.open") {
      if (parseControlBool(value, boolValue)) {
        frame.editorOpenSet = true;
        frame.editorOpen = boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.tool") {
      if (parseEditorTool(value, editorTool)) {
        frame.editorToolSet = true;
        frame.editorTool = editorTool;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.preset") {
      if (parseEditorPreset(value, editorPreset)) {
        frame.editorPresetSet = true;
        frame.editorPreset = editorPreset;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.apply") {
      if (parseControlBool(value, boolValue)) {
        frame.editorApply = frame.editorApply || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.apply_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.editorApplyFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.delete") {
      if (parseControlBool(value, boolValue)) {
        frame.editorDelete = frame.editorDelete || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.delete_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.editorDeleteFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.undo") {
      if (parseControlBool(value, boolValue)) {
        frame.editorUndo = frame.editorUndo || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.undo_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.editorUndoFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.redo") {
      if (parseControlBool(value, boolValue)) {
        frame.editorRedo = frame.editorRedo || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.redo_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.editorRedoFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.cursor") {
      if (parseControlVec3(value, vecValue)) {
        frame.editorCursorSet = true;
        frame.editorCursorMeters = vecValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "editor.select") {
      editorSelectId = std::string(value);
      if (!editorSelectId.empty()) {
        frame.editorSelectSet = true;
        frame.editorSelectId = editorSelectId;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "dev_menu.select" || key == "mechanic") {
      if (parseDevMechanic(value, mechanic)) {
        frame.mechanicSet = true;
        frame.mechanic = mechanic;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "mechanic.execute") {
      if (parseControlBool(value, boolValue)) {
        frame.executeMechanic = frame.executeMechanic || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "mechanic.execute_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.executeMechanicFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "tactical_toggle_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.tacticalToggleFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "pause_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.pauseFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "step_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.stepFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "resume_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.resumeFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "save_load_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.saveLoadFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "acceptance_demo") {
      if (parseControlBool(value, boolValue)) {
        frame.acceptanceDemo = frame.acceptanceDemo || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "jump") {
      if (parseControlBool(value, boolValue)) {
        frame.jump = frame.jump || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "dash") {
      if (parseControlBool(value, boolValue)) {
        frame.dash = frame.dash || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "stance") {
      if (value == "crouched" || value == "crouch") {
        frame.stanceSet = true;
        frame.crouched = true;
        frame.applied = true;
      } else if (value == "standing" || value == "stand") {
        frame.stanceSet = true;
        frame.crouched = false;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "move.forward") {
      if (parseControlFloat(value, floatValue)) {
        frame.moveForward = std::clamp(floatValue, -1.0F, 1.0F);
        frame.moveSet = true;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "move.right") {
      if (parseControlFloat(value, floatValue)) {
        frame.moveRight = std::clamp(floatValue, -1.0F, 1.0F);
        frame.moveSet = true;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "player.position" || key == "player.position_meters") {
      if (parseControlVec3(value, vecValue)) {
        frame.playerPositionSet = true;
        frame.playerPositionMeters = vecValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "player.position_ft") {
      if (parseControlVec3(value, vecValue)) {
        frame.playerPositionSet = true;
        frame.playerPositionMeters = feetToMeters(vecValue);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "look.yaw") {
      if (parseControlFloat(value, floatValue)) {
        frame.yaw = std::clamp(floatValue, -6.283185F, 6.283185F);
        frame.yawSet = true;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "look.pitch") {
      if (parseControlFloat(value, floatValue)) {
        frame.pitch = std::clamp(floatValue, -0.8F, 0.8F);
        frame.pitchSet = true;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "look.yaw_delta") {
      if (parseControlFloat(value, floatValue)) {
        frame.yawDelta += std::clamp(floatValue, -0.8F, 0.8F);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "look.pitch_delta") {
      if (parseControlFloat(value, floatValue)) {
        frame.pitchDelta += std::clamp(floatValue, -0.8F, 0.8F);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "interact") {
      if (parseControlBool(value, boolValue)) {
        frame.interact = frame.interact || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "attack") {
      if (parseControlBool(value, boolValue)) {
        frame.attack = frame.attack || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "reset") {
      if (parseControlBool(value, boolValue)) {
        frame.reset = frame.reset || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "reset_frames") {
      if (parseControlFrameList(value, frameList)) {
        frame.resetFrames = std::move(frameList);
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else if (key == "quit") {
      if (parseControlBool(value, boolValue)) {
        frame.quit = frame.quit || boolValue;
        frame.applied = true;
      } else {
        frame.parseError = true;
        frame.status = "parse_error";
      }
    } else {
      frame.parseError = true;
      frame.status = "parse_error";
    }
  }

  if (frame.applied) {
    setControlStatus(frame, "applied");
  }
  return frame;
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

ParseResult parseOptions(int argc, const char* const* argv) {
  ParseResult result;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    if (arg == "--package" && hasValue(i, argc)) {
      if (result.fixturePathSet || result.packagePathSet) {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
      result.options.fixturePath = argv[++i];
      result.packagePathSet = true;
    } else if (arg == "--fixture" && hasValue(i, argc)) {
      if (result.packagePathSet || result.fixturePathSet) {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
      result.options.fixturePath = argv[++i];
      result.fixturePathSet = true;
    } else if (arg == "--renderer" && hasValue(i, argc)) {
      const std::string_view renderer{argv[++i]};
      if (renderer == "null") {
        result.options.backend = iggy3d::RendererBackendKind::Null;
        result.options.autoBackend = false;
      } else if (renderer == "vulkan") {
        result.options.backend = iggy3d::RendererBackendKind::Vulkan;
        result.options.autoBackend = false;
      } else if (renderer == "auto") {
        result.options.backend = iggy3d::RendererBackendKind::Null;
        result.options.autoBackend = true;
      } else {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
      }
    } else if (arg == "--renderer=null") {
      result.options.backend = iggy3d::RendererBackendKind::Null;
      result.options.autoBackend = false;
    } else if (arg == "--renderer=vulkan") {
      result.options.backend = iggy3d::RendererBackendKind::Vulkan;
      result.options.autoBackend = false;
    } else if (arg == "--renderer=auto") {
      result.options.backend = iggy3d::RendererBackendKind::Null;
      result.options.autoBackend = true;
    } else if (arg == "--require-renderer") {
      result.options.requireRenderer = true;
    } else if (arg == "--strict-vulkan") {
      result.options.strictVulkan = true;
      result.options.backend = iggy3d::RendererBackendKind::Vulkan;
      result.options.requireRenderer = true;
    } else if (arg == "--window") {
      if (result.options.noWindowFlagSeen) {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
      result.options.window = true;
      result.options.windowFlagSeen = true;
    } else if (arg == "--no-window") {
      if (result.options.windowFlagSeen) {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
      result.options.window = false;
      result.options.noWindowFlagSeen = true;
    } else if (arg == "--interactive") {
      result.options.interactive = true;
    } else if (arg == "--scripted-playable-smoke") {
      result.options.scriptedPlayableSmoke = true;
      result.options.inputBackend = VisualInputBackend::Scripted;
      result.options.backend = iggy3d::RendererBackendKind::Vulkan;
      result.options.window = true;
      result.options.windowFlagSeen = true;
    } else if (arg == "--scripted-kinematic-input") {
      result.options.scriptedKinematicInput = true;
      result.options.interactive = true;
      result.options.inputBackend = VisualInputBackend::Scripted;
    } else if (arg == "--scripted-crouch-input") {
      result.options.scriptedCrouchInput = true;
      result.options.interactive = true;
      result.options.inputBackend = VisualInputBackend::Scripted;
    } else if (arg == "--dev-menu") {
      result.options.devMenu = true;
      result.options.interactive = true;
    } else if (arg == "--codex-control" && hasValue(i, argc)) {
      result.options.codexControlPath = argv[++i];
      result.options.codexControlPathSet = true;
      result.options.devMenu = true;
      result.options.interactive = true;
      result.options.inputBackend = VisualInputBackend::Scripted;
    } else if (arg == "--hold-seconds" && hasValue(i, argc)) {
      const std::string_view count{argv[++i]};
      std::uint32_t value = 0U;
      for (const char c : count) {
        if (c < '0' || c > '9') {
          result.ok = false;
          result.reason = "visual_demo_config_invalid";
          return result;
        }
        value = value * 10U + static_cast<std::uint32_t>(c - '0');
      }
      result.options.holdSeconds = value;
    } else if (arg == "--input" && hasValue(i, argc)) {
      const std::string_view input{argv[++i]};
      if (input == "keyboard") {
        result.options.inputBackend = VisualInputBackend::Keyboard;
      } else if (input == "gamepad") {
        result.options.inputBackend = VisualInputBackend::Gamepad;
      } else if (input == "auto") {
        result.options.inputBackend = VisualInputBackend::Auto;
      } else {
        result.ok = false;
        result.reason = "visual_demo_config_invalid";
        return result;
      }
    } else if (arg == "--frames" && hasValue(i, argc)) {
      const std::string_view count{argv[++i]};
      std::uint32_t value = 0U;
      for (const char c : count) {
        if (c < '0' || c > '9') {
          result.ok = false;
          result.reason = "visual_demo_config_invalid";
          return result;
        }
        value = value * 10U + static_cast<std::uint32_t>(c - '0');
      }
      result.options.frames = value == 0U ? 1U : value;
      result.options.framesExplicit = true;
    } else if (arg == "--shader-root" && hasValue(i, argc)) {
      result.options.shaderRoot = argv[++i];
    } else if (arg == "--diagnostics-dir" && hasValue(i, argc)) {
      result.options.diagnosticsDir = argv[++i];
    } else if (arg == "--print-render-receipt") {
      result.options.printReceipt = true;
    } else if (arg == "--validation=off" || arg == "--validation=optional" ||
               arg == "--validation=required" || arg == "--sync-validation=off" ||
               arg == "--sync-validation=optional" || arg == "--sync-validation=required") {
      continue;
    } else {
      result.ok = false;
      result.reason = "visual_demo_config_invalid";
      return result;
    }
  }
  return result;
}

bool hasFrameLimit(const VisualOptions& options) {
  return !options.interactive || options.scriptedPlayableSmoke || options.framesExplicit;
}

std::uint32_t receiptFrameCount(const VisualOptions& options, std::uint32_t framesPresented) {
  return hasFrameLimit(options) ? options.frames : framesPresented;
}

iggy3d::RenderReceipt baseReceipt(std::string_view result, std::string_view reasonCode) {
  iggy3d::RenderReceipt receipt;
  iggy3d::appendReceiptField(receipt, "receipt_version", "1");
  iggy3d::appendReceiptField(receipt, "repo", "iggy3d");
  iggy3d::appendReceiptField(receipt, "app", "iggy3d_visual_demo");
  iggy3d::appendReceiptField(receipt, "packet_order", "3");
  iggy3d::appendReceiptField(receipt, "result", result);
  iggy3d::appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

std::filesystem::path resolveFixturePath(const std::filesystem::path& fixturePath,
                                         const iggy3d::PackageRuntimeLookup& lookup) {
  if (fixturePath.is_absolute()) {
    return fixturePath.lexically_normal();
  }
  auto firstComponent = fixturePath.begin();
  if (firstComponent != fixturePath.end() && firstComponent->string() == "fixtures") {
    return (lookup.packageRoot / fixturePath).lexically_normal();
  }
  return (lookup.resourceRoot / fixturePath).lexically_normal();
}

int printFailure(std::string_view reasonCode) {
  std::cout << iggy3d::formatRenderReceipt(baseReceipt("fail", reasonCode));
  return 1;
}

void appendReceiptFieldIfMissing(iggy3d::RenderReceipt& receipt,
                                 std::string_view key,
                                 std::string_view value) {
  if (!iggy3d::hasReceiptField(receipt, key)) {
    iggy3d::appendReceiptField(receipt, key, value);
  }
}

void appendReceiptFieldIfMissing(iggy3d::RenderReceipt& receipt,
                                 std::string_view key,
                                 const char* value) {
  appendReceiptFieldIfMissing(
      receipt, key,
      value == nullptr ? std::string_view{"unavailable"} : std::string_view{value});
}

void appendReceiptFieldIfMissing(iggy3d::RenderReceipt& receipt,
                                 std::string_view key,
                                 bool value) {
  if (!iggy3d::hasReceiptField(receipt, key)) {
    iggy3d::appendReceiptField(receipt, key, value);
  }
}

void appendReceiptFieldIfMissing(iggy3d::RenderReceipt& receipt,
                                 std::string_view key,
                                 std::uint64_t value) {
  if (!iggy3d::hasReceiptField(receipt, key)) {
    iggy3d::appendReceiptField(receipt, key, value);
  }
}

void appendVulkanBootFields(iggy3d::RenderReceipt& receipt,
                            iggy3d::RendererBackendKind backend,
                            bool surfaceCreated,
                            bool swapchainReady,
                            iggy3d::RendererLifecycleState lifecycle) {
  if (backend != iggy3d::RendererBackendKind::Vulkan) {
    return;
  }
#if defined(__APPLE__)
  appendReceiptFieldIfMissing(receipt, "platform", "macos");
  appendReceiptFieldIfMissing(receipt, "platform_lane", "moltenvk");
#else
  appendReceiptFieldIfMissing(receipt, "platform", "unknown");
  appendReceiptFieldIfMissing(receipt, "platform_lane", "native_vulkan");
#endif
#if defined(IGGY3D_HAS_VULKAN)
  appendReceiptFieldIfMissing(receipt, "vulkan_loader_found", true);
  appendReceiptFieldIfMissing(receipt, "portability_enumeration_available", true);
#else
  appendReceiptFieldIfMissing(receipt, "vulkan_loader_found", false);
  appendReceiptFieldIfMissing(receipt, "portability_enumeration_available", "unavailable");
#endif
#if defined(IGGY3D_VULKAN_ICD_FOUND_VALUE) && IGGY3D_VULKAN_ICD_FOUND_VALUE
  appendReceiptFieldIfMissing(receipt, "vulkan_icd_found", true);
#else
  appendReceiptFieldIfMissing(receipt, "vulkan_icd_found", false);
#endif
#if defined(__APPLE__) && defined(IGGY3D_HAS_VULKAN)
  appendReceiptFieldIfMissing(receipt, "moltenvk_available", surfaceCreated);
#else
  appendReceiptFieldIfMissing(receipt, "moltenvk_available", "unavailable");
#endif
  appendReceiptFieldIfMissing(receipt, "surface_created", surfaceCreated);
  appendReceiptFieldIfMissing(receipt, "swapchain_ready", swapchainReady);
  appendReceiptFieldIfMissing(receipt, "renderer_lifecycle",
                              iggy3d::rendererLifecycleStateName(lifecycle));
}

int printReceiptAndReturn(iggy3d::RenderReceipt receipt, int exitCode) {
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return exitCode;
}

void appendWindowReceiptFields(iggy3d::RenderReceipt& receipt,
                               const WindowReceiptFields& fields) {
  if (!fields.requested) {
    return;
  }
  iggy3d::appendReceiptField(receipt, "window_mode", "window");
  iggy3d::appendReceiptField(receipt, "window_shell", fields.sdlAvailable ? "sdl3" : "unavailable");
  iggy3d::appendReceiptField(receipt, "sdl3_available", fields.sdlAvailable);
  iggy3d::appendReceiptField(receipt, "window_created", fields.created);
  iggy3d::appendReceiptField(receipt, "window_opened", fields.opened);
  iggy3d::appendReceiptField(receipt, "window_closed", fields.closed);
  iggy3d::appendReceiptField(receipt, "window_width", static_cast<std::uint64_t>(fields.windowWidth));
  iggy3d::appendReceiptField(receipt, "window_height",
                             static_cast<std::uint64_t>(fields.windowHeight));
  iggy3d::appendReceiptField(receipt, "drawable_width",
                             static_cast<std::uint64_t>(fields.drawableWidth));
  iggy3d::appendReceiptField(receipt, "drawable_height",
                             static_cast<std::uint64_t>(fields.drawableHeight));
  iggy3d::appendReceiptField(receipt, "drawable", fields.drawable);
  iggy3d::appendReceiptField(receipt, "event_poll_count",
                             static_cast<std::uint64_t>(fields.eventPollCount));
  iggy3d::appendReceiptField(receipt, "quit_requested", fields.quitRequested);
}

#if !defined(IGGY3D_HAS_SDL3)
int printWindowSdlUnavailable() {
  WindowReceiptFields fields;
  fields.requested = true;
  fields.sdlAvailable = false;

  iggy3d::RenderReceipt receipt = baseReceipt("skip", "sdl3_unavailable");
  iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
  appendWindowReceiptFields(receipt, fields);
  iggy3d::appendReceiptField(receipt, "backend", "null");
  iggy3d::appendReceiptField(receipt, "frames", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "frames_presented", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "draw_count", static_cast<std::uint64_t>(0));
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return 77;
}
#endif

#if defined(IGGY3D_HAS_SDL3)
int printWindowUnavailable(WindowReceiptFields fields) {
  fields.requested = true;
  fields.sdlAvailable = true;

  iggy3d::RenderReceipt receipt = baseReceipt("fail", "visual_demo_window_unavailable");
  iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
  appendWindowReceiptFields(receipt, fields);
  iggy3d::appendReceiptField(receipt, "backend", "null");
  iggy3d::appendReceiptField(receipt, "frames", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "frames_presented", static_cast<std::uint64_t>(0));
  iggy3d::appendReceiptField(receipt, "draw_count", static_cast<std::uint64_t>(0));
  std::cout << iggy3d::formatRenderReceipt(receipt);
  return 1;
}
#endif

iggy3d::EntityId entityIdByName(const iggy3d::Session& session, std::string_view stableName) {
  const iggy3d::EntityState* entity = session.state().world.findByStableName(std::string(stableName));
  return entity == nullptr ? iggy3d::EntityId{} : entity->id;
}

const iggy3d::EntityState* playerEntity(const iggy3d::Session& session) {
  return session.state().world.findByStableName("player");
}

void recordTrainingDummyCombat(PlayableReceiptFields& fields, const iggy3d::Session& session) {
  const iggy3d::EntityState* dummy = session.state().world.findByStableName("training_dummy");
  if (dummy == nullptr) {
    fields.trainingDummyCombatTracked = false;
    fields.trainingDummyHitPoints = 0;
    fields.trainingDummyDefeated = false;
    return;
  }

  for (const iggy3d::CombatantState& combatant : session.state().combat.combatants) {
    if (combatant.entity == dummy->id) {
      fields.trainingDummyCombatTracked = true;
      fields.trainingDummyHitPoints =
          static_cast<std::uint64_t>(std::max(combatant.hitPoints, 0));
      fields.trainingDummyDefeated = combatant.defeated;
      return;
    }
  }

  fields.trainingDummyCombatTracked = false;
  fields.trainingDummyHitPoints = 0;
  fields.trainingDummyDefeated = false;
}

void recordKinematicMovementResult(PlayableReceiptFields& fields,
                                   const iggy3d::MovementResult& result) {
  fields.kinematicMovementAttempted = true;
  fields.kinematicMovementAccepted = iggy3d::movementSucceeded(result);
  fields.movementClamped = result.movementClamped;
  fields.movementSlid = result.movementSlid;
  fields.groundSnapApplied = result.groundSnapApplied;
  fields.movementReason = result.reasonCode;
  fields.movementPolicyBand =
      result.movementPolicyBand.empty() ? "unavailable" : result.movementPolicyBand;
  fields.movementDistanceMeters = debugFloat(result.distanceMeters);
  fields.movementHorizontalDistanceMeters = debugFloat(result.horizontalDistanceMeters);
  fields.movementVerticalDeltaMeters = debugFloat(result.verticalDeltaMeters);
  fields.movementGradePercent = debugFloat(result.gradePercent);
  fields.slopeTravelDirection = result.slopeTravelDirection;
  fields.movementStartX = debugFloat(result.start.x);
  fields.movementStartY = debugFloat(result.start.y);
  fields.movementStartZ = debugFloat(result.start.z);
  fields.movementDestinationX = debugFloat(result.destination.x);
  fields.movementDestinationY = debugFloat(result.destination.y);
  fields.movementDestinationZ = debugFloat(result.destination.z);
  fields.movementFinalX = debugFloat(result.finalPosition.x);
  fields.movementFinalY = debugFloat(result.finalPosition.y);
  fields.movementFinalZ = debugFloat(result.finalPosition.z);
  fields.groundSampleValid = !result.movementPolicyBand.empty();
  fields.groundWalkable = result.slopeUpDot > 0.0F && result.speedMultiplier > 0.0F &&
                          result.movementPolicyBand != "blocked";
  fields.carefulFooting = result.carefulFooting;
  fields.slopeAngleDegrees = debugFloat(result.slopeAngleDegrees);
  fields.slopeUpDot = debugFloat(result.slopeUpDot);
  fields.speedMultiplier = debugFloat(result.speedMultiplier);
  fields.staminaCostMultiplier = debugFloat(result.staminaCostMultiplier);
  fields.stepPenaltyMultiplier = debugFloat(result.stepPenaltyMultiplier);
  fields.hitSurfaceId = result.hitSurfaceId.empty() ? "none" : result.hitSurfaceId;
}

void recordTraversalResult(PlayableReceiptFields& fields,
                           const iggy3d::TraversalResult& result) {
  fields.traversalAttempted = true;
  fields.traversalAccepted = iggy3d::traversalApplied(result);
  fields.traversalMechanic = iggy3d::traversalMechanicName(result.mechanic);
  fields.traversalReason = result.reasonCode == nullptr ? "unavailable" : result.reasonCode;
  fields.traversalSlotId = result.slotId.empty() ? "none" : result.slotId;
  fields.traversalSlotKind = result.slotKind.empty() ? "none" : result.slotKind;
  fields.traversalSlotHeightBand =
      result.slotHeightBand.empty() ? "none" : result.slotHeightBand;
  fields.traversalTargetId = result.targetId.empty() ? "none" : result.targetId;
  fields.traversalLandingSurfaceId =
      result.landingSurfaceId.empty() ? "none" : result.landingSurfaceId;
  fields.traversalSlotLedgeHeightMeters = debugFloat(result.slotLedgeHeightMeters);
  fields.traversalSlotUsableWidthMeters = debugFloat(result.slotUsableWidthMeters);
  fields.traversalSlotStartRangeMeters = debugFloat(result.slotStartRangeMeters);
  fields.traversalSlotFacingDot = debugFloat(result.slotFacingDot);
  fields.traversalDistanceMeters = debugFloat(result.travel.distanceMeters);
  fields.traversalHorizontalDistanceMeters = debugFloat(result.travel.horizontalDistanceMeters);
  fields.traversalVerticalDeltaMeters = debugFloat(result.travel.verticalDeltaMeters);
  fields.traversalGradePercent = debugFloat(result.travel.gradePercent);
  fields.traversalDirection = iggy3d::movementTravelDirectionName(result.travel.direction);
  fields.traversalStartX = debugFloat(result.start.x);
  fields.traversalStartY = debugFloat(result.start.y);
  fields.traversalStartZ = debugFloat(result.start.z);
  fields.traversalFinalX = debugFloat(result.finalPosition.x);
  fields.traversalFinalY = debugFloat(result.finalPosition.y);
  fields.traversalFinalZ = debugFloat(result.finalPosition.z);
}

void recordTraversalIntentResult(PlayableReceiptFields& fields,
                                 const iggy3d::TraversalIntentResult& result) {
  fields.traversalIntentRequested = result.requested;
  fields.traversalIntentConsumed = result.consumedInput;
  fields.traversalIntentAccepted = result.accepted;
  fields.traversalIntentFallbackJumpAllowed = result.fallbackJumpAllowed;
  fields.traversalIntentTrigger = iggy3d::traversalIntentTriggerName(result.trigger);
  fields.traversalIntentStatus =
      result.reasonCode == nullptr ? "unavailable" : result.reasonCode;
  fields.traversalIntentSelectedMechanic =
      result.traversalAttempted ? iggy3d::traversalMechanicName(result.selectedMechanic)
                                : "none";
  fields.jumpInputObserved =
      fields.jumpInputObserved || result.trigger == iggy3d::TraversalIntentTrigger::Jump;
  if (result.traversalAttempted) {
    recordTraversalResult(fields, result.traversal);
  }
}

void recordPlayerMotorResult(PlayableReceiptFields& fields,
                             const iggy3d::PlayerMotorResult& result) {
  fields.playerMotorActive = true;
  fields.playerGrounded = result.grounded;
  fields.playerLanded = fields.playerLanded || result.landed;
  fields.jumpInputObserved = fields.jumpInputObserved || result.jumpRequested;
  fields.jumpAccepted = fields.jumpAccepted || result.jumpAccepted;
  fields.groundSnapApplied = fields.groundSnapApplied || result.groundSnapApplied;
  fields.airMoveIntentObserved = fields.airMoveIntentObserved || result.airMoveIntent;
  fields.airControlActive = fields.airControlActive || result.airControlActive;
  fields.airMovementClamped = fields.airMovementClamped || result.airMovementClamped;
  fields.airMovementSlid = fields.airMovementSlid || result.airMovementSlid;
  fields.dashInputObserved = fields.dashInputObserved || result.dashRequested;
  fields.dashAccepted = fields.dashAccepted || result.dashAccepted;
  fields.dashActive = fields.dashActive || result.dashActive;
  fields.dashMovementClamped = fields.dashMovementClamped || result.dashMovementClamped;
  fields.dashMovementSlid = fields.dashMovementSlid || result.dashMovementSlid;
  fields.groundSampleValid = result.groundSampleValid;
  fields.groundContact = result.groundContact;
  fields.groundWalkable = result.groundWalkable;
  fields.carefulFooting = result.carefulFooting;
  fields.groundDistanceMeters = debugFloat(result.groundDistanceMeters);
  fields.groundNormalX = debugFloat(result.groundNormal.x);
  fields.groundNormalY = debugFloat(result.groundNormal.y);
  fields.groundNormalZ = debugFloat(result.groundNormal.z);
  fields.slopeAngleDegrees = debugFloat(result.slopeAngleDegrees);
  fields.slopeUpDot = debugFloat(result.slopeUpDot);
  fields.speedMultiplier = debugFloat(result.speedMultiplier);
  fields.staminaCostMultiplier = debugFloat(result.staminaCostMultiplier);
  fields.stepPenaltyMultiplier = debugFloat(result.stepPenaltyMultiplier);
  if (!result.movementPolicyBand.empty()) {
    fields.movementPolicyBand = result.movementPolicyBand;
  }
  fields.groundSurfaceId =
      result.groundSurfaceId.empty() ? "none" : result.groundSurfaceId;
  fields.playerMotorPhase = iggy3d::playerMotorPhaseName(result.phase);
  fields.playerMotorReason = result.reasonCode == nullptr ? "unavailable" : result.reasonCode;
  fields.verticalVelocityState =
      std::string(velocityState(result.verticalVelocityMetersPerSecond));
  fields.horizontalVelocityState =
      std::string(velocityState(result.horizontalSpeedMetersPerSecond));
  fields.dashCooldownState =
      result.dashCooldownRemainingSeconds > 0.0F ? "cooling" : "ready";
  if (!result.hitSurfaceId.empty()) {
    fields.hitSurfaceId = result.hitSurfaceId;
  }
}

void recordRuntimeDebugSnapshot(PlayableReceiptFields& fields,
                                const iggy3d::RuntimeDebugSnapshot& snapshot) {
  fields.debugOverlayEnabled = true;
  fields.debugOverlayOpen = snapshot.enabled;
  fields.debugOverlayReason = snapshot.reasonCode == nullptr ? "unavailable" : snapshot.reasonCode;
  fields.debugPlayerPositionAvailable = snapshot.playerPositionAvailable;
  fields.debugSpeedAvailable = snapshot.speedAvailable;
  fields.debugDistanceAvailable = snapshot.hasSpawnDistance;
  if (snapshot.status != iggy3d::RuntimeDebugSnapshotStatus::Ok) {
    return;
  }

  fields.debugPlayerPhase = iggy3d::playerMotorPhaseName(snapshot.motorPhase);
  fields.debugMovementPolicyBand =
      snapshot.movementPolicyBand.empty() ? "not_attempted" : snapshot.movementPolicyBand;
  fields.debugGroundSurfaceId =
      snapshot.groundSurfaceId.empty() ? "none" : snapshot.groundSurfaceId;
  fields.debugHitSurfaceId = snapshot.hitSurfaceId.empty() ? "none" : snapshot.hitSurfaceId;
  fields.debugGroundSampleValid = snapshot.groundSampleValid;
  fields.debugGroundContact = snapshot.groundContact;
  fields.debugGroundWalkable = snapshot.groundWalkable;
  fields.debugCarefulFooting = snapshot.carefulFooting;
  fields.debugGroundDistanceMeters = debugFloat(snapshot.groundDistanceMeters);
  fields.debugGroundNormalX = debugFloat(snapshot.groundNormal.x);
  fields.debugGroundNormalY = debugFloat(snapshot.groundNormal.y);
  fields.debugGroundNormalZ = debugFloat(snapshot.groundNormal.z);
  fields.debugSlopeAngleDegrees = debugFloat(snapshot.slopeAngleDegrees);
  fields.debugSlopeUpDot = debugFloat(snapshot.slopeUpDot);
  fields.debugSpeedMultiplier = debugFloat(snapshot.speedMultiplier);
  fields.debugStaminaCostMultiplier = debugFloat(snapshot.staminaCostMultiplier);
  fields.debugStepPenaltyMultiplier = debugFloat(snapshot.stepPenaltyMultiplier);
  fields.debugMovementHorizontalDistanceMeters =
      debugFloat(snapshot.movementHorizontalDistanceMeters);
  fields.debugMovementVerticalDeltaMeters = debugFloat(snapshot.movementVerticalDeltaMeters);
  fields.debugMovementGradePercent = debugFloat(snapshot.movementGradePercent);
  fields.debugSlopeTravelDirection = snapshot.slopeTravelDirection;
  fields.debugMovedThisFrameMeters = debugFloat(snapshot.movedThisFrameMeters);
  fields.debugHorizontalSpeedMetersPerSecond =
      debugFloat(snapshot.horizontalSpeedMetersPerSecond);
  fields.debugVerticalSpeedMetersPerSecond =
      debugFloat(snapshot.verticalSpeedMetersPerSecond);
  fields.debugDistanceFromSpawnMeters = debugFloat(snapshot.distanceFromSpawnMeters);
  fields.debugPositionX = debugFloat(snapshot.position.x);
  fields.debugPositionY = debugFloat(snapshot.position.y);
  fields.debugPositionZ = debugFloat(snapshot.position.z);
  fields.debugYawRadians = debugFloat(snapshot.yawRadians);
  fields.debugPitchRadians = debugFloat(snapshot.pitchRadians);
  fields.debugTraversalPreviewAvailable = snapshot.traversalPreviewAvailable;
  fields.debugTraversalPreviewReady = snapshot.traversalPreviewReady;
  fields.debugTraversalPreviewCandidateAvailable =
      snapshot.traversalPreviewCandidateAvailable;
  fields.debugTraversalPreviewStatus = snapshot.traversalPreviewStatus;
  fields.debugTraversalPreviewHudCode = snapshot.traversalPreviewHudCode;
  fields.debugTraversalPreviewMechanic = snapshot.traversalPreviewMechanic;
  fields.debugTraversalPreviewSlotId = snapshot.traversalPreviewSlotId;
  fields.debugTraversalPreviewSlotKind = snapshot.traversalPreviewSlotKind;
  fields.debugTraversalPreviewSlotHeightBand = snapshot.traversalPreviewSlotHeightBand;
  fields.debugTraversalPreviewTargetId = snapshot.traversalPreviewTargetId;
  fields.debugTraversalPreviewLandingSurfaceId =
      snapshot.traversalPreviewLandingSurfaceId;
  fields.debugTraversalPreviewSlotLedgeHeightMeters =
      debugFloat(snapshot.traversalPreviewSlotLedgeHeightMeters);
  fields.debugTraversalPreviewSlotUsableWidthMeters =
      debugFloat(snapshot.traversalPreviewSlotUsableWidthMeters);
  fields.debugTraversalPreviewSlotStartRangeMeters =
      debugFloat(snapshot.traversalPreviewSlotStartRangeMeters);
  fields.debugTraversalPreviewSlotFacingDot =
      debugFloat(snapshot.traversalPreviewSlotFacingDot);
  fields.debugTraversalAvailable = snapshot.traversalDebugAvailable;
  fields.debugTraversalIntentRequested = snapshot.traversalIntentRequested;
  fields.debugTraversalIntentConsumed = snapshot.traversalIntentConsumed;
  fields.debugTraversalIntentAccepted = snapshot.traversalIntentAccepted;
  fields.debugTraversalIntentFallbackJumpAllowed =
      snapshot.traversalIntentFallbackJumpAllowed;
  fields.debugTraversalAttempted = snapshot.traversalAttempted;
  fields.debugTraversalAccepted = snapshot.traversalAccepted;
  fields.debugTraversalIntentTrigger = snapshot.traversalIntentTrigger;
  fields.debugTraversalIntentStatus = snapshot.traversalIntentStatus;
  fields.debugTraversalIntentSelectedMechanic =
      snapshot.traversalIntentSelectedMechanic;
  fields.debugTraversalMechanic = snapshot.traversalMechanic;
  fields.debugTraversalReason = snapshot.traversalReason;
  fields.debugTraversalSlotId = snapshot.traversalSlotId;
  fields.debugTraversalSlotKind = snapshot.traversalSlotKind;
  fields.debugTraversalSlotHeightBand = snapshot.traversalSlotHeightBand;
  fields.debugTraversalTargetId = snapshot.traversalTargetId;
  fields.debugTraversalLandingSurfaceId = snapshot.traversalLandingSurfaceId;
  fields.debugTraversalSlotLedgeHeightMeters =
      debugFloat(snapshot.traversalSlotLedgeHeightMeters);
  fields.debugTraversalSlotUsableWidthMeters =
      debugFloat(snapshot.traversalSlotUsableWidthMeters);
  fields.debugTraversalSlotStartRangeMeters =
      debugFloat(snapshot.traversalSlotStartRangeMeters);
  fields.debugTraversalSlotFacingDot = debugFloat(snapshot.traversalSlotFacingDot);
}

iggy3d::CommandRecord moveCommand(iggy3d::Vec3 point) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Move;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasPoint = true;
  command.payload.target.point = point;
  return command;
}

iggy3d::CommandRecord interactCommand(iggy3d::EntityId target) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Interact;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target;
  return command;
}

iggy3d::CommandRecord retryCommand(iggy3d::CommandId sourceCommandId) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Retry;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.retrySourceCommandId = sourceCommandId;
  return command;
}

iggy3d::CommandRecord attackCommand(iggy3d::EntityId target) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = {1};
  command.kind = iggy3d::CommandKind::Attack;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.target.hasEntity = true;
  command.payload.target.entity = target;
  command.payload.attackDamage = 3;
  return command;
}

iggy3d::CommandRecord controlCommand(iggy3d::CommandKind kind) {
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.kind = kind;
  command.source = iggy3d::CommandSource::LocalPlayer;
  return command;
}

std::string_view clockModeName(iggy3d::ClockMode mode) {
  switch (mode) {
    case iggy3d::ClockMode::Normal:
      return "normal";
    case iggy3d::ClockMode::Slow:
      return "slow";
    case iggy3d::ClockMode::Paused:
      return "paused";
  }
  return "unknown";
}

std::string_view saveLoadStatusName(iggy3d::SaveLoadStatus status) {
  switch (status) {
    case iggy3d::SaveLoadStatus::Ok:
      return "ok";
    case iggy3d::SaveLoadStatus::InvalidSourceState:
      return "invalid_source_state";
    case iggy3d::SaveLoadStatus::EncodeFailed:
      return "encode_failed";
    case iggy3d::SaveLoadStatus::DecodeFailed:
      return "decode_failed";
    case iggy3d::SaveLoadStatus::CompatibilityFailed:
      return "compatibility_failed";
    case iggy3d::SaveLoadStatus::InvalidEnvelope:
      return "invalid_envelope";
    case iggy3d::SaveLoadStatus::InvalidReference:
      return "invalid_reference";
    case iggy3d::SaveLoadStatus::InvalidCommandLog:
      return "invalid_command_log";
    case iggy3d::SaveLoadStatus::HashMismatch:
      return "hash_mismatch";
    case iggy3d::SaveLoadStatus::ReplacementFailed:
      return "replacement_failed";
  }
  return "unknown";
}

bool accepted(const iggy3d::SessionCommandResult& result) {
  return result.command.admission == iggy3d::CommandAdmissionStatus::Accepted;
}

bool submitAndDrain(iggy3d::Session& session, const iggy3d::CommandRecord& command) {
  const iggy3d::SessionCommandResult submitted = session.submitCommand(command);
  if (!accepted(submitted)) {
    return false;
  }
  if (submitted.executedImmediately) {
    return true;
  }
  const iggy3d::StatusResult run = session.runUntilIdle(8);
  return run.status == iggy3d::ResultStatus::Ok;
}

void recordResetResult(PlayableReceiptFields& fields,
                       const iggy3d::Session& session,
                       const iggy3d::SessionResetResult& reset) {
  fields.resetExecuted = reset.reset;
  fields.resetBaselineRestored =
      reset.reset && reset.baselineHash != 0U && reset.baselineHash == reset.currentHash &&
      session.state().currentStateHash == reset.currentHash;
  fields.resetCommandLogCleared =
      reset.reset && session.state().commandLog.size() == 0U &&
      session.state().commandLog.nextSequence() == 1U;
  fields.resetNextCommandId = session.state().nextCommandId;
}

void recordControlCommandResult(PlayableReceiptFields& fields,
                                iggy3d::CommandKind kind,
                                const iggy3d::Session& session,
                                const iggy3d::SessionCommandResult& result,
                                std::uint64_t tickBefore) {
  const bool ran = accepted(result) && result.executedImmediately;
  const std::uint64_t tickAfter = session.state().clock.tickIndex;
  switch (kind) {
    case iggy3d::CommandKind::ToggleTacticalMode:
      fields.tacticalToggleExecuted = fields.tacticalToggleExecuted || ran;
      fields.tacticalModeActive =
          fields.tacticalModeActive || session.state().clock.mode == iggy3d::ClockMode::Slow;
      break;
    case iggy3d::CommandKind::Pause:
      fields.pauseExecuted = fields.pauseExecuted || ran;
      fields.pauseTickBefore = tickBefore;
      fields.pauseTickAfter = tickAfter;
      fields.pauseTickFrozen =
          fields.pauseTickFrozen || (ran && tickAfter == tickBefore &&
                                     session.state().clock.mode == iggy3d::ClockMode::Paused);
      break;
    case iggy3d::CommandKind::StepTacticalTick:
      fields.stepExecuted = fields.stepExecuted || ran;
      fields.stepTickBefore = tickBefore;
      fields.stepTickAfter = tickAfter;
      fields.stepAdvancedOnce =
          fields.stepAdvancedOnce || (ran && tickAfter == tickBefore + 1U &&
                                      session.state().clock.mode == iggy3d::ClockMode::Paused);
      break;
    case iggy3d::CommandKind::Resume:
      fields.resumeExecuted = fields.resumeExecuted || ran;
      fields.resumeTickAfter = tickAfter;
      fields.tacticalModeActive =
          fields.tacticalModeActive || session.state().clock.mode == iggy3d::ClockMode::Slow;
      break;
    default:
      break;
  }
}

void submitAndRecordControlCommand(iggy3d::Session& session,
                                   iggy3d::CommandKind kind,
                                   PlayableReceiptFields& fields) {
  const std::uint64_t tickBefore = session.state().clock.tickIndex;
  const iggy3d::SessionCommandResult submitted = session.submitCommand(controlCommand(kind));
  recordControlCommandResult(fields, kind, session, submitted, tickBefore);
}

bool runSaveLoadRoundtrip(iggy3d::Session& session,
                          const iggy3d::SessionCreateRequest& create,
                          PlayableReceiptFields& fields) {
  fields.saveLoadRequested = true;
  fields.saveLoadRoundtripPassed = false;
  fields.saveLoadHashMatched = false;
  fields.saveLoadSessionReplaced = false;

  const iggy3d::StateHashValue sourceHash = session.stateHash();
  const iggy3d::SaveStateResult saved = iggy3d::saveSessionStateEncoded(session.state());
  fields.saveLoadSavedHash = saved.savedStateHash;
  fields.saveLoadEncodedBytes = saved.encodedSaveText.size();
  if (saved.status != iggy3d::SaveLoadStatus::Ok) {
    fields.saveLoadStatus = std::string("save_") + std::string(saveLoadStatusName(saved.status));
    return false;
  }

  iggy3d::Result<iggy3d::Session> fresh = iggy3d::Session::create(create);
  if (fresh.status != iggy3d::ResultStatus::Ok) {
    fields.saveLoadStatus = "session_create_failed";
    return false;
  }

  iggy3d::Session loaded = std::move(fresh.value);
  const iggy3d::SaveCompatibilityRequest compatibility{
      saved.envelope, session.state().identity.packageId, session.state().identity.scenarioId};
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saved.encodedSaveText, compatibility);
  fields.saveLoadLoadedHash = load.loadedHash;
  if (load.status != iggy3d::SaveLoadStatus::Ok) {
    fields.saveLoadStatus = std::string("load_") + std::string(saveLoadStatusName(load.status));
    return false;
  }

  fields.saveLoadHashMatched = load.loadedHash == sourceHash && loaded.stateHash() == sourceHash;
  if (!fields.saveLoadHashMatched) {
    fields.saveLoadStatus = "hash_mismatch";
    return false;
  }

  session = std::move(loaded);
  fields.saveLoadRoundtripPassed = true;
  fields.saveLoadSessionReplaced = true;
  fields.saveLoadReplayStable = true;
  fields.saveLoadStatus = "ok";
  return true;
}

bool objectiveComplete(const iggy3d::Session& session) {
  for (const iggy3d::ObjectiveRecord& objective : session.state().objectives.objectives) {
    if (objective.objectiveId == "collect_gold_key" &&
        objective.status == iggy3d::ObjectiveStatus::Complete) {
      return true;
    }
  }
  return false;
}

iggy3d::Vec3 roomOriginOffsetFromPlayerSpawn(const iggy3d::RoomAsset& room) {
  for (const iggy3d::RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.id == "player_spawn") {
      return anchor.positionMeters;
    }
  }
  return {};
}

iggy3d::Vec3 negated(iggy3d::Vec3 value) {
  return {-value.x, -value.y, -value.z};
}

void attachRoomProjection(const iggy3d::PackageLoadResult& package,
                          const iggy3d::RoomAsset* activeRoom,
                          iggy3d::SceneProjectionResult& scene) {
  if (activeRoom == nullptr) {
    return;
  }
  const iggy3d::RoomAsset& room = *activeRoom;
  scene.room.loaded = true;
  scene.room.assetId = room.id;
  scene.room.version = room.version;
  scene.room.sourceToml = room.sourceFile;
  scene.room.sourceSubset = room.sourceSubset;
  scene.room.staticMeshCount = room.staticMeshes.size();
  scene.room.materialCount = package.materials.materials.size();
  scene.room.anchorCount = room.anchors.size();
  scene.room.meshes.clear();
  scene.room.meshes.reserve(room.staticMeshes.size());
  const iggy3d::Vec3 roomOriginOffset = roomOriginOffsetFromPlayerSpawn(room);
  for (const iggy3d::RoomAnchorAsset& anchor : room.anchors) {
    scene.room.keyAnchorVisible =
        scene.room.keyAnchorVisible || anchor.runtimeStableName == "gold_key";
    scene.room.dummyAnchorVisible =
        scene.room.dummyAnchorVisible || anchor.runtimeStableName == "training_dummy";
  }
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    iggy3d::SceneRoomMeshItem item;
    item.id = mesh.id;
    item.role = mesh.role;
    item.position = mesh.positionMeters - roomOriginOffset;
    item.size = mesh.sizeMeters;
    scene.room.floorVisible = scene.room.floorVisible || mesh.role == "floor";
    scene.room.wallVisible = scene.room.wallVisible || mesh.role == "wall";
    scene.room.openingVisible = scene.room.openingVisible || mesh.role == "opening";
    scene.room.propVisible = scene.room.propVisible || mesh.role == "prop";
    scene.room.meshes.push_back(std::move(item));
  }
}

void attachEditorGhostProjection(const EditorModeState& editor,
                                 iggy3d::SceneProjectionResult& scene) {
  if (!editor.enabled || !editor.open || !editor.ghostVisible || !scene.room.loaded) {
    return;
  }
  iggy3d::SceneRoomMeshItem ghost;
  ghost.id = "editor_ghost";
  ghost.role = editor.ghostRole;
  ghost.position = editor.ghostPositionMeters;
  ghost.size = editor.ghostSizeMeters;
  scene.room.meshes.push_back(std::move(ghost));
}

float snapToEditorGrid(float value) {
  constexpr float kGridStepsPerMeter = 2.0F;
  return std::round(value * kGridStepsPerMeter) / kGridStepsPerMeter;
}

iggy3d::Vec3 snappedEditorCursor(iggy3d::Vec3 value) {
  return {snapToEditorGrid(value.x), snapToEditorGrid(value.y), snapToEditorGrid(value.z)};
}

iggy3d::Vec3 defaultEditorCursorWorld(const iggy3d::Session& session, float yaw) {
  const iggy3d::EntityState* player = playerEntity(session);
  const iggy3d::Vec3 playerPosition = player == nullptr ? iggy3d::Vec3{} : player->transform.position;
  const iggy3d::Vec3 forward{std::sin(yaw), 0.0F, -std::cos(yaw)};
  iggy3d::Vec3 cursor = playerPosition + forward * 2.0F;
  cursor.y = 0.0F;
  return snappedEditorCursor(cursor);
}

iggy3d::Vec3 editorCameraEyeWorld(const iggy3d::Session& session, bool crouched) {
  const iggy3d::EntityState* player = playerEntity(session);
  const iggy3d::Vec3 playerPosition =
      player == nullptr ? iggy3d::Vec3{} : player->transform.position;
  return playerPosition +
         iggy3d::Vec3{0.0F, crouched ? kCrouchedEyeHeightMeters : kStandingEyeHeightMeters, 0.0F};
}

iggy3d::Vec3 editorLocalFromWorld(iggy3d::Vec3 world, iggy3d::Vec3 roomWorldOffset) {
  return world - roomWorldOffset;
}

std::string editorPrimitiveIdForSurface(const EditorModeState& editor,
                                        const std::string& surfaceId) {
  std::string best = "none";
  const auto choose = [&](const std::string& candidate) {
    if (surfaceId == candidate ||
        (surfaceId.size() > candidate.size() && surfaceId.starts_with(candidate) &&
         surfaceId[candidate.size()] == '_')) {
      if (best == "none" || candidate.size() > best.size()) {
        best = candidate;
      }
    }
  };
  for (const iggy3d::EditableRoomFloor& floor : editor.session.document().floors) {
    choose(floor.id);
  }
  for (const iggy3d::EditableRoomWall& wall : editor.session.document().walls) {
    choose(wall.id);
  }
  return best;
}

bool selectEditorPrimitiveUnderProbe(EditorModeState& editor) {
  if (!editor.probeHit) {
    editor.selectionSource = "none";
    return false;
  }
  const std::string primitiveId = editorPrimitiveIdForSurface(editor, editor.probeSurfaceId);
  if (primitiveId == "none") {
    editor.selectionSource = "probe_miss";
    return false;
  }
  editor.selectedId = primitiveId;
  editor.selectionSource = "reticle";
  editor.lastCommand = "select";
  editor.lastStatus = "room_edit_selected";
  return true;
}

iggy3d::Vec3 placementCursorFromProbe(const iggy3d::CollisionQueryResult& hit,
                                      const iggy3d::SpatialSurfaceSet& collisionSurfaces) {
  iggy3d::Vec3 cursor = hit.pointMeters;
  if (hit.role != iggy3d::CollisionSurfaceRole::Walkable || hit.normal.y < 0.5F) {
    (void)collisionSurfaces;
    cursor.y = 0.0F;
  }
  return snappedEditorCursor(cursor);
}

void clearEditorProbe(EditorModeState& editor, std::string status) {
  editor.probeAvailable = false;
  editor.probeHit = false;
  editor.placementValid = false;
  editor.probeStatus = std::move(status);
  editor.probeSurfaceId = "none";
  editor.probeSurfaceRole = "none";
  editor.probeDistanceMeters = 0.0F;
  editor.probePointMeters = {};
  editor.probeNormal = {0.0F, 1.0F, 0.0F};
  editor.ghostVisible = false;
  editor.ghostRole = "none";
  editor.ghostPositionMeters = {};
  editor.ghostSizeMeters = {};
}

bool editorToolUsesPlacement(EditorTool tool) {
  return tool == EditorTool::PlaceFloor || tool == EditorTool::PlaceWall;
}

bool editorToolUsesSelection(EditorTool tool) {
  return tool == EditorTool::Select || tool == EditorTool::Semantics || tool == EditorTool::Delete;
}

std::string editorGhostRoleFor(const EditorModeState& editor) {
  if (!editor.placementValid) {
    return "editor_ghost_invalid";
  }
  if (editorToolUsesSelection(editor.tool)) {
    return "editor_ghost_select";
  }
  return "editor_ghost_valid";
}

void updateEditorGhost(EditorModeState& editor, float yaw) {
  editor.ghostVisible = editor.open && (editor.placementValid || editor.probeAvailable);
  if (!editor.ghostVisible) {
    editor.ghostRole = "none";
    editor.ghostPositionMeters = {};
    editor.ghostSizeMeters = {};
    return;
  }

  editor.ghostRole = editorGhostRoleFor(editor);
  if (editor.tool == EditorTool::PlaceFloor) {
    editor.ghostPositionMeters =
        iggy3d::Vec3{editor.cursorWorldMeters.x, editor.cursorWorldMeters.y - 0.05F,
                     editor.cursorWorldMeters.z};
    editor.ghostSizeMeters = {2.0F, 0.10F, 2.0F};
    return;
  }
  if (editor.tool == EditorTool::PlaceWall) {
    const bool alongX = std::fabs(std::cos(yaw)) >= std::fabs(std::sin(yaw));
    editor.ghostPositionMeters =
        iggy3d::Vec3{editor.cursorWorldMeters.x, editor.cursorWorldMeters.y + 0.75F,
                     editor.cursorWorldMeters.z};
    editor.ghostSizeMeters = alongX ? iggy3d::Vec3{2.0F, 1.5F, 0.20F}
                                    : iggy3d::Vec3{0.20F, 1.5F, 2.0F};
    return;
  }
  editor.ghostPositionMeters = editor.probeHit ? editor.probePointMeters : editor.cursorWorldMeters;
  editor.ghostSizeMeters = {0.35F, 0.35F, 0.35F};
}

void updateEditorProbe(EditorModeState& editor,
                       const iggy3d::Session& session,
                       const iggy3d::SpatialSurfaceSet& collisionSurfaces,
                       float yaw,
                       float pitch,
                       bool crouched,
                       bool manualCursor) {
  if (!editor.enabled || !editor.open) {
    clearEditorProbe(editor, "editor_probe_disabled");
    return;
  }
  if (manualCursor) {
    editor.cursorWorldMeters = snappedEditorCursor(editor.cursorWorldMeters);
    editor.probeAvailable = true;
    editor.probeHit = false;
    editor.placementValid = true;
    editor.probeStatus = "manual_cursor";
    editor.probeSurfaceId = "none";
    editor.probeSurfaceRole = "manual";
    editor.probeDistanceMeters = 0.0F;
    editor.probePointMeters = editor.cursorWorldMeters;
    editor.probeNormal = {0.0F, 1.0F, 0.0F};
    updateEditorGhost(editor, yaw);
    return;
  }
  if (collisionSurfaces.empty()) {
    editor.cursorWorldMeters = defaultEditorCursorWorld(session, yaw);
    clearEditorProbe(editor, "editor_probe_empty_surface_set");
    updateEditorGhost(editor, yaw);
    return;
  }
  const iggy3d::Vec3 eye = editorCameraEyeWorld(session, crouched);
  const float cosPitch = std::cos(pitch);
  const iggy3d::Vec3 forward{std::sin(yaw) * cosPitch, std::sin(pitch),
                             -std::cos(yaw) * cosPitch};
  const iggy3d::Vec3 rayEnd = eye + forward * 24.0F;
  const iggy3d::CollisionQueryResult hit =
      iggy3d::querySegment(collisionSurfaces, eye, rayEnd, iggy3d::CollisionQueryKind::All);
  editor.probeAvailable = true;
  editor.probeHit = hit.status == iggy3d::CollisionQueryStatus::Hit;
  editor.probeStatus = hit.reasonCode;
  if (editor.probeHit) {
    editor.probeSurfaceId = hit.surfaceId.empty() ? "none" : hit.surfaceId;
    editor.probeSurfaceRole = std::string(iggy3d::collisionSurfaceRoleName(hit.role));
    editor.probeDistanceMeters = hit.distanceMeters;
    editor.probePointMeters = hit.pointMeters;
    editor.probeNormal = hit.normal;
    editor.cursorWorldMeters = placementCursorFromProbe(hit, collisionSurfaces);
    editor.placementValid = editorToolUsesPlacement(editor.tool) || editorToolUsesSelection(editor.tool);
  } else {
    editor.probeSurfaceId = "none";
    editor.probeSurfaceRole = "none";
    editor.probeDistanceMeters = 0.0F;
    editor.probePointMeters = {};
    editor.probeNormal = {0.0F, 1.0F, 0.0F};
    editor.cursorWorldMeters = defaultEditorCursorWorld(session, yaw);
    editor.placementValid = false;
  }
  updateEditorGhost(editor, yaw);
}

iggy3d::EditableRoomSemantics floorSemanticsForPreset(EditorPreset preset) {
  switch (preset) {
    case EditorPreset::Floor:
    case EditorPreset::SolidWall:
    case EditorPreset::ClamberWall:
    case EditorPreset::ProjectileWall:
      return iggy3d::defaultFloorSemantics("debug_floor");
  }
  return iggy3d::defaultFloorSemantics("debug_floor");
}

iggy3d::EditableRoomSemantics wallSemanticsForPreset(EditorPreset preset) {
  switch (preset) {
    case EditorPreset::Floor:
    case EditorPreset::SolidWall:
      return iggy3d::defaultWallSemantics("debug_wall");
    case EditorPreset::ClamberWall: {
      iggy3d::EditableRoomSemantics semantics =
          iggy3d::defaultWallSemantics("debug_wall");
      semantics.traversalTags = {"clamber"};
      return semantics;
    }
    case EditorPreset::ProjectileWall: {
      iggy3d::EditableRoomSemantics semantics;
      semantics.materialId = "debug_wall";
      semantics.blocksProjectile = true;
      return semantics;
    }
  }
  return iggy3d::defaultWallSemantics("debug_wall");
}

std::string makeEditorFloorId(EditorModeState& editor) {
  return "edit_floor_" + std::to_string(editor.nextFloorIndex++);
}

std::string makeEditorWallId(EditorModeState& editor) {
  return "edit_wall_" + std::to_string(editor.nextWallIndex++);
}

iggy3d::EditableRoomFloor makeEditorFloor(EditorModeState& editor,
                                          iggy3d::Vec3 roomLocalCursor) {
  iggy3d::EditableRoomFloor floor;
  floor.id = makeEditorFloorId(editor);
  floor.centerMeters = {roomLocalCursor.x, roomLocalCursor.y - 0.05F, roomLocalCursor.z};
  floor.sizeMeters = {2.0F, 0.10F, 2.0F};
  floor.semantics = floorSemanticsForPreset(editor.preset);
  return floor;
}

iggy3d::EditableRoomWall makeEditorWall(EditorModeState& editor,
                                        iggy3d::Vec3 roomLocalCursor,
                                        float yaw) {
  const iggy3d::Vec3 right{std::cos(yaw), 0.0F, std::sin(yaw)};
  const bool alongX = std::fabs(right.x) >= std::fabs(right.z);
  iggy3d::EditableRoomWall wall;
  wall.id = makeEditorWallId(editor);
  wall.bottomY = roomLocalCursor.y;
  wall.heightMeters = 1.5F;
  wall.thicknessMeters = 0.20F;
  if (alongX) {
    wall.startMeters = {roomLocalCursor.x - 1.0F, roomLocalCursor.y, roomLocalCursor.z};
    wall.endMeters = {roomLocalCursor.x + 1.0F, roomLocalCursor.y, roomLocalCursor.z};
  } else {
    wall.startMeters = {roomLocalCursor.x, roomLocalCursor.y, roomLocalCursor.z - 1.0F};
    wall.endMeters = {roomLocalCursor.x, roomLocalCursor.y, roomLocalCursor.z + 1.0F};
  }
  wall.semantics = wallSemanticsForPreset(editor.preset);
  return wall;
}

void selectNextEditorPrimitive(EditorModeState& editor) {
  const iggy3d::EditableRoomDocument& document = editor.session.document();
  std::vector<std::string> ids;
  ids.reserve(document.floors.size() + document.walls.size());
  for (const iggy3d::EditableRoomFloor& floor : document.floors) {
    ids.push_back(floor.id);
  }
  for (const iggy3d::EditableRoomWall& wall : document.walls) {
    ids.push_back(wall.id);
  }
  if (ids.empty()) {
    editor.selectedId = "none";
    editor.lastCommand = "select";
    editor.lastStatus = "room_edit_missing_primitive";
    return;
  }
  const auto found = std::find(ids.begin(), ids.end(), editor.selectedId);
  if (found == ids.end() || ++std::vector<std::string>::const_iterator(found) == ids.end()) {
    editor.selectedId = ids.front();
  } else {
    editor.selectedId = *found;
  }
  editor.lastCommand = "select";
  editor.lastStatus = "room_edit_selected";
}

bool editorSelectionExists(const EditorModeState& editor) {
  const iggy3d::EditableRoomDocument& document = editor.session.document();
  return iggy3d::findEditableFloor(document, editor.selectedId) != nullptr ||
         iggy3d::findEditableWall(document, editor.selectedId) != nullptr;
}

void recordEditorCommandResult(EditorModeState& editor,
                               const char* commandName,
                               const iggy3d::RoomEditResult& result) {
  editor.lastCommand = commandName;
  editor.lastStatus = result.reasonCode == nullptr ? "room_edit_unknown" : result.reasonCode;
  if (!result.primitiveId.empty()) {
    editor.selectedId = result.primitiveId;
  }
}

void updateEditorReceiptFields(PlayableReceiptFields& fields,
                               const EditorModeState& editor) {
  fields.editorEnabled = editor.enabled;
  fields.editorOpen = editor.open;
  fields.editorTool = std::string(editorToolName(editor.tool));
  fields.editorPreset = std::string(editorPresetName(editor.preset));
  fields.editorLastCommand = editor.lastCommand;
  fields.editorLastStatus = editor.lastStatus;
  fields.editorSelectedId = editor.selectedId;
  fields.editorCursorX = debugFloat(editor.cursorWorldMeters.x);
  fields.editorCursorY = debugFloat(editor.cursorWorldMeters.y);
  fields.editorCursorZ = debugFloat(editor.cursorWorldMeters.z);
  fields.editorProbeAvailable = editor.probeAvailable;
  fields.editorProbeHit = editor.probeHit;
  fields.editorPlacementValid = editor.placementValid;
  fields.editorProbeStatus = editor.probeStatus;
  fields.editorProbeSurfaceId = editor.probeSurfaceId;
  fields.editorProbeSurfaceRole = editor.probeSurfaceRole;
  fields.editorProbeDistanceMeters = debugFloat(editor.probeDistanceMeters);
  fields.editorProbeX = debugFloat(editor.probePointMeters.x);
  fields.editorProbeY = debugFloat(editor.probePointMeters.y);
  fields.editorProbeZ = debugFloat(editor.probePointMeters.z);
  fields.editorProbeNormalX = debugFloat(editor.probeNormal.x);
  fields.editorProbeNormalY = debugFloat(editor.probeNormal.y);
  fields.editorProbeNormalZ = debugFloat(editor.probeNormal.z);
  fields.editorGhostVisible = editor.ghostVisible;
  fields.editorGhostRole = editor.ghostRole;
  fields.editorGhostX = debugFloat(editor.ghostPositionMeters.x);
  fields.editorGhostY = debugFloat(editor.ghostPositionMeters.y);
  fields.editorGhostZ = debugFloat(editor.ghostPositionMeters.z);
  fields.editorGhostSizeX = debugFloat(editor.ghostSizeMeters.x);
  fields.editorGhostSizeY = debugFloat(editor.ghostSizeMeters.y);
  fields.editorGhostSizeZ = debugFloat(editor.ghostSizeMeters.z);
  fields.editorSelectionSource = editor.selectionSource;
  fields.editorFloorCount = editor.session.document().floors.size();
  fields.editorWallCount = editor.session.document().walls.size();
  fields.editorRuntimeStaticMeshCount = editor.runtimeStaticMeshCount;
  fields.editorRuntimeSurfaceCount = editor.runtimeSurfaceCount;
  fields.editorRuntimeTraversalSlotCount = editor.runtimeTraversalSlotCount;
  fields.editorApplyCount = editor.applyCount;
  fields.editorDeleteCount = editor.deleteCount;
  fields.editorUndoCount = editor.undoCount;
  fields.editorRedoCount = editor.redoCount;
  fields.editorBakeOk = editor.bakeOk;
  fields.editorBakeReason = editor.bakeReason;
}

void rebuildEditorRuntimeRoom(const iggy3d::RoomAsset& baseRoom,
                              EditorModeState& editor,
                              iggy3d::RoomAsset& runtimeRoom,
                              iggy3d::SpatialSurfaceSet& collisionSurfaces,
                              iggy3d::Vec3 roomWorldOffset) {
  runtimeRoom = baseRoom;
  const iggy3d::RoomBakeResult bake =
      iggy3d::bakeEditableRoomDocument(editor.session.document());
  editor.bakeOk = bake.ok;
  editor.bakeReason = bake.reasonCode == nullptr ? "room_bake_failed" : bake.reasonCode;
  if (bake.ok) {
    runtimeRoom.staticMeshes.insert(runtimeRoom.staticMeshes.end(),
                                    bake.room.staticMeshes.begin(),
                                    bake.room.staticMeshes.end());
    runtimeRoom.spatialSurfaces.insert(runtimeRoom.spatialSurfaces.end(),
                                       bake.room.spatialSurfaces.begin(),
                                       bake.room.spatialSurfaces.end());
  }
  collisionSurfaces = iggy3d::buildSpatialSurfaceSet(runtimeRoom, roomWorldOffset);
  editor.runtimeStaticMeshCount = runtimeRoom.staticMeshes.size();
  editor.runtimeSurfaceCount = runtimeRoom.spatialSurfaces.size();
  editor.runtimeTraversalSlotCount =
      iggy3d::buildMovementTraversalSlotRegistry(runtimeRoom, roomWorldOffset).slots.size();
}

void executeEditorApply(EditorModeState& editor,
                        const iggy3d::RoomAsset& baseRoom,
                        iggy3d::RoomAsset& runtimeRoom,
                        iggy3d::SpatialSurfaceSet& collisionSurfaces,
                        iggy3d::Vec3 roomWorldOffset,
                        float yaw) {
  const iggy3d::Vec3 localCursor =
      editorLocalFromWorld(editor.cursorWorldMeters, roomWorldOffset);
  switch (editor.tool) {
    case EditorTool::Select:
      if (!selectEditorPrimitiveUnderProbe(editor)) {
        selectNextEditorPrimitive(editor);
        if (editor.selectedId != "none") {
          editor.selectionSource = "cycle";
        }
      }
      break;
    case EditorTool::PlaceFloor: {
      if (!editor.placementValid) {
        editor.lastCommand = "add_floor";
        editor.lastStatus = "room_edit_placement_invalid";
        break;
      }
      iggy3d::RoomEditResult result =
          editor.session.submit(iggy3d::addFloorCommand(makeEditorFloor(editor, localCursor)));
      recordEditorCommandResult(editor, "add_floor", result);
      if (result.status == iggy3d::RoomEditStatus::Applied) {
        ++editor.applyCount;
      }
      break;
    }
    case EditorTool::PlaceWall: {
      if (!editor.placementValid) {
        editor.lastCommand = "add_wall";
        editor.lastStatus = "room_edit_placement_invalid";
        break;
      }
      iggy3d::RoomEditResult result = editor.session.submit(
          iggy3d::addWallCommand(makeEditorWall(editor, localCursor, yaw)));
      recordEditorCommandResult(editor, "add_wall", result);
      if (result.status == iggy3d::RoomEditStatus::Applied) {
        ++editor.applyCount;
      }
      break;
    }
    case EditorTool::Semantics: {
      selectEditorPrimitiveUnderProbe(editor);
      const iggy3d::EditableRoomDocument& document = editor.session.document();
      iggy3d::RoomEditResult result;
      if (iggy3d::findEditableFloor(document, editor.selectedId) != nullptr) {
        result = editor.session.submit(iggy3d::setFloorSemanticsCommand(
            editor.selectedId, floorSemanticsForPreset(editor.preset)));
        recordEditorCommandResult(editor, "set_floor_semantics", result);
      } else {
        result = editor.session.submit(iggy3d::setWallSemanticsCommand(
            editor.selectedId, wallSemanticsForPreset(editor.preset)));
        recordEditorCommandResult(editor, "set_wall_semantics", result);
      }
      if (result.status == iggy3d::RoomEditStatus::Applied) {
        ++editor.applyCount;
      }
      break;
    }
    case EditorTool::Delete: {
      selectEditorPrimitiveUnderProbe(editor);
      iggy3d::RoomEditResult result;
      const iggy3d::EditableRoomDocument& document = editor.session.document();
      if (iggy3d::findEditableFloor(document, editor.selectedId) != nullptr) {
        result = editor.session.submit(iggy3d::deleteFloorCommand(editor.selectedId));
      } else {
        result = editor.session.submit(iggy3d::deleteWallCommand(editor.selectedId));
      }
      recordEditorCommandResult(editor, "delete", result);
      if (result.status == iggy3d::RoomEditStatus::Applied) {
        ++editor.deleteCount;
        editor.selectedId = "none";
      }
      break;
    }
  }
  rebuildEditorRuntimeRoom(baseRoom, editor, runtimeRoom, collisionSurfaces, roomWorldOffset);
}

void executeEditorDelete(EditorModeState& editor,
                         const iggy3d::RoomAsset& baseRoom,
                         iggy3d::RoomAsset& runtimeRoom,
                         iggy3d::SpatialSurfaceSet& collisionSurfaces,
                         iggy3d::Vec3 roomWorldOffset) {
  const EditorTool previousTool = editor.tool;
  editor.tool = EditorTool::Delete;
  executeEditorApply(editor, baseRoom, runtimeRoom, collisionSurfaces, roomWorldOffset, 0.0F);
  editor.tool = previousTool;
}

void executeEditorUndo(EditorModeState& editor,
                       const iggy3d::RoomAsset& baseRoom,
                       iggy3d::RoomAsset& runtimeRoom,
                       iggy3d::SpatialSurfaceSet& collisionSurfaces,
                       iggy3d::Vec3 roomWorldOffset) {
  const iggy3d::RoomEditResult result = editor.session.undo();
  recordEditorCommandResult(editor, "undo", result);
  if (result.status == iggy3d::RoomEditStatus::UndoApplied) {
    ++editor.undoCount;
    if (!editorSelectionExists(editor)) {
      editor.selectedId = "none";
    }
  }
  rebuildEditorRuntimeRoom(baseRoom, editor, runtimeRoom, collisionSurfaces, roomWorldOffset);
}

void executeEditorRedo(EditorModeState& editor,
                       const iggy3d::RoomAsset& baseRoom,
                       iggy3d::RoomAsset& runtimeRoom,
                       iggy3d::SpatialSurfaceSet& collisionSurfaces,
                       iggy3d::Vec3 roomWorldOffset) {
  const iggy3d::RoomEditResult result = editor.session.redo();
  recordEditorCommandResult(editor, "redo", result);
  if (result.status == iggy3d::RoomEditStatus::RedoApplied) {
    ++editor.redoCount;
  }
  rebuildEditorRuntimeRoom(baseRoom, editor, runtimeRoom, collisionSurfaces, roomWorldOffset);
}

void appendEditorDebugHudLines(iggy3d::DebugProjectionResult& debug,
                               const EditorModeState& editor) {
  if (!editor.enabled || !editor.open) {
    return;
  }
  debug.runtimeDebugHudLines.push_back("EDIT " + std::string(editorToolName(editor.tool)) +
                                       " " + std::string(editorPresetName(editor.preset)));
  debug.runtimeDebugHudLines.push_back("CUR " + debugFloat(editor.cursorWorldMeters.x) + " " +
                                       debugFloat(editor.cursorWorldMeters.y) + " " +
                                       debugFloat(editor.cursorWorldMeters.z));
  debug.runtimeDebugHudLines.push_back("PROBE " + editor.probeStatus + " " +
                                       editor.probeSurfaceId + " " +
                                       editor.probeSurfaceRole);
  debug.runtimeDebugHudLines.push_back("GHOST " + editor.ghostRole + " valid=" +
                                       std::string(editor.placementValid ? "true" : "false"));
  debug.runtimeDebugHudLines.push_back("SEL " + editor.selectedId);
  debug.runtimeDebugHudLines.push_back("LAST " + editor.lastCommand + " " +
                                       editor.lastStatus);
  debug.runtimeDebugHudLines.push_back("ROOM floors=" +
                                       std::to_string(editor.session.document().floors.size()) +
                                       " walls=" +
                                       std::to_string(editor.session.document().walls.size()) +
                                       " surfaces=" +
                                       std::to_string(editor.runtimeSurfaceCount) +
                                       " slots=" +
                                       std::to_string(editor.runtimeTraversalSlotCount));
}

iggy3d::Vec3 cross(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return {lhs.y * rhs.z - lhs.z * rhs.y,
          lhs.z * rhs.x - lhs.x * rhs.z,
          lhs.x * rhs.y - lhs.y * rhs.x};
}

iggy3d::Vec3 normalizedOr(iggy3d::Vec3 value, iggy3d::Vec3 fallback) {
  const float magnitudeSquared = iggy3d::lengthSquared(value);
  if (magnitudeSquared <= 0.000001F) {
    return fallback;
  }
  return value / std::sqrt(magnitudeSquared);
}

iggy3d::Vec3 lookForwardVector(float yaw, float pitch) {
  const float cosPitch = std::cos(pitch);
  return normalizedOr({std::sin(yaw) * cosPitch, std::sin(pitch),
                       -std::cos(yaw) * cosPitch},
                      {0.0F, 0.0F, -1.0F});
}

std::string abilityCommandReason(const iggy3d::SessionCommandResult& submitted) {
  if (accepted(submitted)) {
    return "ability_command_accepted";
  }
  switch (submitted.command.rejection) {
    case iggy3d::CommandRejectionReason::AbilitySlotBusy:
      return "ability_slot_busy";
    case iggy3d::CommandRejectionReason::AbilityOnCooldown:
      return "ability_on_cooldown";
    case iggy3d::CommandRejectionReason::AbilityInsufficientResource:
      return "ability_insufficient_resource";
    case iggy3d::CommandRejectionReason::InvalidTargetPoint:
      return "ability_invalid_direction";
    case iggy3d::CommandRejectionReason::InvalidActor:
      return "ability_invalid_caster";
    default:
      break;
  }
  return "ability_command_rejected";
}

void recordAbilityPolicy(PlayableReceiptFields& fields, const iggy3d::Session& session) {
  const iggy3d::AbilityDefinition* definition =
      iggy3d::findAbilityDefinition(iggy3d::AbilityId::ArcaneBolt);
  if (definition == nullptr) {
    fields.abilityCooldownState = "unavailable";
    return;
  }

  const iggy3d::EntityState* player = playerEntity(session);
  std::uint32_t resourceRemaining = definition->maxResource;
  iggy3d::CommandTick nextRechargeTick = 0;
  iggy3d::CommandTick readyTick = 0;
  if (player != nullptr) {
    for (const iggy3d::AbilityActorState& actor : session.state().abilities.actors) {
      if (actor.actor == player->id) {
        resourceRemaining = actor.arcaneFocus;
        nextRechargeTick = actor.arcaneFocusNextRechargeTick;
        readyTick = actor.arcaneBoltReadyTick;
        break;
      }
    }
  }

  fields.abilityResourceCost = definition->resourceCost;
  fields.abilityResourceMax = definition->maxResource;
  fields.abilityResourceRemaining = resourceRemaining;
  fields.abilityResourceNextRechargeTick = nextRechargeTick;
  fields.abilityResourceRechargeRemainingTicks =
      nextRechargeTick > session.state().clock.tickIndex
          ? nextRechargeTick - session.state().clock.tickIndex
          : 0;
  fields.abilityResourceState =
      resourceRemaining >= definition->maxResource
          ? "full"
          : (nextRechargeTick == 0U ? "depleted" : "recharging");
  fields.abilityCooldownReadyTick = readyTick;
  fields.abilityCooldownRemainingTicks =
      readyTick > session.state().clock.tickIndex ? readyTick - session.state().clock.tickIndex : 0;
  fields.abilityCooldownState =
      fields.abilityCooldownRemainingTicks == 0U ? "ready" : "cooling_down";
}

void recordAbilityProjectile(PlayableReceiptFields& fields,
                             const iggy3d::AbilityProjectileState& projectile) {
  fields.abilityRuntimeOwnedProjectile = projectile.spawned;
  fields.spellProjectileSpawned = projectile.spawned;
  fields.spellProjectileActive = projectile.spawned && projectile.projectile.active;
  fields.spellProjectileImpact = projectile.impact;
  fields.spellProjectileVisible = iggy3d::abilityProjectileVisible(projectile);
  fields.spellProjectileStatus =
      std::string(iggy3d::abilityTickStatusName(projectile.tickStatus));
  fields.spellProjectileReason = projectile.reasonCode;
  fields.spellProjectileHitSurfaceId = projectile.hitSurfaceId;
  fields.spellProjectilePositionX = debugFloat(projectile.projectile.positionMeters.x);
  fields.spellProjectilePositionY = debugFloat(projectile.projectile.positionMeters.y);
  fields.spellProjectilePositionZ = debugFloat(projectile.projectile.positionMeters.z);
  fields.abilityImpactKind = std::string(iggy3d::abilityImpactKindName(projectile.impactKind));
  fields.abilityHitEntity = iggy3d::isValid(projectile.hitEntity);
  fields.abilityHitEntityId = iggy3d::toUint64(projectile.hitEntity);
  fields.abilityHitStableName = projectile.hitStableName;
  fields.abilityDamageApplied = projectile.damageApplied > 0;
  fields.abilityDamageAmount =
      static_cast<std::uint64_t>(std::max(projectile.damageApplied, 0));
  fields.abilityTargetDefeated = projectile.targetDefeated;
}

void submitAbilityCastCommand(iggy3d::Session& session,
                              float yaw,
                              float pitch,
                              PlayableReceiptFields& fields) {
  fields.spellInputObserved = true;
  const iggy3d::Vec3 forward = lookForwardVector(yaw, pitch);
  const iggy3d::EntityState* player = playerEntity(session);
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = player == nullptr ? iggy3d::kInvalidEntityId : player->id;
  command.kind = iggy3d::CommandKind::CastAbility;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.ability = iggy3d::CommandAbilityKind::ArcaneBolt;
  command.payload.abilityDirection = forward;

  const iggy3d::SessionCommandResult submitted = session.submitCommand(command);
  const bool castAccepted = accepted(submitted);
  fields.abilityCastRequested = true;
  ++fields.abilityCastRequestCount;
  fields.abilityCastAccepted = castAccepted;
  fields.abilityId = "arcane_bolt";
  fields.abilityCastStatus = castAccepted ? "accepted" : "rejected";
  fields.abilityCastReason = abilityCommandReason(submitted);
  if (castAccepted) {
    if (fields.abilityCastAcceptCount > 0U) {
      fields.abilityRecastAccepted = true;
    }
    ++fields.abilityCastAcceptCount;
  } else {
    ++fields.abilityCastRejectCount;
    switch (submitted.command.rejection) {
      case iggy3d::CommandRejectionReason::AbilitySlotBusy:
        fields.abilitySlotBusyRejected = true;
        ++fields.abilitySlotBusyRejectCount;
        break;
      case iggy3d::CommandRejectionReason::AbilityOnCooldown:
        fields.abilityCooldownRejected = true;
        ++fields.abilityCooldownRejectCount;
        break;
      case iggy3d::CommandRejectionReason::AbilityInsufficientResource:
        fields.abilityResourceRejected = true;
        ++fields.abilityResourceRejectCount;
        break;
      default:
        break;
    }
  }
  recordAbilityPolicy(fields, session);
  recordAbilityProjectile(fields, session.state().transient.abilityRuntime.arcaneBolt);
}

void submitAbilityCastAtEntity(iggy3d::Session& session,
                               iggy3d::EntityId target,
                               PlayableReceiptFields& fields) {
  const iggy3d::EntityState* player = playerEntity(session);
  const iggy3d::EntityState* targetEntity = session.state().world.findById(target);
  const iggy3d::Vec3 origin =
      player == nullptr ? iggy3d::Vec3{} : player->transform.position + iggy3d::Vec3{0.0F, 1.2F, 0.0F};
  const iggy3d::Vec3 aim =
      targetEntity == nullptr ? origin + iggy3d::Vec3{0.0F, 0.0F, -1.0F}
                              : targetEntity->transform.position + iggy3d::Vec3{0.0F, 0.6F, 0.0F};

  fields.spellInputObserved = true;
  iggy3d::CommandRecord command;
  command.playerSlot = 0;
  command.actor = player == nullptr ? iggy3d::kInvalidEntityId : player->id;
  command.kind = iggy3d::CommandKind::CastAbility;
  command.source = iggy3d::CommandSource::LocalPlayer;
  command.payload.ability = iggy3d::CommandAbilityKind::ArcaneBolt;
  command.payload.abilityDirection = normalizedOr(aim - origin, {0.0F, 0.0F, -1.0F});

  const iggy3d::SessionCommandResult submitted = session.submitCommand(command);
  const bool castAccepted = accepted(submitted);
  fields.abilityCastRequested = true;
  ++fields.abilityCastRequestCount;
  fields.abilityCastAccepted = castAccepted;
  fields.abilityId = "arcane_bolt";
  fields.abilityCastStatus = castAccepted ? "accepted" : "rejected";
  fields.abilityCastReason = abilityCommandReason(submitted);
  if (castAccepted) {
    if (fields.abilityCastAcceptCount > 0U) {
      fields.abilityRecastAccepted = true;
    }
    ++fields.abilityCastAcceptCount;
  } else {
    ++fields.abilityCastRejectCount;
  }
  recordAbilityPolicy(fields, session);
  recordAbilityProjectile(fields, session.state().transient.abilityRuntime.arcaneBolt);
}

void stepSessionAbilityProjectiles(iggy3d::Session& session,
                                   const iggy3d::SpatialSurfaceSet& collisionSurfaces,
                                   PlayableReceiptFields& fields) {
  const bool hadAbilityWork =
      iggy3d::abilityRuntimeHasActiveProjectile(session.state().transient.abilityRuntime) ||
      !session.state().transient.pendingExecutionSequences.empty();
  const iggy3d::StatusResult tick = session.tick(&collisionSurfaces);
  const iggy3d::AbilityProjectileState& projectile =
      session.state().transient.abilityRuntime.arcaneBolt;
  if (tick.status != iggy3d::ResultStatus::Ok) {
    fields.abilityTickStatus = "invalid_state";
    fields.abilityTickReason = tick.error.code;
  } else if (hadAbilityWork || projectile.spawned) {
    fields.abilityTickStatus = std::string(iggy3d::abilityTickStatusName(projectile.tickStatus));
    fields.abilityTickReason = projectile.reasonCode;
  }
  recordAbilityPolicy(fields, session);
  recordAbilityProjectile(fields, projectile);
}

void attachAbilityProjectileProjection(const iggy3d::AbilityProjectileState& projectile,
                                       iggy3d::SceneProjectionResult& scene) {
  scene.projectiles.clear();
  scene.projectileCount = 0U;
  if (!projectile.spawned) {
    return;
  }
  iggy3d::SceneProjectileItem item;
  item.id = projectile.projectileId;
  item.positionMeters = projectile.projectile.positionMeters;
  item.previousPositionMeters = projectile.previousPositionMeters;
  item.impactPointMeters =
      projectile.impact ? projectile.impactPointMeters : projectile.projectile.positionMeters;
  item.impactNormal = projectile.impactNormal;
  item.active = projectile.projectile.active;
  item.impact = projectile.impact;
  item.hitSurfaceId = projectile.hitSurfaceId;
  scene.projectiles.push_back(std::move(item));
  scene.projectileCount = scene.projectiles.size();
}

std::optional<iggy3d::TraversalCandidatePreviewResult> buildTraversalCandidatePreview(
    const iggy3d::Session& session,
    iggy3d::EntityId actor,
    float yaw,
    const iggy3d::RoomAsset* activeRoom,
    const iggy3d::SpatialSurfaceSet& collisionSurfaces,
    const iggy3d::Vec3& roomWorldOffset) {
  if (!iggy3d::isValid(actor)) {
    return std::nullopt;
  }

  iggy3d::TraversalCandidatePreviewRequest previewRequest;
  previewRequest.actor = actor;
  previewRequest.forward = {std::sin(yaw), 0.0F, -std::cos(yaw)};
  previewRequest.room = activeRoom;
  previewRequest.collisionSurfaces = &collisionSurfaces;
  previewRequest.roomWorldOffsetMeters = roomWorldOffset;
  return iggy3d::previewTraversalCandidate(session.state().world, previewRequest);
}

void resetPlayerMotorAfterTraversal(iggy3d::PlayerMotorState& motor,
                                    iggy3d::EntityId actor,
                                    const iggy3d::TraversalResult& result) {
  motor = iggy3d::PlayerMotorState{};
  motor.actor = actor;
  if (result.mechanic != iggy3d::TraversalMechanic::WireWalk) {
    return;
  }

  motor.phase = iggy3d::PlayerMotorPhase::WireWalk;
  motor.grounded = false;
  motor.jumpAvailable = true;
  motor.wireWalkRailStartMeters = result.railStartPosition;
  motor.wireWalkRailEndMeters = result.railEndPosition;
  motor.wireWalkAxis = result.railAxis;
  motor.wireWalkCoordinateMeters = result.railCoordinateMeters;
}

bool runScriptedPlayableStep(iggy3d::Session& session,
                             std::uint32_t frameIndex,
                             PlayableReceiptFields& fields) {
  const iggy3d::EntityId key = entityIdByName(session, "gold_key");
  const iggy3d::EntityId dummy = entityIdByName(session, "training_dummy");
  if (frameIndex == 0U) {
    const iggy3d::SessionCommandResult interact = session.submitCommand(interactCommand(key));
    fields.targetDiscovered = interact.command.payload.target.hasEntity;
    fields.initialReachFailed =
        interact.command.rejection == iggy3d::CommandRejectionReason::OutOfRange;
    return true;
  }
  if (frameIndex == 1U) {
    fields.reachPassed = submitAndDrain(session, moveCommand({2.0F, 0.0F, 0.0F}));
    return fields.reachPassed;
  }
  if (frameIndex == 2U) {
    fields.interactionExecuted = submitAndDrain(session, retryCommand(1));
    fields.objectiveComplete = objectiveComplete(session);
    return fields.interactionExecuted;
  }
  if (frameIndex == 3U) {
    return submitAndDrain(session, moveCommand({2.0F, 0.0F, 1.0F}));
  }
  if (frameIndex == 4U) {
    fields.attackExecuted = submitAndDrain(session, attackCommand(dummy));
    return fields.attackExecuted;
  }
  if (frameIndex == 5U) {
    const iggy3d::SessionResetResult reset = session.resetToBaseline();
    recordResetResult(fields, session, reset);
    return fields.resetExecuted;
  }
  return true;
}

void runCodexAcceptanceDemoStep(iggy3d::Session& session,
                                const iggy3d::SessionCreateRequest& create,
                                const iggy3d::SpatialSurfaceSet& collisionSurfaces,
                                std::uint32_t frameIndex,
                                PlayableReceiptFields& fields) {
  fields.acceptanceDemoRequested = true;
  const iggy3d::EntityId key = entityIdByName(session, "gold_key");
  const iggy3d::EntityId dummy = entityIdByName(session, "training_dummy");

  if (frameIndex == 0U) {
    const iggy3d::SessionCommandResult interact = session.submitCommand(interactCommand(key));
    fields.targetDiscovered = interact.command.payload.target.hasEntity;
    fields.initialReachFailed =
        interact.command.rejection == iggy3d::CommandRejectionReason::OutOfRange;
    return;
  }
  if (frameIndex == 1U) {
    fields.reachPassed = submitAndDrain(session, moveCommand({0.0F, 0.0F, -2.0F}));
    return;
  }
  if (frameIndex == 2U) {
    fields.retryExecuted = submitAndDrain(session, retryCommand(1));
    fields.interactionExecuted = fields.retryExecuted;
    fields.objectiveComplete = objectiveComplete(session);
    return;
  }
  if (frameIndex == 3U) {
    submitAndRecordControlCommand(session, iggy3d::CommandKind::ToggleTacticalMode, fields);
    return;
  }
  if (frameIndex == 4U) {
    submitAndRecordControlCommand(session, iggy3d::CommandKind::Pause, fields);
    return;
  }
  if (frameIndex == 5U) {
    submitAndRecordControlCommand(session, iggy3d::CommandKind::StepTacticalTick, fields);
    return;
  }
  if (frameIndex == 6U) {
    submitAndRecordControlCommand(session, iggy3d::CommandKind::Resume, fields);
    return;
  }
  if (frameIndex == 7U) {
    submitAbilityCastAtEntity(session, dummy, fields);
    return;
  }
  if (frameIndex == 60U) {
    const bool loaded = runSaveLoadRoundtrip(session, create, fields);
    recordTrainingDummyCombat(fields, session);
    fields.acceptancePreResetSavedState =
        loaded && fields.objectiveComplete && fields.abilityDamageApplied &&
        fields.trainingDummyCombatTracked && fields.trainingDummyHitPoints == 3U;
    return;
  }
  if (frameIndex == 70U) {
    const iggy3d::SessionResetResult reset = session.resetToBaseline();
    recordResetResult(fields, session, reset);
    recordAbilityPolicy(fields, session);
    recordAbilityProjectile(fields, session.state().transient.abilityRuntime.arcaneBolt);
    recordTrainingDummyCombat(fields, session);
    fields.acceptanceDemoComplete =
        fields.targetDiscovered && fields.initialReachFailed && fields.reachPassed &&
        fields.retryExecuted && fields.interactionExecuted && fields.objectiveComplete &&
        fields.tacticalToggleExecuted && fields.pauseTickFrozen && fields.stepAdvancedOnce &&
        fields.resumeExecuted && fields.abilityCastAccepted &&
        fields.acceptancePreResetSavedState && fields.saveLoadRoundtripPassed &&
        fields.saveLoadHashMatched && fields.resetBaselineRestored &&
        fields.resetCommandLogCleared;
    return;
  }

  if (iggy3d::abilityRuntimeHasActiveProjectile(session.state().transient.abilityRuntime) ||
      frameIndex > 7U) {
    stepSessionAbilityProjectiles(session, collisionSurfaces, fields);
  }
}

void appendPlayableReceiptFields(iggy3d::RenderReceipt& receipt,
                                 const PlayableReceiptFields& fields) {
  if (!fields.playable) {
    return;
  }
  iggy3d::appendReceiptField(receipt, "camera_mode", "first_person");
  iggy3d::appendReceiptField(receipt, "interactive_mode", fields.interactiveMode);
  iggy3d::appendReceiptField(receipt, "input_backend", inputBackendName(fields.inputBackend));
  iggy3d::appendReceiptField(receipt, "target_discovered", fields.targetDiscovered);
  iggy3d::appendReceiptField(receipt, "initial_reach_gate",
                             fields.initialReachFailed ? "fail" : "not_attempted");
  iggy3d::appendReceiptField(receipt, "reach_gate",
                             fields.reachPassed ? "pass"
                                                : (fields.initialReachFailed ? "fail"
                                                                             : "not_attempted"));
  iggy3d::appendReceiptField(receipt, "interaction_executed", fields.interactionExecuted);
  iggy3d::appendReceiptField(receipt, "retry_executed", fields.retryExecuted);
  iggy3d::appendReceiptField(receipt, "attack_executed", fields.attackExecuted);
  iggy3d::appendReceiptField(receipt, "objective_complete", fields.objectiveComplete);
  iggy3d::appendReceiptField(receipt, "acceptance_demo_requested",
                             fields.acceptanceDemoRequested);
  iggy3d::appendReceiptField(receipt, "acceptance_demo_complete",
                             fields.acceptanceDemoComplete);
  iggy3d::appendReceiptField(receipt, "acceptance_pre_reset_saved_state",
                             fields.acceptancePreResetSavedState);
  iggy3d::appendReceiptField(receipt, "retry_available", fields.retryAvailable);
  iggy3d::appendReceiptField(receipt, "reset_executed", fields.resetExecuted);
  iggy3d::appendReceiptField(receipt, "reset_baseline_restored",
                             fields.resetBaselineRestored);
  iggy3d::appendReceiptField(receipt, "reset_command_log_cleared",
                             fields.resetCommandLogCleared);
  iggy3d::appendReceiptField(receipt, "reset_next_command_id", fields.resetNextCommandId);
  iggy3d::appendReceiptField(receipt, "runtime_clock_mode", fields.runtimeClockMode);
  iggy3d::appendReceiptField(receipt, "tactical_toggle_executed",
                             fields.tacticalToggleExecuted);
  iggy3d::appendReceiptField(receipt, "tactical_mode_active", fields.tacticalModeActive);
  iggy3d::appendReceiptField(receipt, "pause_executed", fields.pauseExecuted);
  iggy3d::appendReceiptField(receipt, "pause_tick_frozen", fields.pauseTickFrozen);
  iggy3d::appendReceiptField(receipt, "pause_tick_before", fields.pauseTickBefore);
  iggy3d::appendReceiptField(receipt, "pause_tick_after", fields.pauseTickAfter);
  iggy3d::appendReceiptField(receipt, "step_executed", fields.stepExecuted);
  iggy3d::appendReceiptField(receipt, "step_tick_advanced_once", fields.stepAdvancedOnce);
  iggy3d::appendReceiptField(receipt, "step_tick_before", fields.stepTickBefore);
  iggy3d::appendReceiptField(receipt, "step_tick_after", fields.stepTickAfter);
  iggy3d::appendReceiptField(receipt, "resume_executed", fields.resumeExecuted);
  iggy3d::appendReceiptField(receipt, "resume_tick_after", fields.resumeTickAfter);
  iggy3d::appendReceiptField(receipt, "save_load_replay_stable", fields.saveLoadReplayStable);
  iggy3d::appendReceiptField(receipt, "save_load_requested", fields.saveLoadRequested);
  iggy3d::appendReceiptField(receipt, "save_load_roundtrip_passed",
                             fields.saveLoadRoundtripPassed);
  iggy3d::appendReceiptField(receipt, "save_load_hash_matched", fields.saveLoadHashMatched);
  iggy3d::appendReceiptField(receipt, "save_load_session_replaced",
                             fields.saveLoadSessionReplaced);
  iggy3d::appendReceiptField(receipt, "save_load_saved_hash", fields.saveLoadSavedHash);
  iggy3d::appendReceiptField(receipt, "save_load_loaded_hash", fields.saveLoadLoadedHash);
  iggy3d::appendReceiptField(receipt, "save_load_encoded_bytes", fields.saveLoadEncodedBytes);
  iggy3d::appendReceiptField(receipt, "save_load_status", fields.saveLoadStatus);
  iggy3d::appendReceiptField(receipt, "tactical_view_available", false);
  iggy3d::appendReceiptField(receipt, "kinematic_controller_active", fields.kinematicControllerActive);
  iggy3d::appendReceiptField(receipt, "kinematic_movement_attempted", fields.kinematicMovementAttempted);
  iggy3d::appendReceiptField(receipt, "kinematic_movement_accepted", fields.kinematicMovementAccepted);
  iggy3d::appendReceiptField(receipt, "kinematic_command_log_integrated",
                             fields.kinematicCommandLogIntegrated);
  iggy3d::appendReceiptField(receipt, "movement_reason", fields.movementReason);
  iggy3d::appendReceiptField(receipt, "movement_policy_band", fields.movementPolicyBand);
  iggy3d::appendReceiptField(receipt, "movement_distance_meters",
                             fields.movementDistanceMeters);
  iggy3d::appendReceiptField(receipt, "movement_horizontal_distance_meters",
                             fields.movementHorizontalDistanceMeters);
  iggy3d::appendReceiptField(receipt, "movement_vertical_delta_meters",
                             fields.movementVerticalDeltaMeters);
  iggy3d::appendReceiptField(receipt, "movement_grade_percent",
                             fields.movementGradePercent);
  iggy3d::appendReceiptField(receipt, "slope_travel_direction",
                             fields.slopeTravelDirection);
  iggy3d::appendReceiptField(receipt, "traversal_attempted", fields.traversalAttempted);
  iggy3d::appendReceiptField(receipt, "traversal_accepted", fields.traversalAccepted);
  iggy3d::appendReceiptField(receipt, "traversal_mechanic", fields.traversalMechanic);
  iggy3d::appendReceiptField(receipt, "traversal_reason", fields.traversalReason);
  iggy3d::appendReceiptField(receipt, "traversal_slot_id", fields.traversalSlotId);
  iggy3d::appendReceiptField(receipt, "traversal_slot_kind", fields.traversalSlotKind);
  iggy3d::appendReceiptField(receipt, "traversal_slot_height_band",
                             fields.traversalSlotHeightBand);
  iggy3d::appendReceiptField(receipt, "traversal_target_id", fields.traversalTargetId);
  iggy3d::appendReceiptField(receipt, "traversal_landing_surface_id",
                             fields.traversalLandingSurfaceId);
  iggy3d::appendReceiptField(receipt, "traversal_slot_ledge_height_meters",
                             fields.traversalSlotLedgeHeightMeters);
  iggy3d::appendReceiptField(receipt, "traversal_slot_usable_width_meters",
                             fields.traversalSlotUsableWidthMeters);
  iggy3d::appendReceiptField(receipt, "traversal_slot_start_range_meters",
                             fields.traversalSlotStartRangeMeters);
  iggy3d::appendReceiptField(receipt, "traversal_slot_facing_dot",
                             fields.traversalSlotFacingDot);
  iggy3d::appendReceiptField(receipt, "traversal_distance_meters",
                             fields.traversalDistanceMeters);
  iggy3d::appendReceiptField(receipt, "traversal_horizontal_distance_meters",
                             fields.traversalHorizontalDistanceMeters);
  iggy3d::appendReceiptField(receipt, "traversal_vertical_delta_meters",
                             fields.traversalVerticalDeltaMeters);
  iggy3d::appendReceiptField(receipt, "traversal_grade_percent",
                             fields.traversalGradePercent);
  iggy3d::appendReceiptField(receipt, "traversal_direction", fields.traversalDirection);
  iggy3d::appendReceiptField(receipt, "traversal_intent_requested",
                             fields.traversalIntentRequested);
  iggy3d::appendReceiptField(receipt, "traversal_intent_consumed",
                             fields.traversalIntentConsumed);
  iggy3d::appendReceiptField(receipt, "traversal_intent_accepted",
                             fields.traversalIntentAccepted);
  iggy3d::appendReceiptField(receipt, "traversal_intent_fallback_jump_allowed",
                             fields.traversalIntentFallbackJumpAllowed);
  iggy3d::appendReceiptField(receipt, "traversal_intent_trigger",
                             fields.traversalIntentTrigger);
  iggy3d::appendReceiptField(receipt, "traversal_intent_status",
                             fields.traversalIntentStatus);
  iggy3d::appendReceiptField(receipt, "traversal_intent_selected_mechanic",
                             fields.traversalIntentSelectedMechanic);
  iggy3d::appendReceiptField(receipt, "traversal_start_x", fields.traversalStartX);
  iggy3d::appendReceiptField(receipt, "traversal_start_y", fields.traversalStartY);
  iggy3d::appendReceiptField(receipt, "traversal_start_z", fields.traversalStartZ);
  iggy3d::appendReceiptField(receipt, "traversal_final_x", fields.traversalFinalX);
  iggy3d::appendReceiptField(receipt, "traversal_final_y", fields.traversalFinalY);
  iggy3d::appendReceiptField(receipt, "traversal_final_z", fields.traversalFinalZ);
  iggy3d::appendReceiptField(receipt, "movement_start_x", fields.movementStartX);
  iggy3d::appendReceiptField(receipt, "movement_start_y", fields.movementStartY);
  iggy3d::appendReceiptField(receipt, "movement_start_z", fields.movementStartZ);
  iggy3d::appendReceiptField(receipt, "movement_destination_x",
                             fields.movementDestinationX);
  iggy3d::appendReceiptField(receipt, "movement_destination_y",
                             fields.movementDestinationY);
  iggy3d::appendReceiptField(receipt, "movement_destination_z",
                             fields.movementDestinationZ);
  iggy3d::appendReceiptField(receipt, "movement_final_x", fields.movementFinalX);
  iggy3d::appendReceiptField(receipt, "movement_final_y", fields.movementFinalY);
  iggy3d::appendReceiptField(receipt, "movement_final_z", fields.movementFinalZ);
  iggy3d::appendReceiptField(receipt, "ground_surface_id", fields.groundSurfaceId);
  iggy3d::appendReceiptField(receipt, "ground_sample_valid", fields.groundSampleValid);
  iggy3d::appendReceiptField(receipt, "ground_contact", fields.groundContact);
  iggy3d::appendReceiptField(receipt, "ground_walkable", fields.groundWalkable);
  iggy3d::appendReceiptField(receipt, "careful_footing", fields.carefulFooting);
  iggy3d::appendReceiptField(receipt, "ground_distance_meters",
                             fields.groundDistanceMeters);
  iggy3d::appendReceiptField(receipt, "ground_normal_x", fields.groundNormalX);
  iggy3d::appendReceiptField(receipt, "ground_normal_y", fields.groundNormalY);
  iggy3d::appendReceiptField(receipt, "ground_normal_z", fields.groundNormalZ);
  iggy3d::appendReceiptField(receipt, "slope_angle_degrees", fields.slopeAngleDegrees);
  iggy3d::appendReceiptField(receipt, "slope_up_dot", fields.slopeUpDot);
  iggy3d::appendReceiptField(receipt, "speed_multiplier", fields.speedMultiplier);
  iggy3d::appendReceiptField(receipt, "stamina_cost_multiplier",
                             fields.staminaCostMultiplier);
  iggy3d::appendReceiptField(receipt, "step_penalty_multiplier",
                             fields.stepPenaltyMultiplier);
  iggy3d::appendReceiptField(receipt, "movement_clamped", fields.movementClamped);
  iggy3d::appendReceiptField(receipt, "movement_slid", fields.movementSlid);
  iggy3d::appendReceiptField(receipt, "ground_snap_applied", fields.groundSnapApplied);
  iggy3d::appendReceiptField(receipt, "crouch_available", fields.crouchAvailable);
  iggy3d::appendReceiptField(receipt, "crouch_active", fields.crouchActive);
  iggy3d::appendReceiptField(receipt, "crouch_input_observed", fields.crouchInputObserved);
  iggy3d::appendReceiptField(receipt, "player_motor_active", fields.playerMotorActive);
  iggy3d::appendReceiptField(receipt, "player_grounded", fields.playerGrounded);
  iggy3d::appendReceiptField(receipt, "player_landed", fields.playerLanded);
  iggy3d::appendReceiptField(receipt, "player_motor_phase", fields.playerMotorPhase);
  iggy3d::appendReceiptField(receipt, "player_motor_reason", fields.playerMotorReason);
  iggy3d::appendReceiptField(receipt, "vertical_velocity_state", fields.verticalVelocityState);
  iggy3d::appendReceiptField(receipt, "horizontal_velocity_state",
                             fields.horizontalVelocityState);
  iggy3d::appendReceiptField(receipt, "jump_input_observed", fields.jumpInputObserved);
  iggy3d::appendReceiptField(receipt, "jump_accepted", fields.jumpAccepted);
  iggy3d::appendReceiptField(receipt, "air_move_intent_observed",
                             fields.airMoveIntentObserved);
  iggy3d::appendReceiptField(receipt, "air_control_active", fields.airControlActive);
  iggy3d::appendReceiptField(receipt, "air_movement_clamped", fields.airMovementClamped);
  iggy3d::appendReceiptField(receipt, "air_movement_slid", fields.airMovementSlid);
  iggy3d::appendReceiptField(receipt, "dash_input_observed", fields.dashInputObserved);
  iggy3d::appendReceiptField(receipt, "dash_accepted", fields.dashAccepted);
  iggy3d::appendReceiptField(receipt, "dash_active", fields.dashActive);
  iggy3d::appendReceiptField(receipt, "dash_cooldown_state", fields.dashCooldownState);
  iggy3d::appendReceiptField(receipt, "dash_movement_clamped", fields.dashMovementClamped);
  iggy3d::appendReceiptField(receipt, "dash_movement_slid", fields.dashMovementSlid);
  iggy3d::appendReceiptField(receipt, "spell_input_observed", fields.spellInputObserved);
  iggy3d::appendReceiptField(receipt, "spell_projectile_spawned",
                             fields.spellProjectileSpawned);
  iggy3d::appendReceiptField(receipt, "spell_projectile_active",
                             fields.spellProjectileActive);
  iggy3d::appendReceiptField(receipt, "spell_projectile_impact",
                             fields.spellProjectileImpact);
  iggy3d::appendReceiptField(receipt, "spell_projectile_visible",
                             fields.spellProjectileVisible);
  iggy3d::appendReceiptField(receipt, "spell_projectile_status",
                             fields.spellProjectileStatus);
  iggy3d::appendReceiptField(receipt, "spell_projectile_reason",
                             fields.spellProjectileReason);
  iggy3d::appendReceiptField(receipt, "spell_projectile_hit_surface_id",
                             fields.spellProjectileHitSurfaceId);
  iggy3d::appendReceiptField(receipt, "spell_projectile_position_x",
                             fields.spellProjectilePositionX);
  iggy3d::appendReceiptField(receipt, "spell_projectile_position_y",
                             fields.spellProjectilePositionY);
  iggy3d::appendReceiptField(receipt, "spell_projectile_position_z",
                             fields.spellProjectilePositionZ);
  iggy3d::appendReceiptField(receipt, "ability_id", fields.abilityId);
  iggy3d::appendReceiptField(receipt, "ability_cast_requested",
                             fields.abilityCastRequested);
  iggy3d::appendReceiptField(receipt, "ability_cast_accepted", fields.abilityCastAccepted);
  iggy3d::appendReceiptField(receipt, "ability_cast_status", fields.abilityCastStatus);
  iggy3d::appendReceiptField(receipt, "ability_cast_reason", fields.abilityCastReason);
  iggy3d::appendReceiptField(receipt, "ability_cast_request_count",
                             fields.abilityCastRequestCount);
  iggy3d::appendReceiptField(receipt, "ability_cast_accept_count",
                             fields.abilityCastAcceptCount);
  iggy3d::appendReceiptField(receipt, "ability_cast_reject_count",
                             fields.abilityCastRejectCount);
  iggy3d::appendReceiptField(receipt, "ability_slot_busy_rejected",
                             fields.abilitySlotBusyRejected);
  iggy3d::appendReceiptField(receipt, "ability_slot_busy_reject_count",
                             fields.abilitySlotBusyRejectCount);
  iggy3d::appendReceiptField(receipt, "ability_cooldown_rejected",
                             fields.abilityCooldownRejected);
  iggy3d::appendReceiptField(receipt, "ability_cooldown_reject_count",
                             fields.abilityCooldownRejectCount);
  iggy3d::appendReceiptField(receipt, "ability_resource_rejected",
                             fields.abilityResourceRejected);
  iggy3d::appendReceiptField(receipt, "ability_resource_reject_count",
                             fields.abilityResourceRejectCount);
  iggy3d::appendReceiptField(receipt, "ability_recast_accepted",
                             fields.abilityRecastAccepted);
  iggy3d::appendReceiptField(receipt, "ability_resource_remaining",
                             fields.abilityResourceRemaining);
  iggy3d::appendReceiptField(receipt, "ability_resource_cost", fields.abilityResourceCost);
  iggy3d::appendReceiptField(receipt, "ability_resource_max", fields.abilityResourceMax);
  iggy3d::appendReceiptField(receipt, "ability_resource_state",
                             fields.abilityResourceState);
  iggy3d::appendReceiptField(receipt, "ability_resource_next_recharge_tick",
                             fields.abilityResourceNextRechargeTick);
  iggy3d::appendReceiptField(receipt, "ability_resource_recharge_remaining_ticks",
                             fields.abilityResourceRechargeRemainingTicks);
  iggy3d::appendReceiptField(receipt, "ability_cooldown_state",
                             fields.abilityCooldownState);
  iggy3d::appendReceiptField(receipt, "ability_cooldown_ready_tick",
                             fields.abilityCooldownReadyTick);
  iggy3d::appendReceiptField(receipt, "ability_cooldown_remaining_ticks",
                             fields.abilityCooldownRemainingTicks);
  iggy3d::appendReceiptField(receipt, "ability_tick_status", fields.abilityTickStatus);
  iggy3d::appendReceiptField(receipt, "ability_tick_reason", fields.abilityTickReason);
  iggy3d::appendReceiptField(receipt, "ability_impact_kind", fields.abilityImpactKind);
  iggy3d::appendReceiptField(receipt, "ability_runtime_owned_projectile",
                             fields.abilityRuntimeOwnedProjectile);
  iggy3d::appendReceiptField(receipt, "ability_hit_entity", fields.abilityHitEntity);
  iggy3d::appendReceiptField(receipt, "ability_hit_entity_id", fields.abilityHitEntityId);
  iggy3d::appendReceiptField(receipt, "ability_hit_stable_name",
                             fields.abilityHitStableName);
  iggy3d::appendReceiptField(receipt, "ability_damage_applied",
                             fields.abilityDamageApplied);
  iggy3d::appendReceiptField(receipt, "ability_damage_amount",
                             fields.abilityDamageAmount);
  iggy3d::appendReceiptField(receipt, "ability_target_defeated",
                             fields.abilityTargetDefeated);
  iggy3d::appendReceiptField(receipt, "training_dummy_combat_tracked",
                             fields.trainingDummyCombatTracked);
  iggy3d::appendReceiptField(receipt, "training_dummy_hit_points",
                             fields.trainingDummyHitPoints);
  iggy3d::appendReceiptField(receipt, "training_dummy_defeated",
                             fields.trainingDummyDefeated);
  iggy3d::appendReceiptField(receipt, "debug_overlay_enabled", fields.debugOverlayEnabled);
  iggy3d::appendReceiptField(receipt, "debug_overlay_open", fields.debugOverlayOpen);
  iggy3d::appendReceiptField(receipt, "debug_overlay_toggle_observed",
                             fields.debugOverlayToggleObserved);
  iggy3d::appendReceiptField(receipt, "debug_overlay_reason", fields.debugOverlayReason);
  iggy3d::appendReceiptField(receipt, "debug_overlay_visible",
                             fields.debugOverlayVisible);
  iggy3d::appendReceiptField(receipt, "debug_overlay_surface", fields.debugOverlaySurface);
  iggy3d::appendReceiptField(receipt, "debug_title_fallback_active",
                             fields.debugTitleFallbackActive);
  iggy3d::appendReceiptField(receipt, "debug_player_position_available",
                             fields.debugPlayerPositionAvailable);
  iggy3d::appendReceiptField(receipt, "debug_speed_available", fields.debugSpeedAvailable);
  iggy3d::appendReceiptField(receipt, "debug_distance_available",
                             fields.debugDistanceAvailable);
  iggy3d::appendReceiptField(receipt, "debug_player_phase", fields.debugPlayerPhase);
  iggy3d::appendReceiptField(receipt, "debug_movement_policy_band",
                             fields.debugMovementPolicyBand);
  iggy3d::appendReceiptField(receipt, "debug_ground_surface_id",
                             fields.debugGroundSurfaceId);
  iggy3d::appendReceiptField(receipt, "debug_hit_surface_id", fields.debugHitSurfaceId);
  iggy3d::appendReceiptField(receipt, "debug_ground_sample_valid",
                             fields.debugGroundSampleValid);
  iggy3d::appendReceiptField(receipt, "debug_ground_contact",
                             fields.debugGroundContact);
  iggy3d::appendReceiptField(receipt, "debug_ground_walkable",
                             fields.debugGroundWalkable);
  iggy3d::appendReceiptField(receipt, "debug_careful_footing",
                             fields.debugCarefulFooting);
  iggy3d::appendReceiptField(receipt, "debug_ground_distance_meters",
                             fields.debugGroundDistanceMeters);
  iggy3d::appendReceiptField(receipt, "debug_ground_normal_x",
                             fields.debugGroundNormalX);
  iggy3d::appendReceiptField(receipt, "debug_ground_normal_y",
                             fields.debugGroundNormalY);
  iggy3d::appendReceiptField(receipt, "debug_ground_normal_z",
                             fields.debugGroundNormalZ);
  iggy3d::appendReceiptField(receipt, "debug_slope_angle_degrees",
                             fields.debugSlopeAngleDegrees);
  iggy3d::appendReceiptField(receipt, "debug_slope_up_dot", fields.debugSlopeUpDot);
  iggy3d::appendReceiptField(receipt, "debug_speed_multiplier",
                             fields.debugSpeedMultiplier);
  iggy3d::appendReceiptField(receipt, "debug_stamina_cost_multiplier",
                             fields.debugStaminaCostMultiplier);
  iggy3d::appendReceiptField(receipt, "debug_step_penalty_multiplier",
                             fields.debugStepPenaltyMultiplier);
  iggy3d::appendReceiptField(receipt, "debug_movement_horizontal_distance_meters",
                             fields.debugMovementHorizontalDistanceMeters);
  iggy3d::appendReceiptField(receipt, "debug_movement_vertical_delta_meters",
                             fields.debugMovementVerticalDeltaMeters);
  iggy3d::appendReceiptField(receipt, "debug_movement_grade_percent",
                             fields.debugMovementGradePercent);
  iggy3d::appendReceiptField(receipt, "debug_slope_travel_direction",
                             fields.debugSlopeTravelDirection);
  iggy3d::appendReceiptField(receipt, "debug_moved_this_frame_meters",
                             fields.debugMovedThisFrameMeters);
  iggy3d::appendReceiptField(receipt, "debug_horizontal_speed_meters_per_second",
                             fields.debugHorizontalSpeedMetersPerSecond);
  iggy3d::appendReceiptField(receipt, "debug_vertical_speed_meters_per_second",
                             fields.debugVerticalSpeedMetersPerSecond);
  iggy3d::appendReceiptField(receipt, "debug_distance_from_spawn_meters",
                             fields.debugDistanceFromSpawnMeters);
  iggy3d::appendReceiptField(receipt, "debug_position_x", fields.debugPositionX);
  iggy3d::appendReceiptField(receipt, "debug_position_y", fields.debugPositionY);
  iggy3d::appendReceiptField(receipt, "debug_position_z", fields.debugPositionZ);
  iggy3d::appendReceiptField(receipt, "debug_yaw_radians", fields.debugYawRadians);
  iggy3d::appendReceiptField(receipt, "debug_pitch_radians", fields.debugPitchRadians);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_available",
                             fields.debugTraversalPreviewAvailable);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_ready",
                             fields.debugTraversalPreviewReady);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_candidate_available",
                             fields.debugTraversalPreviewCandidateAvailable);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_status",
                             fields.debugTraversalPreviewStatus);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_hud_code",
                             fields.debugTraversalPreviewHudCode);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_mechanic",
                             fields.debugTraversalPreviewMechanic);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_slot_id",
                             fields.debugTraversalPreviewSlotId);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_slot_kind",
                             fields.debugTraversalPreviewSlotKind);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_slot_height_band",
                             fields.debugTraversalPreviewSlotHeightBand);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_target_id",
                             fields.debugTraversalPreviewTargetId);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_landing_surface_id",
                             fields.debugTraversalPreviewLandingSurfaceId);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_slot_ledge_height_meters",
                             fields.debugTraversalPreviewSlotLedgeHeightMeters);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_slot_usable_width_meters",
                             fields.debugTraversalPreviewSlotUsableWidthMeters);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_slot_start_range_meters",
                             fields.debugTraversalPreviewSlotStartRangeMeters);
  iggy3d::appendReceiptField(receipt, "debug_traversal_preview_slot_facing_dot",
                             fields.debugTraversalPreviewSlotFacingDot);
  iggy3d::appendReceiptField(receipt, "debug_traversal_available",
                             fields.debugTraversalAvailable);
  iggy3d::appendReceiptField(receipt, "debug_traversal_intent_requested",
                             fields.debugTraversalIntentRequested);
  iggy3d::appendReceiptField(receipt, "debug_traversal_intent_consumed",
                             fields.debugTraversalIntentConsumed);
  iggy3d::appendReceiptField(receipt, "debug_traversal_intent_accepted",
                             fields.debugTraversalIntentAccepted);
  iggy3d::appendReceiptField(receipt, "debug_traversal_intent_fallback_jump_allowed",
                             fields.debugTraversalIntentFallbackJumpAllowed);
  iggy3d::appendReceiptField(receipt, "debug_traversal_attempted",
                             fields.debugTraversalAttempted);
  iggy3d::appendReceiptField(receipt, "debug_traversal_accepted",
                             fields.debugTraversalAccepted);
  iggy3d::appendReceiptField(receipt, "debug_traversal_intent_trigger",
                             fields.debugTraversalIntentTrigger);
  iggy3d::appendReceiptField(receipt, "debug_traversal_intent_status",
                             fields.debugTraversalIntentStatus);
  iggy3d::appendReceiptField(receipt, "debug_traversal_intent_selected_mechanic",
                             fields.debugTraversalIntentSelectedMechanic);
  iggy3d::appendReceiptField(receipt, "debug_traversal_mechanic",
                             fields.debugTraversalMechanic);
  iggy3d::appendReceiptField(receipt, "debug_traversal_reason",
                             fields.debugTraversalReason);
  iggy3d::appendReceiptField(receipt, "debug_traversal_slot_id",
                             fields.debugTraversalSlotId);
  iggy3d::appendReceiptField(receipt, "debug_traversal_slot_kind",
                             fields.debugTraversalSlotKind);
  iggy3d::appendReceiptField(receipt, "debug_traversal_slot_height_band",
                             fields.debugTraversalSlotHeightBand);
  iggy3d::appendReceiptField(receipt, "debug_traversal_target_id",
                             fields.debugTraversalTargetId);
  iggy3d::appendReceiptField(receipt, "debug_traversal_landing_surface_id",
                             fields.debugTraversalLandingSurfaceId);
  iggy3d::appendReceiptField(receipt, "debug_traversal_slot_ledge_height_meters",
                             fields.debugTraversalSlotLedgeHeightMeters);
  iggy3d::appendReceiptField(receipt, "debug_traversal_slot_usable_width_meters",
                             fields.debugTraversalSlotUsableWidthMeters);
  iggy3d::appendReceiptField(receipt, "debug_traversal_slot_start_range_meters",
                             fields.debugTraversalSlotStartRangeMeters);
  iggy3d::appendReceiptField(receipt, "debug_traversal_slot_facing_dot",
                             fields.debugTraversalSlotFacingDot);
  iggy3d::appendReceiptField(receipt, "stance", fields.stance);
  iggy3d::appendReceiptField(receipt, "eye_height_meters", fields.eyeHeightMeters);
  iggy3d::appendReceiptField(receipt, "actor_height_meters", fields.actorHeightMeters);
  iggy3d::appendReceiptField(receipt, "movement_speed_meters_per_second",
                             fields.movementSpeedMetersPerSecond);
  iggy3d::appendReceiptField(receipt, "dev_menu_enabled", fields.devMenuEnabled);
  iggy3d::appendReceiptField(receipt, "dev_menu_open", fields.devMenuOpen);
  iggy3d::appendReceiptField(receipt, "dev_menu_toggle_observed",
                             fields.devMenuToggleObserved);
  iggy3d::appendReceiptField(receipt, "dev_menu_selected_mechanic",
                             fields.devMenuSelectedMechanic);
  iggy3d::appendReceiptField(receipt, "dev_menu_execute_requested",
                             fields.devMenuExecuteRequested);
  iggy3d::appendReceiptField(receipt, "dev_menu_execution_status",
                             fields.devMenuExecutionStatus);
  iggy3d::appendReceiptField(receipt, "editor_enabled", fields.editorEnabled);
  iggy3d::appendReceiptField(receipt, "editor_open", fields.editorOpen);
  iggy3d::appendReceiptField(receipt, "editor_toggle_observed",
                             fields.editorToggleObserved);
  iggy3d::appendReceiptField(receipt, "editor_tool", fields.editorTool);
  iggy3d::appendReceiptField(receipt, "editor_preset", fields.editorPreset);
  iggy3d::appendReceiptField(receipt, "editor_apply_requested",
                             fields.editorApplyRequested);
  iggy3d::appendReceiptField(receipt, "editor_delete_requested",
                             fields.editorDeleteRequested);
  iggy3d::appendReceiptField(receipt, "editor_undo_requested",
                             fields.editorUndoRequested);
  iggy3d::appendReceiptField(receipt, "editor_redo_requested",
                             fields.editorRedoRequested);
  iggy3d::appendReceiptField(receipt, "editor_last_command",
                             fields.editorLastCommand);
  iggy3d::appendReceiptField(receipt, "editor_last_status",
                             fields.editorLastStatus);
  iggy3d::appendReceiptField(receipt, "editor_selected_id", fields.editorSelectedId);
  iggy3d::appendReceiptField(receipt, "editor_cursor_x", fields.editorCursorX);
  iggy3d::appendReceiptField(receipt, "editor_cursor_y", fields.editorCursorY);
  iggy3d::appendReceiptField(receipt, "editor_cursor_z", fields.editorCursorZ);
  iggy3d::appendReceiptField(receipt, "editor_probe_available",
                             fields.editorProbeAvailable);
  iggy3d::appendReceiptField(receipt, "editor_probe_hit", fields.editorProbeHit);
  iggy3d::appendReceiptField(receipt, "editor_placement_valid",
                             fields.editorPlacementValid);
  iggy3d::appendReceiptField(receipt, "editor_probe_status", fields.editorProbeStatus);
  iggy3d::appendReceiptField(receipt, "editor_probe_surface_id",
                             fields.editorProbeSurfaceId);
  iggy3d::appendReceiptField(receipt, "editor_probe_surface_role",
                             fields.editorProbeSurfaceRole);
  iggy3d::appendReceiptField(receipt, "editor_probe_distance_meters",
                             fields.editorProbeDistanceMeters);
  iggy3d::appendReceiptField(receipt, "editor_probe_x", fields.editorProbeX);
  iggy3d::appendReceiptField(receipt, "editor_probe_y", fields.editorProbeY);
  iggy3d::appendReceiptField(receipt, "editor_probe_z", fields.editorProbeZ);
  iggy3d::appendReceiptField(receipt, "editor_probe_normal_x",
                             fields.editorProbeNormalX);
  iggy3d::appendReceiptField(receipt, "editor_probe_normal_y",
                             fields.editorProbeNormalY);
  iggy3d::appendReceiptField(receipt, "editor_probe_normal_z",
                             fields.editorProbeNormalZ);
  iggy3d::appendReceiptField(receipt, "editor_ghost_visible", fields.editorGhostVisible);
  iggy3d::appendReceiptField(receipt, "editor_ghost_role", fields.editorGhostRole);
  iggy3d::appendReceiptField(receipt, "editor_ghost_x", fields.editorGhostX);
  iggy3d::appendReceiptField(receipt, "editor_ghost_y", fields.editorGhostY);
  iggy3d::appendReceiptField(receipt, "editor_ghost_z", fields.editorGhostZ);
  iggy3d::appendReceiptField(receipt, "editor_ghost_size_x", fields.editorGhostSizeX);
  iggy3d::appendReceiptField(receipt, "editor_ghost_size_y", fields.editorGhostSizeY);
  iggy3d::appendReceiptField(receipt, "editor_ghost_size_z", fields.editorGhostSizeZ);
  iggy3d::appendReceiptField(receipt, "editor_selection_source",
                             fields.editorSelectionSource);
  iggy3d::appendReceiptField(receipt, "editor_floor_count", fields.editorFloorCount);
  iggy3d::appendReceiptField(receipt, "editor_wall_count", fields.editorWallCount);
  iggy3d::appendReceiptField(receipt, "editor_runtime_static_mesh_count",
                             fields.editorRuntimeStaticMeshCount);
  iggy3d::appendReceiptField(receipt, "editor_runtime_surface_count",
                             fields.editorRuntimeSurfaceCount);
  iggy3d::appendReceiptField(receipt, "editor_runtime_traversal_slot_count",
                             fields.editorRuntimeTraversalSlotCount);
  iggy3d::appendReceiptField(receipt, "editor_apply_count", fields.editorApplyCount);
  iggy3d::appendReceiptField(receipt, "editor_delete_count", fields.editorDeleteCount);
  iggy3d::appendReceiptField(receipt, "editor_undo_count", fields.editorUndoCount);
  iggy3d::appendReceiptField(receipt, "editor_redo_count", fields.editorRedoCount);
  iggy3d::appendReceiptField(receipt, "editor_bake_ok", fields.editorBakeOk);
  iggy3d::appendReceiptField(receipt, "editor_bake_reason", fields.editorBakeReason);
  iggy3d::appendReceiptField(receipt, "codex_control_configured",
                             fields.codexControlConfigured);
  iggy3d::appendReceiptField(receipt, "codex_control_read", fields.codexControlRead);
  iggy3d::appendReceiptField(receipt, "codex_control_applied", fields.codexControlApplied);
  iggy3d::appendReceiptField(receipt, "codex_control_status", fields.codexControlStatus);
  iggy3d::appendReceiptField(receipt, "codex_control_path", fields.codexControlPath);
  iggy3d::appendReceiptField(receipt, "hit_surface_id", fields.hitSurfaceId);
  iggy3d::appendReceiptField(receipt, "mouse_look_available", fields.mouseLookAvailable);
  iggy3d::appendReceiptField(receipt, "mouse_look_used", fields.mouseLookUsed);
  iggy3d::appendReceiptField(receipt, "gamepad_available", fields.gamepadAvailable);
  iggy3d::appendReceiptField(receipt, "gamepad_name", fields.gamepadName);
  iggy3d::appendReceiptField(receipt, "gamepad_mapping", fields.gamepadMapping);
  iggy3d::appendReceiptField(receipt, "gamepad_left_stick_used", fields.gamepadLeftStickUsed);
  iggy3d::appendReceiptField(receipt, "gamepad_right_stick_used", fields.gamepadRightStickUsed);
  iggy3d::appendReceiptField(receipt, "gamepad_action_button", fields.gamepadActionButton);
}

#if defined(IGGY3D_HAS_SDL3)
struct GamepadSession {
  SDL_Gamepad* gamepad = nullptr;
  SDL_Joystick* joystick = nullptr;
  bool subsystemInitialized = false;
  bool crossDown = false;
  bool eastDown = false;
  bool startDown = false;
  bool r2Down = false;
  bool r1Down = false;
};

bool nameLooksLikeDualSense(std::string_view name) {
  return name.find("DualSense") != std::string_view::npos ||
         name.find("PS5") != std::string_view::npos ||
         name.find("Sony") != std::string_view::npos ||
         name.find("Wireless Controller") != std::string_view::npos;
}

GamepadSession openFirstGamepad(PlayableReceiptFields& fields) {
  GamepadSession session;
  if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
    return session;
  }
  session.subsystemInitialized = true;
  int count = 0;
  SDL_JoystickID* ids = SDL_GetGamepads(&count);
  if (ids != nullptr && count > 0) {
    session.gamepad = SDL_OpenGamepad(ids[0]);
  }
  SDL_free(ids);
  if (session.gamepad != nullptr) {
    fields.gamepadAvailable = true;
    const char* name = SDL_GetGamepadName(session.gamepad);
    fields.gamepadName = name == nullptr ? "sdl_gamepad" : name;
    fields.gamepadMapping =
        nameLooksLikeDualSense(fields.gamepadName) ? "dualsense_default" : "sdl_gamepad";
    fields.gamepadActionButton = "cross";
  } else {
    int joystickCount = 0;
    SDL_JoystickID* joystickIds = SDL_GetJoysticks(&joystickCount);
    if (joystickIds != nullptr && joystickCount > 0) {
      session.joystick = SDL_OpenJoystick(joystickIds[0]);
    }
    SDL_free(joystickIds);
    if (session.joystick != nullptr) {
      fields.gamepadAvailable = true;
      const char* name = SDL_GetJoystickName(session.joystick);
      fields.gamepadName = name == nullptr ? "sdl_joystick" : name;
      fields.gamepadMapping = nameLooksLikeDualSense(fields.gamepadName)
                                  ? "dualsense_joystick_fallback"
                                  : "sdl_joystick_fallback";
      fields.gamepadActionButton = "button0";
    }
  }
  return session;
}

void closeGamepad(GamepadSession& session) {
  if (session.gamepad != nullptr) {
    SDL_CloseGamepad(session.gamepad);
    session.gamepad = nullptr;
  }
  if (session.joystick != nullptr) {
    SDL_CloseJoystick(session.joystick);
    session.joystick = nullptr;
  }
  if (session.subsystemInitialized) {
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    session.subsystemInitialized = false;
  }
}

float axisValue(Sint16 value) {
  constexpr float kDeadZone = 9000.0F;
  const float asFloat = static_cast<float>(value);
  if (std::abs(asFloat) < kDeadZone) {
    return 0.0F;
  }
  return std::clamp(asFloat / 32767.0F, -1.0F, 1.0F);
}

bool pressedEdge(bool current, bool& previous) {
  const bool edge = current && !previous;
  previous = current;
  return edge;
}

void applyMouseLook(float& yaw, float& pitch, PlayableReceiptFields& fields) {
  float mouseX = 0.0F;
  float mouseY = 0.0F;
  const SDL_MouseButtonFlags buttons = SDL_GetRelativeMouseState(&mouseX, &mouseY);
  fields.mouseLookAvailable = true;
  const bool dragging = (buttons & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK | SDL_BUTTON_MMASK)) != 0U;
  if (!dragging || !std::isfinite(mouseX) || !std::isfinite(mouseY)) {
    return;
  }

  const float dx = std::clamp(mouseX, -80.0F, 80.0F);
  const float dy = std::clamp(mouseY, -80.0F, 80.0F);
  if (dx == 0.0F && dy == 0.0F) {
    return;
  }

  constexpr float kMouseLookScale = 0.004F;
  yaw += dx * kMouseLookScale;
  pitch -= dy * kMouseLookScale;
  fields.mouseLookUsed = true;
}
#endif

iggy3d::Mat4 perspectiveMat4(float verticalFovRadians,
                             float aspect,
                             float nearPlane,
                             float farPlane) {
  const float f = 1.0F / std::tan(verticalFovRadians * 0.5F);
  iggy3d::Mat4 result{{{}}};
  result.m[0] = f / aspect;
  result.m[5] = -f;
  result.m[10] = farPlane / (nearPlane - farPlane);
  result.m[11] = -(farPlane * nearPlane) / (farPlane - nearPlane);
  result.m[14] = -1.0F;
  return result;
}

iggy3d::Mat4 viewFromCamera(iggy3d::Vec3 eye,
                            iggy3d::Vec3 forward,
                            iggy3d::Vec3 up) {
  const iggy3d::Vec3 f = normalizedOr(forward, {0.0F, 0.0F, -1.0F});
  const iggy3d::Vec3 r = normalizedOr(cross(f, up), {1.0F, 0.0F, 0.0F});
  const iggy3d::Vec3 u = cross(r, f);
  iggy3d::Mat4 result = iggy3d::identityMat4();
  result.m[0] = r.x;
  result.m[1] = r.y;
  result.m[2] = r.z;
  result.m[3] = -iggy3d::dot(r, eye);
  result.m[4] = u.x;
  result.m[5] = u.y;
  result.m[6] = u.z;
  result.m[7] = -iggy3d::dot(u, eye);
  result.m[8] = -f.x;
  result.m[9] = -f.y;
  result.m[10] = -f.z;
  result.m[11] = iggy3d::dot(f, eye);
  return result;
}

iggy3d::FrameInput makeFrame(const iggy3d::SceneProjectionResult& scene,
                             const iggy3d::DebugProjectionResult& debug,
                             std::uint64_t frameIndex,
                             std::uint32_t viewportWidth,
                             std::uint32_t viewportHeight,
                             bool firstPerson,
                             float yaw,
                             float pitch,
                             float eyeHeightMeters) {
  iggy3d::FrameInput frame;
  frame.viewport = {viewportWidth, viewportHeight,
                    static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight)};
  frame.clock = {scene.sourceTick == iggy3d::kInvalidCommandTick ? 0U : scene.sourceTick,
                 frameIndex, 0.0F, 1.0F / 60.0F};
  frame.camera.mode =
      firstPerson ? iggy3d::RenderCameraMode::FirstPerson : iggy3d::RenderCameraMode::ThirdPerson;
  iggy3d::Vec3 firstPersonEye{0.0F, eyeHeightMeters, 0.0F};
  if (firstPerson) {
    for (const iggy3d::SceneItem& item : scene.items) {
      if (item.kind == iggy3d::SceneItemKind::Player || item.stableName == "player") {
        firstPersonEye = item.transform.position + iggy3d::Vec3{0.0F, eyeHeightMeters, 0.0F};
        break;
      }
    }
  }
  const float cosPitch = std::cos(pitch);
  frame.camera.worldEye = firstPerson ? firstPersonEye : iggy3d::Vec3{0.0F, 2.0F, 5.0F};
  frame.camera.worldForward = firstPerson
                                  ? iggy3d::Vec3{std::sin(yaw) * cosPitch, std::sin(pitch),
                                                 -std::cos(yaw) * cosPitch}
                                  : iggy3d::Vec3{0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  frame.camera.viewFromWorld =
      viewFromCamera(frame.camera.worldEye, frame.camera.worldForward, frame.camera.worldUp);
  frame.camera.clipFromView =
      perspectiveMat4(68.0F * 3.14159265358979323846F / 180.0F,
                      frame.viewport.aspectRatio, frame.camera.nearPlane,
                      frame.camera.farPlane);
  frame.camera.clipFromWorld = frame.camera.clipFromView * frame.camera.viewFromWorld;
  frame.projections.scene = &scene;
  frame.projections.debug = &debug;
  return frame;
}

}  // namespace

int main(int argc, const char* const* argv) {
  const ParseResult parsed = parseOptions(argc, argv);
  if (!parsed.ok) {
    return printFailure(parsed.reason);
  }

  if (parsed.options.window) {
#if !defined(IGGY3D_HAS_SDL3)
    return printWindowSdlUnavailable();
#endif
  }

  iggy3d::PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = iggy3d::PackageMode::BuildTreeVisual;
  lookupConfig.shaderRootOverride = parsed.options.shaderRoot;
  lookupConfig.diagnosticsDirOverride = parsed.options.diagnosticsDir;
  lookupConfig.requireShaderRoot = parsed.options.strictVulkan;
  lookupConfig.requireGraphicsRuntime = parsed.options.requireRenderer;
  const iggy3d::PackageLookupResult lookup = iggy3d::resolvePackageRuntimeLookup(lookupConfig);
  if (lookup.outcome != iggy3d::RenderOutcome::Ok) {
    return printFailure(lookup.reason.code);
  }

  const std::filesystem::path fixturePath =
      resolveFixturePath(parsed.options.fixturePath, lookup.lookup);
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{fixturePath.string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok) {
    return printFailure("visual_demo_package_lookup_failed");
  }
  const bool roomAvailable = !package.rooms.empty();
  const iggy3d::RoomAsset baseRoom =
      roomAvailable ? package.rooms.front() : iggy3d::RoomAsset{};
  iggy3d::RoomAsset runtimeRoom = baseRoom;
  const iggy3d::RoomAsset* activeRoom = roomAvailable ? &runtimeRoom : nullptr;
  const iggy3d::Vec3 roomWorldOffset =
      activeRoom == nullptr ? iggy3d::Vec3{} : negated(roomOriginOffsetFromPlayerSpawn(*activeRoom));
  iggy3d::SpatialSurfaceSet collisionSurfaces =
      activeRoom == nullptr ? iggy3d::SpatialSurfaceSet{}
                            : iggy3d::buildSpatialSurfaceSet(*activeRoom, roomWorldOffset);

  iggy3d::SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  iggy3d::Result<iggy3d::Session> sessionResult = iggy3d::Session::create(create);
  if (sessionResult.status != iggy3d::ResultStatus::Ok) {
    return printFailure("visual_demo_package_lookup_failed");
  }
  iggy3d::Session session = std::move(sessionResult.value);

  iggy3d::RendererCreateInfo rendererCreate;
  rendererCreate.backend = parsed.options.autoBackend ? iggy3d::RendererBackendKind::Null
                                                      : parsed.options.backend;
  rendererCreate.config.renderer =
      rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan ? iggy3d::RendererMode::Vulkan
                                                                    : iggy3d::RendererMode::Null;
  rendererCreate.config.shaderRoot = lookup.lookup.shaderRoot;
  rendererCreate.config.diagnosticsDir = lookup.lookup.diagnosticsDir;
  rendererCreate.config.strictVulkan = parsed.options.strictVulkan;
  rendererCreate.config.allowSoftwareVulkan =
      rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan;
  rendererCreate.config.rendererRequirement =
      parsed.options.requireRenderer ? iggy3d::RendererRequirement::Required
                                     : iggy3d::RendererRequirement::Optional;

  WindowReceiptFields windowFields;
  windowFields.requested = parsed.options.window;
  std::uint32_t viewportWidth = 1280U;
  std::uint32_t viewportHeight = 720U;
#if defined(IGGY3D_HAS_SDL3)
  std::optional<iggy3d::SdlWindow> window;
#if defined(IGGY3D_APP_VULKAN_BACKEND)
  iggy3d::SdlVulkanSurfaceProvider sdlVulkanProvider;
#endif
  if (parsed.options.window) {
    windowFields.sdlAvailable = true;
    iggy3d::SdlWindowCreateInfo windowCreate;
    windowCreate.title = std::string(kVisualDemoWindowTitle);
    windowCreate.width = viewportWidth;
    windowCreate.height = viewportHeight;
    windowCreate.vulkan = rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan;
    window.emplace(windowCreate);
    windowFields.created = window->nativeWindow() != nullptr;
    windowFields.opened = window->isOpen();
    windowFields.drawable = window->isDrawable();
    const iggy3d::SdlWindowEventState& eventState = window->eventState();
    windowFields.windowWidth = eventState.windowWidth;
    windowFields.windowHeight = eventState.windowHeight;
    windowFields.drawableWidth = eventState.drawableWidth;
    windowFields.drawableHeight = eventState.drawableHeight;
    windowFields.quitRequested = eventState.quitRequested;
    if (!windowFields.created) {
      return printWindowUnavailable(windowFields);
    }
  }
#endif

  iggy3d::RendererApi renderer;
  iggy3d::RendererLifecycleState rendererLifecycleForReceipt =
      iggy3d::RendererLifecycleState::NotInitialized;
  bool vulkanSurfaceCreated = false;
  bool vulkanSwapchainReady = false;
  if (rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan) {
    if (!parsed.options.window) {
      iggy3d::RenderReceipt receipt = baseReceipt("fail", "vulkan_surface_provider_missing");
      iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
      appendWindowReceiptFields(receipt, windowFields);
      appendVulkanBootFields(receipt, rendererCreate.backend, false, false,
                             iggy3d::RendererLifecycleState::NotInitialized);
      return printReceiptAndReturn(std::move(receipt), 1);
    }
#if defined(IGGY3D_HAS_SDL3) && defined(IGGY3D_APP_VULKAN_BACKEND)
    if (!window.has_value() || !window->nativeWindow()) {
      return printWindowUnavailable(windowFields);
    }
    const iggy3d::SdlVulkanExtensionList extensions =
        sdlVulkanProvider.requiredInstanceExtensions(*window);
    if (extensions.outcome != iggy3d::RenderOutcome::Ok) {
      iggy3d::RenderReceipt receipt;
      appendReceiptFieldIfMissing(receipt, "receipt_version", "1");
      appendReceiptFieldIfMissing(receipt, "repo", "iggy3d");
      appendReceiptFieldIfMissing(receipt, "app", "iggy3d_visual_demo");
      appendReceiptFieldIfMissing(receipt, "visual_demo", "bounded");
      appendWindowReceiptFields(receipt, windowFields);
      appendVulkanBootFields(receipt, rendererCreate.backend, false, false,
                             iggy3d::RendererLifecycleState::NotInitialized);
      appendReceiptFieldIfMissing(receipt, "result", "fail");
      appendReceiptFieldIfMissing(receipt, "reason_code", extensions.reason.code);
      return printReceiptAndReturn(std::move(receipt), 1);
    }
    iggy3d::VulkanBackendCreateInfo backendInfo;
    backendInfo.config = rendererCreate.config;
    backendInfo.drawableWidth = windowFields.drawableWidth == 0U ? viewportWidth
                                                                 : windowFields.drawableWidth;
    backendInfo.drawableHeight = windowFields.drawableHeight == 0U ? viewportHeight
                                                                   : windowFields.drawableHeight;
    backendInfo.surfaceProvider.requiredInstanceExtensions = extensions.names;
    backendInfo.surfaceProvider.createSurface =
        [&sdlVulkanProvider, &window](VkInstance instance, VkSurfaceKHR* surface) {
          if (!window.has_value()) {
            iggy3d::RenderReceipt receipt;
            iggy3d::appendReceiptField(receipt, "surface_provider", "sdl3");
            iggy3d::appendReceiptField(receipt, "surface_created", false);
            iggy3d::appendReceiptField(receipt, "result", "fail");
            iggy3d::appendReceiptField(receipt, "reason_code", "sdl_window_missing");
            return receipt;
          }
          const iggy3d::SdlVulkanSurfaceCreateResult created =
              sdlVulkanProvider.createSurface(*window, instance);
          if (surface != nullptr) {
            *surface = created.surface;
          }
          iggy3d::RenderReceipt receipt;
          iggy3d::appendReceiptField(receipt, "surface_provider", "sdl3");
          iggy3d::appendReceiptField(receipt, "surface_created",
                                     created.outcome == iggy3d::RenderOutcome::Ok &&
                                         created.surface != VK_NULL_HANDLE);
          iggy3d::appendReceiptField(receipt, "result",
                                     created.outcome == iggy3d::RenderOutcome::Ok ? "pass"
                                                                                  : "fail");
          iggy3d::appendReceiptField(receipt, "reason_code", created.reason.code);
          return receipt;
        };
    renderer =
        iggy3d::RendererApi(std::make_unique<iggy3d::VulkanBackend>(std::move(backendInfo)));
    rendererLifecycleForReceipt = renderer.lifecycleState();
    vulkanSurfaceCreated = renderer.lifecycleState() == iggy3d::RendererLifecycleState::Ready;
    vulkanSwapchainReady = renderer.lifecycleState() == iggy3d::RendererLifecycleState::Ready;
    if (renderer.lifecycleState() != iggy3d::RendererLifecycleState::Ready) {
      iggy3d::RenderReceipt receipt = renderer.diagnostics();
      appendReceiptFieldIfMissing(receipt, "receipt_version", "1");
      appendReceiptFieldIfMissing(receipt, "repo", "iggy3d");
      appendReceiptFieldIfMissing(receipt, "app", "iggy3d_visual_demo");
      appendReceiptFieldIfMissing(receipt, "visual_demo", "bounded");
      appendWindowReceiptFields(receipt, windowFields);
      appendVulkanBootFields(receipt, rendererCreate.backend, vulkanSurfaceCreated,
                             vulkanSwapchainReady, renderer.lifecycleState());
      appendReceiptFieldIfMissing(receipt, "frames", static_cast<std::uint64_t>(0));
      appendReceiptFieldIfMissing(receipt, "frames_presented", static_cast<std::uint64_t>(0));
      appendReceiptFieldIfMissing(receipt, "result", "fail");
      appendReceiptFieldIfMissing(receipt, "reason_code", "visual_demo_renderer_unavailable");
      return printReceiptAndReturn(std::move(receipt), 1);
    }
#else
    iggy3d::RenderReceipt receipt = baseReceipt("fail", "visual_demo_renderer_unavailable");
    iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
    appendWindowReceiptFields(receipt, windowFields);
    appendVulkanBootFields(receipt, rendererCreate.backend, false, false,
                           iggy3d::RendererLifecycleState::NotInitialized);
    return printReceiptAndReturn(std::move(receipt), 1);
#endif
  } else {
    renderer = iggy3d::createRenderer(rendererCreate);
    rendererLifecycleForReceipt = renderer.lifecycleState();
    if (!renderer.hasBackend() && parsed.options.requireRenderer) {
      return printFailure("visual_demo_renderer_unavailable");
    }
  }

  PlayableReceiptFields playableFields;
  playableFields.playable = parsed.options.interactive || parsed.options.scriptedPlayableSmoke;
  playableFields.interactiveMode = parsed.options.interactive;
  playableFields.inputBackend = parsed.options.inputBackend;
  playableFields.saveLoadReplayStable = parsed.options.scriptedPlayableSmoke;
  if (playableFields.playable) {
    recordStance(playableFields, false);
    recordTrainingDummyCombat(playableFields, session);
  }
  DevMenuState devMenu;
  devMenu.enabled = parsed.options.devMenu || parsed.options.codexControlPathSet;
  devMenu.open = parsed.options.devMenu;
  EditorModeState editor;
  editor.enabled = playableFields.playable && !parsed.options.scriptedPlayableSmoke;
  editor.cursorWorldMeters = {};
  if (editor.enabled && activeRoom != nullptr) {
    rebuildEditorRuntimeRoom(baseRoom, editor, runtimeRoom, collisionSurfaces, roomWorldOffset);
  }
  playableFields.devMenuEnabled = devMenu.enabled;
  playableFields.devMenuOpen = devMenu.open;
  playableFields.devMenuSelectedMechanic = std::string(devMechanicName(devMenu.selected));
  updateEditorReceiptFields(playableFields, editor);
  playableFields.codexControlConfigured = parsed.options.codexControlPathSet;
  playableFields.codexControlPath =
      parsed.options.codexControlPathSet ? parsed.options.codexControlPath.string()
                                         : "unavailable";
  playableFields.codexControlStatus =
      parsed.options.codexControlPathSet ? "not_read" : "disabled";
  playableFields.kinematicControllerActive =
      playableFields.playable && !parsed.options.scriptedPlayableSmoke && !collisionSurfaces.empty();
  iggy3d::PlayerMotorState playerMotor;
  if (const iggy3d::EntityState* player = playerEntity(session)) {
    playerMotor.actor = player->id;
  }
  bool debugOverlayOpen = false;
  std::optional<iggy3d::Vec3> debugPreviousPosition;
  std::optional<iggy3d::Vec3> debugSpawnPosition;
  if (const iggy3d::EntityState* player = playerEntity(session)) {
    debugPreviousPosition = player->transform.position;
    debugSpawnPosition = player->transform.position;
  }
  std::optional<iggy3d::MovementResult> lastMovementResult;
  std::optional<iggy3d::PlayerMotorResult> lastMotorResult;
#if defined(IGGY3D_HAS_SDL3)
  GamepadSession gamepad;
  if (playableFields.playable &&
      (parsed.options.inputBackend == VisualInputBackend::Gamepad ||
       parsed.options.inputBackend == VisualInputBackend::Auto)) {
    gamepad = openFirstGamepad(playableFields);
    if (parsed.options.inputBackend == VisualInputBackend::Auto) {
      playableFields.inputBackend =
          playableFields.gamepadAvailable ? VisualInputBackend::Gamepad
                                          : VisualInputBackend::Keyboard;
    }
    if (parsed.options.inputBackend == VisualInputBackend::Gamepad &&
        !playableFields.gamepadAvailable) {
      playableFields.inputBackend = VisualInputBackend::Keyboard;
    }
  }
#endif

  iggy3d::RenderSubmitResult submit;
  std::uint32_t framesPresented = 0U;
  float yaw = 0.0F;
  float pitch = 0.0F;
  bool quitRequested = false;
  bool devMenuToggleDown = false;
  bool devMenuNextDown = false;
  bool devMenuPreviousDown = false;
  bool devMenuExecuteDown = false;
  bool editorToggleDown = false;
  bool editorNextDown = false;
  bool editorPreviousDown = false;
  bool editorPresetDown = false;
  bool editorApplyDown = false;
  bool editorDeleteDown = false;
  bool editorUndoDown = false;
  bool editorRedoDown = false;
  bool spellFireDown = false;
  bool debugOverlayToggleDown = false;
  const auto interactiveStart = std::chrono::steady_clock::now();
  std::uint32_t frameIndex = 0U;
  while (true) {
    if (hasFrameLimit(parsed.options) && frameIndex >= parsed.options.frames) {
      break;
    }
    if (playableFields.playable && parsed.options.holdSeconds > 0U) {
      const auto elapsed = std::chrono::steady_clock::now() - interactiveStart;
      if (elapsed >= std::chrono::seconds(parsed.options.holdSeconds)) {
        break;
      }
    }
    std::optional<iggy3d::TraversalResult> frameTraversalResult;
    std::optional<iggy3d::TraversalIntentResult> frameTraversalIntentResult;
    std::optional<iggy3d::TraversalCandidatePreviewResult> frameTraversalPreviewResult;
#if defined(IGGY3D_HAS_SDL3)
    if (window.has_value()) {
      window->pollEvents();
      ++windowFields.eventPollCount;
      const iggy3d::SdlWindowEventState& eventState = window->eventState();
      windowFields.opened = window->isOpen();
      windowFields.drawable = window->isDrawable();
      windowFields.windowWidth = eventState.windowWidth;
      windowFields.windowHeight = eventState.windowHeight;
      windowFields.drawableWidth = eventState.drawableWidth;
      windowFields.drawableHeight = eventState.drawableHeight;
      windowFields.quitRequested = eventState.quitRequested;
      if (!window->isOpen()) {
        break;
      }
      if (playableFields.playable && windowFields.quitRequested) {
        break;
      }
      if (windowFields.drawableWidth > 0U && windowFields.drawableHeight > 0U) {
        viewportWidth = windowFields.drawableWidth;
        viewportHeight = windowFields.drawableHeight;
      }
    }
#endif
    if (parsed.options.scriptedPlayableSmoke) {
      if (!runScriptedPlayableStep(session, frameIndex, playableFields)) {
        iggy3d::RenderReceipt receipt = baseReceipt("fail", "visual_demo_scripted_action_failed");
        appendWindowReceiptFields(receipt, windowFields);
        appendPlayableReceiptFields(receipt, playableFields);
        return printReceiptAndReturn(std::move(receipt), 1);
      }
    }
#if defined(IGGY3D_HAS_SDL3)
    if (playableFields.playable && !parsed.options.scriptedPlayableSmoke) {
      if (gamepad.gamepad != nullptr) {
        SDL_UpdateGamepads();
      }
      if (gamepad.joystick != nullptr) {
        SDL_UpdateJoysticks();
      }
      const CodexControlFrame codexControl = readCodexControlFile(parsed.options);
      if (codexControl.configured) {
        playableFields.codexControlRead = playableFields.codexControlRead || codexControl.read;
        playableFields.codexControlApplied =
            playableFields.codexControlApplied || codexControl.applied;
        playableFields.codexControlStatus = codexControl.status;
      }
      const bool codexExecuteThisFrame =
          controlFrameListed(codexControl.executeMechanicFrames, frameIndex);
      const bool codexTacticalToggleThisFrame =
          controlFrameListed(codexControl.tacticalToggleFrames, frameIndex);
      const bool codexPauseThisFrame = controlFrameListed(codexControl.pauseFrames, frameIndex);
      const bool codexStepThisFrame = controlFrameListed(codexControl.stepFrames, frameIndex);
      const bool codexResumeThisFrame =
          controlFrameListed(codexControl.resumeFrames, frameIndex);
      const bool codexSaveLoadThisFrame =
          controlFrameListed(codexControl.saveLoadFrames, frameIndex);
      const bool codexResetThisFrame = controlFrameListed(codexControl.resetFrames, frameIndex);
      const bool codexAcceptanceDemo = codexControl.applied && codexControl.acceptanceDemo;
      const bool codexEditorApplyThisFrame =
          controlFrameListed(codexControl.editorApplyFrames, frameIndex);
      const bool codexEditorDeleteThisFrame =
          controlFrameListed(codexControl.editorDeleteFrames, frameIndex);
      const bool codexEditorUndoThisFrame =
          controlFrameListed(codexControl.editorUndoFrames, frameIndex);
      const bool codexEditorRedoThisFrame =
          controlFrameListed(codexControl.editorRedoFrames, frameIndex);
      iggy3d::Vec3 movement{};
      bool actionRequested = false;
      bool attackRequested = false;
      bool resetRequested = false;
      bool tacticalToggleRequested = false;
      bool pauseRequested = false;
      bool stepRequested = false;
      bool resumeRequested = false;
      bool saveLoadRequested = false;
      bool jumpRequested = false;
      bool dashRequested = false;
      bool crouchHeld = parsed.options.scriptedCrouchInput;
      bool spellFireRequested = false;
      bool devToggleRequested = false;
      bool devNextRequested = false;
      bool devPreviousRequested = false;
      bool devExecuteRequested = false;
      bool devExecuteThisFrame = false;
      bool editorToggleRequested = false;
      bool editorNextRequested = false;
      bool editorPreviousRequested = false;
      bool editorPresetRequested = false;
      bool editorApplyRequested = false;
      bool editorDeleteRequested = false;
      bool editorUndoRequested = false;
      bool editorRedoRequested = false;
      bool debugOverlayToggleRequested = false;
      if (parsed.options.scriptedKinematicInput) {
        movement = {1.0F, 0.0F, 0.0F};
      } else {
        const bool keyboardInputEnabled =
            playableFields.inputBackend == VisualInputBackend::Keyboard ||
            playableFields.inputBackend == VisualInputBackend::Gamepad ||
            parsed.options.inputBackend == VisualInputBackend::Auto;
        if (keyboardInputEnabled) {
          int keyCount = 0;
          const bool* keys = SDL_GetKeyboardState(&keyCount);
          if (keys != nullptr) {
            const bool w = SDL_SCANCODE_W < keyCount && keys[SDL_SCANCODE_W];
            const bool s = SDL_SCANCODE_S < keyCount && keys[SDL_SCANCODE_S];
            const bool a = SDL_SCANCODE_A < keyCount && keys[SDL_SCANCODE_A];
            const bool d = SDL_SCANCODE_D < keyCount && keys[SDL_SCANCODE_D];
            const bool left = SDL_SCANCODE_LEFT < keyCount && keys[SDL_SCANCODE_LEFT];
            const bool right = SDL_SCANCODE_RIGHT < keyCount && keys[SDL_SCANCODE_RIGHT];
            const bool up = SDL_SCANCODE_UP < keyCount && keys[SDL_SCANCODE_UP];
            const bool down = SDL_SCANCODE_DOWN < keyCount && keys[SDL_SCANCODE_DOWN];
            const bool fireSpell = SDL_SCANCODE_F < keyCount && keys[SDL_SCANCODE_F];
            devToggleRequested = SDL_SCANCODE_F1 < keyCount && keys[SDL_SCANCODE_F1];
            editorToggleRequested = SDL_SCANCODE_F2 < keyCount && keys[SDL_SCANCODE_F2];
            debugOverlayToggleRequested =
                SDL_SCANCODE_F3 < keyCount && keys[SDL_SCANCODE_F3];
            if (editor.enabled && editor.open) {
              editorApplyRequested =
                  (SDL_SCANCODE_SPACE < keyCount && keys[SDL_SCANCODE_SPACE]) ||
                  (SDL_SCANCODE_RETURN < keyCount && keys[SDL_SCANCODE_RETURN]);
              editorDeleteRequested =
                  (SDL_SCANCODE_DELETE < keyCount && keys[SDL_SCANCODE_DELETE]) ||
                  (SDL_SCANCODE_BACKSPACE < keyCount && keys[SDL_SCANCODE_BACKSPACE]);
              editorPresetRequested = SDL_SCANCODE_TAB < keyCount && keys[SDL_SCANCODE_TAB];
              editorUndoRequested = SDL_SCANCODE_Z < keyCount && keys[SDL_SCANCODE_Z];
              editorRedoRequested = SDL_SCANCODE_Y < keyCount && keys[SDL_SCANCODE_Y];
              editorPreviousRequested = SDL_SCANCODE_Q < keyCount && keys[SDL_SCANCODE_Q];
              editorNextRequested = SDL_SCANCODE_E < keyCount && keys[SDL_SCANCODE_E];
              if (SDL_SCANCODE_1 < keyCount && keys[SDL_SCANCODE_1]) {
                editor.tool = EditorTool::Select;
              }
              if (SDL_SCANCODE_2 < keyCount && keys[SDL_SCANCODE_2]) {
                editor.tool = EditorTool::PlaceFloor;
              }
              if (SDL_SCANCODE_3 < keyCount && keys[SDL_SCANCODE_3]) {
                editor.tool = EditorTool::PlaceWall;
              }
              if (SDL_SCANCODE_4 < keyCount && keys[SDL_SCANCODE_4]) {
                editor.tool = EditorTool::Semantics;
              }
              if (SDL_SCANCODE_5 < keyCount && keys[SDL_SCANCODE_5]) {
                editor.tool = EditorTool::Delete;
              }
            } else if (devMenu.enabled && devMenu.open) {
              devExecuteRequested =
                  (SDL_SCANCODE_SPACE < keyCount && keys[SDL_SCANCODE_SPACE]) ||
                  (SDL_SCANCODE_RETURN < keyCount && keys[SDL_SCANCODE_RETURN]);
              if (SDL_SCANCODE_1 < keyCount && keys[SDL_SCANCODE_1]) {
                devMenu.selected = DevMechanic::Walk;
              }
              if (SDL_SCANCODE_2 < keyCount && keys[SDL_SCANCODE_2]) {
                devMenu.selected = DevMechanic::Crouch;
              }
              if (SDL_SCANCODE_3 < keyCount && keys[SDL_SCANCODE_3]) {
                devMenu.selected = DevMechanic::Jump;
              }
              if (SDL_SCANCODE_4 < keyCount && keys[SDL_SCANCODE_4]) {
                devMenu.selected = DevMechanic::Dash;
              }
              if (SDL_SCANCODE_5 < keyCount && keys[SDL_SCANCODE_5]) {
                devMenu.selected = DevMechanic::Spell;
              }
              if (SDL_SCANCODE_6 < keyCount && keys[SDL_SCANCODE_6]) {
                devMenu.selected = DevMechanic::Vault;
              }
              if (SDL_SCANCODE_7 < keyCount && keys[SDL_SCANCODE_7]) {
                devMenu.selected = DevMechanic::Clamber;
              }
              if (SDL_SCANCODE_8 < keyCount && keys[SDL_SCANCODE_8]) {
                devMenu.selected = DevMechanic::WireWalk;
              }
            } else {
              jumpRequested = SDL_SCANCODE_SPACE < keyCount && keys[SDL_SCANCODE_SPACE];
              dashRequested =
                  (SDL_SCANCODE_LSHIFT < keyCount && keys[SDL_SCANCODE_LSHIFT]) ||
                  (SDL_SCANCODE_RSHIFT < keyCount && keys[SDL_SCANCODE_RSHIFT]);
            }
            if (devMenu.enabled && !devMenu.open && pressedEdge(fireSpell, spellFireDown)) {
              spellFireRequested = true;
            }
            crouchHeld =
                crouchHeld || (SDL_SCANCODE_LCTRL < keyCount && keys[SDL_SCANCODE_LCTRL]) ||
                (SDL_SCANCODE_C < keyCount && keys[SDL_SCANCODE_C]);
            if (left) {
              yaw -= 0.035F;
            }
            if (right) {
              yaw += 0.035F;
            }
            if (up) {
              pitch += 0.020F;
            }
            if (down) {
              pitch -= 0.020F;
            }
            const iggy3d::Vec3 forward{std::sin(yaw), 0.0F, -std::cos(yaw)};
            const iggy3d::Vec3 rightVec{std::cos(yaw), 0.0F, std::sin(yaw)};
            movement = movement + forward * (w ? 1.0F : 0.0F);
            movement = movement - forward * (s ? 1.0F : 0.0F);
            movement = movement - rightVec * (a ? 1.0F : 0.0F);
            movement = movement + rightVec * (d ? 1.0F : 0.0F);
            actionRequested = SDL_SCANCODE_E < keyCount && keys[SDL_SCANCODE_E];
            resetRequested = SDL_SCANCODE_R < keyCount && keys[SDL_SCANCODE_R];
            quitRequested = SDL_SCANCODE_ESCAPE < keyCount && keys[SDL_SCANCODE_ESCAPE];
          }
        }
        if (playableFields.inputBackend == VisualInputBackend::Gamepad &&
            (gamepad.gamepad != nullptr || gamepad.joystick != nullptr)) {
          float leftX = 0.0F;
          float leftY = 0.0F;
          float rightX = 0.0F;
          float rightY = 0.0F;
          float r2 = 0.0F;
          bool slowLook = false;
          bool crossDown = false;
          bool eastDown = false;
          bool startDown = false;
          bool r2Down = false;
          bool r1Down = false;
          bool crouchDown = false;
          bool northDown = false;
          bool dpadLeft = false;
          bool dpadRight = false;
          if (gamepad.gamepad != nullptr) {
            leftX = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_LEFTX));
            leftY = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_LEFTY));
            rightX = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_RIGHTX));
            rightY = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_RIGHTY));
            r2 = axisValue(SDL_GetGamepadAxis(gamepad.gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
            slowLook = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
            crossDown = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
            eastDown = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_EAST);
            startDown = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_START);
            r1Down = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
            crouchDown = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK);
            northDown = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_NORTH);
            dpadLeft = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
            dpadRight = SDL_GetGamepadButton(gamepad.gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
            r2Down = r2 > 0.2F;
          } else {
            const int axisCount = SDL_GetNumJoystickAxes(gamepad.joystick);
            const int buttonCount = SDL_GetNumJoystickButtons(gamepad.joystick);
            leftX =
                axisCount > 0 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 0)) : 0.0F;
            leftY =
                axisCount > 1 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 1)) : 0.0F;
            rightX =
                axisCount > 2 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 2)) : 0.0F;
            rightY =
                axisCount > 3 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 3)) : 0.0F;
            r2 = axisCount > 5 ? axisValue(SDL_GetJoystickAxis(gamepad.joystick, 5)) : 0.0F;
            slowLook = buttonCount > 4 && SDL_GetJoystickButton(gamepad.joystick, 4);
            crossDown = buttonCount > 0 && SDL_GetJoystickButton(gamepad.joystick, 0);
            eastDown = buttonCount > 1 && SDL_GetJoystickButton(gamepad.joystick, 1);
            startDown =
                buttonCount > 9
                    ? SDL_GetJoystickButton(gamepad.joystick, 9)
                    : (buttonCount > 7 && SDL_GetJoystickButton(gamepad.joystick, 7));
            r1Down = buttonCount > 5 && SDL_GetJoystickButton(gamepad.joystick, 5);
            crouchDown = buttonCount > 10 && SDL_GetJoystickButton(gamepad.joystick, 10);
            r2Down =
                r2 > 0.2F || (buttonCount > 7 && SDL_GetJoystickButton(gamepad.joystick, 7));
          }
          if (editor.enabled && startDown && eastDown) {
            editorToggleRequested = true;
            eastDown = false;
            startDown = false;
          }
          if (devMenu.enabled && !editor.open && startDown && northDown) {
            devToggleRequested = true;
            startDown = false;
          }
          if (editor.enabled && editor.open) {
            editorPreviousRequested = dpadLeft;
            editorNextRequested = dpadRight;
            editorPresetRequested = northDown;
            editorApplyRequested = crossDown;
            editorDeleteRequested = eastDown;
            crossDown = false;
            eastDown = false;
            northDown = false;
          } else if (devMenu.enabled && devMenu.open) {
            devPreviousRequested = dpadLeft;
            devNextRequested = dpadRight;
            devExecuteRequested = crossDown;
            crossDown = false;
          }
          crouchHeld = crouchHeld || crouchDown;
          playableFields.gamepadLeftStickUsed =
              playableFields.gamepadLeftStickUsed || leftX != 0.0F || leftY != 0.0F;
          playableFields.gamepadRightStickUsed =
              playableFields.gamepadRightStickUsed || rightX != 0.0F || rightY != 0.0F;
          const float lookScale = slowLook ? 0.020F : 0.045F;
          yaw += rightX * lookScale;
          pitch -= rightY * lookScale;
          const iggy3d::Vec3 forward{std::sin(yaw), 0.0F, -std::cos(yaw)};
          const iggy3d::Vec3 rightVec{std::cos(yaw), 0.0F, std::sin(yaw)};
          movement = movement + forward * (-leftY);
          movement = movement + rightVec * leftX;
          const bool crossPressed = pressedEdge(crossDown, gamepad.crossDown);
          jumpRequested = jumpRequested || crossPressed;
          dashRequested = dashRequested || pressedEdge(r1Down, gamepad.r1Down);
          attackRequested = pressedEdge(r2Down, gamepad.r2Down);
          if (attackRequested) {
            playableFields.gamepadActionButton = "r2";
          }
          resetRequested = pressedEdge(eastDown, gamepad.eastDown);
          quitRequested = pressedEdge(startDown, gamepad.startDown);
        }
      }
      applyMouseLook(yaw, pitch, playableFields);
      if (codexControl.applied) {
        if (codexControl.devMenuOpenSet) {
          devMenu.open = codexControl.devMenuOpen;
        }
        if (codexControl.debugOverlayOpenSet) {
          debugOverlayOpen = codexControl.debugOverlayOpen;
        }
        if (editor.enabled) {
          if (codexControl.editorOpenSet) {
            editor.open = codexControl.editorOpen;
            if (editor.open) {
              devMenu.open = false;
            }
          }
          if (codexControl.editorToolSet) {
            editor.tool = codexControl.editorTool;
          }
          if (codexControl.editorPresetSet) {
            editor.preset = codexControl.editorPreset;
          }
          if (codexControl.editorCursorSet) {
            editor.cursorWorldMeters = snappedEditorCursor(codexControl.editorCursorMeters);
          }
          if (codexControl.editorSelectSet) {
            editor.selectedId = codexControl.editorSelectId;
          }
        }
        if (codexControl.mechanicSet) {
          devMenu.selected = codexControl.mechanic;
        }
        if (codexControl.yawSet) {
          yaw = codexControl.yaw;
        }
        if (codexControl.pitchSet) {
          pitch = codexControl.pitch;
        }
        yaw += codexControl.yawDelta;
        pitch += codexControl.pitchDelta;
        if (codexControl.moveSet) {
          const iggy3d::Vec3 forward{std::sin(yaw), 0.0F, -std::cos(yaw)};
          const iggy3d::Vec3 rightVec{std::cos(yaw), 0.0F, std::sin(yaw)};
          movement = movement + forward * codexControl.moveForward;
          movement = movement + rightVec * codexControl.moveRight;
        }
        if (codexControl.stanceSet) {
          crouchHeld = codexControl.crouched;
        }
        actionRequested = actionRequested || codexControl.interact;
        attackRequested = attackRequested || codexControl.attack;
        tacticalToggleRequested = tacticalToggleRequested || codexTacticalToggleThisFrame;
        pauseRequested = pauseRequested || codexPauseThisFrame;
        stepRequested = stepRequested || codexStepThisFrame;
        resumeRequested = resumeRequested || codexResumeThisFrame;
        saveLoadRequested = saveLoadRequested || codexSaveLoadThisFrame;
        resetRequested = resetRequested || codexControl.reset || codexResetThisFrame;
        jumpRequested = jumpRequested || codexControl.jump;
        dashRequested = dashRequested || codexControl.dash;
        quitRequested = quitRequested || codexControl.quit;
        devExecuteRequested =
            devExecuteRequested || codexControl.executeMechanic || codexExecuteThisFrame;
        editorApplyRequested = editorApplyRequested || codexControl.editorApply;
        editorDeleteRequested = editorDeleteRequested || codexControl.editorDelete;
        editorUndoRequested = editorUndoRequested || codexControl.editorUndo;
        editorRedoRequested = editorRedoRequested || codexControl.editorRedo;
      }
      if (editor.enabled && pressedEdge(editorToggleRequested, editorToggleDown)) {
        editor.open = !editor.open;
        if (editor.open) {
          devMenu.open = false;
        }
        playableFields.editorToggleObserved = true;
      }
      if (devMenu.enabled && pressedEdge(devToggleRequested, devMenuToggleDown)) {
        devMenu.open = !devMenu.open;
        if (devMenu.open) {
          editor.open = false;
        }
        playableFields.devMenuToggleObserved = true;
      }
      if (pressedEdge(debugOverlayToggleRequested, debugOverlayToggleDown)) {
        debugOverlayOpen = !debugOverlayOpen;
        playableFields.debugOverlayToggleObserved = true;
      }
      if (devMenu.enabled && devMenu.open &&
          pressedEdge(devPreviousRequested, devMenuPreviousDown)) {
        devMenu.selected = previousDevMechanic(devMenu.selected);
      }
      if (devMenu.enabled && devMenu.open && pressedEdge(devNextRequested, devMenuNextDown)) {
        devMenu.selected = nextDevMechanic(devMenu.selected);
      }
      if (editor.enabled && editor.open &&
          pressedEdge(editorPreviousRequested, editorPreviousDown)) {
        editor.tool = previousEditorTool(editor.tool);
      }
      if (editor.enabled && editor.open && pressedEdge(editorNextRequested, editorNextDown)) {
        editor.tool = nextEditorTool(editor.tool);
      }
      if (editor.enabled && editor.open && pressedEdge(editorPresetRequested, editorPresetDown)) {
        editor.preset = nextEditorPreset(editor.preset);
      }
      updateEditorProbe(editor, session, collisionSurfaces, yaw, pitch, crouchHeld,
                        codexControl.editorCursorSet);
      const bool editorApplyThisFrame =
          pressedEdge(editorApplyRequested, editorApplyDown) || codexEditorApplyThisFrame;
      const bool editorDeleteThisFrame =
          pressedEdge(editorDeleteRequested, editorDeleteDown) || codexEditorDeleteThisFrame;
      const bool editorUndoThisFrame =
          pressedEdge(editorUndoRequested, editorUndoDown) || codexEditorUndoThisFrame;
      const bool editorRedoThisFrame =
          pressedEdge(editorRedoRequested, editorRedoDown) || codexEditorRedoThisFrame;
      if (editor.enabled && editor.open) {
        debugOverlayOpen = true;
        if (editorApplyThisFrame) {
          playableFields.editorApplyRequested = true;
          executeEditorApply(editor, baseRoom, runtimeRoom, collisionSurfaces, roomWorldOffset,
                             yaw);
        }
        if (editorDeleteThisFrame) {
          playableFields.editorDeleteRequested = true;
          executeEditorDelete(editor, baseRoom, runtimeRoom, collisionSurfaces, roomWorldOffset);
        }
        if (editorUndoThisFrame) {
          playableFields.editorUndoRequested = true;
          executeEditorUndo(editor, baseRoom, runtimeRoom, collisionSurfaces, roomWorldOffset);
        }
        if (editorRedoThisFrame) {
          playableFields.editorRedoRequested = true;
          executeEditorRedo(editor, baseRoom, runtimeRoom, collisionSurfaces, roomWorldOffset);
        }
        updateEditorProbe(editor, session, collisionSurfaces, yaw, pitch, crouchHeld,
                          codexControl.editorCursorSet);
        movement = {};
        actionRequested = false;
        attackRequested = false;
        jumpRequested = false;
        dashRequested = false;
        spellFireRequested = false;
      }
      updateEditorReceiptFields(playableFields, editor);
      const bool devExecutePressed = pressedEdge(devExecuteRequested, devMenuExecuteDown);
      if (devMenu.enabled && (devExecutePressed || codexExecuteThisFrame)) {
        devMenu.executeRequested = true;
        devExecuteThisFrame = true;
      }
      if (devMenu.enabled && devMenu.selected == DevMechanic::Jump && devExecuteThisFrame) {
        jumpRequested = true;
      }
      if (devMenu.enabled && devMenu.selected == DevMechanic::Dash && devExecuteThisFrame) {
        dashRequested = true;
      }
      if (devMenu.enabled && devMenu.selected == DevMechanic::Spell &&
          devExecuteThisFrame) {
        spellFireRequested = true;
      }
      if (devMenu.enabled && devMenu.selected == DevMechanic::Spell && attackRequested) {
        spellFireRequested = true;
        attackRequested = false;
      }
      if (devMenu.enabled && !devMenu.open && attackRequested) {
        spellFireRequested = true;
        attackRequested = false;
      }
      if (devMenu.enabled && devMenu.selected == DevMechanic::Crouch &&
          devMenu.executeRequested) {
        crouchHeld = true;
      }
      playableFields.devMenuEnabled = devMenu.enabled;
      playableFields.devMenuOpen = devMenu.open;
      playableFields.devMenuSelectedMechanic = std::string(devMechanicName(devMenu.selected));
      playableFields.devMenuExecuteRequested = devMenu.executeRequested;
      if (devMenu.executeRequested) {
        playableFields.devMenuExecutionStatus =
            devMenu.selected == DevMechanic::Crouch
                ? "applied"
                : (devMenu.selected == DevMechanic::Walk
                       ? "selected"
                       : (devMenu.selected == DevMechanic::Jump ||
                                  devMenu.selected == DevMechanic::Dash ||
                                  devMenu.selected == DevMechanic::Spell
                              ? "pending"
                              : (devMenu.selected == DevMechanic::Vault ||
                                         devMenu.selected == DevMechanic::Clamber ||
                                         devMenu.selected == DevMechanic::WireWalk
                                     ? (playableFields.traversalAttempted
                                            ? (playableFields.traversalAccepted ? "applied"
                                                                               : "blocked")
                                            : "pending")
                                     : "stubbed")));
      } else {
        playableFields.devMenuExecutionStatus = "not_requested";
      }
      if (pitch > 0.8F) {
        pitch = 0.8F;
      }
      if (pitch < -0.8F) {
        pitch = -0.8F;
      }
      if (tacticalToggleRequested) {
        submitAndRecordControlCommand(session, iggy3d::CommandKind::ToggleTacticalMode,
                                      playableFields);
      }
      if (pauseRequested) {
        submitAndRecordControlCommand(session, iggy3d::CommandKind::Pause, playableFields);
      }
      if (stepRequested) {
        submitAndRecordControlCommand(session, iggy3d::CommandKind::StepTacticalTick,
                                      playableFields);
      }
      if (resumeRequested) {
        submitAndRecordControlCommand(session, iggy3d::CommandKind::Resume, playableFields);
      }
      playableFields.runtimeClockMode = std::string(clockModeName(session.state().clock.mode));
      recordStance(playableFields, crouchHeld);
      const iggy3d::EntityState* player = playerEntity(session);
      if (player != nullptr &&
          (!iggy3d::isValid(playerMotor.actor) || playerMotor.actor != player->id)) {
        playerMotor = iggy3d::PlayerMotorState{};
        playerMotor.actor = player->id;
      }
      if (player != nullptr && codexControl.applied && codexControl.playerPositionSet) {
        iggy3d::SessionState& mutableState = session.mutableStateForOwnedSystems();
        if (const iggy3d::EntityState* mutablePlayer =
                mutableState.world.findById(player->id)) {
          iggy3d::Transform3 transform = mutablePlayer->transform;
          transform.position = codexControl.playerPositionMeters;
          if (mutableState.world.updateTransform(player->id, transform).status ==
              iggy3d::WorldStatus::Ok) {
            playerMotor = iggy3d::PlayerMotorState{};
            playerMotor.actor = player->id;
            debugPreviousPosition = codexControl.playerPositionMeters;
            debugSpawnPosition = codexControl.playerPositionMeters;
            player = mutableState.world.findById(player->id);
          }
        }
      }
      if (debugOverlayOpen && player != nullptr) {
        frameTraversalPreviewResult =
            buildTraversalCandidatePreview(session, player->id, yaw, activeRoom,
                                           collisionSurfaces, roomWorldOffset);
      }
      if (player != nullptr && devMenu.enabled &&
          (devMenu.selected == DevMechanic::Vault ||
           devMenu.selected == DevMechanic::Clamber ||
           devMenu.selected == DevMechanic::WireWalk) &&
          devExecuteThisFrame) {
        iggy3d::SessionState& mutableState = session.mutableStateForOwnedSystems();
        iggy3d::TraversalRequest traversalRequest;
        traversalRequest.actor = player->id;
        switch (devMenu.selected) {
          case DevMechanic::Clamber:
            traversalRequest.mechanic = iggy3d::TraversalMechanic::Clamber;
            break;
          case DevMechanic::WireWalk:
            traversalRequest.mechanic = iggy3d::TraversalMechanic::WireWalk;
            break;
          default:
            traversalRequest.mechanic = iggy3d::TraversalMechanic::Vault;
            break;
        }
        traversalRequest.forward = {std::sin(yaw), 0.0F, -std::cos(yaw)};
        traversalRequest.room = activeRoom;
        traversalRequest.collisionSurfaces = &collisionSurfaces;
        traversalRequest.roomWorldOffsetMeters = roomWorldOffset;
        const iggy3d::TraversalResult traversalResult =
            iggy3d::executeTraversalMechanic(mutableState.world, traversalRequest);
        frameTraversalResult = traversalResult;
        recordTraversalResult(playableFields, traversalResult);
        playableFields.devMenuExecutionStatus =
            iggy3d::traversalApplied(traversalResult) ? "applied" : "blocked";
        if (iggy3d::traversalApplied(traversalResult)) {
          resetPlayerMotorAfterTraversal(playerMotor, player->id, traversalResult);
          movement = {};
          lastMovementResult.reset();
          lastMotorResult.reset();
          player = mutableState.world.findById(player->id);
        }
      }
      const bool devMenuOwnsInputThisFrame =
          devMenu.enabled && devMenu.open && devMenu.executeRequested;
      if (player != nullptr && (jumpRequested || actionRequested) &&
          !devMenuOwnsInputThisFrame) {
        iggy3d::SessionState& mutableState = session.mutableStateForOwnedSystems();
        iggy3d::TraversalIntentRequest traversalIntent;
        traversalIntent.actor = player->id;
        traversalIntent.jumpPressed = jumpRequested;
        traversalIntent.interactPressed = actionRequested;
        traversalIntent.forward = {std::sin(yaw), 0.0F, -std::cos(yaw)};
        traversalIntent.room = activeRoom;
        traversalIntent.collisionSurfaces = &collisionSurfaces;
        traversalIntent.roomWorldOffsetMeters = roomWorldOffset;
        const iggy3d::TraversalIntentResult traversalIntentResult =
            iggy3d::executeTraversalIntent(mutableState.world, traversalIntent);
        frameTraversalIntentResult = traversalIntentResult;
        if (traversalIntentResult.traversalAttempted) {
          frameTraversalResult = traversalIntentResult.traversal;
        }
        recordTraversalIntentResult(playableFields, traversalIntentResult);
        if (traversalIntentResult.consumedInput) {
          jumpRequested = false;
          actionRequested = false;
        }
        if (traversalIntentResult.accepted) {
          resetPlayerMotorAfterTraversal(playerMotor, player->id, traversalIntentResult.traversal);
          movement = {};
          lastMovementResult.reset();
          lastMotorResult.reset();
          player = mutableState.world.findById(player->id);
        }
      }
      if (player != nullptr && playerMotor.phase == iggy3d::PlayerMotorPhase::Grounded &&
          iggy3d::lengthSquared(movement) > 0.01F) {
        const float magnitudeSquared = iggy3d::lengthSquared(movement);
        if (magnitudeSquared > 1.0F) {
          movement = movement / std::sqrt(magnitudeSquared);
        }
        iggy3d::SessionState& mutableState = session.mutableStateForOwnedSystems();
        iggy3d::MovementSystemContext movementContext{
            &mutableState.world, &mutableState.config, &collisionSurfaces};
        iggy3d::KinematicMovementRequest movementRequest;
        movementRequest.actor = player->id;
        movementRequest.intent = movement;
        movementRequest.mode = iggy3d::MovementMode::Walk;
        movementRequest.params.maxSpeedMetersPerSecond =
            crouchHeld ? kCrouchedSpeedMetersPerSecond : kStandingSpeedMetersPerSecond;
        movementRequest.params.heightMeters =
            crouchHeld ? kCrouchedActorHeightMeters : kStandingActorHeightMeters;
        movementRequest.params.groundSnapMeters = 0.75F;
        movementRequest.seconds = 1.0F / 60.0F;
        movementRequest.sourceCommandId = iggy3d::kInvalidCommandId;
        const iggy3d::MovementResult movementResult =
            iggy3d::executeKinematicMovement(movementContext, movementRequest);
        lastMovementResult = movementResult;
        recordKinematicMovementResult(playableFields, movementResult);
      }
      if (resetRequested) {
        const iggy3d::SessionResetResult reset = session.resetToBaseline();
        recordResetResult(playableFields, session, reset);
        if (reset.reset) {
          playerMotor = iggy3d::PlayerMotorState{};
          if (const iggy3d::EntityState* resetPlayer = playerEntity(session)) {
            playerMotor.actor = resetPlayer->id;
            debugPreviousPosition = resetPlayer->transform.position;
            debugSpawnPosition = resetPlayer->transform.position;
          }
          lastMovementResult.reset();
          lastMotorResult.reset();
          recordAbilityPolicy(playableFields, session);
          recordAbilityProjectile(playableFields,
                                  session.state().transient.abilityRuntime.arcaneBolt);
          recordTrainingDummyCombat(playableFields, session);
        }
      }
      if (iggy3d::isValid(playerMotor.actor)) {
        iggy3d::SessionState& mutableState = session.mutableStateForOwnedSystems();
        iggy3d::PlayerMotorContext motorContext{&mutableState.world, &collisionSurfaces};
        iggy3d::PlayerMotorInput motorInput;
        motorInput.moveIntent = movement;
        motorInput.jumpPressed = jumpRequested;
        motorInput.dashPressed = dashRequested;
        motorInput.crouched = crouchHeld;
        motorInput.seconds = 1.0F / 60.0F;
        const iggy3d::PlayerMotorResult motorResult =
            iggy3d::updatePlayerMotor(motorContext, playerMotor, motorInput);
        lastMotorResult = motorResult;
        recordPlayerMotorResult(playableFields, motorResult);
        if (devMenu.enabled && devMenu.selected == DevMechanic::Jump &&
            devMenu.executeRequested) {
          playableFields.devMenuExecutionStatus =
              playableFields.jumpAccepted ? "applied" : "blocked";
        }
        if (devMenu.enabled && devMenu.selected == DevMechanic::Dash &&
            devMenu.executeRequested) {
          playableFields.devMenuExecutionStatus =
              playableFields.dashAccepted ? "applied" : "blocked";
        }
      } else if (jumpRequested || dashRequested) {
        playableFields.jumpInputObserved = playableFields.jumpInputObserved || jumpRequested;
        playableFields.dashInputObserved = playableFields.dashInputObserved || dashRequested;
        playableFields.playerMotorReason = "invalid_actor";
        if (devMenu.enabled &&
            (devMenu.selected == DevMechanic::Jump || devMenu.selected == DevMechanic::Dash) &&
            devMenu.executeRequested) {
          playableFields.devMenuExecutionStatus = "blocked";
        }
      }
      if (spellFireRequested) {
        submitAbilityCastCommand(session, yaw, pitch, playableFields);
        if (devMenu.enabled && devMenu.selected == DevMechanic::Spell &&
            devMenu.executeRequested) {
          playableFields.devMenuExecutionStatus =
              playableFields.abilityCastAccepted ? "applied" : "blocked";
        }
      }
      if (codexAcceptanceDemo) {
        runCodexAcceptanceDemoStep(session, create, collisionSurfaces, frameIndex, playableFields);
      } else {
        stepSessionAbilityProjectiles(session, collisionSurfaces, playableFields);
      }
      if (!codexAcceptanceDemo && saveLoadRequested) {
        const bool loaded = runSaveLoadRoundtrip(session, create, playableFields);
        if (loaded) {
          playerMotor = iggy3d::PlayerMotorState{};
          if (const iggy3d::EntityState* loadedPlayer = playerEntity(session)) {
            playerMotor.actor = loadedPlayer->id;
            debugPreviousPosition = loadedPlayer->transform.position;
          }
          lastMovementResult.reset();
          lastMotorResult.reset();
          recordAbilityPolicy(playableFields, session);
          recordAbilityProjectile(playableFields,
                                  session.state().transient.abilityRuntime.arcaneBolt);
          recordTrainingDummyCombat(playableFields, session);
        }
      }
      if (devMenu.enabled && devMenu.selected == DevMechanic::Spell &&
          devMenu.executeRequested) {
        playableFields.devMenuExecutionStatus =
            session.state().transient.abilityRuntime.arcaneBolt.spawned ? "applied" : "blocked";
      }
      if (actionRequested || attackRequested) {
        const iggy3d::EntityId key = entityIdByName(session, "gold_key");
        const iggy3d::EntityId dummy = entityIdByName(session, "training_dummy");
        const iggy3d::EntityState* keyEntity = session.state().world.findByStableName("gold_key");
        if (attackRequested || (keyEntity != nullptr && !keyEntity->active)) {
          playableFields.attackExecuted = submitAndDrain(session, attackCommand(dummy));
        } else {
          const iggy3d::SessionCommandResult interaction = session.submitCommand(interactCommand(key));
          playableFields.targetDiscovered = interaction.command.payload.target.hasEntity;
          playableFields.initialReachFailed =
              interaction.command.rejection == iggy3d::CommandRejectionReason::OutOfRange;
          if (accepted(interaction)) {
            const iggy3d::StatusResult run = session.runUntilIdle(8);
            playableFields.interactionExecuted = run.status == iggy3d::ResultStatus::Ok;
          }
        }
        playableFields.objectiveComplete = objectiveComplete(session);
      }
      recordTrainingDummyCombat(playableFields, session);
      playableFields.runtimeClockMode = std::string(clockModeName(session.state().clock.mode));
      if (quitRequested) {
        break;
      }
    }
#endif
    iggy3d::SceneProjectionResult scene = iggy3d::buildSceneProjection(session.state());
    attachRoomProjection(package, activeRoom, scene);
    attachEditorGhostProjection(editor, scene);
    attachAbilityProjectileProjection(session.state().transient.abilityRuntime.arcaneBolt, scene);
    iggy3d::DebugProjectionResult debug = iggy3d::buildDebugProjection(session.state());
    iggy3d::RuntimeDebugSnapshot debugSnapshot;
    if (playableFields.playable) {
      if (!frameTraversalPreviewResult.has_value() && debugOverlayOpen &&
          iggy3d::isValid(playerMotor.actor)) {
        frameTraversalPreviewResult =
            buildTraversalCandidatePreview(session, playerMotor.actor, yaw, activeRoom,
                                           collisionSurfaces, roomWorldOffset);
      }
      iggy3d::RuntimeDebugSnapshotRequest debugRequest;
      debugRequest.enabled = debugOverlayOpen;
      debugRequest.session = &session.state();
      debugRequest.actor = playerMotor.actor;
      debugRequest.hasPreviousPosition = debugPreviousPosition.has_value();
      debugRequest.previousPosition =
          debugPreviousPosition.has_value() ? *debugPreviousPosition : iggy3d::Vec3{};
      debugRequest.hasSpawnPosition = debugSpawnPosition.has_value();
      debugRequest.spawnPosition =
          debugSpawnPosition.has_value() ? *debugSpawnPosition : iggy3d::Vec3{};
      debugRequest.deltaSeconds = 1.0F / 60.0F;
      debugRequest.motorState = &playerMotor;
      debugRequest.motorResult = lastMotorResult.has_value() ? &*lastMotorResult : nullptr;
      debugRequest.movementResult =
          lastMovementResult.has_value() ? &*lastMovementResult : nullptr;
      debugRequest.traversalIntentResult =
          frameTraversalIntentResult.has_value() ? &*frameTraversalIntentResult : nullptr;
      debugRequest.traversalResult =
          frameTraversalResult.has_value() ? &*frameTraversalResult : nullptr;
      debugRequest.traversalPreviewResult =
          frameTraversalPreviewResult.has_value() ? &*frameTraversalPreviewResult : nullptr;
      debugRequest.yawRadians = yaw;
      debugRequest.pitchRadians = pitch;
      debugSnapshot = iggy3d::buildRuntimeDebugSnapshot(debugRequest);
      recordRuntimeDebugSnapshot(playableFields, debugSnapshot);
      iggy3d::appendRuntimeDebugSnapshot(debug, debugSnapshot);
      appendEditorDebugHudLines(debug, editor);
#if defined(IGGY3D_HAS_SDL3)
      if (window.has_value()) {
        window->setTitle(debugOverlayWindowTitle(debugSnapshot));
        playableFields.debugOverlayVisible = debugOverlayOpen;
        playableFields.debugTitleFallbackActive = debugOverlayOpen;
        playableFields.debugOverlaySurface =
            debugOverlayOpen && rendererCreate.backend == iggy3d::RendererBackendKind::Vulkan
                ? "vulkan_hud+window_title"
                : (debugOverlayOpen ? "window_title" : "closed");
      } else {
        playableFields.debugOverlayVisible = false;
        playableFields.debugTitleFallbackActive = false;
        playableFields.debugOverlaySurface =
            debugOverlayOpen ? "receipt_projection" : "closed";
      }
#else
      playableFields.debugOverlayVisible = false;
      playableFields.debugTitleFallbackActive = false;
      playableFields.debugOverlaySurface =
          debugOverlayOpen ? "receipt_projection" : "closed";
#endif
      if (debugSnapshot.playerPositionAvailable) {
        debugPreviousPosition = debugSnapshot.position;
      }
    }
    const float eyeHeightMeters =
        playableFields.crouchActive ? kCrouchedEyeHeightMeters : kStandingEyeHeightMeters;
    submit = renderer.submitFrame(makeFrame(scene, debug, frameIndex + 1U, viewportWidth,
                                            viewportHeight, playableFields.playable, yaw, pitch,
                                            eyeHeightMeters));
    if (submit.outcome != iggy3d::RenderOutcome::Ok) {
      iggy3d::RenderReceipt receipt = submit.receipt;
      appendReceiptFieldIfMissing(receipt, "receipt_version", "1");
      appendReceiptFieldIfMissing(receipt, "repo", "iggy3d");
      appendReceiptFieldIfMissing(receipt, "app", "iggy3d_visual_demo");
      appendReceiptFieldIfMissing(receipt, "visual_demo", "bounded");
      appendWindowReceiptFields(receipt, windowFields);
      appendPlayableReceiptFields(receipt, playableFields);
      appendVulkanBootFields(receipt, rendererCreate.backend, vulkanSurfaceCreated,
                             vulkanSwapchainReady, renderer.lifecycleState());
      appendReceiptFieldIfMissing(receipt, "frames",
                                  static_cast<std::uint64_t>(
                                      receiptFrameCount(parsed.options, framesPresented)));
      appendReceiptFieldIfMissing(receipt, "frames_presented",
                                  static_cast<std::uint64_t>(framesPresented));
      appendReceiptFieldIfMissing(receipt, "result", "fail");
      appendReceiptFieldIfMissing(receipt, "reason_code", "visual_demo_frame_failed");
      return printReceiptAndReturn(std::move(receipt), 1);
    }
    ++framesPresented;
    ++frameIndex;
    if (playableFields.playable) {
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
  }

#if defined(IGGY3D_HAS_SDL3)
  closeGamepad(gamepad);
  if (window.has_value()) {
    if (renderer.hasBackend()) {
      rendererLifecycleForReceipt = renderer.lifecycleState();
      static_cast<void>(renderer.waitIdle());
      renderer.shutdown();
    }
    const iggy3d::SdlWindowEventState& eventState = window->eventState();
    windowFields.opened = window->isOpen();
    windowFields.drawable = window->isDrawable();
    windowFields.windowWidth = eventState.windowWidth;
    windowFields.windowHeight = eventState.windowHeight;
    windowFields.drawableWidth = eventState.drawableWidth;
    windowFields.drawableHeight = eventState.drawableHeight;
    windowFields.quitRequested = eventState.quitRequested;
    window.reset();
    windowFields.closed = true;
  }
#endif

  if (framesPresented == 0U) {
    submit.outcome = iggy3d::RenderOutcome::Ok;
    submit.reason = {"null_renderer_ok", "null renderer ok"};
    iggy3d::appendReceiptField(submit.receipt, "receipt_version", "1");
    iggy3d::appendReceiptField(submit.receipt, "repo", "iggy3d");
    iggy3d::appendReceiptField(submit.receipt, "backend", "null");
    iggy3d::appendReceiptField(submit.receipt, "draw_count", static_cast<std::uint64_t>(0));
    iggy3d::appendReceiptField(submit.receipt, "frame_input_valid", "unavailable");
    iggy3d::appendReceiptField(submit.receipt, "result", "pass");
    iggy3d::appendReceiptField(submit.receipt, "reason_code", "null_renderer_ok");
  }

  iggy3d::RenderReceipt receipt = submit.receipt;
  iggy3d::appendReceiptField(receipt, "app", "iggy3d_visual_demo");
  iggy3d::appendReceiptField(receipt, "visual_demo", "bounded");
  appendWindowReceiptFields(receipt, windowFields);
  appendVulkanBootFields(receipt, rendererCreate.backend, vulkanSurfaceCreated, vulkanSwapchainReady,
                         rendererLifecycleForReceipt);
  appendPlayableReceiptFields(receipt, playableFields);
  iggy3d::appendReceiptField(receipt, "package_mode", iggy3d::packageModeName(lookup.lookup.packageMode));
  iggy3d::appendReceiptField(receipt, "resource_root_source", lookup.lookup.resourceRootSource);
  iggy3d::appendReceiptField(receipt, "shader_root_source", lookup.lookup.shaderRootSource);
  iggy3d::appendReceiptField(receipt, "diagnostics_dir_source", lookup.lookup.diagnosticsDirSource);
  iggy3d::appendReceiptField(receipt, "frames",
                             static_cast<std::uint64_t>(
                                 receiptFrameCount(parsed.options, framesPresented)));
  iggy3d::appendReceiptField(receipt, "frames_presented",
                             static_cast<std::uint64_t>(framesPresented));
  if (!iggy3d::hasReceiptField(receipt, "reason_code")) {
    iggy3d::appendReceiptField(receipt, "reason_code", "visual_demo_ok");
  }

  if (parsed.options.printReceipt) {
    std::cout << iggy3d::formatRenderReceipt(receipt);
  } else {
    std::cout << "visual_demo.status=ok\n";
    std::cout << "reason_code=visual_demo_ok\n";
  }
  return 0;
}
