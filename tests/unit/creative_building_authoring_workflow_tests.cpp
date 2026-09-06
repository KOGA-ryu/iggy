#include "EditorDesktopCommands.hpp"
#include "EditorState.hpp"
#include "EditorWorldLayout.hpp"

#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/history/History.hpp"
#include "app/iggy3d/creative/world/WorldLayoutBuildingUsability.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutProvenance.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/physics/PhysicsSpatialSurfaceColliderBake.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
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

bool near(double lhs, double rhs, double epsilon = 1.0e-9) {
  return std::fabs(lhs - rhs) <= epsilon;
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
    const app::CreativeDesktopCommandContext& context,
    Payload payload) {
  app::CreativeDesktopCommandFrame frame;
  frame.push(id, std::move(payload));
  return app::dispatchCreativeDesktopCommands(frame, context);
}

std::size_t countKind(const cr::CreativeDocument& document,
                      cr::CreativeObjectKind kind) {
  return static_cast<std::size_t>(std::count_if(
      document.objects().begin(), document.objects().end(),
      [kind](const cr::CreativeObject& object) { return object.kind == kind; }));
}

bool allObjectsRetainSemanticOwnership(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& source) {
  return std::all_of(
      document.objects().begin(), document.objects().end(),
      [&source](const cr::CreativeObject& object) {
        return cr::resolveCreativeWorldLayoutObjectProvenance(source, object)
            .owned;
      });
}

bool containsCollisionRole(
    const iggy3d::PhysicsSpatialSurfaceColliderBakeResult& physics,
    iggy3d::CollisionSurfaceRole role) {
  return std::find(physics.sourceRoles.begin(), physics.sourceRoles.end(),
                   role) != physics.sourceRoles.end();
}

bool hasObjectTopPlane(const cr::CreativeDocument& document,
                       cr::CreativeObjectKind kind, double top) {
  return std::any_of(
      document.objects().begin(), document.objects().end(),
      [kind, top](const cr::CreativeObject& object) {
        const cr::CreativeTransformedBounds bounds =
            cr::resolveCreativeObjectBounds(object);
        return object.kind == kind && bounds.valid &&
               near(bounds.worldBounds.max.y, top);
      });
}

bool residentialStoreysResolveAgainstTheDocumentGrid() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Half Meter Residential Blockout");
  static_cast<void>(document.assignId(9900U));
  cr::CreativeGridSettings grid = document.gridSettings();
  grid.cellSizeMeters = 0.5;
  static_cast<void>(document.setGridSettings(grid));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "half_meter_residential_blockout");
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings blockout =
      app::makeCreativeEditorWorldLayoutBlockoutDraft();
  blockout.storeys.count = 3U;
  blockout.facade.includeEntrance = false;
  blockout.facade.includeExteriorWindows = false;

  const app::CreativeDesktopCommandResult staged = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
      context,
      app::CreativeDesktopWorldLayoutBuildingBlockoutPayload{blockout});
  app::CreativeEditorWorldLayoutBuildingBlockoutSettings resolved;
  const bool resolvedReadable =
      app::readCreativeEditorWorldLayoutBuildingBlockoutSettings(
          editor.worldLayout, 0U, resolved);
  const cr::CreativeWorldLayoutLevelDimensions ground =
      cr::measureCreativeWorldLayoutLevelDimensions(
          appState.facade.document().gridSettings(),
          editor.worldLayout.source, 0U);
  const cr::CreativeWorldLayoutLevelDimensions middle =
      cr::measureCreativeWorldLayoutLevelDimensions(
          appState.facade.document().gridSettings(),
          editor.worldLayout.source, 1U);
  const cr::CreativeWorldLayoutLevelDimensions top =
      cr::measureCreativeWorldLayoutLevelDimensions(
          appState.facade.document().gridSettings(),
          editor.worldLayout.source, 2U);
  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);

  return expect(
      staged.accepted && resolvedReadable &&
          resolved.architecturalProfileKind ==
              cr::CreativeWorldLayoutArchitecturalProfileKind::Residential &&
          resolved.floorToFloorCells == 6U &&
          ground.accepted && middle.accepted && top.accepted &&
          near(ground.floorTopMeters, 0.0) &&
          near(middle.floorTopMeters, 3.0) &&
          near(top.floorTopMeters, 6.0) &&
          near(ground.upperSurfaceTopMeters, middle.floorBottomMeters) &&
          near(middle.upperSurfaceTopMeters, top.floorBottomMeters) &&
          generated.accepted &&
          hasObjectTopPlane(appState.facade.document(),
                            cr::CreativeObjectKind::Floor, 0.0) &&
          hasObjectTopPlane(appState.facade.document(),
                            cr::CreativeObjectKind::Floor, 3.0) &&
          hasObjectTopPlane(appState.facade.document(),
                            cr::CreativeObjectKind::Floor, 6.0) &&
          hasObjectTopPlane(appState.facade.document(),
                            cr::CreativeObjectKind::Ceiling,
                            ground.upperSurfaceTopMeters) &&
          hasObjectTopPlane(appState.facade.document(),
                            cr::CreativeObjectKind::Ceiling,
                            middle.upperSurfaceTopMeters),
      "residential three-storey geometry resolves against a half-meter grid");
}

bool twoStoreyBuildingSurvivesTheCompleteAuthoringWorkflow() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("Building Authoring Workflow");
  static_cast<void>(document.assignId(9901U));
  static_cast<void>(appState.facade.installDocument(std::move(document)));

  app::CreativeEditorState editor;
  app::resetCreativeEditorWorldLayout(editor.worldLayout,
                                      "building_authoring_workflow");
  std::string saveId = "unused";
  const app::CreativeDesktopCommandContext context{
      appState, editor, std::filesystem::path{}, &saveId};

  app::CreativeEditorWorldLayoutBuildingBlockoutSettings blockout;
  blockout.shell.footprint = {{0, 0}, {12, 10}};
  blockout.shell.floorTopLayer = 0.0;
  blockout.floorToFloorCells = 3U;
  blockout.shell.wallThicknessCells = 0.25;
  blockout.shell.floorThicknessLayers = 1U;
  blockout.shell.roofThicknessLayers = 1U;
  blockout.pattern = cr::CreativeWorldLayoutBuildingBlockoutPattern::SingleRoom;
  blockout.connectRooms = false;
  blockout.facade.includeEntrance = true;
  blockout.facade.includeExteriorWindows = false;
  blockout.storeys.count = 2U;
  blockout.storeys.connectStoreys = true;

  const app::CreativeDesktopCommandResult staged = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCreateBuildingBlockout,
      context,
      app::CreativeDesktopWorldLayoutBuildingBlockoutPayload{blockout});
  if (!expect(staged.accepted && staged.changed &&
                  staged.worldLayoutChanged && !staged.sceneChanged &&
                  editor.worldLayout.source.buildings.size() == 1U &&
                  editor.worldLayout.source.levels.size() == 2U &&
                  editor.worldLayout.source.rooms.size() == 2U &&
                  editor.worldLayout.source.verticalConnectors.size() == 1U &&
                  editor.worldLayout.source.openings.size() == 1U &&
                  appState.facade.document().objectCount() == 0U &&
                  cr::creativeUndoDepth(appState.history) == 0U,
              "desktop command stages one complete two-storey recipe")) {
    return false;
  }

  const cr::CreativeWorldLayoutLevelDimensions groundDimensions =
      cr::measureCreativeWorldLayoutLevelDimensions(
          appState.facade.document().gridSettings(),
          editor.worldLayout.source, 0U);
  const cr::CreativeWorldLayoutLevelDimensions upperDimensions =
      cr::measureCreativeWorldLayoutLevelDimensions(
          appState.facade.document().gridSettings(),
          editor.worldLayout.source, 1U);
  const cr::CreativeWorldLayoutBuildingDimensions buildingDimensions =
      cr::measureCreativeWorldLayoutBuildingDimensions(
          appState.facade.document().gridSettings(),
          editor.worldLayout.source, 0U);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(editor.worldLayout.source);
  const std::size_t continuousFacadeCount =
      expanded.accepted
          ? static_cast<std::size_t>(std::count_if(
                expanded.expanded.walls.begin(),
                expanded.expanded.walls.end(),
                [](const cr::CreativeWorldLayoutWall& wall) {
                  return wall.profile ==
                             cr::CreativeWorldLayoutWallProfile::Exterior &&
                         near(wall.baseLayer, 0.0) &&
                         wall.heightCells == 6U;
                }))
          : 0U;
  const double expectedFloorToFloorMeters =
      static_cast<double>(blockout.floorToFloorCells) *
      appState.facade.document().gridSettings().cellSizeMeters;
  if (!expect(groundDimensions.accepted && upperDimensions.accepted &&
                  buildingDimensions.accepted &&
                  groundDimensions.hasUpperLevel &&
                  groundDimensions.upperLevelIndex == 1U &&
                  !upperDimensions.hasUpperLevel &&
                  near(groundDimensions.floorToFloorMeters,
                       expectedFloorToFloorMeters) &&
                  near(groundDimensions.wallTopMeters,
                       groundDimensions.nextFloorTopMeters) &&
                  near(groundDimensions.upperSurfaceTopMeters,
                       upperDimensions.floorBottomMeters) &&
                  groundDimensions.clearHeightMeters <
                      groundDimensions.floorToFloorMeters &&
                  buildingDimensions.occupiedLevelCount == 2U &&
                  buildingDimensions.uniformFloorToFloor &&
                  continuousFacadeCount == 4U,
              "canonical fixture owns two datums, clear height, and four continuous facades")) {
    return false;
  }

  const app::CreativeDesktopCommandResult generated = dispatchOne(
      app::CreativeDesktopCommandId::WorldLayoutConfirm, context);
  std::vector<double> generatedFloorTopPlanes;
  double generatedUpperFloorBottom = 0.0;
  bool foundGeneratedUpperFloor = false;
  bool foundGeneratedCeiling = false;
  bool generatedCeilingsBackUpperSlab = true;
  for (const cr::CreativeObject& object :
       appState.facade.document().objects()) {
    const cr::CreativeTransformedBounds bounds =
        cr::resolveCreativeObjectBounds(object);
    if (object.kind == cr::CreativeObjectKind::Floor && bounds.valid) {
      const double top = bounds.worldBounds.max.y;
      if (std::none_of(generatedFloorTopPlanes.begin(),
                       generatedFloorTopPlanes.end(),
                       [top](double existing) {
                         return near(top, existing);
                       })) {
        generatedFloorTopPlanes.push_back(top);
      }
      if (near(top, upperDimensions.floorTopMeters)) {
        if (!foundGeneratedUpperFloor) {
          generatedUpperFloorBottom = bounds.worldBounds.min.y;
        }
        foundGeneratedUpperFloor = true;
        generatedUpperFloorBottom =
            std::min(generatedUpperFloorBottom, bounds.worldBounds.min.y);
      }
    } else if (object.kind == cr::CreativeObjectKind::Ceiling &&
               bounds.valid) {
      foundGeneratedCeiling = true;
      generatedCeilingsBackUpperSlab =
          generatedCeilingsBackUpperSlab &&
          near(bounds.worldBounds.max.y,
               groundDimensions.upperSurfaceTopMeters);
    }
  }
  std::sort(generatedFloorTopPlanes.begin(),
            generatedFloorTopPlanes.end());
  const bool generatedStoreyPlanesMatch =
      generatedFloorTopPlanes.size() == 2U &&
      foundGeneratedUpperFloor && foundGeneratedCeiling &&
      generatedCeilingsBackUpperSlab &&
      near(generatedFloorTopPlanes[0],
           groundDimensions.floorTopMeters) &&
      near(generatedFloorTopPlanes[1],
           upperDimensions.floorTopMeters) &&
      near(groundDimensions.upperSurfaceTopMeters,
           generatedUpperFloorBottom);
  if (!expect(generated.accepted && generated.changed &&
                  generated.sceneChanged &&
                  editor.worldLayout.generatedRevision ==
                      editor.worldLayout.revision &&
                  countKind(appState.facade.document(),
                            cr::CreativeObjectKind::Floor) >= 2U &&
                  countKind(appState.facade.document(),
                            cr::CreativeObjectKind::Ceiling) >= 1U &&
                  countKind(appState.facade.document(),
                            cr::CreativeObjectKind::Roof) == 1U &&
                  countKind(appState.facade.document(),
                            cr::CreativeObjectKind::Stair) == 1U &&
                  countKind(appState.facade.document(),
                            cr::CreativeObjectKind::Door) == 1U &&
                  cr::creativeUndoDepth(appState.history) == 1U &&
                  generatedStoreyPlanesMatch,
              "generation installs floors, ceiling, roof, and stair atomically")) {
    return false;
  }

  const app::CreativeDesktopCommandResult selectedGround = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutLevelOperation, context,
      app::CreativeDesktopWorldLayoutLevelOperationPayload{
          app::CreativeEditorWorldLayoutLevelOperation::Select, 0U, 0U});
  const app::CreativeDesktopCommandResult wallTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Wall});
  const app::CreativeDesktopCommandResult wallBegin = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Begin, {1.0, 2.0}});
  const std::uint64_t wallSourceRevision = editor.worldLayout.revision;
  const std::uint64_t wallDocumentRevision =
      appState.facade.document().revision();
  const std::uint64_t wallUndoDepth = cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult wallPreview = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Update, {11.0, 2.0}});
  if (!expect(selectedGround.accepted && wallTool.accepted &&
                  wallBegin.accepted && !wallBegin.changed &&
                  wallPreview.accepted && wallPreview.changed &&
                  wallPreview.sceneChanged &&
                  !wallPreview.worldLayoutChanged &&
                  editor.worldLayout.source.walls.empty() &&
                  editor.worldLayout.revision == wallSourceRevision &&
                  appState.facade.document().revision() ==
                      wallDocumentRevision &&
                  cr::creativeUndoDepth(appState.history) == wallUndoDepth,
              "wall drag previews exact 3D without mutating source or history")) {
    return false;
  }

  const app::CreativeDesktopCommandResult wallCommitted = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasGesture, context,
      app::CreativeDesktopWorldLayoutGesturePayload{
          app::CreativeEditorWorldLayoutGesturePhase::Commit, {11.0, 2.0}});
  if (!expect(wallCommitted.accepted && wallCommitted.changed &&
                  wallCommitted.worldLayoutChanged &&
                  wallCommitted.sceneChanged &&
                  editor.worldLayout.source.walls.size() == 1U &&
                  editor.worldLayout.generatedRevision ==
                      editor.worldLayout.revision &&
                  cr::creativeUndoDepth(appState.history) == wallUndoDepth + 1U,
              "wall release commits one synchronized edit")) {
    return false;
  }

  const app::CreativeDesktopCommandResult doorTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Door});
  const app::CreativeDesktopCommandResult doorPlaced = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{4.0, 2.0}});
  if (!expect(doorTool.accepted && doorPlaced.accepted &&
                  doorPlaced.changed && doorPlaced.worldLayoutChanged &&
                  doorPlaced.sceneChanged &&
                  editor.worldLayout.source.openings.size() == 2U &&
                  editor.worldLayout.source.openings[1].hostKind ==
                      cr::CreativeWorldLayoutOpeningHostKind::Wall &&
                  editor.worldLayout.source.openings[1].wallIndex == 0U &&
                  editor.worldLayout.source.openings[1].kind ==
                      cr::CreativeBuildingOpeningKind::Door &&
                  cr::creativeUndoDepth(appState.history) == wallUndoDepth + 2U,
              "door point snaps to the authored partition and commits once")) {
    return false;
  }

  const app::CreativeDesktopCommandResult windowTool = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutSetTool, context,
      app::CreativeDesktopWorldLayoutToolPayload{
          app::CreativeEditorWorldLayoutTool::Window});
  const app::CreativeDesktopCommandResult windowPlaced = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutCanvasPoint, context,
      app::CreativeDesktopWorldLayoutPointPayload{{3.0, 0.0}});
  if (!expect(windowTool.accepted && windowPlaced.accepted &&
                  windowPlaced.changed && windowPlaced.worldLayoutChanged &&
                  windowPlaced.sceneChanged &&
                  editor.worldLayout.source.openings.size() == 3U &&
                  editor.worldLayout.source.openings[2].hostKind ==
                      cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                  editor.worldLayout.source.openings[2].kind ==
                      cr::CreativeBuildingOpeningKind::Window &&
                  cr::creativeUndoDepth(appState.history) == wallUndoDepth + 3U,
              "window point snaps to the exterior shell and commits once")) {
    return false;
  }

  app::CreativeEditorWorldLayoutLevelSettings upperSettings;
  if (!expect(app::readCreativeEditorWorldLayoutLevelSettings(
                  editor.worldLayout, 1U, upperSettings),
              "upper level exposes shared property settings")) {
    return false;
  }
  upperSettings.name = "Upper Storey Edited";
  upperSettings.wallHeightCells = 4U;
  const std::string upperStableKey =
      editor.worldLayout.source.levels[1].stableKey;
  const auto propertyPayload =
      [&](app::CreativeDesktopWorldLayoutPropertyEditPhase phase) {
        return app::CreativeDesktopWorldLayoutPropertyEditPayload{
            phase, cr::CreativeWorldLayoutTable::Level, 1U, upperStableKey,
            upperSettings};
      };
  const std::uint64_t propertySourceRevision = editor.worldLayout.revision;
  const std::uint64_t propertyDocumentRevision =
      appState.facade.document().revision();
  const std::uint64_t propertyUndoDepth =
      cr::creativeUndoDepth(appState.history);
  const app::CreativeDesktopCommandResult propertyPreview = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      propertyPayload(
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Preview));
  if (!expect(propertyPreview.accepted && propertyPreview.sceneChanged &&
                  !propertyPreview.worldLayoutChanged &&
                  app::creativeEditorWorldLayoutPreviewActive(
                      editor.worldLayout) &&
                  editor.worldLayout.source.levels[1].wallHeightCells == 3U &&
                  editor.worldLayout.revision == propertySourceRevision &&
                  appState.facade.document().revision() ==
                      propertyDocumentRevision &&
                  cr::creativeUndoDepth(appState.history) == propertyUndoDepth,
              "shared property preview remains transient")) {
    return false;
  }

  const app::CreativeDesktopCommandResult propertyCommitted = dispatchPayload(
      app::CreativeDesktopCommandId::WorldLayoutEditSourceProperty, context,
      propertyPayload(
          app::CreativeDesktopWorldLayoutPropertyEditPhase::Commit));
  if (!expect(propertyCommitted.accepted && propertyCommitted.changed &&
                  propertyCommitted.worldLayoutChanged &&
                  propertyCommitted.sceneChanged &&
                  !app::creativeEditorWorldLayoutPreviewActive(
                      editor.worldLayout) &&
                  editor.worldLayout.source.levels[1].name ==
                      "Upper Storey Edited" &&
                  editor.worldLayout.source.levels[1].wallHeightCells == 4U &&
                  cr::creativeUndoDepth(appState.history) ==
                      propertyUndoDepth + 1U,
              "shared property commit rebuilds the authored building once")) {
    return false;
  }

  const cr::CreativeDocument& authored = appState.facade.document();
  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &authored;
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult roomBake =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::SpatialSurfaceSet surfaces =
      iggy3d::buildSpatialSurfaceSet(roomBake.room);
  const iggy3d::PhysicsSpatialSurfaceColliderBakeResult physics =
      iggy3d::bakePhysicsAabbCollidersFromSpatialSurfaces({&surfaces, {}});
  cr::CreativeWorldLayoutBuildingUsabilityConfig usabilityConfig;
  usabilityConfig.gridCellSizeMeters = authored.gridSettings().cellSizeMeters;
  const cr::CreativeWorldLayoutBuildingUsabilityReceipt usability =
      cr::validateCreativeWorldLayoutBuildingUsability(
          {&editor.worldLayout.source, &authored, usabilityConfig});
  if (!expect(countKind(authored, cr::CreativeObjectKind::Door) == 2U &&
                  countKind(authored, cr::CreativeObjectKind::Window) == 1U &&
                  allObjectsRetainSemanticOwnership(
                      authored, editor.worldLayout.source) &&
                  usability.accepted && usability.usable &&
                  usability.reachableRoomCount == 2U &&
                  roomBake.receipt.accepted &&
                  roomBake.receipt.objectCount == authored.objectCount() &&
                  roomBake.receipt.bakedSpatialSurfaceCount > 0U && physics.ok &&
                  physics.colliderCount > 0U &&
                  containsCollisionRole(physics,
                                        iggy3d::CollisionSurfaceRole::Walkable) &&
                  containsCollisionRole(physics,
                                        iggy3d::CollisionSurfaceRole::Blocker),
              "authored building keeps provenance and honest collision facts")) {
    return false;
  }

  const app::CreativeDesktopCommandResult undone =
      dispatchOne(app::CreativeDesktopCommandId::Undo, context);
  const bool undoRestoredBoth =
      undone.accepted &&
      editor.worldLayout.source.levels[1].name != "Upper Storey Edited" &&
      editor.worldLayout.source.levels[1].wallHeightCells == 3U &&
      editor.worldLayout.generatedRevision == editor.worldLayout.revision &&
      cr::creativeUndoDepth(appState.history) == propertyUndoDepth;
  const app::CreativeDesktopCommandResult redone =
      dispatchOne(app::CreativeDesktopCommandId::Redo, context);
  return expect(undoRestoredBoth && redone.accepted &&
                    editor.worldLayout.source.levels[1].name ==
                        "Upper Storey Edited" &&
                    editor.worldLayout.source.levels[1].wallHeightCells == 4U &&
                    editor.worldLayout.generatedRevision ==
                        editor.worldLayout.revision &&
                    cr::creativeUndoDepth(appState.history) ==
                        propertyUndoDepth + 1U &&
                    countKind(appState.facade.document(),
                              cr::CreativeObjectKind::Door) == 2U &&
                    countKind(appState.facade.document(),
                              cr::CreativeObjectKind::Window) == 1U,
                "undo and redo restore source and generated scene together");
}

}  // namespace

int main() {
  if (!residentialStoreysResolveAgainstTheDocumentGrid() ||
      !twoStoreyBuildingSurvivesTheCompleteAuthoringWorkflow()) {
    return 1;
  }
  std::cout << "creative building authoring workflow tests passed\n";
  return 0;
}
