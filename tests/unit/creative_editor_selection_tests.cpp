#include "EditorFrame.hpp"
#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"
#include "EditorPicking.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

app::ObjectVisualPickBounds candidate(cr::CreativeObjectId id,
                                      float minimumX,
                                      bool visible = true,
                                      bool locked = false) {
  app::ObjectVisualPickBounds result;
  result.id = id;
  result.bounds.min = {minimumX, -0.5F, -0.5F};
  result.bounds.max = {minimumX + 0.5F, 0.5F, 0.5F};
  result.visible = visible;
  result.locked = locked;
  return result;
}

app::WorldRay positiveXRay() {
  return {true, {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}};
}

bool hiddenIsExcludedAndLockedRemainsInspectable() {
  const std::vector<app::ObjectVisualPickBounds> candidates{
      candidate(1U, 1.0F, false, false),
      candidate(2U, 2.0F, true, true),
      candidate(3U, 3.0F),
  };
  const app::ObjectVisualPickStack stack =
      app::pickVisualBoundsObjectStack(candidates, positiveXRay());
  const app::ObjectVisualPickResult nearest =
      app::pickNearestVisualBoundsObject(candidates, positiveXRay());
  return expect(stack.rayValid && stack.count == 2U &&
                    stack.totalHitCount == 2U &&
                    stack.hiddenExcludedCount == 1U,
                "hidden objects are excluded from the 3D hit stack") &&
         expect(stack.items[0].objectId == 2U && stack.items[0].locked &&
                    stack.lockedHitCount == 1U && nearest.objectId == 2U,
                "locked objects remain selectable for inspection");
}

bool overlapOrderingAndCyclingAreDeterministic() {
  const std::vector<app::ObjectVisualPickBounds> candidates{
      candidate(9U, 2.0F),
      candidate(4U, 2.0F),
      candidate(7U, 4.0F),
  };
  const app::ObjectVisualPickStack stack =
      app::pickVisualBoundsObjectStack(candidates, positiveXRay());
  return expect(stack.count == 3U && stack.items[0].objectId == 4U &&
                    stack.items[1].objectId == 9U &&
                    stack.items[2].objectId == 7U,
                "equal-distance overlaps order by object id") &&
         expect(app::cycleObjectVisualPick(stack, cr::kInvalidObjectId) == 4U &&
                    app::cycleObjectVisualPick(stack, 4U) == 9U &&
                    app::cycleObjectVisualPick(stack, 9U) == 7U &&
                    app::cycleObjectVisualPick(stack, 7U) == 4U,
                "repeated plain selection cycles and wraps");
}

bool overlapStackIsBoundedAndMatchesBruteForce() {
  std::vector<app::ObjectVisualPickBounds> candidates;
  candidates.reserve(app::kObjectVisualPickHitCapacity + 1U);
  for (std::size_t index = 0U;
       index <= app::kObjectVisualPickHitCapacity; ++index) {
    candidates.push_back(candidate(
        static_cast<cr::CreativeObjectId>(index + 1U),
        1.0F + static_cast<float>(index)));
  }
  const app::ObjectVisualPickStack indexed =
      app::pickVisualBoundsObjectStack(candidates, positiveXRay());
  const app::ObjectVisualPickStack brute =
      app::pickVisualBoundsObjectStackBruteForce(candidates, positiveXRay());
  bool same = indexed.count == brute.count;
  for (std::size_t index = 0U; same && index < indexed.count; ++index) {
    same = indexed.items[index].objectId == brute.items[index].objectId &&
           indexed.items[index].entryDistance ==
               brute.items[index].entryDistance;
  }
  return expect(indexed.count == app::kObjectVisualPickHitCapacity &&
                    indexed.totalHitCount ==
                        app::kObjectVisualPickHitCapacity + 1U &&
                    indexed.truncated,
                "overlap stack reports its fixed capacity") &&
         expect(indexed.items.front().objectId == 1U &&
                    indexed.items[indexed.count - 1U].objectId ==
                        app::kObjectVisualPickHitCapacity,
                "bounded stack retains the nearest hits") &&
         expect(same && brute.truncated &&
                    indexed.totalHitCount == brute.totalHitCount,
                "indexed and brute-force stacks agree");
}

bool ps5RejectClearsObjectAndDraftingSelection() {
  cr::CreativeDocument document = cr::CreativeDocument::create("Select test");
  static_cast<void>(document.assignId(901U));
  cr::CreativeDocumentCreateRequest create;
  create.kind = cr::CreativeObjectKind::Crate;
  create.name = "Selected crate";
  const cr::CreativeDocumentCreateReceipt created =
      document.createObject(create);
  if (!created.accepted) {
    return expect(false, "selection fixture object is created");
  }

  cr::CreativeAppState appState;
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      appState.facade.installDocument(std::move(document));
  const std::array selectedIds{created.objectId};
  const cr::CreativeSelectionReceipt selected =
      appState.facade.selectTargets(selectedIds, created.objectId);
  if (!installed.accepted || !selected.accepted) {
    return expect(false, "selection fixture is installed and selected");
  }

  app::CreativeEditorState editor;
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0] = {
      cr::CreativeHeldItemKind::ObjectSelect,
      cr::CreativeObjectKind::Unknown};
  editor.worldLayout.selection = {
      app::CreativeEditorWorldLayoutSelectionKind::Building, 0U};
  cr::CreativeWorldActionFrame actions;
  const std::size_t rejectIndex =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Reject);
  actions.down[rejectIndex] = true;
  actions.pressed[rejectIndex] = true;
  iggy3d::RenderCameraFrame camera;
  app::CreativeEditorPickFrame pickFrame;
  const app::CreativeEditorWorldInteractionFrameRequest request{
      appState, editor, actions, cr::kCreativeInputModifierNone,
      camera, pickFrame, {0, 0, 640U, 480U}};

  app::processCreativeEditorHeldItemFrame(request);

  return expect(cr::selectedTargetCount(
                    appState.facade.selectionState()) == 0U &&
                    appState.facade.selectionState().selectedTarget.value ==
                        cr::kInvalidId,
                "PS5 Circle clears the object selection") &&
         expect(editor.worldLayout.selection.kind ==
                    app::CreativeEditorWorldLayoutSelectionKind::None,
                "selection clear synchronizes the drafting surface");
}

}  // namespace

int main() {
  const bool ok = hiddenIsExcludedAndLockedRemainsInspectable() &&
                  overlapOrderingAndCyclingAreDeterministic() &&
                  overlapStackIsBoundedAndMatchesBruteForce() &&
                  ps5RejectClearsObjectAndDraftingSelection();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
