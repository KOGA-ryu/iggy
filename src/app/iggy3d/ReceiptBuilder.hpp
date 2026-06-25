#pragma once

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/input/InputAction.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductActiveRoomState.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ProductViewportState.hpp"
#include "app/iggy3d/SaveBridge.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

struct ProductAppWindowState {
  bool requested = false;
  bool sdlAvailable = false;
  bool created = false;
  bool drawable = false;
  bool openingMenuVisible = false;
  bool menuTextDrawn = false;
  bool selectedRowDrawn = false;
  bool mouseMenuSelectUsed = false;
  bool gamepadAvailable = false;
  bool gamepadMenuSelectUsed = false;
  std::string gamepadName = "unavailable";
  std::string gamepadMapping = "unavailable";
  FrontendSettingsTab selectedSettingsTab = FrontendSettingsTab::None;
  bool runtimeSessionCreated = false;
  bool gameplayActive = false;
  std::string launchAction = "none";
  std::string launchStatus = "not_requested";
  std::string packageLoadStatus = "not_requested";
  std::string worldSetupTitle = "New World";
  std::string worldSetupStatus = "not_requested";
  std::string worldCreationStatus = "not_requested";
  std::string worldCreationReasonCode = "not_requested";
  std::string worldCreationWorldId = "none";
  std::string worldCreationWorldTitle = "none";
  bool worldCreationInitialSaveRequested = false;
  bool worldCreationInitialSaveWritten = false;
  std::string worldCreationInitialSaveId = "none";
  std::string worldCreationInitialSaveTitle = "none";
  std::string worldCreationRouteAfterCreate = "world_setup";
  std::string asciiRoomDraftText;
  std::string asciiRoomDraftRoomId = "ascii_preview";
  std::string asciiRoomDraftSourceName = "automation_ascii_room";
  std::string asciiRoomPreviewStatus = "not_requested";
  std::string asciiRoomPreviewReasonCode = "not_requested";
  std::string asciiRoomPreviewFailedStage = "not_started";
  std::string asciiRoomPreviewRoomId = "none";
  std::string asciiRoomPreviewSourceName = "none";
  bool asciiRoomPreviewReady = false;
  std::uint64_t asciiRoomPreviewWidth = 0;
  std::uint64_t asciiRoomPreviewHeight = 0;
  std::uint64_t asciiRoomPreviewFloorCount = 0;
  std::uint64_t asciiRoomPreviewWallCount = 0;
  std::uint64_t asciiRoomPreviewMarkerCount = 0;
  std::uint64_t asciiRoomPreviewElevatedFloorCount = 0;
  std::uint64_t asciiRoomPreviewRampCount = 0;
  std::uint64_t asciiRoomPreviewBlockedSlopeCount = 0;
  std::uint64_t asciiRoomPreviewStaticMeshCount = 0;
  std::uint64_t asciiRoomPreviewAnchorCount = 0;
  std::uint64_t asciiRoomPreviewSpatialSurfaceCount = 0;
  bool asciiRoomPreviewAssetTextWritten = false;
  std::uint64_t asciiRoomPreviewAssetTextBytes = 0;
  std::string asciiRoomActivationStatus = "not_requested";
  std::string asciiRoomActivationReasonCode = "not_requested";
  std::string asciiRoomActivationRoomId = "none";
  std::string asciiRoomActivationPackageId = "none";
  std::string asciiRoomActivationScenarioId = "none";
  bool asciiRoomActivationSessionCreated = false;
  bool asciiRoomActivationPlayerSpawned = false;
  std::uint64_t asciiRoomActivationPlayerCount = 0;
  std::uint64_t asciiRoomActivationEntityCount = 0;
  std::uint64_t asciiRoomActivationNpcCount = 0;
  std::uint64_t asciiRoomActivationPickupCount = 0;
  std::uint64_t asciiRoomActivationDoorCount = 0;
  std::uint64_t asciiRoomActivationMarkerEntityCount = 0;
  std::uint64_t asciiRoomActivationObjectiveCount = 0;
  std::uint64_t asciiRoomActivationWallCount = 0;
  std::uint64_t asciiRoomActivationMarkerCount = 0;
  std::uint64_t asciiRoomActivationRuntimeHash = 0;
  ProductActiveRoomState activeRoom;
  ProductActiveRoomCollisionState activeRoomCollision;
  std::string productSaveStatus = "not_requested";
  std::string productSaveReasonCode = "not_requested";
  std::string productSaveDurableReason = "not_requested";
  std::string productSaveSource = "none";
  std::string productSaveSaveId = "none";
  bool productSaveSessionSaved = false;
  std::string activeProductSaveId = "none";
  std::string productSaveLoadStatus = "not_requested";
  std::string productSaveLoadReasonCode = "not_requested";
  std::string productSaveLoadSaveId = "none";
  std::string productSaveLoadSource = "none";
  std::string productSaveLoadSelectedId = "none";
  bool productSaveLoadSelectedEnabled = false;
  std::string selectedProductSaveId = "none";
  bool selectedProductSaveEnabled = false;
  std::string selectedProductSaveStatus = "none";
  bool saveDeleteConfirmationOpen = false;
  std::string saveDeleteCandidateId = "none";
  bool saveDeleteCandidateEnabled = false;
  std::string saveDeleteStatus = "not_requested";
  std::string saveDeleteReasonCode = "not_requested";
  std::string saveDeleteType = "none";
  bool saveDeleteRecoverable = false;
  bool saveDeleteExecuted = false;
  bool deletedSaveBrowserOpen = false;
  std::uint64_t deletedSaveCount = 0;
  std::uint64_t deletedCompatibleSaveCount = 0;
  std::string deletedSelectedSaveId = "none";
  bool deletedSelectedSaveEnabled = false;
  std::string deletedSelectedSaveStatus = "none";
  std::string saveRecoverStatus = "not_requested";
  std::string saveRecoverReasonCode = "not_requested";
  bool saveRecoverExecuted = false;
  std::string saveRecoverSaveId = "none";
  bool saveRecoverSnapshotRecovered = false;
  bool saveRecoverSnapshotMissing = false;
  std::uint64_t productSaveLoadPreviousHash = 0;
  std::uint64_t productSaveLoadLoadedHash = 0;
  bool productSaveLoadSessionLoaded = false;
  std::uint64_t runtimeStateHash = 0;
  ProductViewportState viewport;
  std::uint64_t sceneItemCount = 0;
  std::uint64_t debugItemCount = 0;
  bool playerVisible = false;
  bool roomVisible = false;
  bool objectiveVisible = false;
  bool rendererMutatedRuntime = false;
  bool scriptedGameplaySmoke = false;
  bool gameplayInputUsed = false;
  bool gameplayCommandSubmitted = false;
  bool gameplayCommandAccepted = false;
  bool gameplayTickAdvanced = false;
  bool playerPositionChanged = false;
  bool gameplayMovementAttempted = false;
  bool gameplayMovementBlocked = false;
  std::string gameplayMovementStatus = "not_requested";
  bool gameplayMovementDebugAvailable = false;
  std::string gameplayMovementReasonCode = "not_requested";
  std::string gameplayMovementBlockedReason = "none";
  std::string gameplayMovementHitSurfaceId = "none";
  bool gameplayMovementGroundSnapApplied = false;
  bool gameplayMovementClamped = false;
  bool gameplayMovementSlid = false;
  std::uint64_t gameplayMovementCollisionSweepCount = 0;
  std::string gameplayMovementPolicyBand = "none";
  std::string gameplayMovementSlopeTravelDirection = "stationary";
  float gameplayMovementSlopeAngleDegrees = 0.0F;
  float gameplayMovementSpeedMultiplier = 1.0F;
  float gameplayMovementStartX = 0.0F;
  float gameplayMovementStartY = 0.0F;
  float gameplayMovementStartZ = 0.0F;
  float gameplayMovementFinalX = 0.0F;
  float gameplayMovementFinalY = 0.0F;
  float gameplayMovementFinalZ = 0.0F;
  float gameplayMovementHorizontalDistanceMeters = 0.0F;
  float gameplayMovementVerticalDeltaMeters = 0.0F;
  float gameplayMovementGradePercent = 0.0F;
  bool gameplayCollisionSurfacesUsed = false;
  std::uint64_t gameplayCollisionSurfaceCount = 0;
  std::string gameplayTickReasonCode = "not_requested";
  bool targetDiscovered = false;
  std::string gameplayTargetStatus = "not_requested";
  std::string gameplayTargetAction = "none";
  std::uint64_t gameplayTargetEntityId = 0;
  std::string gameplayTargetStableName = "none";
  std::string gameplayTargetKind = "none";
  float gameplayTargetDistanceMeters = 0.0F;
  bool gameplayTargetSupportsCommand = false;
  bool interactionExecuted = false;
  bool attackExecuted = false;
  std::string productTransitionLastAction = "none";
  std::string productTransitionStatus = "not_requested";
  bool productTransitionReturnedToGameplay = false;
  bool productTransitionReturnedToTitle = false;
  bool productTransitionSessionPreserved = false;
  std::string gameplayInputSource = "none";
  std::string gameplayCommandKind = "none";
  std::string gameplayCommandStatus = "not_requested";
  std::string gameplayReachGate = "not_attempted";
  std::string gameplayLastRejection = "none";
  MenuOwner inputOwner = MenuOwner::None;
  InputAction lastInputAction = InputAction::None;
  bool lastInputAccepted = false;
  bool gameplayInputSuppressed = false;
  bool automationControlRequested = false;
  bool automationControlLoaded = false;
  std::string automationControlPath;
  std::string automationControlStatus = "not_requested";
  std::string automationControlScope = "none";
  std::uint64_t automationControlLineCount = 0;
  std::uint64_t automationControlAppliedCount = 0;
  std::string automationControlLastKey = "none";
  std::string automationControlLastAction = "none";
  MenuOwner automationControlLastOwner = MenuOwner::None;
  std::string automationControlLastResult = "none";
  std::uint64_t framesPresented = 0;
  std::uint64_t eventPollCount = 0;
  std::uint64_t menuRowCount = 0;
  std::string status = "window_not_requested";
};

RenderReceipt buildProductAppReceipt(const ProductAppOptions& options,
                                     const ProductWorldTemplate& world,
                                     const FrontendState& frontend,
                                     const FrontendSettings& settings,
                                     const ProductAppWindowState& window,
                                     const ProductSaveBridgeResult& saves);

}  // namespace iggy3d
