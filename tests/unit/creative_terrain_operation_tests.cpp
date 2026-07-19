#include "app/iggy3d/creative/recipes/TerrainOperation.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeTerrainHeightField makeField(
    cr::CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  cr::CreativeTerrainHeightField field;
  static_cast<void>(field.replace(bounds, heights));
  return field;
}

cr::CreativeTerrainGeneratorRecipe flatRecipe(std::uint16_t height) {
  cr::CreativeTerrainGeneratorRecipe recipe;
  recipe.bounds = {{0, 0}, 2U, 2U};
  recipe.baseHeightCells = height;
  recipe.reliefCells = 0U;
  recipe.horizontalScaleCells = 4.0;
  recipe.octaveCount = 1U;
  recipe.slopeDamping = 0.0;
  return recipe;
}

cr::CreativeTerrainCompositionRecipe hardReplace() {
  cr::CreativeTerrainCompositionRecipe recipe;
  recipe.featherCells = 0U;
  return recipe;
}

cr::CreativeTerrainOperationMutationRequest addRequest(
    std::uint16_t height) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = cr::CreativeTerrainOperationMutationKind::Add;
  request.generation = flatRecipe(height);
  request.composition = hardReplace();
  return request;
}

bool orderedReplayAndMutationLifecycle() {
  cr::CreativeTerrainField legacy;
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 2U, 2U};
  constexpr std::array<std::uint16_t, 4U> baseHeights{3U, 3U, 3U, 3U};
  const cr::CreativeTerrainHeightField base = makeField(bounds, baseHeights);
  cr::CreativeTerrainOperationStack stack;

  const cr::CreativeTerrainOperationMutationPlan first =
      cr::planCreativeTerrainOperationMutation(legacy, base, stack,
                                               addRequest(5U));
  const cr::CreativeTerrainOperationMutationPlan second =
      cr::planCreativeTerrainOperationMutation(legacy, first.heightField,
                                               first.stack, addRequest(10U));
  cr::CreativeTerrainOperationMutationRequest move;
  move.kind = cr::CreativeTerrainOperationMutationKind::Move;
  move.operationId = second.receipt.operationId;
  move.targetIndex = 0U;
  const cr::CreativeTerrainOperationMutationPlan moved =
      cr::planCreativeTerrainOperationMutation(
          legacy, second.heightField, second.stack, move);
  cr::CreativeTerrainOperationMutationRequest disable;
  disable.kind = cr::CreativeTerrainOperationMutationKind::SetEnabled;
  disable.operationId = first.receipt.operationId;
  disable.enabled = false;
  const cr::CreativeTerrainOperationMutationPlan disabled =
      cr::planCreativeTerrainOperationMutation(
          legacy, moved.heightField, moved.stack, disable);

  cr::CreativeTerrainOperationMutationRequest removeSecond;
  removeSecond.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  removeSecond.operationId = second.receipt.operationId;
  const cr::CreativeTerrainOperationMutationPlan removedSecond =
      cr::planCreativeTerrainOperationMutation(
          legacy, disabled.heightField, disabled.stack, removeSecond);
  cr::CreativeTerrainOperationMutationRequest removeFirst;
  removeFirst.kind = cr::CreativeTerrainOperationMutationKind::Remove;
  removeFirst.operationId = first.receipt.operationId;
  const cr::CreativeTerrainOperationMutationPlan removedAll =
      cr::planCreativeTerrainOperationMutation(
          legacy, removedSecond.heightField, removedSecond.stack, removeFirst);

  return expect(first.receipt.accepted && first.receipt.changed &&
                    first.receipt.operationId == 1U &&
                    first.stack.baseHeightField.heightAt({0, 0}) == 3U &&
                    first.heightField.heightAt({0, 0}) == 5U,
                "first operation captures the baked base") &&
         expect(second.receipt.accepted &&
                    second.receipt.operationId == 2U &&
                    second.heightField.heightAt({0, 0}) == 10U,
                "later replace operation wins in ordered replay") &&
         expect(moved.receipt.accepted && moved.receipt.changed &&
                    moved.stack.operations.front().id == 2U &&
                    moved.heightField.heightAt({0, 0}) == 5U,
                "reordering deterministically changes replay output") &&
         expect(disabled.receipt.accepted &&
                    disabled.heightField.heightAt({0, 0}) == 10U &&
                    disabled.receipt.replay.disabledOperationCount == 1U,
                "disabled operations remain durable but do not evaluate") &&
         expect(removedSecond.receipt.accepted &&
                    removedSecond.heightField.heightAt({0, 0}) == 3U,
                "removing the only enabled operation exposes baked base") &&
         expect(removedAll.receipt.accepted &&
                    removedAll.stack.operations.empty() &&
                    removedAll.stack.baseHeightField.cellCount() == 0U &&
                    cr::creativeTerrainHeightFieldsEqual(removedAll.heightField,
                                                         base),
                "removing the final operation restores and releases base");
}

bool updateAndReplayAreDeterministic() {
  cr::CreativeTerrainField legacy;
  cr::CreativeTerrainHeightField empty;
  cr::CreativeTerrainOperationStack stack;
  const cr::CreativeTerrainOperationMutationPlan added =
      cr::planCreativeTerrainOperationMutation(legacy, empty, stack,
                                               addRequest(6U));
  cr::CreativeTerrainOperationMutationRequest update;
  update.kind = cr::CreativeTerrainOperationMutationKind::Update;
  update.operationId = added.receipt.operationId;
  update.generation = flatRecipe(12U);
  update.composition = hardReplace();
  const cr::CreativeTerrainOperationMutationPlan updated =
      cr::planCreativeTerrainOperationMutation(
          legacy, added.heightField, added.stack, update);
  const cr::CreativeTerrainOperationReplayResult replayA =
      cr::replayCreativeTerrainOperations(legacy, updated.stack);
  const cr::CreativeTerrainOperationReplayResult replayB =
      cr::replayCreativeTerrainOperations(legacy, updated.stack);

  return expect(updated.receipt.accepted && updated.receipt.changed &&
                    updated.heightField.heightAt({1, 1}) == 12U,
                "updating a recipe regenerates the derived field") &&
         expect(replayA.receipt.accepted && replayB.receipt.accepted &&
                    replayA.receipt.heightHash == replayB.receipt.heightHash &&
                    cr::creativeTerrainHeightFieldsEqual(replayA.heightField,
                                                         replayB.heightField),
                "ordered replay and output hash are deterministic");
}

bool driftInvalidRequestsAndCapacityFailClosed() {
  cr::CreativeTerrainField legacy;
  cr::CreativeTerrainHeightField empty;
  cr::CreativeTerrainOperationStack stack;
  cr::CreativeTerrainOperationMutationPlan current =
      cr::planCreativeTerrainOperationMutation(legacy, empty, stack,
                                               addRequest(4U));
  constexpr std::array<std::uint16_t, 4U> driftHeights{7U, 7U, 7U, 7U};
  const cr::CreativeTerrainHeightField drifted =
      makeField({{0, 0}, 2U, 2U}, driftHeights);
  const cr::CreativeTerrainOperationMutationPlan driftRejected =
      cr::planCreativeTerrainOperationMutation(
          legacy, drifted, current.stack, addRequest(8U));

  cr::CreativeTerrainOperationMutationRequest invalidMove;
  invalidMove.kind = cr::CreativeTerrainOperationMutationKind::Move;
  invalidMove.operationId = current.receipt.operationId;
  invalidMove.targetIndex = 3U;
  const cr::CreativeTerrainOperationMutationPlan moveRejected =
      cr::planCreativeTerrainOperationMutation(
          legacy, current.heightField, current.stack, invalidMove);

  while (current.stack.operations.size() <
         cr::kCreativeTerrainOperationCapacity) {
    current = cr::planCreativeTerrainOperationMutation(
        legacy, current.heightField, current.stack, addRequest(4U));
  }
  const cr::CreativeTerrainOperationMutationPlan capacityRejected =
      cr::planCreativeTerrainOperationMutation(
          legacy, current.heightField, current.stack, addRequest(4U));

  return expect(!driftRejected.receipt.accepted &&
                    driftRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::InvalidStack &&
                    driftRejected.stack.operations.empty(),
                "derived-field drift rejects without a partial plan") &&
         expect(!moveRejected.receipt.accepted &&
                    moveRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::InvalidRequest,
                "out-of-range reorder fails closed") &&
         expect(!capacityRejected.receipt.accepted &&
                    capacityRejected.receipt.status ==
                        cr::CreativeTerrainOperationMutationStatus::CapacityExceeded &&
                    capacityRejected.stack.operations.empty(),
                "operation capacity rejects atomically");
}

bool documentOwnsReplayAndDirectReplacementCollapsesProvenance() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Operations");
  constexpr cr::CreativeTerrainHeightFieldBounds bounds{{0, 0}, 2U, 2U};
  cr::CreativeTerrainOperationMutationRequest add = addRequest(4U);
  add.composition.mode = cr::CreativeTerrainCompositionMode::Raise;
  const std::uint64_t revisionBefore = document.revision();
  const cr::CreativeTerrainOperationMutationReceipt operation =
      document.applyTerrainOperationMutation(add);
  const cr::CreativeTerrainControlEdit control{
      cr::CreativeTerrainEditKind::Upsert, {{0, 0}, 10U, 1U}};
  const cr::CreativeTerrainMutationReceipt terrain =
      document.applyTerrainControlEdits(std::span{&control, 1U});
  const bool controlReplayedThroughStack =
      document.terrainOperationStack().operations.size() == 1U &&
      document.terrainHeightField().heightAt({0, 0}) == 10U;
  constexpr std::array<std::uint16_t, 4U> replacementHeights{7U, 7U, 7U,
                                                             7U};
  const cr::CreativeTerrainHeightFieldReplaceReceipt replacement =
      document.replaceTerrainHeightField(bounds, replacementHeights);

  return expect(operation.accepted && operation.changed &&
                    document.revision() > revisionBefore,
                "document commits stack and derived terrain atomically") &&
         expect(terrain.accepted && terrain.changed &&
                    terrain.controlCountAfter == 1U &&
                    controlReplayedThroughStack,
                "legacy control edit replays the active operation stack") &&
         expect(replacement.accepted && replacement.changed &&
                    document.terrainOperationStack().operations.empty() &&
                    document.terrainOperationStack()
                            .baseHeightField.cellCount() == 0U &&
                    document.terrainHeightField().heightAt({0, 0}) == 7U,
                "direct replacement becomes a baked field and clears provenance");
}

}  // namespace

int main() {
  return orderedReplayAndMutationLifecycle() &&
                 updateAndReplayAreDeterministic() &&
                 driftInvalidRequestsAndCapacityFailClosed() &&
                 documentOwnsReplayAndDirectReplacementCollapsesProvenance()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
