#pragma once

#include <string_view>
#include <vector>

#include "app/frontend/FrontendState.hpp"

namespace iggy3d {

struct DevToolsMenuModel {
  std::vector<FrontendDevToolsCategory> categories;
  FrontendDevToolsCategory selected = FrontendDevToolsCategory::Session;
};

DevToolsMenuModel buildDevToolsMenuModel(FrontendDevToolsCategory selected);
std::string_view devToolsCategoryLabel(FrontendDevToolsCategory category);

}  // namespace iggy3d
