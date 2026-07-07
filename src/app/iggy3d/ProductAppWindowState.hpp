#pragma once

#include <cstdint>
#include <string>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/frontend/MenuInput.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/gameplay/GameplayStore.hpp"
#include "app/iggy3d/gameplay/MovementTuning.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/input/InputDeviceStore.hpp"
#include "app/iggy3d/debug/DebugHudStore.hpp"
#include "app/iggy3d/creative/CreativeAuthoringStore.hpp"
#include "app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp"
#include "app/iggy3d/gameplay/WallRunState.hpp"
#include "app/iggy3d/save/SaveSessionStore.hpp"
#include "app/iggy3d/ProductCreativeUndoState.hpp"
#include "app/iggy3d/gameplay/TraversalState.hpp"
#include "app/iggy3d/gameplay/DashState.hpp"
#include "app/iggy3d/gameplay/PhysicsMovementPlannerState.hpp"
#include "app/iggy3d/ProductCreativeUiProjectionState.hpp"
#include "app/iggy3d/gameplay/ResetState.hpp"
#include "app/iggy3d/automation/AutomationControlState.hpp"
#include "app/iggy3d/ProductCreativeUiLastState.hpp"
#include "app/iggy3d/window/PresentPathStore.hpp"
#include "app/iggy3d/window/ProductVulkanMenuState.hpp"
#include "app/iggy3d/gameplay/CollisionState.hpp"
#include "app/iggy3d/gameplay/GameplayMovementInfo.hpp"
#include "app/iggy3d/ProductStartupState.hpp"
#include "app/iggy3d/ProductCreativeUiInputState.hpp"
#include "app/iggy3d/ProductCreativeDocumentRevisionState.hpp"
#include "app/iggy3d/gameplay/CommandState.hpp"
#include "app/iggy3d/gameplay/JumpState.hpp"
#include "app/iggy3d/view/ViewportState.hpp"
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
  bool gamepadMenuSelectUsed = false;
  InputDeviceStore inputDevice;
  DebugHudStore debugHud;
  CreativeAuthoringStore creativeAuthoring;
  FrontendSettingsTab selectedSettingsTab = FrontendSettingsTab::None;
  GameplayStore gameplay;
  std::string launchAction = "none";
  std::string launchStatus = "not_requested";
  std::string packageLoadStatus = "not_requested";
  ProductStartupState startup;
  ProductRoomStore room;
  SaveSessionStore saveSession;
  ProductCreativeDocumentRevisionState creativeDocumentRevision;
  bool creativeDocumentChangedThisFrame = false;
  ProductCreativeUndoState creativeUndo;
  bool creativeBakedRoomStale = false;
  std::uint64_t creativeBakedRoomStaleDocumentId = 0;
  std::uint64_t creativeBakedRoomStaleRevision = 0;
  std::string creativeBakedRoomStaleStatus =
      "creative_baked_room_not_observed";
  std::string creativeBakedRoomStaleReasonCode =
      "creative_baked_room_not_observed";
  // TV1-H: mirrors the creative facade's active tool being Navigate this frame.
  // Contexts that only carry the window (mouse-capture policy, projection
  // camera-anchor override) read this instead of the facade so the fly camera
  // and its capture re-engage are gated on Navigate-active-in-creative-document.
  bool creativeNavigateActive = false;
  std::uint64_t runtimeStateHash = 0;
  // Window-owned monotonic creative world generation used by viewport fly state.
  std::uint64_t creativeWorldEpoch = 0;
  ProductViewportState viewport;
  ProductAutomationControlState automationControl;
  PresentPathStore presentPath;
  ProductVulkanMenuState productVulkanMenu;
  ProductCreativeUiProjectionState creativeUiProjection;
  ProductCreativeUiInputState creativeUiInput;
  ProductCreativeUiLastState creativeUiLast;
  ProductCreativeUiCommandDiagnostics creativeUiCommand;
  ProductCreativeBakedRoomRefreshDiagnostics creativeBakedRoomAutoRefresh;
  std::uint64_t framesPresented = 0;
  std::uint64_t eventPollCount = 0;
  std::uint64_t menuRowCount = 0;
  std::string status = "window_not_requested";
};

}  // namespace iggy3d
