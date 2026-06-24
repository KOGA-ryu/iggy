#include "app/frontend/DevToolsMenu.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool categoryOrderIncludesInput() {
  const std::vector<iggy3d::FrontendDevToolsCategory>& categories =
      iggy3d::devToolsCategoryOrder();
  return expect(categories.size() == 10U, "dev category count") &&
         expect(categories[0] == iggy3d::FrontendDevToolsCategory::Session,
                "session first") &&
         expect(categories[1] == iggy3d::FrontendDevToolsCategory::Input,
                "input second") &&
         expect(categories[9] == iggy3d::FrontendDevToolsCategory::Performance,
                "performance tenth");
}

bool modelReadoutsAreStable() {
  const iggy3d::DevToolsMenuModel model =
      iggy3d::buildDevToolsMenuModel(iggy3d::FrontendDevToolsCategory::Input);
  return expect(model.selected == iggy3d::FrontendDevToolsCategory::Input,
                "selected input") &&
         expect(model.selectedEnabled, "input readout enabled") &&
         expect(model.selectedAction == "none", "read only action") &&
         expect(model.selectedDisabledReason == "none", "no disabled reason") &&
         expect(model.runtimeReadoutCount == 4U, "input readout count") &&
         expect(iggy3d::devToolsCategoryLabel(iggy3d::FrontendDevToolsCategory::Input) ==
                    "Input",
                "input label");
}

bool backRoutesCloseToParentSurfaces() {
  const iggy3d::DevToolsMenuModel model =
      iggy3d::buildDevToolsMenuModel(iggy3d::FrontendDevToolsCategory::Session);
  const auto starter =
      iggy3d::routeDevToolsAction(model, iggy3d::MenuOwner::Starter,
                                  iggy3d::FrontendAction::Back);
  const auto gameplay =
      iggy3d::routeDevToolsAction(model, iggy3d::MenuOwner::Gameplay,
                                  iggy3d::FrontendAction::Back);
  const auto pause =
      iggy3d::routeDevToolsAction(model, iggy3d::MenuOwner::Pause,
                                  iggy3d::FrontendAction::Back);
  return expect(starter.accepted, "starter back accepted") &&
         expect(starter.inputOwner == iggy3d::MenuOwner::Starter,
                "starter back owner") &&
         expect(starter.nextScreen == iggy3d::FrontendScreen::Starter,
                "starter back screen") &&
         expect(starter.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "starter back child") &&
         expect(starter.gameplayInputSuppressed, "starter back suppresses") &&
         expect(starter.status == "dev_tools_closed_to_starter",
                "starter back status") &&
         expect(gameplay.accepted, "gameplay back accepted") &&
         expect(gameplay.inputOwner == iggy3d::MenuOwner::Gameplay,
                "gameplay back owner") &&
         expect(gameplay.nextScreen == iggy3d::FrontendScreen::Gameplay,
                "gameplay back screen") &&
         expect(!gameplay.gameplayInputSuppressed,
                "gameplay back unsuppressed") &&
         expect(gameplay.status == "dev_tools_closed_to_gameplay",
                "gameplay back status") &&
         expect(pause.accepted, "pause back accepted") &&
         expect(pause.inputOwner == iggy3d::MenuOwner::Pause,
                "pause back owner") &&
         expect(pause.nextScreen == iggy3d::FrontendScreen::Pause,
                "pause back screen") &&
         expect(pause.gameplayInputSuppressed, "pause back suppresses") &&
         expect(pause.status == "dev_tools_closed_to_pause",
                "pause back status");
}

bool invalidBackAndApplyAreDeterministic() {
  iggy3d::DevToolsMenuModel disabled =
      iggy3d::buildDevToolsMenuModel(iggy3d::FrontendDevToolsCategory::Input);
  disabled.selectedEnabled = false;
  disabled.selectedDisabledReason = "developer_tools_disabled";
  const iggy3d::DevToolsMenuModel model =
      iggy3d::buildDevToolsMenuModel(iggy3d::FrontendDevToolsCategory::Renderer);

  const auto invalid =
      iggy3d::routeDevToolsAction(model, iggy3d::MenuOwner::None,
                                  iggy3d::FrontendAction::Back);
  const auto apply =
      iggy3d::routeDevToolsAction(model, iggy3d::MenuOwner::Gameplay,
                                  iggy3d::FrontendAction::Apply);
  const auto unsupported =
      iggy3d::routeDevToolsAction(model, iggy3d::MenuOwner::Gameplay,
                                  iggy3d::FrontendAction::Save);
  const auto disabledApply =
      iggy3d::routeDevToolsAction(disabled, iggy3d::MenuOwner::Gameplay,
                                  iggy3d::FrontendAction::Apply);

  return expect(!invalid.accepted, "invalid parent not accepted") &&
         expect(invalid.status == "dev_tools_invalid_parent",
                "invalid parent status") &&
         expect(apply.accepted, "apply accepted") &&
         expect(apply.inputOwner == iggy3d::MenuOwner::DevTools,
                "apply owner") &&
         expect(apply.nextScreen == iggy3d::FrontendScreen::DevOverlay,
                "apply stay screen") &&
         expect(apply.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "apply stay child") &&
         expect(apply.gameplayInputSuppressed, "apply suppresses") &&
         expect(apply.status == "dev_tools_category_selected",
                "apply status") &&
         expect(!unsupported.accepted, "unsupported not accepted") &&
         expect(unsupported.status == "not_dev_tools_action",
                "unsupported status") &&
         expect(!disabledApply.accepted, "disabled not accepted") &&
         expect(disabledApply.status == "developer_tools_disabled",
                "disabled status");
}

}  // namespace

int main() {
  const bool ok = categoryOrderIncludesInput() && modelReadoutsAreStable() &&
                  backRoutesCloseToParentSurfaces() &&
                  invalidBackAndApplyAreDeterministic();
  return ok ? 0 : 1;
}
