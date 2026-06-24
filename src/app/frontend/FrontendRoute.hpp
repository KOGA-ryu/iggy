#pragma once

#include <cstdint>
#include <string_view>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/MenuInput.hpp"

namespace iggy3d {

enum class FrontendTransitionRequest : std::uint8_t {
  None,
  LaunchGameplay,
  ReturnToTitle,
  Save,
  SaveAndExit,
  Exit,
};

struct FrontendRouteResult {
  bool accepted = false;
  MenuOwner inputOwner = MenuOwner::None;
  FrontendScreen nextScreen = FrontendScreen::BootStatus;
  FrontendScreen nextChildScreen = FrontendScreen::Gameplay;
  FrontendTransitionRequest requestedTransition = FrontendTransitionRequest::None;
  bool closeRequested = false;
  std::string_view status = "frontend_route_ignored";
  bool gameplayInputSuppressed = false;
  std::string_view receiptReason = "frontend_route_ignored";
  FrontendAction selectedAction = FrontendAction::None;
};

std::string_view frontendTransitionRequestName(FrontendTransitionRequest request);

FrontendRouteResult makeIgnoredFrontendRouteResult(
    MenuOwner owner = MenuOwner::None,
    FrontendScreen screen = FrontendScreen::BootStatus,
    FrontendScreen childScreen = FrontendScreen::Gameplay,
    FrontendAction selectedAction = FrontendAction::None);

FrontendRouteResult makeAcceptedFrontendRouteResult(
    MenuOwner owner,
    FrontendScreen nextScreen,
    FrontendScreen nextChildScreen,
    FrontendTransitionRequest transition,
    bool closeRequested,
    bool gameplayInputSuppressed,
    std::string_view status,
    std::string_view receiptReason,
    FrontendAction selectedAction);

}  // namespace iggy3d
