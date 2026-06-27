#pragma once

#include <string>
#include <vector>

namespace iggy3d {

struct ProductAppWindowState;

enum class FeedbackTone {
  Neutral,
  Pass,
  Warn,
  Fail,
};

struct GameplayFeedbackLine {
  std::string label;
  std::string value;
  FeedbackTone tone = FeedbackTone::Neutral;
  bool visible = false;
};

struct GameplayFeedback {
  bool visible = false;
  bool targetFeedbackVisible = false;
  bool commandFeedbackVisible = false;
  bool reachFeedbackVisible = false;
  bool combatFeedbackVisible = false;
  bool interactionFeedbackVisible = false;
  std::string targetStatus = "not_attempted";
  std::string commandStatus = "not_requested";
  std::string commandKind = "none";
  std::string reachStatus = "not_attempted";
  std::string rejectionReason = "none";
  std::string resultStatus = "not_attempted";
  std::vector<GameplayFeedbackLine> lines;
};

GameplayFeedback buildGameplayFeedback(
    const ProductAppWindowState& window);

}  // namespace iggy3d
