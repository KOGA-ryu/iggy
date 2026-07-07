#include "app/iggy3d/gameplay/GameplayFeedback.hpp"

#include <utility>

#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {
namespace {

bool isTargetCommand(const ProductAppWindowState& window) {
  return window.gameplayCommand.kind == "attack" ||
         window.gameplayCommand.kind == "interact";
}

FeedbackTone targetTone(const std::string& status) {
  if (status == "discovered") {
    return FeedbackTone::Pass;
  }
  if (status == "no_target") {
    return FeedbackTone::Warn;
  }
  return FeedbackTone::Neutral;
}

FeedbackTone reachTone(const std::string& status) {
  if (status == "pass") {
    return FeedbackTone::Pass;
  }
  if (status == "fail") {
    return FeedbackTone::Warn;
  }
  return FeedbackTone::Neutral;
}

FeedbackTone commandTone(const std::string& status) {
  if (status == "accepted") {
    return FeedbackTone::Pass;
  }
  if (status == "rejected" || status == "missing_player") {
    return FeedbackTone::Fail;
  }
  if (status == "no_target") {
    return FeedbackTone::Warn;
  }
  return FeedbackTone::Neutral;
}

std::string resultStatusFor(const ProductAppWindowState& window) {
  if (window.attackExecuted) {
    return "attack_executed";
  }
  if (window.interactionExecuted) {
    return "interaction_executed";
  }
  if (window.gameplayCommand.kind == "reset" && window.gameplayCommand.accepted) {
    return "reset_executed";
  }
  if (!window.gameplayCommand.submitted && window.gameplayCommand.status == "not_requested") {
    return "not_attempted";
  }
  if (window.gameplayCommand.status == "no_target") {
    return "no_target";
  }
  return window.gameplayCommand.status;
}

FeedbackTone resultTone(const std::string& status) {
  if (status == "attack_executed" || status == "interaction_executed" ||
      status == "reset_executed" || status == "accepted") {
    return FeedbackTone::Pass;
  }
  if (status == "rejected" || status == "missing_player") {
    return FeedbackTone::Fail;
  }
  if (status == "no_target") {
    return FeedbackTone::Warn;
  }
  return FeedbackTone::Neutral;
}

void addLine(GameplayFeedback& feedback,
             std::string label,
             std::string value,
             FeedbackTone tone,
             bool visible) {
  feedback.lines.push_back(
      GameplayFeedbackLine{std::move(label), std::move(value), tone, visible});
}

}  // namespace

GameplayFeedback buildGameplayFeedback(
    const ProductAppWindowState& window) {
  GameplayFeedback feedback;
  feedback.visible = window.gameplay.gameplayActive;
  feedback.commandKind = window.gameplayCommand.kind;
  feedback.commandStatus = window.gameplayCommand.status;
  feedback.reachStatus = window.gameplayReachGate;
  feedback.rejectionReason = window.gameplayLastRejection;
  feedback.resultStatus = resultStatusFor(window);

  feedback.targetStatus =
      window.targetDiscovered
          ? "discovered"
          : (isTargetCommand(window) && window.gameplayCommand.status == "no_target"
                 ? "no_target"
                 : "not_attempted");

  feedback.targetFeedbackVisible =
      feedback.visible && (window.targetDiscovered || isTargetCommand(window));
  feedback.commandFeedbackVisible =
      feedback.visible &&
      (window.gameplayCommand.submitted || window.gameplayCommand.status != "not_requested");
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
  addLine(feedback, "REJECT", feedback.rejectionReason, FeedbackTone::Fail,
          feedback.visible && feedback.rejectionReason != "none");

  return feedback;
}

}  // namespace iggy3d
