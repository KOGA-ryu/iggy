#pragma once

#include <filesystem>
#include <functional>

#include "app/iggy3d/product/Automation.hpp"

namespace iggy3d {

struct ProductAppWindowState;
enum class FrontendSettingsTab : int;

struct ProductAutomationControlContext {
  const std::filesystem::path& automationControlPath;
  ProductAppWindowState& window;
  FrontendSettingsTab& settingsTab;
  std::function<bool(const ProductAutomationCommand&)> applyCommand;
  std::function<MenuOwner()> currentOwner;
  std::function<bool(MenuOwner)> gameplaySuppressed;
};

void applyProductAutomationControl(ProductAutomationControlContext& context);

bool automationFailurePreservesLoaded(std::string_view status);

}  // namespace iggy3d
