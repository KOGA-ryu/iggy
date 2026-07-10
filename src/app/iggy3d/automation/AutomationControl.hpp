#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include "app/frontend/MenuInput.hpp"

#include "app/iggy3d/automation/Automation.hpp"

namespace iggy3d {

struct ProductAppWindowState;
struct FrontendState;
enum class FrontendSettingsTab : int;

// Owned automation-control state -- extracted from the ProductAppWindowState god-struct
// (docs/appkernel_build_map_v0_1.md, L2). Domain: automation. Behavior-identical.
struct ProductAutomationControlState {
  bool requested = false;
  bool loaded = false;
  std::string path;
  std::string status = "not_requested";
  std::string scope = "none";
  std::uint64_t lineCount = 0;
  std::uint64_t appliedCount = 0;
  std::string lastKey = "none";
  std::string lastAction = "none";
  MenuOwner lastOwner = MenuOwner::None;
  std::string lastResult = "none";
};

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
