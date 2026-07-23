#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/recipes/CreativeRecipe.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingTraversal.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutReconciliation.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

app::CreativeDesktopCommandResult dispatchOne(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id);
  return app::dispatchCreativeDesktopCommands(frame, context);
}

template <typename Payload>
app::CreativeDesktopCommandResult dispatchPayload(
    app::CreativeDesktopCommandId id,
    const app::CreativeDesktopCommandContext& context, Payload payload) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(payload));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

class TemporarySaveRoot {
 public:
  TemporarySaveRoot()
      : path_(std::filesystem::temp_directory_path() /
              "iggy3d_building_refinement_workflow_tests") {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
    std::filesystem::create_directories(path_, error);
  }

  ~TemporarySaveRoot() {
    std::error_code error;
    std::filesystem::remove_all(path_, error);
  }

  [[nodiscard]] const std::filesystem::path& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

const cr::CreativeObject* findGeneratedSource(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout, cr::CreativeWorldLayoutTable table,
    std::size_t index) {
  for (const cr::CreativeObject& object : document.objects()) {
    const cr::CreativeWorldLayoutObjectProvenance provenance =
        cr::resolveCreativeWorldLayoutObjectProvenance(layout, object);
    if (provenance.owned && provenance.table == table &&
        provenance.index == index) {
      return &object;
    }
  }
  return nullptr;
}

bool findConflictForObject(
    const cr::CreativeWorldLayoutCompileResult& compiled,
    cr::CreativeObjectId objectId, std::string& instanceKey,
    cr::CreativeWorldLayoutRecipeMemberConflict& output) {
  for (const cr::CreativeWorldLayoutRecipeChange& change :
       compiled.recipeChanges) {
    for (const cr::CreativeWorldLayoutRecipeMemberConflict& conflict :
         change.memberConflicts) {
      if (conflict.objectId == objectId) {
        instanceKey = change.instanceKey;
        output = conflict;
        return true;
      }
    }
  }
  return false;
}

bool sameTransform(const cr::CreativeTransform& lhs,
                   const cr::CreativeTransform& rhs) noexcept {
  const auto near = [](double first, double second) {
    return std::fabs(first - second) <= 1.0e-9;
  };
  return near(lhs.position.x, rhs.position.x) &&
         near(lhs.position.y, rhs.position.y) &&
         near(lhs.position.z, rhs.position.z) &&
         near(lhs.rotationEulerRadians.x, rhs.rotationEulerRadians.x) &&
         near(lhs.rotationEulerRadians.y, rhs.rotationEulerRadians.y) &&
         near(lhs.rotationEulerRadians.z, rhs.rotationEulerRadians.z) &&
         near(lhs.scale.x, rhs.scale.x) && near(lhs.scale.y, rhs.scale.y) &&
         near(lhs.scale.z, rhs.scale.z);
}

bool runtimeTraversalReady(const app::CreativeEditorState& editor,
                           const cr::CreativeAppState& appState) {
  const cr::CreativeWorldLayoutBuildingTraversalReceipt traversal =
      cr::validateCreativeWorldLayoutBuildingTraversal(
          {&editor.worldLayout.source, &appState.facade.document(), {}});
  return traversal.accepted && traversal.traversable &&
         traversal.roomCount == 2U && traversal.passageCount == 1U &&
         traversal.connectorCount == 1U;
}

app::CreativeDesktopCommandResult setScale(
    const app::CreativeDesktopCommandContext& context,
    cr::CreativeObjectId objectId, cr::CreativeVec3 scale) {
  const cr::CreativeObject* object =
      context.appState.facade.findObject(objectId);
  if (object == nullptr) {
    return {};
  }
  cr::CreativeTransform transform = object->transform;
  transform.scale = scale;
  return dispatchPayload(
      app::CreativeDesktopCommandId::SetObjectTransform, context,
      app::CreativeDesktopTransformPayload{objectId, transform, false, false,
                                           true});
}

bool multiStoreyRefinementsSurviveTheCompleteLifecycle() {
  TemporarySaveRoot saveRoot;
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Building Refinement Workflow");
  static_cast<void>(document.assignId(9971U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "building_refinement_workflow");
  std::string saveId = "building_refinement_workflow";
  const app::CreativeDesktopCommandContext context{
      appState, editor, saveRoot.path(), &saveId};

  app::CreativeEditorWorldLayoutBuildingBlockoutSettings blockout;
  blockout.shell.footprint = {{0, 0}, {12, 10}};
  blockout.floorToFloorCells = 3U;
  blockout.shell.wallThicknessCells = 0.25;
  blockout.pattern =
      cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  blockout.connectRooms = false;
  blockout.facade.includeEntrance = true;
  blockout.facade.includeExteriorWindows = false;
  blockout.storeys.count = 2U;
  blockout.storeys.connectStoreys = true;
  const app::CreativeDesktopCommandResult staged = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
      context,
      app::CreativeDesktopWorldLayoutBuildingBlockoutPayload{blockout});
  if (!staged.accepted || editor.worldLayout.source.buildings.size() != 1U) {
    return expect(false, "two-storey refinement fixture stages");
  }

  // One-to-one building-owned geometry is the intentionally invertible raw-3D
  // refinement case. Condensed room, wall, opening, and connector output stays
  // source-edited because it has no honest one-object inverse.
  cr::CreativeWorldLayoutBox plinth;
  plinth.buildingIndex = 0U;
  plinth.kind = cr::CreativeObjectKind::Floor;
  plinth.stableKey = "refinement_plinth";
  plinth.name = "Refinement Plinth";
  plinth.footprint = {{14, 0}, {16, 2}};
  plinth.anchorLayer = 0.0;
  plinth.layerCount = 1U;
  editor.worldLayout.source.boxes.push_back(plinth);
  ++editor.worldLayout.revision;
  ++editor.worldLayout.sourceEpoch;

  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeObject* generatedPlinth = findGeneratedSource(
      appState.facade.document(), editor.worldLayout.source,
      cr::CreativeWorldLayoutTable::Box, 0U);
  if (!generated.accepted || generatedPlinth == nullptr ||
      !runtimeTraversalReady(editor, appState)) {
    return expect(false,
                  "building, plinth, entrance, and stair generate together");
  }
  const cr::CreativeObjectId plinthId = generatedPlinth->id;

  const app::CreativeDesktopCommandResult scaledForAdoption =
      setScale(context, plinthId, {2.0, 1.0, 1.0});
  const cr::CreativeWorldLayoutRect footprintBeforeAdoption =
      editor.worldLayout.source.boxes[0].footprint;
  const app::CreativeDesktopCommandResult adopted = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutAdoptObjectSource, context,
      app::CreativeDesktopWorldLayoutObjectSourcePayload{plinthId});
  const cr::CreativeObject* adoptedPlinth =
      appState.facade.document().findObject(plinthId);
  const bool adoptionReady =
      scaledForAdoption.accepted && scaledForAdoption.changed &&
      adopted.accepted && adopted.changed && adopted.worldLayoutChanged &&
      adopted.sceneChanged && adoptedPlinth != nullptr &&
      (editor.worldLayout.source.boxes[0].footprint.minimum !=
           footprintBeforeAdoption.minimum ||
       editor.worldLayout.source.boxes[0].footprint.maximum !=
           footprintBeforeAdoption.maximum) &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision;
  if (!adoptionReady) {
    std::cerr << "adoption result: transform=" << scaledForAdoption.accepted
              << '/' << scaledForAdoption.changed << " adopt="
              << adopted.accepted << '/' << adopted.changed << '/'
              << adopted.worldLayoutChanged << '/' << adopted.sceneChanged
              << " message=" << adopted.message << " object="
              << (adoptedPlinth != nullptr) << " footprint="
              << editor.worldLayout.source.boxes[0].footprint.minimum.x << ','
              << editor.worldLayout.source.boxes[0].footprint.minimum.z << '-'
              << editor.worldLayout.source.boxes[0].footprint.maximum.x << ','
              << editor.worldLayout.source.boxes[0].footprint.maximum.z
              << " revisions=" << editor.worldLayout.generatedRevision << '/'
              << editor.worldLayout.revision << '\n';
  }
  if (!expect(adoptionReady,
              "representable 3D refinement explicitly adopts into its box source")) {
    return false;
  }

  const app::CreativeDesktopCommandResult scaledForOverride =
      setScale(context, plinthId, {1.0, 1.0, 1.5});
  const cr::CreativeObject* refinedPlinth =
      appState.facade.document().findObject(plinthId);
  if (refinedPlinth == nullptr) {
    return expect(false, "adopted plinth remains addressable");
  }
  const cr::CreativeTransform refinement = refinedPlinth->transform;
  app::CreativeEditorWorldLayoutBoxSettings changedBox;
  if (!app::readCreativeEditorWorldLayoutBoxSettings(editor.worldLayout, 0U,
                                                      changedBox)) {
    return expect(false, "plinth source settings remain editable");
  }
  ++changedBox.footprint.maximum.x;
  const app::CreativeDesktopCommandResult sourceChanged = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetBoxSettings, context,
      app::CreativeDesktopWorldLayoutBoxSettingsPayload{0U, changedBox});
  const app::CreativeDesktopCommandResult blockedOverride = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeWorldLayoutCompileResult overrideConflict =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       editor.worldLayout.source);
  std::string overrideInstance;
  cr::CreativeWorldLayoutRecipeMemberConflict overrideMember;
  if (!findConflictForObject(overrideConflict, plinthId, overrideInstance,
                             overrideMember)) {
    return expect(false, "concurrent plinth edit reports its exact member");
  }
  const cr::CreativeWorldLayoutConflictDecision keepRefinement =
      cr::makeCreativeWorldLayoutMemberConflictDecision(
          overrideInstance, overrideMember,
          cr::CreativeWorldLayoutConflictResolution::KeepRefinement);
  const app::CreativeDesktopCommandResult overridden = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context,
      app::CreativeDesktopWorldLayoutConfirmPayload{{keepRefinement}, {}});
  const cr::CreativeObject* overriddenPlinth =
      appState.facade.document().findObject(plinthId);
  if (!expect(scaledForOverride.accepted && sourceChanged.accepted &&
                  sourceChanged.worldLayoutChanged &&
                  !blockedOverride.accepted &&
                  blockedOverride.message ==
                      "creative_world_layout_refinement_conflict" &&
                  overridden.accepted && overridden.changed &&
                  overriddenPlinth != nullptr &&
                  sameTransform(overriddenPlinth->transform, refinement),
              "Keep 3D is an explicit stable override, never a silent winner")) {
    return false;
  }

  const app::CreativeDesktopCommandResult renamed = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutRenameSource, context,
      app::CreativeDesktopWorldLayoutSourceRenamePayload{
          cr::CreativeWorldLayoutTable::Building, 0U,
          editor.worldLayout.source.buildings[0].stableKey,
          "Refinement House"});
  const app::CreativeDesktopCommandResult regeneratedSibling = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeObject* preservedPlinth =
      appState.facade.document().findObject(plinthId);
  if (!expect(renamed.accepted && regeneratedSibling.accepted &&
                  preservedPlinth != nullptr &&
                  sameTransform(preservedPlinth->transform, refinement) &&
                  runtimeTraversalReady(editor, appState),
              "unrelated building regeneration preserves the explicit override")) {
    return false;
  }

  const app::CreativeDesktopCommandResult saved = dispatchOne(
      app::CreativeDesktopCommandId::SaveDocument, context);
  const app::CreativeDesktopCommandResult cleared = dispatchOne(
      app::CreativeDesktopCommandId::NewDocument, context);
  const app::CreativeDesktopCommandResult reopened = dispatchOne(
      app::CreativeDesktopCommandId::OpenDocument, context);
  const cr::CreativeObject* reopenedPlinth =
      appState.facade.document().findObject(plinthId);
  if (!expect(saved.accepted && cleared.accepted && reopened.accepted &&
                  reopened.documentReplaced && reopenedPlinth != nullptr &&
                  sameTransform(reopenedPlinth->transform, refinement) &&
                  editor.worldLayout.source.boxes.size() == 1U &&
                  editor.worldLayout.generatedRevision ==
                      editor.worldLayout.revision &&
                  runtimeTraversalReady(editor, appState),
              "source, generated identity, override, and traversal survive save and reopen")) {
    return false;
  }

  app::CreativeEditorWorldLayoutBoxSettings regeneratedBox;
  if (!app::readCreativeEditorWorldLayoutBoxSettings(editor.worldLayout, 0U,
                                                      regeneratedBox)) {
    return expect(false, "reopened plinth source remains editable");
  }
  ++regeneratedBox.footprint.maximum.z;
  const app::CreativeDesktopCommandResult sourceChangedAgain = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetBoxSettings, context,
      app::CreativeDesktopWorldLayoutBoxSettingsPayload{0U, regeneratedBox});
  const cr::CreativeWorldLayoutCompileResult regenerateConflict =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       editor.worldLayout.source);
  std::string regenerateInstance;
  cr::CreativeWorldLayoutRecipeMemberConflict regenerateMember;
  if (!findConflictForObject(regenerateConflict, plinthId, regenerateInstance,
                             regenerateMember)) {
    return expect(false,
                  "reopened override remains a state-bound source conflict");
  }
  const cr::CreativeWorldLayoutConflictDecision useSource =
      cr::makeCreativeWorldLayoutMemberConflictDecision(
          regenerateInstance, regenerateMember,
          cr::CreativeWorldLayoutConflictResolution::UseSource);
  const app::CreativeDesktopCommandResult regenerated = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context,
      app::CreativeDesktopWorldLayoutConfirmPayload{{useSource}, {}});
  const cr::CreativeObject* regeneratedPlinth =
      appState.facade.document().findObject(plinthId);
  const cr::CreativeWorldLayoutCompileResult stableAfterRegenerate =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       editor.worldLayout.source);
  if (!expect(sourceChangedAgain.accepted && regenerated.accepted &&
                  regenerated.changed && regeneratedPlinth != nullptr &&
                  !sameTransform(regeneratedPlinth->transform, refinement) &&
                  stableAfterRegenerate.receipt.accepted &&
                  stableAfterRegenerate.receipt.status ==
                      cr::CreativeWorldLayoutStatus::NoChange &&
                  runtimeTraversalReady(editor, appState),
              "Use 2D explicitly regenerates the exact refined member in place")) {
    return false;
  }

  const app::CreativeDesktopCommandResult scaledForDetach =
      setScale(context, plinthId, {1.25, 1.0, 1.0});
  const cr::CreativeObject* detachCandidate =
      appState.facade.document().findObject(plinthId);
  if (detachCandidate == nullptr) {
    return expect(false, "regenerated plinth remains addressable");
  }
  const cr::CreativeTransform detachedTransform = detachCandidate->transform;
  const std::string plinthStableKey =
      editor.worldLayout.source.boxes[0].stableKey;
  const app::CreativeDesktopCommandResult sourceRemoved = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutDeleteSource, context,
      app::CreativeDesktopWorldLayoutSourcePayload{
          cr::CreativeWorldLayoutTable::Box, 0U, plinthStableKey});
  const app::CreativeDesktopCommandResult blockedDetach = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  const cr::CreativeWorldLayoutCompileResult detachConflict =
      cr::buildCreativeWorldLayoutPlan(appState.facade.document(),
                                       editor.worldLayout.source);
  std::string detachInstance;
  cr::CreativeWorldLayoutRecipeMemberConflict detachMember;
  if (!findConflictForObject(detachConflict, plinthId, detachInstance,
                             detachMember)) {
    return expect(false, "removed refined plinth reports its exact member");
  }
  const cr::CreativeWorldLayoutConflictDecision detachDecision =
      cr::makeCreativeWorldLayoutMemberConflictDecision(
          detachInstance, detachMember,
          cr::CreativeWorldLayoutConflictResolution::DetachMember);
  const app::CreativeDesktopCommandResult detached = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context,
      app::CreativeDesktopWorldLayoutConfirmPayload{{detachDecision}, {}});
  const cr::CreativeObject* detachedPlinth =
      appState.facade.document().findObject(plinthId);
  const bool detachedState =
      detachedPlinth != nullptr &&
      sameTransform(detachedPlinth->transform, detachedTransform) &&
      cr::creativeRecipeObjectInstanceKey(*detachedPlinth).empty() &&
      !cr::resolveCreativeWorldLayoutObjectProvenance(
           editor.worldLayout.source, *detachedPlinth)
           .owned;
  if (!expect(scaledForDetach.accepted && sourceRemoved.accepted &&
                  sourceRemoved.worldLayoutChanged &&
                  !blockedDetach.accepted && detached.accepted &&
                  detached.changed && editor.worldLayout.source.boxes.empty() &&
                  detachedState && runtimeTraversalReady(editor, appState),
              "Detach keeps refined 3D identity while releasing source ownership")) {
    return false;
  }

  const app::CreativeDesktopCommandResult savedDetached = dispatchOne(
      app::CreativeDesktopCommandId::SaveDocument, context);
  const app::CreativeDesktopCommandResult clearedAgain = dispatchOne(
      app::CreativeDesktopCommandId::NewDocument, context);
  const app::CreativeDesktopCommandResult reopenedDetached = dispatchOne(
      app::CreativeDesktopCommandId::OpenDocument, context);
  const cr::CreativeObject* durableDetached =
      appState.facade.document().findObject(plinthId);
  return expect(savedDetached.accepted && clearedAgain.accepted &&
                    reopenedDetached.accepted && durableDetached != nullptr &&
                    sameTransform(durableDetached->transform,
                                  detachedTransform) &&
                    editor.worldLayout.source.boxes.empty() &&
                    cr::creativeRecipeObjectInstanceKey(*durableDetached)
                        .empty() &&
                    runtimeTraversalReady(editor, appState),
                "detached refinement and traversable building survive another reopen");
}

}  // namespace

int main() {
  return multiStoreyRefinementsSurviveTheCompleteLifecycle()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
