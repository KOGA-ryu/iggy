#include "ProductFilesystemTestSupport.hpp"

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/creative/BakedActiveRoomRefresh.hpp"
#include "app/iggy3d/creative/CreativeWorldOperations.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::ProductAppOptions testOptions(std::string_view name) {
  return iggy3d::test::productTestOptions(
      "iggy3d_creative_no_window_bake",
      name,
      iggy3d::ProductWindowMode::NoWindow);
}

iggy3d::ProductCreativeNewWorldLaunchRequest launchRequest(
    std::string_view title) {
  iggy3d::ProductCreativeNewWorldLaunchRequest request;
  request.title = std::string{title};
  request.requestedAtUtc = "2026-07-07T12:00:00Z";
  return request;
}

cr::CreativeDocumentCreateReceipt createBoundsObject(
    cr::Facade& facade,
    cr::CreativeObjectKind kind,
    cr::CreativeBounds bounds) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  request.transform.position = {
      (bounds.min.x + bounds.max.x) * 0.5,
      (bounds.min.y + bounds.max.y) * 0.5,
      (bounds.min.z + bounds.max.z) * 0.5,
  };
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request);
}

cr::CreativeDocumentCreateReceipt createFloor(cr::Facade& facade) {
  return createBoundsObject(facade,
                            cr::CreativeObjectKind::Floor,
                            {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
}

cr::CreativeDocumentCreateReceipt createCrate(cr::Facade& facade) {
  return createBoundsObject(facade,
                            cr::CreativeObjectKind::Crate,
                            {{1.0, 0.0, 5.0}, {2.0, 1.0, 6.0}});
}

iggy3d::ProductCreativeBakedActiveRoomRefreshRequest refreshRequest() {
  iggy3d::ProductCreativeBakedActiveRoomRefreshRequest request;
  request.roomId = "creative_no_window_bake_room";
  request.sourceName = "tests/product_creative_no_window_bake_scenario";
  request.sourceSubset = "unit";
  request.activationHook = [](iggy3d::Session&,
                              const iggy3d::RoomAsset&,
                              const cr::CreativeDocument&) {};
  return request;
}

iggy3d::ProductCreativeBakedActiveRoomRefreshResult refreshBakedRoom(
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    const cr::CreativeAppState& app) {
  return iggy3d::refreshProductCreativeBakedActiveRoom(refreshRequest(),
                                                       activeSession,
                                                       window,
                                                       app);
}

bool sameVec3(iggy3d::Vec3 lhs, iggy3d::Vec3 rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

const iggy3d::RoomStaticMeshAsset* findMeshByRole(
    const iggy3d::RoomAsset& room,
    std::string_view role) {
  for (const iggy3d::RoomStaticMeshAsset& mesh : room.staticMeshes) {
    if (mesh.role == role) {
      return &mesh;
    }
  }
  return nullptr;
}

bool expectRefreshForFloorAndCrate(
    const iggy3d::ProductCreativeBakedActiveRoomRefreshResult& refreshed,
    const iggy3d::ProductAppWindowState& window,
    const cr::CreativeDocument& document,
    iggy3d::Vec3 expectedCrateCenter,
    std::string_view label) {
  const std::string prefix{label};
  const iggy3d::RoomStaticMeshAsset* floor =
      findMeshByRole(iggy3d::activeRoom(window).room, "floor");
  const iggy3d::RoomStaticMeshAsset* crate =
      findMeshByRole(iggy3d::activeRoom(window).room, "prop");

  return expect(refreshed.accepted, prefix + " refresh accepted") &&
         expect(refreshed.status == "product_creative_baked_room_refreshed",
                prefix + " refresh status") &&
         expect(refreshed.reasonCode ==
                    "product_creative_baked_room_refreshed",
                prefix + " refresh reason") &&
         expect(refreshed.documentId == document.id(),
                prefix + " document id") &&
         expect(refreshed.objectCount == document.objectCount(),
                prefix + " object count") &&
         expect(refreshed.bakeMeasured, prefix + " bake measured") &&
         expect(refreshed.bakedDocumentRevision == document.revision(),
                prefix + " baked revision") &&
         expect(refreshed.bakeReceipt.accepted,
                prefix + " bake receipt accepted") &&
         expect(refreshed.bakeReceipt.reasonCode == "creative_room_baked",
                prefix + " bake reason") &&
         expect(refreshed.bakeReceipt.objectCount == document.objectCount(),
                prefix + " bake object count") &&
         expect(refreshed.bakeReceipt.consideredObjectCount ==
                    document.objectCount(),
                prefix + " bake considered count") &&
         expect(refreshed.staticMeshCount == 2U,
                prefix + " refresh mesh count") &&
         expect(refreshed.spatialSurfaceCount == 3U,
                prefix + " refresh surface count") &&
         expect(refreshed.staticMeshSourceCount == 2U,
                prefix + " mesh source count") &&
         expect(refreshed.spatialSurfaceSourceCount == 3U,
                prefix + " surface source count") &&
         expect(refreshed.activeRoomLoaded,
                prefix + " active room loaded result") &&
         expect(refreshed.collisionReady,
                prefix + " collision ready result") &&
         expect(refreshed.collisionQuerySurfaceCount == 3U,
                prefix + " collision query count result") &&
         expect(iggy3d::activeRoom(window).loaded, prefix + " active room loaded") &&
         expect(iggy3d::activeRoom(window).staticMeshCount == 2U,
                prefix + " active mesh count") &&
         expect(iggy3d::activeRoom(window).spatialSurfaceCount == 3U,
                prefix + " active surface count") &&
         expect(iggy3d::activeRoomCollision(window).ready,
                prefix + " active collision ready") &&
         expect(iggy3d::activeRoomCollision(window).querySurfaceCount == 3U,
                prefix + " active collision query count") &&
         expect(floor != nullptr, prefix + " floor mesh present") &&
         expect(crate != nullptr, prefix + " crate mesh present") &&
         expect(crate != nullptr &&
                    sameVec3(crate->positionMeters, expectedCrateCenter),
                prefix + " crate center");
}

bool expectRefreshForFloorOnly(
    const iggy3d::ProductCreativeBakedActiveRoomRefreshResult& refreshed,
    const iggy3d::ProductAppWindowState& window,
    const cr::CreativeDocument& document) {
  const iggy3d::RoomStaticMeshAsset* floor =
      findMeshByRole(iggy3d::activeRoom(window).room, "floor");
  const iggy3d::RoomStaticMeshAsset* prop =
      findMeshByRole(iggy3d::activeRoom(window).room, "prop");

  return expect(refreshed.accepted, "floor-only refresh accepted") &&
         expect(refreshed.status == "product_creative_baked_room_refreshed",
                "floor-only refresh status") &&
         expect(refreshed.documentId == document.id(),
                "floor-only document id") &&
         expect(refreshed.objectCount == document.objectCount(),
                "floor-only object count") &&
         expect(refreshed.bakeMeasured, "floor-only bake measured") &&
         expect(refreshed.bakedDocumentRevision == document.revision(),
                "floor-only baked revision") &&
         expect(refreshed.staticMeshCount == 1U,
                "floor-only mesh count") &&
         expect(refreshed.spatialSurfaceCount == 1U,
                "floor-only surface count") &&
         expect(refreshed.staticMeshSourceCount == 1U,
                "floor-only mesh source count") &&
         expect(refreshed.spatialSurfaceSourceCount == 1U,
                "floor-only surface source count") &&
         expect(iggy3d::activeRoom(window).loaded, "floor-only active room loaded") &&
         expect(iggy3d::activeRoom(window).staticMeshCount == 1U,
                "floor-only active mesh count") &&
         expect(iggy3d::activeRoom(window).spatialSurfaceCount == 1U,
                "floor-only active surface count") &&
         expect(iggy3d::activeRoomCollision(window).ready,
                "floor-only collision ready") &&
         expect(iggy3d::activeRoomCollision(window).querySurfaceCount == 1U,
                "floor-only collision query count") &&
         expect(floor != nullptr, "floor-only floor present") &&
         expect(prop == nullptr, "floor-only prop removed");
}

bool creativeDocumentBakesAfterCreateMoveDeleteUndoNoWindow() {
  const iggy3d::ProductAppOptions options =
      testOptions("create_move_delete_undo");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      iggy3d::launchProductCreativeNewWorld(options,
                                            launchRequest("No Window Bake"),
                                            frontend,
                                            activeSession,
                                            window,
                                            app);
  bool ok = true;
  ok &= expect(launched.accepted, "launch accepted");
  ok &= expect(activeSession.has_value(), "active session exists");
  ok &= expect(window.inputDevice.interactionMode == iggy3d::ProductInteractionMode::Creative,
               "creative interaction mode");
  const std::uint64_t revisionAfterLaunch = facade.document().revision();
  ok &= expect(revisionAfterLaunch == 0U, "blank launch revision");
  ok &= expect(facade.document().id() == launched.documentId,
               "facade document id");

  const cr::CreativeDocumentCreateReceipt floor = createFloor(facade);
  const cr::CreativeDocumentCreateReceipt crate = createCrate(facade);
  const std::uint64_t revisionAfterCreate = facade.document().revision();
  ok &= expect(floor.accepted, "floor created");
  ok &= expect(crate.accepted, "crate created");
  ok &= expect(facade.document().objectCount() == 2U,
               "object count after create");
  ok &= expect(revisionAfterCreate == revisionAfterLaunch + 2U,
               "revision after create");
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult initialRefresh =
      refreshBakedRoom(activeSession, window, app);
  ok &= expectRefreshForFloorAndCrate(initialRefresh,
                                      window,
                                      facade.document(),
                                      {1.5F, 0.5F, 5.5F},
                                      "initial");

  cr::pushCreativeUndoSnapshot(app.undoStack, facade.document());
  const std::uint64_t undoDepthBeforeMove = cr::creativeUndoDepth(app.undoStack);
  ok &= expect(undoDepthBeforeMove == 1U, "undo depth before move");
  const cr::CreativeDocumentMutationReceipt moved = cr::moveDocumentObject(
      facade.documentForPersistence(),
      crate.objectId,
      cr::CreativeVec3{4.5, 0.5, 7.5});
  const std::uint64_t revisionAfterMove = facade.document().revision();
  ok &= expect(moved.status == cr::CreativeDocumentMutationStatus::Applied,
               "move applied");
  ok &= expect(moved.changed, "move changed");
  ok &= expect(moved.revisionBefore == revisionAfterCreate,
               "move revision before");
  ok &= expect(revisionAfterMove == revisionAfterCreate + 1U,
               "move revision after");
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult movedRefresh =
      refreshBakedRoom(activeSession, window, app);
  ok &= expectRefreshForFloorAndCrate(movedRefresh,
                                      window,
                                      facade.document(),
                                      {4.5F, 0.5F, 7.5F},
                                      "moved");

  cr::pushCreativeUndoSnapshot(app.undoStack, facade.document());
  const std::uint64_t undoDepthBeforeDelete =
      cr::creativeUndoDepth(app.undoStack);
  ok &= expect(undoDepthBeforeDelete == 2U, "undo depth before delete");
  const cr::CreativeDocumentRemoveReceipt removed =
      facade.removeDocumentObject(crate.objectId);
  const std::uint64_t revisionAfterDelete = facade.document().revision();
  ok &= expect(removed.accepted, "delete accepted");
  ok &= expect(removed.objectRemoved, "delete removed object");
  ok &= expect(removed.objectId == crate.objectId, "delete object id");
  ok &= expect(revisionAfterDelete == revisionAfterMove + 1U,
               "delete revision after");
  ok &= expect(facade.findObject(crate.objectId) == nullptr,
               "crate absent after delete");
  ok &= expect(facade.document().objectCount() == 1U,
               "object count after delete");
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult deletedRefresh =
      refreshBakedRoom(activeSession, window, app);
  ok &= expectRefreshForFloorOnly(deletedRefresh, window, facade.document());

  const cr::CreativeDocumentUndoApplyReceipt undo =
      cr::applyLastCreativeUndoSnapshot(app);
  const cr::CreativeObject* restoredCrate = facade.findObject(crate.objectId);
  ok &= expect(undo.accepted, "undo accepted");
  ok &= expect(undo.changed, "undo changed");
  ok &= expect(undo.depthBefore == 2U, "undo depth before");
  ok &= expect(undo.depthAfter == 1U, "undo depth after");
  ok &= expect(undo.revisionBefore == revisionAfterDelete,
               "undo revision before");
  ok &= expect(undo.revisionAfter == revisionAfterMove,
               "undo revision restored snapshot");
  ok &= expect(restoredCrate != nullptr, "crate restored by undo");
  ok &= expect(restoredCrate != nullptr &&
                   restoredCrate->kind == cr::CreativeObjectKind::Crate,
               "restored crate kind");
  ok &= expect(facade.document().objectCount() == 2U,
               "object count restored by undo");
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult undoRefresh =
      refreshBakedRoom(activeSession, window, app);
  ok &= expectRefreshForFloorAndCrate(undoRefresh,
                                      window,
                                      facade.document(),
                                      {4.5F, 0.5F, 7.5F},
                                      "undo");
  ok &= expect(cr::creativeUndoDepth(app.undoStack) == 1U,
               "move snapshot remains after delete undo");
  return ok;
}

}  // namespace

int main() {
  const bool ok = creativeDocumentBakesAfterCreateMoveDeleteUndoNoWindow();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
