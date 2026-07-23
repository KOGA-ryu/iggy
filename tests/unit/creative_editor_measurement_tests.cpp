#include "EditorMeasurement.hpp"
#include "EditorFrame.hpp"
#include "EditorGizmo.hpp"
#include "EditorPreviewFrameInternal.hpp"
#include "EditorState.hpp"

#include <cmath>
#include <iostream>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;
using namespace iggy3d_creative_app;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool near(double lhs, double rhs, double epsilon = 1.0e-9) {
  return std::abs(lhs - rhs) <= epsilon;
}

CreativeEditorWorldTarget targetAt(cr::CreativeVec3 point) {
  CreativeEditorWorldTarget target;
  target.valid = true;
  target.grid.valid = true;
  target.grid.resolved = true;
  target.grid.hitPoint = point;
  return target;
}

cr::CreativeDocument documentWithBox(cr::CreativeObjectId& objectId) {
  cr::CreativeDocument document = cr::CreativeDocument::create("measure");
  static_cast<void>(document.assignId(77U));
  static_cast<void>(document.setGridSettings(
      {{10.0, 2.0, -5.0}, 2.0, {32U, 32U, 32U}}));
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::Crate;
  request.name = "rotated box";
  request.bounds = {{-1.0, 0.0, -0.5}, {1.0, 2.0, 0.5}};
  request.hasBoundsOverride = true;
  request.transform.position = {4.0, 0.0, 3.0};
  request.transform.rotationEulerRadians.y = 0.5;
  request.hasTransformOverride = true;
  const cr::CreativeDocumentCreateReceipt receipt =
      document.createObject(request);
  objectId = receipt.objectId;
  return document;
}

cr::CreativeWorldLayout openingLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "measure_layout";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building_1";
  layout.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.floorTopLayer = 1.5;
  layout.levels.push_back(level);
  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.start = {0, 0};
  wall.end = {6, 0};
  wall.baseLayer = 1.5;
  wall.heightCells = 3U;
  layout.walls.push_back(wall);
  cr::CreativeWorldLayoutOpening opening;
  opening.wallIndex = 0U;
  opening.centerOffsetCells = 3.0;
  opening.widthCells = 1.0;
  opening.cutoutBottomCells = 0.5;
  opening.cutoutHeightCells = 2.0;
  opening.includeInsert = false;
  opening.stableKey = "opening_1";
  layout.openings.push_back(opening);
  return layout;
}

bool gridSurfaceAndLevelModesAreExact() {
  cr::CreativeObjectId ignored = cr::kInvalidObjectId;
  cr::CreativeDocument document = documentWithBox(ignored);
  CreativeEditorWorldTarget target = targetAt({12.6, 5.1, -1.2});

  CreativeEditorMeasurementPointResult grid =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, nullptr,
           cr::kInvalidCreativeWorldLayoutIndex,
           cr::CreativeMeasurementSnapMode::Grid, 1.0, 0.25});
  bool ok = expect(grid.accepted && grid.point.snapKind ==
                                        cr::CreativeMeasurementSnapKind::Grid,
                   "explicit grid snap succeeds") &&
            expect(near(grid.point.x, 13.0) && near(grid.point.y, 5.0) &&
                       near(grid.point.z, -1.0),
                   "grid snap honors document origin and requested step");

  const CreativeEditorMeasurementPointResult surface =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, nullptr,
           cr::kInvalidCreativeWorldLayoutIndex,
           cr::CreativeMeasurementSnapMode::Surface, 1.0, 0.25});
  ok = expect(surface.accepted && near(surface.point.x, 12.6) &&
                  near(surface.point.y, 5.1) && near(surface.point.z, -1.2) &&
                  surface.point.snapKind ==
                      cr::CreativeMeasurementSnapKind::Surface,
              "surface snap preserves exact hit point") &&
       ok;

  const cr::CreativeWorldLayout layout = openingLayout();
  const CreativeEditorMeasurementPointResult level =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, &layout, 0U,
           cr::CreativeMeasurementSnapMode::Level, 1.0, 0.25});
  return expect(level.accepted && near(level.point.x, 12.6) &&
                    near(level.point.y, 5.0) && near(level.point.z, -1.2) &&
                    level.point.snapKind ==
                        cr::CreativeMeasurementSnapKind::Level,
                "level snap preserves plan point and uses authored floor top") &&
         ok;
}

bool vertexUsesLiveTransformedBoundsAndAutoTolerance() {
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  cr::CreativeDocument document = documentWithBox(objectId);
  const cr::CreativeTransformedBounds bounds =
      cr::resolveCreativeObjectBounds(*document.findObject(objectId));
  CreativeEditorWorldTarget target = targetAt(
      {bounds.corners[7].x + 0.01, bounds.corners[7].y,
       bounds.corners[7].z - 0.01});
  target.objectHit = true;
  target.objectId = objectId;

  CreativeEditorMeasurementPointResult vertex =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, nullptr,
           cr::kInvalidCreativeWorldLayoutIndex,
           cr::CreativeMeasurementSnapMode::Vertex, 1.0, 0.25});
  bool ok = expect(vertex.accepted &&
                       vertex.point.snapKind ==
                           cr::CreativeMeasurementSnapKind::Vertex &&
                       vertex.point.target.value == objectId,
                   "vertex snap retains object provenance") &&
            expect(near(vertex.point.x, bounds.corners[7].x) &&
                       near(vertex.point.y, bounds.corners[7].y) &&
                       near(vertex.point.z, bounds.corners[7].z),
                   "vertex snap uses rotated transformed corners");

  CreativeEditorMeasurementPointResult automatic =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, nullptr,
           cr::kInvalidCreativeWorldLayoutIndex,
           cr::CreativeMeasurementSnapMode::Auto, 1.0, 0.05});
  ok = expect(automatic.accepted && automatic.appliedMode ==
                                        cr::CreativeMeasurementSnapMode::Vertex,
              "Auto chooses a nearby transformed vertex") &&
       ok;
  target.grid.hitPoint = bounds.center;
  automatic = resolveCreativeEditorMeasurementPoint(
      {&document, &target, nullptr, cr::kInvalidCreativeWorldLayoutIndex,
       cr::CreativeMeasurementSnapMode::Auto, 1.0, 0.05});
  return expect(automatic.accepted && automatic.appliedMode ==
                                          cr::CreativeMeasurementSnapMode::Surface,
                "Auto preserves the surface when no vertex is nearby") &&
         ok;
}

bool openingUsesAuthoredCutoutAnchorsAndWinsAuto() {
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  cr::CreativeDocument document = documentWithBox(objectId);
  cr::CreativeWorldLayout layout = openingLayout();
  cr::CreativeObject* object = document.findObject(objectId);
  object->tags.push_back(cr::creativeWorldLayoutTag(layout.stableKey));
  object->tags.push_back(cr::creativeWorldLayoutProvenanceTag(
      layout, cr::CreativeWorldLayoutTable::Opening, 0U));

  CreativeEditorWorldTarget target = targetAt({16.1, 8.1, -5.0});
  target.objectHit = true;
  target.objectId = objectId;
  const CreativeEditorMeasurementPointResult opening =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, &layout, 0U,
           cr::CreativeMeasurementSnapMode::Opening, 1.0, 0.25});
  bool ok = expect(opening.accepted && opening.appliedMode ==
                                           cr::CreativeMeasurementSnapMode::Opening,
                   "opening snap resolves generated-source provenance") &&
            expect(near(opening.point.x, 16.0) && near(opening.point.y, 8.0) &&
                       near(opening.point.z, -5.0),
                   "opening snap selects the nearest exact cutout anchor") &&
            expect(opening.worldLayoutSource.table ==
                           cr::CreativeWorldLayoutTable::Opening &&
                       opening.worldLayoutSource.index == 0U,
                   "opening endpoint exposes its semantic source");
  const CreativeEditorMeasurementPointResult automatic =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, &layout, 0U,
           cr::CreativeMeasurementSnapMode::Auto, 1.0, 0.0001});
  return expect(automatic.accepted && automatic.appliedMode ==
                                          cr::CreativeMeasurementSnapMode::Opening,
                "Auto prioritizes authored opening semantics over vertex proximity") &&
         ok;
}

bool explicitSemanticModesFailClosed() {
  cr::CreativeObjectId ignored = cr::kInvalidObjectId;
  cr::CreativeDocument document = documentWithBox(ignored);
  CreativeEditorWorldTarget target = targetAt({0.0, 0.0, 0.0});
  const CreativeEditorMeasurementPointResult vertex =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, nullptr,
           cr::kInvalidCreativeWorldLayoutIndex,
           cr::CreativeMeasurementSnapMode::Vertex, 1.0, 0.25});
  const CreativeEditorMeasurementPointResult opening =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, nullptr,
           cr::kInvalidCreativeWorldLayoutIndex,
           cr::CreativeMeasurementSnapMode::Opening, 1.0, 0.25});
  const CreativeEditorMeasurementPointResult level =
      resolveCreativeEditorMeasurementPoint(
          {&document, &target, nullptr,
           cr::kInvalidCreativeWorldLayoutIndex,
           cr::CreativeMeasurementSnapMode::Level, 1.0, 0.25});
  CreativeEditorWorldTarget invalid;
  const CreativeEditorMeasurementPointResult invalidTarget =
      resolveCreativeEditorMeasurementPoint(
          {&document, &invalid, nullptr,
           cr::kInvalidCreativeWorldLayoutIndex,
           cr::CreativeMeasurementSnapMode::Surface, 1.0, 0.25});
  return expect(!vertex.accepted && vertex.status ==
                                         CreativeEditorMeasurementPointStatus::MissingObject,
                "explicit vertex does not silently become another snap") &&
         expect(!opening.accepted && opening.status ==
                                          CreativeEditorMeasurementPointStatus::MissingOpening,
                "explicit opening does not silently become another snap") &&
         expect(!level.accepted && level.status ==
                                        CreativeEditorMeasurementPointStatus::MissingLevel,
                "explicit level does not silently become another snap") &&
         expect(!invalidTarget.accepted && invalidTarget.status ==
                                                CreativeEditorMeasurementPointStatus::InvalidTarget,
                "invalid target fails closed");
}

bool heldInteractionUsesFacadeWithoutMutatingAuthoringState() {
  cr::CreativeObjectId ignored = cr::kInvalidObjectId;
  cr::CreativeDocument document = documentWithBox(ignored);
  cr::CreativeAppState appState;
  const cr::CreativeFacadeDocumentInstallReceipt installed =
      appState.facade.installDocument(std::move(document));
  CreativeEditorState editor;
  editor.toolSettings.measurementMode = cr::CreativeMeasurementMode::Distance;
  editor.toolSettings.measurementSnapMode =
      cr::CreativeMeasurementSnapMode::Surface;
  editor.interaction.target = targetAt({1.0, 2.0, 3.0});
  cr::CreativeWorldActionFrame actions;
  iggy3d::RenderCameraFrame camera;
  CreativeEditorPickFrame pickFrame;
  const CreativeEditorWorldInteractionFrameRequest request{
      appState, editor, actions, cr::kCreativeInputModifierNone,
      camera, pickFrame, {}};

  const std::uint64_t revisionBefore = appState.facade.document().revision();
  const std::uint64_t objectCountBefore =
      appState.facade.document().objectCount();
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      appState.facade.document().dirtyFlags();
  const std::uint64_t undoBefore = cr::creativeUndoDepth(appState.history);
  const std::uint64_t redoBefore = cr::creativeRedoDepth(appState.history);
  const std::uint64_t sourceRevisionBefore = editor.worldLayout.revision;
  const std::uint64_t generatedRevisionBefore =
      editor.worldLayout.generatedRevision;

  const CreativeEditorMeasurementActionReceipt first =
      appendCreativeEditorMeasurementPoint(request);
  editor.interaction.target = targetAt({4.0, 6.0, 3.0});
  const CreativeEditorMeasurementActionReceipt preview =
      previewCreativeEditorMeasurementPoint(request);
  const CreativeEditorMeasurementActionReceipt second =
      appendCreativeEditorMeasurementPoint(request);
  const cr::CreativeMeasurementState& measurement =
      appState.facade.measurementState();
  const std::string readout =
      formatCreativeEditorMeasurementReadout(measurement);
  editor.interaction.hotbar.selectedSlot = 0U;
  editor.interaction.hotbar.entries[0].kind =
      cr::CreativeHeldItemKind::Measure;
  const std::string heldStatus =
      creativeEditorHeldItemStatusLabel(editor, &measurement);

  return expect(installed.accepted && first.accepted && preview.accepted &&
                    second.accepted,
                "held measurement route accepts click-preview-click") &&
         expect(measurement.completed && !measurement.active &&
                    measurement.plan.accepted &&
                    near(measurement.plan.directDistanceMeters, 5.0),
                "held route stores one completed Facade measurement") &&
         expect(readout == "Distance 5.000 m | Surface" &&
                    heldStatus.find("Distance 5.000 m | Surface") !=
                        std::string::npos &&
                    heldStatus.find("SNAP Surface") != std::string::npos,
                "HUD consumes the shared numeric readout and configured snap") &&
         expect(appState.facade.document().revision() == revisionBefore &&
                    appState.facade.document().objectCount() ==
                        objectCountBefore &&
                    appState.facade.document().dirtyFlags() == dirtyBefore,
                "transient measurement leaves document output untouched") &&
         expect(cr::creativeUndoDepth(appState.history) == undoBefore &&
                    cr::creativeRedoDepth(appState.history) == redoBefore,
                "transient measurement creates no history entry") &&
         expect(editor.worldLayout.revision == sourceRevisionBefore &&
                    editor.worldLayout.generatedRevision ==
                        generatedRevisionBefore,
                "transient measurement leaves generated source untouched");
}

bool gridProjectionIsSharedAndPreservesMeasurementSemantics() {
  cr::CreativeMeasurementState state = cr::makeDefaultCreativeMeasurementState();
  static_cast<void>(cr::configureMeasurement(
      state, cr::CreativeMeasurementMode::Area,
      cr::CreativeMeasurementAxis::Y, true));
  cr::CreativeMeasurementPoint first;
  first.x = 12.0;
  first.y = 5.0;
  first.z = -4.0;
  first.snapKind = cr::CreativeMeasurementSnapKind::Grid;
  cr::CreativeMeasurementPoint second = first;
  second.x = 16.0;
  second.snapKind = cr::CreativeMeasurementSnapKind::Vertex;
  cr::CreativeMeasurementPoint third = second;
  third.y = 9.0;
  third.z = 0.0;
  third.snapKind = cr::CreativeMeasurementSnapKind::Level;
  static_cast<void>(cr::appendMeasurementPoint(state, first));
  static_cast<void>(cr::appendMeasurementPoint(state, second));
  static_cast<void>(cr::previewMeasurementPoint(state, third));

  const cr::CreativeMeasurementGeometry world =
      cr::buildCreativeMeasurementGeometry(state);
  const cr::CreativeGridSettings grid{{10.0, 1.0, -6.0}, 2.0, {32, 16, 32}};
  const cr::CreativeMeasurementGeometry projected =
      projectCreativeEditorMeasurementGeometryToGrid(world, grid);
  cr::CreativeGridSettings invalidGrid = grid;
  invalidGrid.cellSizeMeters = 0.0;
  const cr::CreativeMeasurementGeometry rejected =
      projectCreativeEditorMeasurementGeometryToGrid(world, invalidGrid);

  return expect(projected.visible && projected.closed &&
                    projected.pointCount == 3U &&
                    projected.segmentCount == 3U,
                "grid projection preserves bounded geometry topology") &&
         expect(near(projected.points[0].x, 1.0) &&
                    near(projected.points[0].y, 2.0) &&
                    near(projected.points[0].z, 1.0) &&
                    near(projected.points[2].x, 3.0) &&
                    near(projected.points[2].y, 4.0) &&
                    near(projected.points[2].z, 3.0),
                "grid projection applies document origin and cell size once") &&
         expect(projected.points[0].snapKind ==
                    cr::CreativeMeasurementSnapKind::Grid &&
                    projected.points[2].snapKind ==
                        cr::CreativeMeasurementSnapKind::Level &&
                    !rejected.visible,
                "grid projection preserves snap provenance and fails closed");
}

bool savedAnnotationsRemainVisibleWithoutTransientState() {
  cr::CreativeAppState appState;
  cr::CreativeDocument document =
      cr::CreativeDocument::create("saved measurement overlay");
  static_cast<void>(document.assignId(78U));
  if (!expect(appState.facade.installDocument(std::move(document)).accepted,
              "saved measurement overlay document installed")) {
    return false;
  }
  static_cast<void>(appState.facade.configureMeasurement(
      cr::CreativeMeasurementMode::Distance,
      cr::CreativeMeasurementAxis::X, false));
  static_cast<void>(appState.facade.appendMeasurementPoint(
      {1.0, 2.0, 3.0, {}, cr::CreativeMeasurementSnapKind::Grid}));
  static_cast<void>(appState.facade.appendMeasurementPoint(
      {4.0, 6.0, 3.0, {}, cr::CreativeMeasurementSnapKind::Surface}));
  const cr::CreativeFacadeMeasurementAnnotationSaveReceipt saved =
      appState.facade.saveMeasurementAnnotation("Aisle width");

  CreativeEditorState editor;
  CreativeEditorSelectionFrame selection;
  CreativeEditorGizmoFrame gizmo;
  iggy3d::FrameInput frame;
  frame.viewport.width = 800U;
  frame.viewport.height = 600U;
  cr::CreativeSpatialProjectionRequest projection;
  CreativeEditorOverlayFrame visible;
  CreativeEditorOverlayFrameRequest request{
      appState, editor, selection, gizmo, frame, projection};
  request.drawableWidth = 800U;
  request.drawableHeight = 600U;
  static_cast<void>(buildCreativeEditorWorldWireframes(request, visible));

  CreativeEditorOverlayFrame hidden;
  request.captureMode = true;
  static_cast<void>(buildCreativeEditorWorldWireframes(request, hidden));
  return expect(saved.accepted && saved.changed &&
                    !appState.facade.measurementState().hasMeasurement,
                "saving clears only transient measurement state") &&
         expect(visible.measurementEdgeCount == 7U,
                "saved distance annotation remains visible in 3D") &&
         expect(hidden.measurementEdgeCount == 0U,
                "capture mode hides durable annotation overlays");
}

}  // namespace

int main() {
  const bool ok = gridSurfaceAndLevelModesAreExact() &&
                  vertexUsesLiveTransformedBoundsAndAutoTolerance() &&
                  openingUsesAuthoredCutoutAnchorsAndWinsAuto() &&
                  explicitSemanticModesFailClosed() &&
                  heldInteractionUsesFacadeWithoutMutatingAuthoringState() &&
                  gridProjectionIsSharedAndPreservesMeasurementSemantics() &&
                  savedAnnotationsRemainVisibleWithoutTransientState();
  if (!ok) {
    return 1;
  }
  std::cout << "creative_editor_measurement_tests: OK\n";
  return 0;
}
