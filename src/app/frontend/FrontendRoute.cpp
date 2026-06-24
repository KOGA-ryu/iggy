#include "app/frontend/FrontendRoute.hpp"

namespace iggy3d {

std::string_view frontendTransitionRequestName(FrontendTransitionRequest request) {
  switch (request) {
    case FrontendTransitionRequest::None:
      return "none";
    case FrontendTransitionRequest::LaunchGameplay:
      return "launch_gameplay";
    case FrontendTransitionRequest::ReturnToTitle:
      return "return_to_title";
    case FrontendTransitionRequest::Save:
      return "save";
    case FrontendTransitionRequest::SaveAndExit:
      return "save_and_exit";
    case FrontendTransitionRequest::Exit:
      return "exit";
  }
  return "unknown";
}

FrontendRouteResult makeIgnoredFrontendRouteResult(
    MenuOwner owner,
    FrontendScreen screen,
    FrontendScreen childScreen,
    FrontendAction selectedAction) {
  FrontendRouteResult result;
  result.inputOwner = owner;
  result.nextScreen = screen;
  result.nextChildScreen = childScreen;
  result.gameplayInputSuppressed = menuOwnerBlocksGameplay(owner);
  result.selectedAction = selectedAction;
  return result;
}

FrontendRouteResult makeAcceptedFrontendRouteResult(
    MenuOwner owner,
    FrontendScreen nextScreen,
    FrontendScreen nextChildScreen,
    FrontendTransitionRequest transition,
    bool closeRequested,
    bool gameplayInputSuppressed,
    std::string_view status,
    std::string_view receiptReason,
    FrontendAction selectedAction) {
  FrontendRouteResult result;
  result.accepted = true;
  result.inputOwner = owner;
  result.nextScreen = nextScreen;
  result.nextChildScreen = nextChildScreen;
  result.requestedTransition = transition;
  result.closeRequested = closeRequested;
  result.gameplayInputSuppressed = gameplayInputSuppressed;
  result.status = status;
  result.receiptReason = receiptReason;
  result.selectedAction = selectedAction;
  return result;
}

}  // namespace iggy3d
