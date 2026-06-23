#pragma once

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/input/InputAction.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
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
  std::uint64_t runtimeStateHash = 0;
  bool gameplayViewVisible = false;
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
  std::string gameplayInputSource = "none";
  std::string gameplayCommandKind = "none";
  std::string gameplayCommandStatus = "not_requested";
  std::string gameplayReachGate = "not_attempted";
  std::string gameplayLastRejection = "none";
  bool cameraControllerActive = false;
  bool lookInputUsed = false;
  std::string cameraInputSource = "none";
  std::string cameraMode = "first_person";
  std::string cameraController = "product_camera";
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
  MenuOwner inputOwner = MenuOwner::None;
  InputAction lastInputAction = InputAction::None;
  bool lastInputAccepted = false;
  bool gameplayInputSuppressed = false;
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
