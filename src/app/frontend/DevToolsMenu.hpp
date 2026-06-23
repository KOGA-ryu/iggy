#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

struct DevToolsMenuModel {
  std::vector<FrontendDevToolsCategory> categories;
  FrontendDevToolsCategory selected = FrontendDevToolsCategory::Session;
  bool selectedEnabled = true;
  std::string_view selectedDisabledReason = "none";
  std::string_view selectedAction = "none";
  std::uint64_t runtimeReadoutCount = 0;
};

DevToolsMenuModel buildDevToolsMenuModel(FrontendDevToolsCategory selected);
std::string_view devToolsCategoryLabel(FrontendDevToolsCategory category);
std::uint64_t devToolsReadoutCount(FrontendDevToolsCategory category);

}  // namespace iggy3d
