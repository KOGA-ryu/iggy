#pragma once

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/input/InputAction.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
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
  std::string worldCreationStatus = "not_requested";
  std::string worldCreationReasonCode = "not_requested";
  std::string worldCreationWorldId = "none";
  bool worldCreationInitialSaveRequested = false;
  bool worldCreationInitialSaveWritten = false;
  std::string worldCreationInitialSaveId = "none";
  std::string worldCreationRouteAfterCreate = "world_setup";
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
  bool saveDeleteExecuted = false;
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
  bool targetDiscovered = false;
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
