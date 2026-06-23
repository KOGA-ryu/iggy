#include "app/frontend/PauseMenu.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool rowOrderAndCommandsAreExact() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 1U, true},
      iggy3d::FrontendAction::SaveAndExit);
  return expect(model.rows.size() == 8U, "pause row count") &&
         expect(model.rows[0].action == iggy3d::FrontendAction::Resume,
                "resume first") &&
         expect(model.rows[1].action == iggy3d::FrontendAction::Save, "save second") &&
         expect(model.rows[2].action == iggy3d::FrontendAction::SaveAndExit,
                "save exit third") &&
         expect(model.rows[3].action == iggy3d::FrontendAction::LoadSave,
                "load fourth") &&
         expect(model.rows[4].action == iggy3d::FrontendAction::Settings,
                "settings fifth") &&
         expect(model.rows[5].action == iggy3d::FrontendAction::DevTools,
                "dev sixth") &&
         expect(model.rows[6].action == iggy3d::FrontendAction::ReturnToTitle,
                "return seventh") &&
         expect(model.rows[7].action == iggy3d::FrontendAction::ExitGame,
                "exit eighth") &&
         expect(model.selectedCommand == "pause_save_and_exit",
                "selected command");
}

bool disabledReasonsAreDeterministic() {
  const iggy3d::PauseMenuModel model = iggy3d::buildPauseMenuModel(
      iggy3d::PauseMenuContext{true, true, true, 0U, false},
      iggy3d::FrontendAction::LoadSave);
  return expect(!model.rows[3].enabled, "load disabled without save") &&
         expect(model.rows[3].disabledReason == "no_compatible_save",
                "load disabled reason") &&
         expect(!model.rows[5].enabled, "dev disabled") &&
         expect(model.rows[5].disabledReason == "developer_tools_disabled",
                "dev disabled reason") &&
         expect(!model.selectedEnabled, "selected load disabled") &&
         expect(model.selectedDisabledReason == "no_compatible_save",
                "selected disabled reason");
}

}  // namespace

int main() {
  const bool ok = rowOrderAndCommandsAreExact() && disabledReasonsAreDeterministic();
  return ok ? 0 : 1;
}
