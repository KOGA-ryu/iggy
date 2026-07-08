#pragma once

#include <cstddef>
#include <vector>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

namespace iggy3d {

enum class OpeningMenuHitArea {
  None,
  StarterAction,
  SettingsTab,
  DevToolsCategory,
  NewWorldCreate,
  NewWorldBack,
  NewWorldPreviousDungeon,
  NewWorldNextDungeon,
  LoadSaveSlot,
  LoadSaveLoad,
  LoadSaveDelete,
  LoadSaveBack,
  DeleteConfirmConfirm,
  DeleteConfirmBack,
  SettingsBack,
  DevToolsBack,
};

struct OpeningMenuHitTestResult {
  bool hit = false;
  OpeningMenuHitArea area = OpeningMenuHitArea::None;
  FrontendAction action = FrontendAction::None;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::None;
  FrontendDevToolsCategory devToolsCategory = FrontendDevToolsCategory::None;
  std::size_t saveSlotIndex = 0;
};

bool openingMenuUsesPauseRows(const FrontendState& frontend);
const std::vector<FrontendAction>& openingMenuActionOrderForFrontend(
    const FrontendState& frontend);
ProductFrontendSurface openingMenuDetailSurfaceFor(
    const FrontendState& frontend);

// Takes the SAME ProductUiDrawListRequest the frame draw path builds (via
// buildProductStarterUiDrawListRequest) so the hit regions it reads are provably
// the ones that were drawn; the request cannot diverge between draw and hit-test.
OpeningMenuHitTestResult openingMenuActionAt(
    const ProductUiDrawListRequest& request,
    float x,
    float y);

}  // namespace iggy3d
