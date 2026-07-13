#include "EditorAuthoredAssets.hpp"
#include "EditorObjectActions.hpp"
#include "EditorPreviewFrame.hpp"
#include "EditorState.hpp"
#include "EditorToolOptions.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"
#include "core/math/Mat4.hpp"

namespace {
namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs) {
  return std::fabs(lhs - rhs) < 1.0e-8;
}

bool vecNear(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) {
  return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

const cr::CreativeObject* clipboardObjectNamed(
    const cr::CreativeClipboard& clipboard,
    std::string_view name) {
  const auto found = std::find_if(
      clipboard.objects.begin(), clipboard.objects.end(),
      [name](const cr::CreativeObject& object) {
        return object.name == name;
      });
  return found == clipboard.objects.end() ? nullptr : &*found;
}

cr::CreativeObjectId createCrate(cr::CreativeDocument& document,
                                 std::string_view name,
                                 cr::CreativeVec3 position,
                                 std::optional<cr::CreativeObjectId> parent =
                                     std::nullopt) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = std::string{name};
  request.transform.position = position;
  request.hasTransformOverride = true;
  request.parentId = parent;
  return document.createObject(request).objectId;
}

void selectOnly(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  cr::CreativeToolInputPacket input;
  input.kind = cr::CreativeToolInputKind::PointerPress;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = static_cast<cr::Id>(objectId);
  static_cast<void>(facade.dispatchToolInput(input));
}

cr::CreativeAuthoredAssetCaptureResult makeTwoCrateDefinition() {
  cr::CreativeDocument source = cr::CreativeDocument::create("Source");
  static_cast<void>(source.assignId(701U));
  const cr::CreativeObjectId left =
      createCrate(source, "Left", {10.0, 0.0, 20.0});
  const cr::CreativeObjectId right =
      createCrate(source, "Right", {12.0, 0.0, 20.0});
  const std::array selected{left, right};
  return cr::captureCreativeAuthoredAsset(
      {&source, selected, "authored_0001", "Twin Crates", 702U});
}

bool captureInstantiateSelectAndUnpack() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  if (!expect(captured.accepted && captured.capturedObjectCount == 2U &&
                  captured.storageDocument.objectCount() == 3U,
              "selection captures with a durable source-root wrapper")) {
    return false;
  }

  cr::CreativeDocument target = cr::CreativeDocument::create("Target");
  static_cast<void>(target.assignId(703U));
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = &captured.definition;
  placement.targetAnchor = {5.0, 1.0, -3.0};
  placement.yawRadians = 1.5707963267948966;
  const cr::CreativeAuthoredAssetInstanceReceipt placed =
      cr::instantiateCreativeAuthoredAssetAtomically(target, placement);
  const cr::CreativeObject* root =
      target.findObject(placed.instanceRootObjectId);
  const cr::CreativeObjectId childId =
      placed.instanceObjectIds.empty() ? cr::kInvalidObjectId
                                       : placed.instanceObjectIds.front();
  const cr::CreativeObject* child = target.findObject(childId);
  const cr::CreativeObjectId selectedRoot =
      cr::resolveCreativeHierarchyInteractionRoot(
          target, std::span{&placed.instanceRootObjectId, 1U}, childId);
  const bool rootFacts =
      root != nullptr &&
      root->kind == cr::CreativeObjectKind::PrefabInstance &&
      !root->visible && root->assetId == "authored_0001";
  const cr::CreativeObjectWorldExtent rootExtent =
      root != nullptr ? cr::resolveCreativeObjectWorldExtent(*root)
                      : cr::CreativeObjectWorldExtent{};
  const cr::CreativeBoundsMetrics sourceMetrics =
      cr::measureCreativeBounds(captured.definition.sourceBounds);
  const cr::CreativeVec3 rotatedSourceCenter =
      cr::rotateCreativeVectorEulerXyz(
          sourceMetrics.center, root != nullptr
                                    ? root->transform.rotationEulerRadians
                                    : cr::CreativeVec3{});
  const cr::CreativeVec3 expectedRootCenter{
      placement.targetAnchor.x + rotatedSourceCenter.x,
      placement.targetAnchor.y + rotatedSourceCenter.y,
      placement.targetAnchor.z + rotatedSourceCenter.z};
  const cr::CreativeVec3 actualRootCenter =
      cr::measureCreativeBounds({rootExtent.min, rootExtent.max}).center;
  const bool childFacts =
      child != nullptr && child->parentId == placed.instanceRootObjectId &&
      selectedRoot == placed.instanceRootObjectId;
  const std::size_t placedObjectCount = target.objectCount();

  const cr::CreativeGroupCommandReceipt unpacked =
      cr::ungroupDocumentObjectAtomically(target,
                                          placed.instanceRootObjectId);
  return expect(placed.accepted && placed.changed &&
                    placedObjectCount == 3U,
                "authored definition instantiates root and expanded children") &&
         expect(rootFacts,
                "instance root carries semantic identity without rendering") &&
         expect(rootExtent.valid &&
                    std::fabs(actualRootCenter.x - expectedRootCenter.x) <
                        1.0e-9 &&
                    std::fabs(actualRootCenter.y - expectedRootCenter.y) <
                        1.0e-9 &&
                    std::fabs(actualRootCenter.z - expectedRootCenter.z) <
                        1.0e-9,
                "instance root resolves local bounds through one transform") &&
         expect(childFacts,
                "visible child resolves to the instance interaction root") &&
         expect(unpacked.accepted && unpacked.changed &&
                    target.findObject(placed.instanceRootObjectId) == nullptr &&
                    target.objectCount() == 2U &&
                    !target.findObject(childId)->parentId.has_value(),
                "unpack removes only the instance container");
}

bool invalidDefinitionCannotPartiallyMutate() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  captured.definition.rootObjectIds.push_back(999'999U);
  cr::CreativeDocument target = cr::CreativeDocument::create("Atomic");
  static_cast<void>(target.assignId(704U));
  const std::uint64_t revisionBefore = target.revision();
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = &captured.definition;
  placement.targetAnchor = {1.0, 0.0, 1.0};
  const cr::CreativeAuthoredAssetInstanceReceipt rejected =
      cr::instantiateCreativeAuthoredAssetAtomically(target, placement);
  return expect(!rejected.accepted && !rejected.changed &&
                    target.objectCount() == 0U &&
                    target.revision() == revisionBefore,
                "missing source-root remap rejects the complete transaction");
}

bool definitionsAreBoundedAndFailClosed() {
  cr::CreativeDocument oversized = cr::CreativeDocument::create("Oversized");
  static_cast<void>(oversized.assignId(708U));
  for (std::size_t index = 0U;
       index <= cr::kCreativeAuthoredAssetObjectCapacity; ++index) {
    static_cast<void>(createCrate(
        oversized, "Part", {static_cast<double>(index), 0.0, 0.0}));
  }
  const cr::CreativeAuthoredAssetLoadResult capacity =
      cr::loadCreativeAuthoredAssetDefinition(
          oversized, "authored_oversized", "Oversized");

  cr::CreativeDocument valid = cr::CreativeDocument::create("Valid");
  static_cast<void>(valid.assignId(709U));
  static_cast<void>(createCrate(valid, "Part", {}));
  const cr::CreativeAuthoredAssetLoadResult identity =
      cr::loadCreativeAuthoredAssetDefinition(valid, "invalid id", "Valid");
  cr::CreativeAuthoredAssetLoadResult loaded =
      cr::loadCreativeAuthoredAssetDefinition(
          valid, "authored_valid", "Valid");
  cr::CreativeAuthoredAssetPlacementRequest request;
  request.definition = &loaded.definition;
  request.targetAnchor.x = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeAuthoredAssetPlacementPlan placement =
      cr::planCreativeAuthoredAssetPlacement(request);

  return expect(!capacity.accepted &&
                    capacity.status ==
                        cr::CreativeAuthoredAssetStatus::CapacityExceeded,
                "definitions reject more than 256 expanded objects") &&
         expect(!identity.accepted &&
                    identity.status ==
                        cr::CreativeAuthoredAssetStatus::InvalidIdentity,
                "durable identifiers reject unsupported characters") &&
         expect(loaded.accepted && !placement.accepted &&
                    placement.status ==
                        cr::CreativeAuthoredAssetStatus::InvalidGeometry,
                "non-finite placement fails before mutation");
}

bool transformedInstanceUpdateFailsClosed() {
  const cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  cr::CreativeDocument target = cr::CreativeDocument::create("Scaled");
  static_cast<void>(target.assignId(714U));
  cr::CreativeAuthoredAssetPlacementRequest placement;
  placement.definition = &captured.definition;
  const cr::CreativeAuthoredAssetInstanceReceipt placed =
      cr::instantiateCreativeAuthoredAssetAtomically(target, placement);
  const cr::CreativeDocumentMutationReceipt scaled = cr::applyDocumentMutation(
      target, placed.instanceRootObjectId, cr::CreativeMutationKind::Scale,
      cr::CreativeMutationPayload{
          cr::ScaleMutation{{2.0, 1.0, 1.0}}});
  const cr::CreativeObject* root =
      target.findObject(placed.instanceRootObjectId);
  cr::CreativeAuthoredAssetInstanceCaptureRequest update;
  update.sourceDocument = &target;
  update.existingDefinition = &captured.definition;
  update.instanceRootObjectId = placed.instanceRootObjectId;
  update.definitionDocumentId = 715U;
  const cr::CreativeAuthoredAssetCaptureResult rejected =
      cr::captureCreativeAuthoredAssetInstance(update);
  return expect(placed.accepted &&
                    cr::documentMutationChanged(scaled.status) &&
                    root != nullptr &&
                    !cr::creativeAuthoredAssetInstanceTransformSupported(
                        *root) &&
                    !rejected.accepted &&
                    rejected.status ==
                        cr::CreativeAuthoredAssetStatus::InvalidGeometry,
                "scaled wrappers reject source update without partial output");
}

bool durableLibraryRoundTripsSelection() {
  const auto nonce = std::chrono::steady_clock::now()
                         .time_since_epoch()
                         .count();
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_authored_asset_test_" + std::to_string(nonce));
  cr::CreativeAppState appState;
  cr::CreativeDocument document = cr::CreativeDocument::create("Library");
  static_cast<void>(document.assignId(705U));
  const cr::CreativeObjectId objectId =
      createCrate(document, "Pillar", {4.0, 0.0, 8.0});
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "library test document installed")) {
    return false;
  }
  selectOnly(appState.facade, objectId);
  app::CreativeEditorAuthoredAssetLibrary library;
  const app::CreativeEditorAuthoredAssetLoadReceipt initialized =
      app::loadCreativeEditorAuthoredAssetLibrary(library, root);
  const app::CreativeEditorAuthoredAssetSaveReceipt saved =
      app::saveCreativeEditorSelectionAsAuthoredAsset(appState, library);
  app::CreativeEditorState editor;
  editor.authoredAssets = std::move(library);
  editor.catalog.model = cr::makeCreativeCatalog(
      std::span<const cr::CreativeObjectKind>{});
  editor.toolOptions.open = true;
  editor.toolOptions.commands =
      app::creativeEditorToolOptionCommandsForEntry(
          {cr::CreativeHeldItemKind::ObjectMove,
           cr::CreativeObjectKind::Unknown});
  app::refreshCreativeEditorObjectActionContext(appState,
                                                editor.toolOptions);
  const auto saveCommand = std::find(
      editor.toolOptions.commands.ids.begin(),
      editor.toolOptions.commands.ids.begin() +
          editor.toolOptions.commands.count,
      app::CreativeEditorToolOptionsCommandId::SaveSelectionAsAsset);
  editor.toolOptions.selectedIndex = static_cast<std::size_t>(
      saveCommand - editor.toolOptions.commands.ids.begin());
  const bool commandSaved =
      saveCommand != editor.toolOptions.commands.ids.begin() +
                         editor.toolOptions.commands.count &&
      app::activateCreativeEditorToolOptionsSelection(appState, editor);
  const std::string equippedAssetId(
      cr::creativeHotbarAssetId(cr::selectedCreativeHotbarEntry(
          editor.interaction.hotbar)));
  const bool catalogPublished = std::any_of(
      editor.catalog.model.entries.begin(),
      editor.catalog.model.entries.end(),
      [&equippedAssetId](const cr::CreativeCatalogEntry& entry) {
        return entry.authoredComposite &&
               cr::creativeHotbarAssetId(entry.hotbarEntry) ==
                   equippedAssetId;
      });
  app::CreativeEditorAuthoredAssetLibrary reloaded;
  const app::CreativeEditorAuthoredAssetLoadReceipt loaded =
      app::loadCreativeEditorAuthoredAssetLibrary(reloaded, root);
  const cr::CreativeAuthoredAssetDefinition* definition =
      app::findCreativeEditorAuthoredAsset(reloaded, saved.assetId);
  const cr::CreativeAuthoredAssetDefinition* commandDefinition =
      app::findCreativeEditorAuthoredAsset(reloaded, equippedAssetId);
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);

  return expect(initialized.accepted && saved.accepted &&
                    saved.durableWriteOk &&
                    saved.capture.definition.assetId == saved.assetId &&
                    saved.capture.definition.content.objects.size() == 1U,
                "durable save preserves complete capture receipt facts") &&
         expect(commandSaved && equippedAssetId == "authored_0002" &&
                    catalogPublished &&
                    cr::selectedCreativeHotbarEntry(
                        editor.interaction.hotbar)
                            .objectKind ==
                        cr::CreativeObjectKind::PrefabInstance,
                "save command publishes and equips the authored asset") &&
         expect(loaded.accepted && loaded.loadedCount == 2U &&
                    definition != nullptr && definition->label == "Pillar" &&
                    definition->content.objects.size() == 1U &&
                    commandDefinition != nullptr,
                "startup scan reconstructs the authored definition");
}

bool updateCommandRoundTripsEditedInstance() {
  const auto nonce = std::chrono::steady_clock::now()
                         .time_since_epoch()
                         .count();
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_authored_asset_update_test_" + std::to_string(nonce));
  cr::CreativeAppState sourceState;
  cr::CreativeDocument source = cr::CreativeDocument::create("Source");
  static_cast<void>(source.assignId(711U));
  const cr::CreativeObjectId sourceObjectId =
      createCrate(source, "Pillar", {});
  if (!expect(sourceState.facade.installDocument(std::move(source)).accepted,
              "update source document installed")) {
    return false;
  }
  selectOnly(sourceState.facade, sourceObjectId);
  app::CreativeEditorAuthoredAssetLibrary library;
  const app::CreativeEditorAuthoredAssetLoadReceipt initialized =
      app::loadCreativeEditorAuthoredAssetLibrary(library, root);
  const app::CreativeEditorAuthoredAssetSaveReceipt saved =
      app::saveCreativeEditorSelectionAsAuthoredAsset(sourceState, library);
  const cr::CreativeAuthoredAssetDefinition* savedDefinition =
      app::findCreativeEditorAuthoredAsset(library, saved.assetId);
  if (!expect(initialized.accepted && saved.accepted &&
                  savedDefinition != nullptr,
              "update source asset initialized")) {
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return false;
  }
  const cr::CreativeAuthoredAssetDefinition originalDefinition =
      *savedDefinition;

  cr::CreativeDocument instances = cr::CreativeDocument::create("Instances");
  static_cast<void>(instances.assignId(712U));
  cr::CreativeAuthoredAssetPlacementRequest firstPlacement;
  firstPlacement.definition = savedDefinition;
  firstPlacement.targetAnchor = {10.0, 0.0, 10.0};
  firstPlacement.yawRadians = 1.5707963267948966;
  const cr::CreativeAuthoredAssetInstanceReceipt first =
      cr::instantiateCreativeAuthoredAssetAtomically(instances,
                                                     firstPlacement);
  cr::CreativeAuthoredAssetPlacementRequest secondPlacement;
  secondPlacement.definition = savedDefinition;
  secondPlacement.targetAnchor = {20.0, 0.0, 20.0};
  const cr::CreativeAuthoredAssetInstanceReceipt second =
      cr::instantiateCreativeAuthoredAssetAtomically(instances,
                                                     secondPlacement);
  const cr::CreativeObjectId addedObjectId = createCrate(
      instances, "Added Detail", {10.0, 0.0, 12.0},
      first.instanceRootObjectId);
  cr::CreativeAppState appState;
  if (!expect(first.accepted && second.accepted &&
                  addedObjectId != cr::kInvalidObjectId &&
                  appState.facade.installDocument(std::move(instances)).accepted,
              "two instances and one local edit installed")) {
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    return false;
  }
  selectOnly(appState.facade, first.instanceRootObjectId);

  app::CreativeEditorState editor;
  editor.authoredAssets = std::move(library);
  const std::vector<cr::CreativeCatalogAsset> catalogAssets =
      app::creativeEditorAuthoredAssetCatalogEntries(editor.authoredAssets);
  editor.catalog.model = cr::makeCreativeCatalog(
      std::span<const cr::CreativeObjectKind>{}, catalogAssets);
  editor.interaction.hotbar.selectedSlot = 0U;
  cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  held = {cr::CreativeHeldItemKind::Material,
          cr::CreativeObjectKind::PrefabInstance};
  static_cast<void>(cr::setCreativeHotbarAsset(
      held, originalDefinition.assetId, originalDefinition.sourceBounds));
  editor.toolOptions.open = true;
  editor.toolOptions.commands =
      app::creativeEditorToolOptionCommandsForEntry(
          {cr::CreativeHeldItemKind::ObjectGroup,
           cr::CreativeObjectKind::Unknown});
  app::refreshCreativeEditorObjectActionContext(appState,
                                                editor.toolOptions);
  const auto updateCommand = std::find(
      editor.toolOptions.commands.ids.begin(),
      editor.toolOptions.commands.ids.begin() +
          editor.toolOptions.commands.count,
      app::CreativeEditorToolOptionsCommandId::UpdateSavedAsset);
  editor.toolOptions.selectedIndex = static_cast<std::size_t>(
      updateCommand - editor.toolOptions.commands.ids.begin());
  const std::uint64_t revisionBeforeUpdate =
      appState.facade.document().revision();
  const bool updated =
      updateCommand != editor.toolOptions.commands.ids.begin() +
                           editor.toolOptions.commands.count &&
      app::activateCreativeEditorToolOptionsSelection(appState, editor);

  const cr::CreativeAuthoredAssetDefinition* updatedDefinition =
      app::findCreativeEditorAuthoredAsset(editor.authoredAssets,
                                          originalDefinition.assetId);
  const cr::CreativeHierarchySelection untouchedSecond =
      cr::resolveCreativeObjectHierarchy(
          appState.facade.document(),
          std::span{&second.instanceRootObjectId, 1U});
  const cr::CreativeObject* originalSourceObject =
      clipboardObjectNamed(originalDefinition.content, "Pillar");
  const cr::CreativeObject* updatedSourceObject =
      updatedDefinition == nullptr
          ? nullptr
          : clipboardObjectNamed(updatedDefinition->content, "Pillar");
  const bool sourceFramePreserved =
      originalSourceObject != nullptr && updatedSourceObject != nullptr &&
      vecNear(originalSourceObject->transform.position,
              updatedSourceObject->transform.position) &&
      vecNear(originalSourceObject->transform.rotationEulerRadians,
              updatedSourceObject->transform.rotationEulerRadians);
  const bool hotbarRefreshed =
      updatedDefinition != nullptr && held.hasAssetBounds &&
      cr::creativeBoundsExactlyEqual(held.assetSourceBounds,
                                     updatedDefinition->sourceBounds);
  const bool catalogRefreshed =
      updatedDefinition != nullptr &&
      std::any_of(editor.catalog.model.entries.begin(),
                  editor.catalog.model.entries.end(),
                  [updatedDefinition](const cr::CreativeCatalogEntry& entry) {
                    return entry.authoredComposite &&
                           cr::creativeHotbarAssetId(entry.hotbarEntry) ==
                               updatedDefinition->assetId &&
                           cr::creativeBoundsExactlyEqual(
                               entry.hotbarEntry.assetSourceBounds,
                               updatedDefinition->sourceBounds);
                  });

  app::CreativeEditorAuthoredAssetLibrary reloaded;
  const app::CreativeEditorAuthoredAssetLoadReceipt loaded =
      app::loadCreativeEditorAuthoredAssetLibrary(reloaded, root);
  const cr::CreativeAuthoredAssetDefinition* durableDefinition =
      app::findCreativeEditorAuthoredAsset(reloaded,
                                          originalDefinition.assetId);
  cr::CreativeDocument replay = cr::CreativeDocument::create("Replay");
  static_cast<void>(replay.assignId(713U));
  cr::CreativeAuthoredAssetPlacementRequest replayPlacement;
  replayPlacement.definition = durableDefinition;
  const cr::CreativeAuthoredAssetInstanceReceipt replayed =
      durableDefinition == nullptr
          ? cr::CreativeAuthoredAssetInstanceReceipt{}
          : cr::instantiateCreativeAuthoredAssetAtomically(replay,
                                                           replayPlacement);
  const cr::CreativeObject* replaySourceObject = nullptr;
  for (cr::CreativeObjectId objectId : replayed.instanceObjectIds) {
    const cr::CreativeObject* object = replay.findObject(objectId);
    if (object != nullptr && object->name == "Pillar") {
      replaySourceObject = object;
      break;
    }
  }
  std::error_code ignored;
  std::filesystem::remove_all(root, ignored);

  return expect(updated && updatedDefinition != nullptr &&
                    updatedDefinition->content.objects.size() == 2U,
                "explicit update captures the edited instance contents") &&
         expect(appState.facade.document().revision() ==
                        revisionBeforeUpdate &&
                    untouchedSecond.accepted &&
                    untouchedSecond.objectIds.size() == 2U,
                "source update does not rewrite existing instances") &&
         expect(sourceFramePreserved,
                "rotated instance update removes placement yaw") &&
         expect(hotbarRefreshed && catalogRefreshed,
                "updated source bounds refresh catalog and hotbar facts") &&
         expect(loaded.accepted && loaded.loadedCount == 1U &&
                    durableDefinition != nullptr &&
                    durableDefinition->content.objects.size() == 2U,
                "updated source survives durable reload") &&
         expect(replayed.accepted &&
                    replayed.instanceObjectIds.size() == 2U &&
                    replaySourceObject != nullptr,
                "reloaded source instantiates every edited child") &&
         expect(replaySourceObject != nullptr &&
                    vecNear(replaySourceObject->transform.position,
                            originalSourceObject->transform.position),
                "reloaded placement preserves the source-local frame");
}

app::CreativeEditorState authoredEditor(
    const cr::CreativeAuthoredAssetDefinition& definition) {
  app::CreativeEditorState editor;
  editor.authoredAssets.definitions.push_back(definition);
  editor.interaction.hotbar.selectedSlot = 0U;
  cr::CreativeHotbarEntry& held = editor.interaction.hotbar.entries[0];
  held = {cr::CreativeHeldItemKind::Material,
          cr::CreativeObjectKind::PrefabInstance};
  static_cast<void>(cr::setCreativeHotbarAsset(
      held, definition.assetId, definition.sourceBounds));
  editor.placeCellSize = 1.0;
  editor.interaction.target.valid = true;
  editor.interaction.target.grid.valid = true;
  editor.interaction.target.grid.faceNormal = {0.0, 1.0, 0.0};
  editor.interaction.target.grid.placerForward = {0.0, 0.0, -1.0};
  editor.interaction.target.grid.placementAnchor = {0.5, 0.0, 0.5};
  editor.interaction.target.grid.adjacentCellBounds =
      {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
  return editor;
}

void setSecondary(cr::CreativeWorldActionFrame& actions,
                  bool down,
                  bool pressed,
                  bool released) {
  const std::size_t index =
      static_cast<std::size_t>(cr::CreativeWorldActionId::Secondary);
  actions.down[index] = down;
  actions.pressed[index] = pressed;
  actions.released[index] = released;
}

bool gestureDeduplicatesAndCommitsOneUndo() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  cr::CreativeAppState appState;
  cr::CreativeDocument target = cr::CreativeDocument::create("Gestures");
  static_cast<void>(target.assignId(706U));
  if (!expect(appState.facade.installDocument(std::move(target)).accepted,
              "gesture document installed")) {
    return false;
  }
  app::CreativeEditorState editor = authoredEditor(captured.definition);
  cr::CreativeWorldActionFrame press;
  setSecondary(press, true, true, false);
  app::processCreativeAuthoredAssetFrame(appState, editor, press, 0U);
  const std::size_t firstCount = appState.facade.document().objectCount();
  const std::uint64_t firstRevision = appState.facade.document().revision();

  cr::CreativeWorldActionFrame held;
  setSecondary(held, true, false, false);
  app::processCreativeAuthoredAssetFrame(
      appState, editor, held, 199'000'000ULL);
  const bool beforeRepeatStable =
      appState.facade.document().revision() == firstRevision;
  app::processCreativeAuthoredAssetFrame(
      appState, editor, held, 200'000'000ULL);
  const bool stationaryDeduplicated =
      appState.facade.document().objectCount() == firstCount;

  editor.interaction.target.grid.placementAnchor = {2.5, 0.0, 0.5};
  editor.interaction.target.grid.adjacentCellBounds =
      {{2.0, 0.0, 0.0}, {3.0, 1.0, 1.0}};
  app::processCreativeAuthoredAssetFrame(
      appState, editor, held, 400'000'000ULL);
  const std::size_t movedCount = appState.facade.document().objectCount();
  cr::CreativeWorldActionFrame release;
  setSecondary(release, false, false, true);
  app::processCreativeAuthoredAssetFrame(
      appState, editor, release, 401'000'000ULL);
  const std::uint64_t completedRevision =
      appState.facade.document().revision();
  app::processCreativeAuthoredAssetFrame(
      appState, editor, press, 500'000'000ULL);
  app::processCreativeAuthoredAssetFrame(
      appState, editor, release, 501'000'000ULL);
  const bool crossGestureDuplicateRejected =
      appState.facade.document().revision() == completedRevision &&
      appState.facade.document().objectCount() == movedCount;
  const std::size_t undoDepth = cr::creativeUndoDepth(appState.history);
  const cr::CreativeHistoryApplyReceipt undo = cr::applyCreativeHistory(
      appState.facade, appState.history, cr::CreativeHistoryDirection::Undo);

  return expect(firstCount == 3U && editor.placedCount == 2U,
                "first press places one expanded instance immediately") &&
         expect(beforeRepeatStable && stationaryDeduplicated,
                "199 ms is stable and repeated target mutates once") &&
         expect(movedCount == 6U,
                "new target at the next cadence places another instance") &&
         expect(crossGestureDuplicateRejected,
                "a later gesture cannot duplicate the occupied target") &&
         expect(undoDepth == 1U && undo.accepted &&
                    undo.objectCountAfter == 0U,
                "press-hold-release commits exactly one undo record");
}

bool interruptionFinalizesChangedGesture() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  cr::CreativeAppState appState;
  cr::CreativeDocument target = cr::CreativeDocument::create("Interrupted");
  static_cast<void>(target.assignId(707U));
  if (!expect(appState.facade.installDocument(std::move(target)).accepted,
              "interruption document installed")) {
    return false;
  }
  app::CreativeEditorState editor = authoredEditor(captured.definition);
  cr::CreativeWorldActionFrame press;
  setSecondary(press, true, true, false);
  app::processCreativeAuthoredAssetFrame(appState, editor, press, 0U);
  app::finalizeCreativeEditorContinuousGestures(
      appState, editor, "creative_authored_asset_test_interruption");
  return expect(appState.facade.document().objectCount() == 3U &&
                    cr::creativeUndoDepth(appState.history) == 1U &&
                    !editor.interaction.authoredAssetStroke.repeat.active &&
                    !editor.interaction.authoredAssetStroke.transaction.active,
                "focus/modal/tool interruption commits and clears the stroke");
}

bool previewUsesCanonicalCompositeProxies() {
  cr::CreativeAuthoredAssetCaptureResult captured =
      makeTwoCrateDefinition();
  app::CreativeEditorState editor = authoredEditor(captured.definition);
  iggy3d::FrameInput frame;
  frame.camera.clipFromWorld = iggy3d::identityMat4();
  frame.camera.clipFromView = iggy3d::identityMat4();
  app::attachCreativeEditorPlacementPreviews(editor, false, frame);
  const bool commandsIncludeAuthoring = [&]() {
    const app::CreativeEditorToolOptionsCommandList commands =
        app::creativeEditorToolOptionCommandsForEntry(
            {cr::CreativeHeldItemKind::ObjectMove,
             cr::CreativeObjectKind::Unknown});
    bool save = false;
    bool edit = false;
    bool update = false;
    for (std::size_t index = 0U; index < commands.count; ++index) {
      save = save || commands.ids[index] ==
                         app::CreativeEditorToolOptionsCommandId::
                             SaveSelectionAsAsset;
      edit = edit || commands.ids[index] ==
                         app::CreativeEditorToolOptionsCommandId::
                             EditGroupContents;
      update = update || commands.ids[index] ==
                             app::CreativeEditorToolOptionsCommandId::
                                 UpdateSavedAsset;
    }
    return commands.count == 11U && save && edit && update;
  }();
  return expect(frame.creativePreview.itemCount == 2U &&
                    frame.creativePreview.items[0].role ==
                        iggy3d::RenderCreativePreviewRole::PlacementValid &&
                    frame.creativePreview.items[1].role ==
                        iggy3d::RenderCreativePreviewRole::Held,
                "authored asset shows target and held proxies") &&
         expect(iggy3d::renderCreativePreviewAssetId(
                    frame.creativePreview.items[0])
                    .empty() &&
                    iggy3d::renderCreativePreviewAssetId(
                        frame.creativePreview.items[1])
                        .empty(),
                "composite proxies never request a missing static mesh") &&
         expect(commandsIncludeAuthoring,
                "object tool exposes edit and save-as-asset commands");
}

}  // namespace

int main() {
  return captureInstantiateSelectAndUnpack() &&
                 invalidDefinitionCannotPartiallyMutate() &&
                 definitionsAreBoundedAndFailClosed() &&
                 transformedInstanceUpdateFailsClosed() &&
                 durableLibraryRoundTripsSelection() &&
                 updateCommandRoundTripsEditedInstance() &&
                 gestureDeduplicatesAndCommitsOneUndo() &&
                 interruptionFinalizesChangedGesture() &&
                 previewUsesCanonicalCompositeProxies()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
