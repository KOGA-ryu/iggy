#include "app/iggy3d/gameplay/GameplayFeedback.hpp"

#include <utility>

#include "app/iggy3d/ProductAppWindowState.hpp"

namespace iggy3d {
namespace {

bool isTargetCommand(const ProductAppWindowState& window) {
  return window.gameplay.gameplayCommand.kind == "attack" ||
         window.gameplay.gameplayCommand.kind == "interact";
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
  if (window.gameplay.attackExecuted) {
    return "attack_executed";
  }
  if (window.gameplay.interactionExecuted) {
    return "interaction_executed";
  }
  if (window.gameplay.gameplayCommand.kind == "reset" && window.gameplay.gameplayCommand.accepted) {
    return "reset_executed";
  }
  if (!window.gameplay.gameplayCommand.submitted && window.gameplay.gameplayCommand.status == "not_requested") {
    return "not_attempted";
  }
  if (window.gameplay.gameplayCommand.status == "no_target") {
    return "no_target";
  }
  return window.gameplay.gameplayCommand.status;
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
  feedback.commandKind = window.gameplay.gameplayCommand.kind;
  feedback.commandStatus = window.gameplay.gameplayCommand.status;
  feedback.reachStatus = window.gameplay.gameplayReachGate;
  feedback.rejectionReason = window.gameplay.gameplayLastRejection;
  feedback.resultStatus = resultStatusFor(window);

  feedback.targetStatus =
      window.gameplay.targetDiscovered
          ? "discovered"
          : (isTargetCommand(window) && window.gameplay.gameplayCommand.status == "no_target"
                 ? "no_target"
                 : "not_attempted");

  feedback.targetFeedbackVisible =
      feedback.visible && (window.gameplay.targetDiscovered || isTargetCommand(window));
  feedback.commandFeedbackVisible =
      feedback.visible &&
      (window.gameplay.gameplayCommand.submitted || window.gameplay.gameplayCommand.status != "not_requested");
  feedback.reachFeedbackVisible =
      feedback.visible && feedback.reachStatus != "not_attempted";
  feedback.combatFeedbackVisible =
      feedback.visible && (window.gameplay.attackExecuted || feedback.commandKind == "attack");
  feedback.interactionFeedbackVisible =
      feedback.visible &&
      (window.gameplay.interactionExecuted || feedback.commandKind == "interact");

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
