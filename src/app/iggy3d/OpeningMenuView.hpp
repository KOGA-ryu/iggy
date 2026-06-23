#pragma once

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/SaveBridge.hpp"

#if defined(IGGY3D_HAS_SDL3)
struct SDL_Renderer;
#endif

namespace iggy3d {

struct DebugProjectionResult;

struct OpeningMenuViewState {
  bool textDrawn = false;
  bool selectedRowDrawn = false;
  bool cameraHeadingDrawn = false;
  unsigned int rowCount = 0;
};

enum class OpeningMenuHitArea {
  None,
  StarterAction,
  SettingsTab,
  DevToolsCategory,
};

struct OpeningMenuHitTestResult {
  bool hit = false;
  OpeningMenuHitArea area = OpeningMenuHitArea::None;
  FrontendAction action = FrontendAction::None;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::None;
  FrontendDevToolsCategory devToolsCategory = FrontendDevToolsCategory::None;
};

OpeningMenuHitTestResult openingMenuActionAt(const FrontendState& frontend,
                                             float x,
                                             float y);

#if defined(IGGY3D_HAS_SDL3)
OpeningMenuViewState drawOpeningMenuView(SDL_Renderer& renderer,
                                         const ProductAppOptions& options,
                                         const ProductWorldTemplate& world,
                                         const FrontendState& frontend,
                                         FrontendSettingsTab selectedSettingsTab,
                                         bool gameplayActive,
                                         std::uint64_t runtimeStateHash,
                                         const ProductPrimitiveDrawList* drawList,
                                         std::size_t sceneItemCount,
                                         const DebugProjectionResult* debug,
                                         float cameraYawDegrees,
                                         float cameraPitchDegrees,
                                         const ProductSaveBridgeResult& saves);
#endif

}  // namespace iggy3d
