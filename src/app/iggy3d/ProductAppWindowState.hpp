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
#include "app/iggy3d/gameplay/WallRunState.hpp"
#include "app/iggy3d/save/SaveSessionStore.hpp"
#include "app/iggy3d/gameplay/TraversalState.hpp"
#include "app/iggy3d/gameplay/DashState.hpp"
#include "app/iggy3d/gameplay/PhysicsMovementPlannerState.hpp"
#include "app/iggy3d/gameplay/ResetState.hpp"
#include "app/iggy3d/automation/AutomationControlState.hpp"
#include "app/iggy3d/window/FrontendWindowShell.hpp"
#include "app/iggy3d/window/PresentPathStore.hpp"
#include "app/iggy3d/window/ProductVulkanMenuState.hpp"
#include "app/iggy3d/gameplay/CollisionState.hpp"
#include "app/iggy3d/gameplay/GameplayMovementInfo.hpp"
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
  FrontendWindowShell frontendShell;
  InputDeviceStore inputDevice;
  DebugHudStore debugHud;
  CreativeAuthoringStore creativeAuthoring;
  GameplayStore gameplay;
  ProductRoomStore room;
  SaveSessionStore saveSession;
  std::uint64_t runtimeStateHash = 0;
  // Window-owned monotonic creative world generation used by viewport fly state.
  std::uint64_t creativeWorldEpoch = 0;
  ProductViewportState viewport;
  ProductAutomationControlState automationControl;
  PresentPathStore presentPath;
  ProductVulkanMenuState productVulkanMenu;
};

}  // namespace iggy3d
