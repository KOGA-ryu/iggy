#include "app/iggy3d/ProductGameplayTape.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {
namespace {

std::string_view trim(std::string_view value) {
  while (!value.empty() &&
         (value.front() == ' ' || value.front() == '\t' ||
          value.front() == '\r')) {
    value.remove_prefix(1);
  }
  while (!value.empty() &&
         (value.back() == ' ' || value.back() == '\t' ||
          value.back() == '\r')) {
    value.remove_suffix(1);
  }
  return value;
}

std::string_view stripComment(std::string_view value) {
  const std::size_t comment = value.find('#');
  if (comment != std::string_view::npos) {
    value = value.substr(0, comment);
  }
  return trim(value);
}

std::vector<std::string_view> splitWords(std::string_view value) {
  std::vector<std::string_view> words;
  value = trim(value);
  while (!value.empty()) {
    std::size_t end = 0;
    while (end < value.size() && value[end] != ' ' && value[end] != '\t') {
      ++end;
    }
    words.push_back(value.substr(0, end));
    value.remove_prefix(end);
    value = trim(value);
  }
  return words;
}

bool parseAction(std::string_view value, ProductGameplayTapeAction& action) {
  if (value == "move") {
    action = ProductGameplayTapeAction::Move;
    return true;
  }
  if (value == "interact") {
    action = ProductGameplayTapeAction::Interact;
    return true;
  }
  if (value == "attack") {
    action = ProductGameplayTapeAction::Attack;
    return true;
  }
  if (value == "wait") {
    action = ProductGameplayTapeAction::Wait;
    return true;
  }
  return false;
}

bool parseRejection(std::string_view value, CommandRejectionReason& reason) {
  if (value == "none") {
    reason = CommandRejectionReason::None;
    return true;
  }
  if (value == "required_item_missing") {
    reason = CommandRejectionReason::RequiredItemMissing;
    return true;
  }
  if (value == "out_of_range") {
    reason = CommandRejectionReason::OutOfRange;
    return true;
  }
  if (value == "invalid_target") {
    reason = CommandRejectionReason::InvalidTarget;
    return true;
  }
  if (value == "target_inactive") {
    reason = CommandRejectionReason::TargetInactive;
    return true;
  }
  if (value == "movement_too_far") {
    reason = CommandRejectionReason::MovementTooFar;
    return true;
  }
  return false;
}

bool parseMovementBlock(std::string_view value, MovementBlockedReason& reason) {
  if (value == "none" || value == "movement_ok") {
    reason = MovementBlockedReason::None;
    return true;
  }
  if (value == "invalid_actor") {
    reason = MovementBlockedReason::InvalidActor;
    return true;
  }
  if (value == "actor_inactive") {
    reason = MovementBlockedReason::ActorInactive;
    return true;
  }
  if (value == "invalid_destination") {
    reason = MovementBlockedReason::InvalidDestination;
    return true;
  }
  if (value == "destination_not_finite") {
    reason = MovementBlockedReason::DestinationNotFinite;
    return true;
  }
  if (value == "movement_too_far") {
    reason = MovementBlockedReason::MovementTooFar;
    return true;
  }
  if (value == "blocked_by_world") {
    reason = MovementBlockedReason::BlockedByWorld;
    return true;
  }
  if (value == "missing_world") {
    reason = MovementBlockedReason::MissingWorld;
    return true;
  }
  if (value == "missing_collision_surfaces") {
    reason = MovementBlockedReason::MissingCollisionSurfaces;
    return true;
  }
  if (value == "invalid_movement_params") {
    reason = MovementBlockedReason::InvalidMovementParams;
    return true;
  }
  if (value == "no_walkable_ground") {
    reason = MovementBlockedReason::NoWalkableGround;
    return true;
  }
  if (value == "slope_rejected") {
    reason = MovementBlockedReason::SlopeRejected;
    return true;
  }
  if (value == "blocked_by_collision") {
    reason = MovementBlockedReason::BlockedByCollision;
    return true;
  }
  if (value == "internal_error") {
    reason = MovementBlockedReason::InternalError;
    return true;
  }
  return false;
}

ProductGameplayTapeParseResult failed(ProductGameplayTapeParseResult result,
                                      std::string status,
                                      std::uint64_t line,
                                      std::string token) {
  result.ok = false;
  result.status = std::move(status);
  result.reasonCode = result.status;
  result.failedLine = line;
  result.failedToken = token.empty() ? "none" : std::move(token);
  result.tape.steps.clear();
  return result;
}

ProductGameplayTapeParseResult parseNormalStep(ProductGameplayTapeParseResult result,
                                               const std::vector<std::string_view>& words,
                                               std::uint64_t line) {
  ProductGameplayTapeAction action = ProductGameplayTapeAction::Wait;
  if (!parseAction(words[0], action)) {
    return failed(std::move(result),
                  "gameplay_tape_unknown_action",
                  line,
                  std::string(words[0]));
  }

  ProductGameplayTapeStep step;
  step.action = action;
  step.sourceLine = line;
  if (action == ProductGameplayTapeAction::Wait) {
    if (words.size() != 1U) {
      return failed(std::move(result),
                    "gameplay_tape_unexpected_token",
                    line,
                    std::string(words[1]));
    }
  } else {
    if (words.size() < 2U) {
      return failed(std::move(result),
                    "gameplay_tape_missing_target",
                    line,
                    std::string(words[0]));
    }
    if (words.size() > 2U) {
      return failed(std::move(result),
                    "gameplay_tape_unexpected_token",
                    line,
                    std::string(words[2]));
    }
    step.targetStableName = std::string(words[1]);
  }

  result.tape.steps.push_back(std::move(step));
  return result;
}

ProductGameplayTapeParseResult parseExpectedRejectionStep(
    ProductGameplayTapeParseResult result,
    const std::vector<std::string_view>& words,
    std::uint64_t line) {
  if (words.size() < 4U) {
    return failed(std::move(result),
                  "gameplay_tape_missing_expected_rejection_fields",
                  line,
                  std::string(words.front()));
  }
  if (words.size() > 4U) {
    return failed(std::move(result),
                  "gameplay_tape_unexpected_token",
                  line,
                  std::string(words[4]));
  }

  CommandRejectionReason expected = CommandRejectionReason::None;
  if (!parseRejection(words[1], expected) ||
      expected == CommandRejectionReason::None) {
    return failed(std::move(result),
                  "gameplay_tape_unknown_rejection_reason",
                  line,
                  std::string(words[1]));
  }

  ProductGameplayTapeAction action = ProductGameplayTapeAction::Wait;
  if (!parseAction(words[2], action)) {
    return failed(std::move(result),
                  "gameplay_tape_unknown_action",
                  line,
                  std::string(words[2]));
  }
  if (action != ProductGameplayTapeAction::Interact) {
    return failed(std::move(result),
                  "gameplay_tape_expected_reject_requires_interact",
                  line,
                  std::string(words[2]));
  }

  ProductGameplayTapeStep step;
  step.action = action;
  step.targetStableName = std::string(words[3]);
  step.expectRejection = true;
  step.expectedRejection = expected;
  step.sourceLine = line;
  result.tape.steps.push_back(std::move(step));
  return result;
}

ProductGameplayTapeParseResult parseExpectedMovementBlockStep(
    ProductGameplayTapeParseResult result,
    const std::vector<std::string_view>& words,
    std::uint64_t line) {
  if (words.size() < 4U) {
    return failed(std::move(result),
                  "gameplay_tape_missing_expected_block_fields",
                  line,
                  std::string(words.front()));
  }
  if (words.size() > 4U) {
    return failed(std::move(result),
                  "gameplay_tape_unexpected_token",
                  line,
                  std::string(words[4]));
  }

  MovementBlockedReason expected = MovementBlockedReason::None;
  if (!parseMovementBlock(words[1], expected) ||
      expected == MovementBlockedReason::None) {
    return failed(std::move(result),
                  "gameplay_tape_unknown_movement_block_reason",
                  line,
                  std::string(words[1]));
  }

  ProductGameplayTapeAction action = ProductGameplayTapeAction::Wait;
  if (!parseAction(words[2], action)) {
    return failed(std::move(result),
                  "gameplay_tape_unknown_action",
                  line,
                  std::string(words[2]));
  }
  if (action != ProductGameplayTapeAction::Move) {
    return failed(std::move(result),
                  "gameplay_tape_expected_block_requires_move",
                  line,
                  std::string(words[2]));
  }

  ProductGameplayTapeStep step;
  step.action = action;
  step.targetStableName = std::string(words[3]);
  step.expectMovementBlock = true;
  step.expectedMovementBlock = expected;
  step.sourceLine = line;
  result.tape.steps.push_back(std::move(step));
  return result;
}

}  // namespace

std::string_view productGameplayTapeActionName(ProductGameplayTapeAction action) {
  switch (action) {
    case ProductGameplayTapeAction::Move:
      return "move";
    case ProductGameplayTapeAction::Interact:
      return "interact";
    case ProductGameplayTapeAction::Attack:
      return "attack";
    case ProductGameplayTapeAction::Wait:
      return "wait";
  }
  return "wait";
}

std::string_view productGameplayTapeRejectionName(CommandRejectionReason reason) {
  switch (reason) {
    case CommandRejectionReason::None:
      return "none";
    case CommandRejectionReason::RequiredItemMissing:
      return "required_item_missing";
    case CommandRejectionReason::OutOfRange:
      return "out_of_range";
    case CommandRejectionReason::InvalidTarget:
      return "invalid_target";
    case CommandRejectionReason::TargetInactive:
      return "target_inactive";
    case CommandRejectionReason::MovementTooFar:
      return "movement_too_far";
    default:
      break;
  }
  return "rejected";
}

std::string_view productGameplayTapeMovementBlockName(MovementBlockedReason reason) {
  switch (reason) {
    case MovementBlockedReason::None:
      return "movement_ok";
    case MovementBlockedReason::InvalidActor:
      return "invalid_actor";
    case MovementBlockedReason::ActorInactive:
      return "actor_inactive";
    case MovementBlockedReason::InvalidDestination:
      return "invalid_destination";
    case MovementBlockedReason::DestinationNotFinite:
      return "destination_not_finite";
    case MovementBlockedReason::MovementTooFar:
      return "movement_too_far";
    case MovementBlockedReason::BlockedByWorld:
      return "blocked_by_world";
    case MovementBlockedReason::MissingWorld:
      return "missing_world";
    case MovementBlockedReason::MissingCollisionSurfaces:
      return "missing_collision_surfaces";
    case MovementBlockedReason::InvalidMovementParams:
      return "invalid_movement_params";
    case MovementBlockedReason::NoWalkableGround:
      return "no_walkable_ground";
    case MovementBlockedReason::SlopeRejected:
      return "slope_rejected";
    case MovementBlockedReason::BlockedByCollision:
      return "blocked_by_collision";
    case MovementBlockedReason::InternalError:
      return "internal_error";
  }
  return "internal_error";
}

ProductGameplayTapeParseResult parseProductGameplayTape(std::string_view text) {
  ProductGameplayTapeParseResult result;
  std::istringstream input{std::string(text)};
  std::string lineText;
  while (std::getline(input, lineText)) {
    ++result.lineCount;
    const std::string_view line = stripComment(lineText);
    if (line.empty()) {
      continue;
    }

    const std::vector<std::string_view> words = splitWords(line);
    if (words.empty()) {
      continue;
    }
    if (words[0] == "expect_reject") {
      result =
          parseExpectedRejectionStep(std::move(result), words, result.lineCount);
    } else if (words[0] == "expect_blocked") {
      result = parseExpectedMovementBlockStep(std::move(result),
                                              words,
                                              result.lineCount);
    } else {
      result = parseNormalStep(std::move(result), words, result.lineCount);
    }
    if (!result.reasonCode.empty() && result.reasonCode != "not_requested") {
      return result;
    }
  }

  if (result.tape.steps.empty()) {
    return failed(std::move(result), "gameplay_tape_empty", 0, "none");
  }

  result.ok = true;
  result.status = "gameplay_tape_loaded";
  result.reasonCode = "gameplay_tape_loaded";
  result.failedLine = 0;
  result.failedToken = "none";
  return result;
}

ProductGameplayTapeParseResult loadProductGameplayTapeFile(
    const std::filesystem::path& path) {
  ProductGameplayTapeParseResult result;
  if (path.empty() || !std::filesystem::exists(path)) {
    result.status = "gameplay_tape_file_missing";
    result.reasonCode = result.status;
    result.failedToken = path.empty() ? "none" : path.generic_string();
    return result;
  }

  std::ifstream input(path);
  if (!input) {
    result.status = "gameplay_tape_read_failed";
    result.reasonCode = result.status;
    result.failedToken = path.generic_string();
    return result;
  }

  std::ostringstream text;
  text << input.rdbuf();
  result = parseProductGameplayTape(text.str());
  if (!result.ok && result.failedToken == "none") {
    result.failedToken = path.generic_string();
  }
  return result;
}

}  // namespace iggy3d
