#include "app/iggy3d/ProductGameplayFeedback.hpp"

#include <utility>

#include "app/iggy3d/ReceiptBuilder.hpp"

namespace iggy3d {
namespace {

bool isTargetCommand(const ProductAppWindowState& window) {
  return window.gameplayCommandKind == "attack" ||
         window.gameplayCommandKind == "interact";
}

ProductFeedbackTone targetTone(const std::string& status) {
  if (status == "discovered") {
    return ProductFeedbackTone::Pass;
  }
  if (status == "no_target") {
    return ProductFeedbackTone::Warn;
  }
  return ProductFeedbackTone::Neutral;
}

ProductFeedbackTone reachTone(const std::string& status) {
  if (status == "pass") {
    return ProductFeedbackTone::Pass;
  }
  if (status == "fail") {
    return ProductFeedbackTone::Warn;
  }
  return ProductFeedbackTone::Neutral;
}

ProductFeedbackTone commandTone(const std::string& status) {
  if (status == "accepted") {
    return ProductFeedbackTone::Pass;
  }
  if (status == "rejected" || status == "missing_player") {
    return ProductFeedbackTone::Fail;
  }
  if (status == "no_target") {
    return ProductFeedbackTone::Warn;
  }
  return ProductFeedbackTone::Neutral;
}

std::string resultStatusFor(const ProductAppWindowState& window) {
  if (window.attackExecuted) {
    return "attack_executed";
  }
  if (window.interactionExecuted) {
    return "interaction_executed";
  }
  if (window.gameplayCommandKind == "reset" && window.gameplayCommandAccepted) {
    return "reset_executed";
  }
  if (!window.gameplayCommandSubmitted && window.gameplayCommandStatus == "not_requested") {
    return "not_attempted";
  }
  if (window.gameplayCommandStatus == "no_target") {
    return "no_target";
  }
  return window.gameplayCommandStatus;
}

ProductFeedbackTone resultTone(const std::string& status) {
  if (status == "attack_executed" || status == "interaction_executed" ||
      status == "reset_executed" || status == "accepted") {
    return ProductFeedbackTone::Pass;
  }
  if (status == "rejected" || status == "missing_player") {
    return ProductFeedbackTone::Fail;
  }
  if (status == "no_target") {
    return ProductFeedbackTone::Warn;
  }
  return ProductFeedbackTone::Neutral;
}

void addLine(ProductGameplayFeedback& feedback,
             std::string label,
             std::string value,
             ProductFeedbackTone tone,
             bool visible) {
  feedback.lines.push_back(
      ProductGameplayFeedbackLine{std::move(label), std::move(value), tone, visible});
}

}  // namespace

ProductGameplayFeedback buildProductGameplayFeedback(
    const ProductAppWindowState& window) {
  ProductGameplayFeedback feedback;
  feedback.visible = window.gameplayActive;
  feedback.commandKind = window.gameplayCommandKind;
  feedback.commandStatus = window.gameplayCommandStatus;
  feedback.reachStatus = window.gameplayReachGate;
  feedback.rejectionReason = window.gameplayLastRejection;
  feedback.resultStatus = resultStatusFor(window);

  feedback.targetStatus =
      window.targetDiscovered
          ? "discovered"
          : (isTargetCommand(window) && window.gameplayCommandStatus == "no_target"
                 ? "no_target"
                 : "not_attempted");

  feedback.targetFeedbackVisible =
      feedback.visible && (window.targetDiscovered || isTargetCommand(window));
  feedback.commandFeedbackVisible =
      feedback.visible &&
      (window.gameplayCommandSubmitted || window.gameplayCommandStatus != "not_requested");
  feedback.reachFeedbackVisible =
      feedback.visible && feedback.reachStatus != "not_attempted";
  feedback.combatFeedbackVisible =
      feedback.visible && (window.attackExecuted || feedback.commandKind == "attack");
  feedback.interactionFeedbackVisible =
      feedback.visible &&
      (window.interactionExecuted || feedback.commandKind == "interact");

  addLine(feedback, "TARGET", feedback.targetStatus, targetTone(feedback.targetStatus),
          feedback.visible);
  addLine(feedback, "REACH", feedback.reachStatus, reachTone(feedback.reachStatus),
          feedback.visible);
  addLine(feedback, "COMMAND", feedback.commandKind + " " + feedback.commandStatus,
          commandTone(feedback.commandStatus), feedback.visible);
  addLine(feedback, "RESULT", feedback.resultStatus,
          resultTone(feedback.resultStatus), feedback.visible);
  addLine(feedback, "REJECT", feedback.rejectionReason, ProductFeedbackTone::Fail,
          feedback.visible && feedback.rejectionReason != "none");

  return feedback;
}

}  // namespace iggy3d
