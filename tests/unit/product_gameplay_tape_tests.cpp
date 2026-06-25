#include "app/iggy3d/ProductGameplayTape.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool parsesOrderedTapeWithExpectedRejection() {
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape(
          "# loop\n"
          "move marker_key_r1_c2\n"
          "expect_reject required_item_missing interact marker_secret_door_r1_c3\n"
          "interact marker_key_r1_c2\n"
          "wait\n");

  return expect(parsed.ok, "parse ok") &&
         expect(parsed.status == "gameplay_tape_loaded", "parse status") &&
         expect(parsed.lineCount == 5U, "line count includes blanks/comments") &&
         expect(parsed.tape.steps.size() == 4U, "step count") &&
         expect(parsed.tape.steps[0].action == iggy3d::ProductGameplayTapeAction::Move,
                "move action") &&
         expect(parsed.tape.steps[0].targetStableName == "marker_key_r1_c2",
                "move target") &&
         expect(parsed.tape.steps[1].expectRejection, "expected reject") &&
         expect(parsed.tape.steps[1].expectedRejection ==
                    iggy3d::CommandRejectionReason::RequiredItemMissing,
                "expected reject reason") &&
         expect(parsed.tape.steps[1].action == iggy3d::ProductGameplayTapeAction::Interact,
                "expected interact") &&
         expect(parsed.tape.steps[3].action == iggy3d::ProductGameplayTapeAction::Wait,
                "wait action");
}

bool rejectsUnknownAction() {
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape("teleport marker_exit_r1_c5\n");
  return expect(!parsed.ok, "unknown action rejected") &&
         expect(parsed.reasonCode == "gameplay_tape_unknown_action",
                "unknown action reason") &&
         expect(parsed.failedLine == 1U, "unknown action line") &&
         expect(parsed.failedToken == "teleport", "unknown action token");
}

bool rejectsUnknownRejectionReason() {
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape(
          "expect_reject banana interact marker_exit_r1_c5\n");
  return expect(!parsed.ok, "unknown rejection rejected") &&
         expect(parsed.reasonCode == "gameplay_tape_unknown_rejection_reason",
                "unknown rejection reason") &&
         expect(parsed.failedToken == "banana", "unknown rejection token");
}

bool rejectsExpectedMoveRejection() {
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape(
          "expect_reject movement_too_far move marker_exit_r1_c5\n");
  return expect(!parsed.ok, "expected move rejected") &&
         expect(parsed.reasonCode ==
                    "gameplay_tape_expected_reject_requires_interact",
                "expected move reason");
}

bool rejectsEmptyTape() {
  const iggy3d::ProductGameplayTapeParseResult parsed =
      iggy3d::parseProductGameplayTape("  \n# comment\n");
  return expect(!parsed.ok, "empty rejected") &&
         expect(parsed.reasonCode == "gameplay_tape_empty", "empty reason");
}

}  // namespace

int main() {
  const bool ok = parsesOrderedTapeWithExpectedRejection() &&
                  rejectsUnknownAction() &&
                  rejectsUnknownRejectionReason() &&
                  rejectsExpectedMoveRejection() &&
                  rejectsEmptyTape();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
