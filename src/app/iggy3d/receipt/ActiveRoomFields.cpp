#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include "app/iggy3d/gameplay/ProductRoomStore.hpp"

namespace iggy3d {

void appendProductActiveRoomFields(RenderReceipt& receipt, const ProductAppWindowState& window) {
  const ProductActiveRoomState& room = activeRoom(window);
  const ProductActiveRoomCollisionState& collision = activeRoomCollision(window);
  const ProductActiveRoomCollisionFreshnessResult& freshness =
      activeRoomCollisionFreshness(window);

  appendReceiptField(receipt, "active_room_loaded", room.loaded);
  appendReceiptField(receipt, "active_room_status", room.status);
  appendReceiptField(receipt, "active_room_reason_code",
                     room.reasonCode);
  appendReceiptField(receipt, "active_room_source", room.source);
  appendReceiptField(receipt, "active_room_id", room.roomId);
  appendReceiptField(receipt, "active_room_source_name",
                     room.sourceName);
  appendReceiptField(receipt, "active_room_source_subset",
                     room.sourceSubset);
  appendReceiptField(receipt, "active_room_has_authored_room",
                     room.hasAuthoredRoom);
  appendReceiptField(receipt, "active_room_authored_floor_count",
                     room.authoredFloorCount);
  appendReceiptField(receipt, "active_room_authored_wall_count",
                     room.authoredWallCount);
  appendReceiptField(receipt, "active_room_authored_object_count",
                     room.authoredObjectCount);
  appendReceiptField(receipt, "active_room_authored_marker_count",
                     room.authoredMarkerCount);
  appendReceiptField(receipt, "active_room_static_mesh_count",
                     room.staticMeshCount);
  appendReceiptField(receipt, "active_room_anchor_count",
                     room.anchorCount);
  appendReceiptField(receipt, "active_room_opening_count",
                     room.openingCount);
  appendReceiptField(receipt, "active_room_spatial_surface_count",
                     room.spatialSurfaceCount);
  appendReceiptField(receipt, "active_room_walkable_surface_count",
                     room.walkableSurfaceCount);
  appendReceiptField(receipt, "active_room_actor_blocker_count",
                     room.actorBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_projectile_blocker_count",
                     room.projectileBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_ready",
                     collision.ready);
  appendReceiptField(receipt, "active_room_collision_status",
                     collision.status);
  appendReceiptField(receipt, "active_room_collision_reason_code",
                     collision.reasonCode);
  appendReceiptField(receipt, "active_room_collision_room_id",
                     collision.roomId);
  appendReceiptField(receipt, "active_room_collision_spatial_surface_count",
                     collision.spatialSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_query_surface_count",
                     collision.querySurfaceCount);
  appendReceiptField(receipt, "active_room_collision_walkable_surface_count",
                     collision.walkableSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_actor_blocker_count",
                     collision.actorBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_projectile_blocker_count",
                     collision.projectileBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_runtime_owned_surface_count",
                     collision.runtimeOwnedSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_runtime_filtered_surface_count",
                     collision.runtimeFilteredSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_door_blocker_count",
                     collision.doorBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_active_door_blocker_count",
                     collision.activeDoorBlockerSurfaceCount);
  appendReceiptField(receipt, "active_room_collision_freshness_rebaked",
                     freshness.rebaked);
  appendReceiptField(receipt, "active_room_collision_freshness_reason_code",
                     freshness.reasonCode);
}

}  // namespace iggy3d
