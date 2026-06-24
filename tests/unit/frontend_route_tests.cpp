#include "app/frontend/FrontendRoute.hpp"

#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << message << '\n';
  }
  return condition;
}

bool transitionRequestNamesAreStable() {
  return expect(iggy3d::frontendTransitionRequestName(
                    iggy3d::FrontendTransitionRequest::None) == "none",
                "none transition name") &&
         expect(iggy3d::frontendTransitionRequestName(
                    iggy3d::FrontendTransitionRequest::LaunchGameplay) ==
                    "launch_gameplay",
                "launch transition name") &&
         expect(iggy3d::frontendTransitionRequestName(
                    iggy3d::FrontendTransitionRequest::ReturnToTitle) ==
                    "return_to_title",
                "return title transition name") &&
         expect(iggy3d::frontendTransitionRequestName(
                    iggy3d::FrontendTransitionRequest::SaveAndExit) ==
                    "save_and_exit",
                "save and exit transition name");
}

bool ignoredResultDefaultsAreDeterministic() {
  const auto result = iggy3d::makeIgnoredFrontendRouteResult();
  return expect(!result.accepted, "ignored not accepted") &&
         expect(result.inputOwner == iggy3d::MenuOwner::None, "ignored owner") &&
         expect(result.nextScreen == iggy3d::FrontendScreen::BootStatus,
                "ignored screen") &&
         expect(result.nextChildScreen == iggy3d::FrontendScreen::Gameplay,
                "ignored child") &&
         expect(result.requestedTransition == iggy3d::FrontendTransitionRequest::None,
                "ignored transition") &&
         expect(!result.closeRequested, "ignored close") &&
         expect(result.status == "frontend_route_ignored", "ignored status") &&
         expect(!result.gameplayInputSuppressed, "ignored suppression") &&
         expect(result.receiptReason == "frontend_route_ignored", "ignored reason") &&
         expect(result.selectedAction == iggy3d::FrontendAction::None,
                "ignored selected action");
}

bool ignoredResultCanCarryCurrentOwnerAndSelection() {
  const auto result = iggy3d::makeIgnoredFrontendRouteResult(
      iggy3d::MenuOwner::Starter,
      iggy3d::FrontendScreen::Starter,
      iggy3d::FrontendScreen::LoadSave,
      iggy3d::FrontendAction::LoadSave);
  return expect(!result.accepted, "custom ignored not accepted") &&
         expect(result.inputOwner == iggy3d::MenuOwner::Starter,
                "custom ignored owner") &&
         expect(result.nextScreen == iggy3d::FrontendScreen::Starter,
                "custom ignored screen") &&
         expect(result.nextChildScreen == iggy3d::FrontendScreen::LoadSave,
                "custom ignored child") &&
         expect(result.gameplayInputSuppressed, "starter suppression carried") &&
         expect(result.selectedAction == iggy3d::FrontendAction::LoadSave,
                "custom ignored action");
}

bool acceptedResultCarriesAllFields() {
  const auto result = iggy3d::makeAcceptedFrontendRouteResult(
      iggy3d::MenuOwner::Pause,
      iggy3d::FrontendScreen::Gameplay,
      iggy3d::FrontendScreen::Settings,
      iggy3d::FrontendTransitionRequest::SaveAndExit,
      true,
      true,
      "pause_save_and_exit",
      "pause_save_and_exit",
      iggy3d::FrontendAction::SaveAndExit);
  return expect(result.accepted, "accepted") &&
         expect(result.inputOwner == iggy3d::MenuOwner::Pause, "accepted owner") &&
         expect(result.nextScreen == iggy3d::FrontendScreen::Gameplay,
                "accepted screen") &&
         expect(result.nextChildScreen == iggy3d::FrontendScreen::Settings,
                "accepted child") &&
         expect(result.requestedTransition ==
                    iggy3d::FrontendTransitionRequest::SaveAndExit,
                "accepted transition") &&
         expect(result.closeRequested, "accepted close") &&
         expect(result.gameplayInputSuppressed, "accepted suppression") &&
         expect(result.status == "pause_save_and_exit", "accepted status") &&
         expect(result.receiptReason == "pause_save_and_exit", "accepted reason") &&
         expect(result.selectedAction == iggy3d::FrontendAction::SaveAndExit,
                "accepted action");
}

}  // namespace

int main() {
  const bool ok = transitionRequestNamesAreStable() &&
                  ignoredResultDefaultsAreDeterministic() &&
                  ignoredResultCanCarryCurrentOwnerAndSelection() &&
                  acceptedResultCarriesAllFields();
  return ok ? 0 : 1;
}
