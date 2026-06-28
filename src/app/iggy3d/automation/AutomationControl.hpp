#pragma once

#include <filesystem>
#include <functional>

#include "app/iggy3d/automation/Automation.hpp"

namespace iggy3d {

struct ProductAppWindowState;
struct FrontendState;
enum class FrontendSettingsTab : int;

struct ProductAutomationControlContext {
  const std::filesystem::path& automationControlPath;
  FrontendState& frontend;
  ProductAppWindowState& window;
  FrontendSettingsTab& settingsTab;
  std::function<bool(const ProductAutomationCommand&)> applyCommand;
  std::function<MenuOwner()> currentOwner;
};

void applyProductAutomationControl(ProductAutomationControlContext& context);

bool automationFailurePreservesLoaded(std::string_view status);

}  // namespace iggy3d
