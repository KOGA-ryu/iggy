#include "app/iggy3d/ProductGameplayFeedback.hpp"

#include <iostream>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

bool hasVisibleLine(const iggy3d::ProductGameplayFeedback& feedback,
                    const char* label,
                    const char* value) {
  for (const iggy3d::ProductGameplayFeedbackLine& line : feedback.lines) {
    if (line.visible && line.label == label && line.value == value) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  bool ok = true;

  iggy3d::ProductAppWindowState neutralWindow;
  neutralWindow.gameplayActive = true;
  const iggy3d::ProductGameplayFeedback neutral =
      iggy3d::buildProductGameplayFeedback(neutralWindow);
  ok &= expect(neutral.visible, "neutral gameplay feedback visible");
  ok &= expect(neutral.targetStatus == "not_attempted", "neutral target status");
  ok &= expect(neutral.commandStatus == "not_requested", "neutral command status");
  ok &= expect(neutral.resultStatus == "not_attempted", "neutral result status");
  ok &= expect(!neutral.combatFeedbackVisible, "neutral combat feedback hidden");
  ok &= expect(hasVisibleLine(neutral, "TARGET", "not_attempted"),
               "neutral target line");

  iggy3d::ProductAppWindowState attackWindow;
  attackWindow.gameplayActive = true;
  attackWindow.targetDiscovered = true;
  attackWindow.gameplayReachGate = "pass";
  attackWindow.gameplayCommandSubmitted = true;
  attackWindow.gameplayCommandAccepted = true;
  attackWindow.gameplayCommandKind = "attack";
  attackWindow.gameplayCommandStatus = "accepted";
  attackWindow.gameplayLastRejection = "none";
  attackWindow.attackExecuted = true;
  const iggy3d::ProductGameplayFeedback attack =
      iggy3d::buildProductGameplayFeedback(attackWindow);
  ok &= expect(attack.targetFeedbackVisible, "attack target feedback visible");
  ok &= expect(attack.reachFeedbackVisible, "attack reach feedback visible");
  ok &= expect(attack.commandFeedbackVisible, "attack command feedback visible");
  ok &= expect(attack.combatFeedbackVisible, "attack combat feedback visible");
  ok &= expect(!attack.interactionFeedbackVisible, "attack interaction hidden");
  ok &= expect(attack.targetStatus == "discovered", "attack target discovered");
  ok &= expect(attack.reachStatus == "pass", "attack reach pass");
  ok &= expect(attack.commandKind == "attack", "attack command kind");
  ok &= expect(attack.commandStatus == "accepted", "attack command accepted");
  ok &= expect(attack.resultStatus == "attack_executed", "attack result");
  ok &= expect(hasVisibleLine(attack, "RESULT", "attack_executed"),
               "attack result line");

  iggy3d::ProductAppWindowState noTargetWindow;
  noTargetWindow.gameplayActive = true;
  noTargetWindow.gameplayInputUsed = true;
  noTargetWindow.gameplayCommandKind = "interact";
  noTargetWindow.gameplayCommandStatus = "no_target";
  noTargetWindow.gameplayReachGate = "not_attempted";
  const iggy3d::ProductGameplayFeedback noTarget =
      iggy3d::buildProductGameplayFeedback(noTargetWindow);
  ok &= expect(noTarget.targetFeedbackVisible, "no target feedback visible");
  ok &= expect(noTarget.interactionFeedbackVisible, "interaction feedback visible");
  ok &= expect(noTarget.targetStatus == "no_target", "no target status");
  ok &= expect(noTarget.commandStatus == "no_target", "no target command status");
  ok &= expect(noTarget.resultStatus == "no_target", "no target result");
  ok &= expect(hasVisibleLine(noTarget, "COMMAND", "interact no_target"),
               "no target command line");

  iggy3d::ProductAppWindowState rejectedWindow;
  rejectedWindow.gameplayActive = true;
  rejectedWindow.gameplayCommandSubmitted = true;
  rejectedWindow.gameplayCommandKind = "attack";
  rejectedWindow.gameplayCommandStatus = "rejected";
  rejectedWindow.gameplayReachGate = "fail";
  rejectedWindow.gameplayLastRejection = "out_of_range";
  const iggy3d::ProductGameplayFeedback rejected =
      iggy3d::buildProductGameplayFeedback(rejectedWindow);
  ok &= expect(rejected.combatFeedbackVisible, "rejected combat feedback visible");
  ok &= expect(rejected.rejectionReason == "out_of_range", "rejection reason");
  ok &= expect(hasVisibleLine(rejected, "REJECT", "out_of_range"),
               "rejection line visible");

  if (!ok) {
    return 1;
  }
  std::cout << "product_gameplay_feedback_tests=pass\n";
  return 0;
}
