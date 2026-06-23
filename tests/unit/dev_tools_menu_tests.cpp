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

}  // namespace

int main() {
  const bool ok = categoryOrderIncludesInput() && modelReadoutsAreStable();
  return ok ? 0 : 1;
}
