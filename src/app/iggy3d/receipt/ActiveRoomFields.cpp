#include "app/iggy3d/receipt/ReceiptFields.hpp"

#include <array>
#include <string_view>

#include "app/iggy3d/gameplay/ProductRoomStore.hpp"

namespace iggy3d {

namespace {

struct ActiveRoomReceiptContext {
  const ProductAppWindowState& window;
  const ProductActiveRoomState& room;
  const ProductActiveRoomCollisionState& collision;
  const ProductActiveRoomCollisionFreshnessResult& freshness;
};

struct ActiveRoomReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const ActiveRoomReceiptContext& context,
                 std::string_view key);
};

const std::array<ActiveRoomReceiptFieldRow, 34> kActiveRoomReceiptFields{{
    {"active_room_loaded",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.loaded);
     }},
    {"active_room_status",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.status);
     }},
    {"active_room_reason_code",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.reasonCode);
     }},
    {"active_room_source",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.source);
     }},
    {"active_room_id",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.roomId);
     }},
    {"active_room_source_name",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.sourceName);
     }},
    {"active_room_source_subset",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.sourceSubset);
     }},
    {"active_room_has_authored_room",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.hasAuthoredRoom);
     }},
    {"active_room_authored_floor_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.authoredFloorCount);
     }},
    {"active_room_authored_wall_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.authoredWallCount);
     }},
    {"active_room_authored_object_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.authoredObjectCount);
     }},
    {"active_room_authored_marker_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.authoredMarkerCount);
     }},
    {"active_room_static_mesh_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.staticMeshCount);
     }},
    {"active_room_anchor_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.anchorCount);
     }},
    {"active_room_opening_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.openingCount);
     }},
    {"active_room_spatial_surface_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.spatialSurfaceCount);
     }},
    {"active_room_walkable_surface_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.walkableSurfaceCount);
     }},
    {"active_room_actor_blocker_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.actorBlockerSurfaceCount);
     }},
    {"active_room_projectile_blocker_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.room.projectileBlockerSurfaceCount);
     }},
    {"active_room_collision_ready",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.ready);
     }},
    {"active_room_collision_status",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.status);
     }},
    {"active_room_collision_reason_code",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.reasonCode);
     }},
    {"active_room_collision_room_id",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.roomId);
     }},
    {"active_room_collision_spatial_surface_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.spatialSurfaceCount);
     }},
    {"active_room_collision_query_surface_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.querySurfaceCount);
     }},
    {"active_room_collision_walkable_surface_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.walkableSurfaceCount);
     }},
    {"active_room_collision_actor_blocker_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.actorBlockerSurfaceCount);
     }},
    {"active_room_collision_projectile_blocker_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.projectileBlockerSurfaceCount);
     }},
    {"active_room_collision_runtime_owned_surface_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.runtimeOwnedSurfaceCount);
     }},
    {"active_room_collision_runtime_filtered_surface_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.runtimeFilteredSurfaceCount);
     }},
    {"active_room_collision_door_blocker_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.doorBlockerSurfaceCount);
     }},
    {"active_room_collision_active_door_blocker_count",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.collision.activeDoorBlockerSurfaceCount);
     }},
    {"active_room_collision_freshness_rebaked",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.freshness.rebaked);
     }},
    {"active_room_collision_freshness_reason_code",
     [](RenderReceipt& receipt,
        const ActiveRoomReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.freshness.reasonCode);
     }},
}};

}  // namespace

void appendProductActiveRoomFields(RenderReceipt& receipt, const ProductAppWindowState& window) {
  const ProductActiveRoomState& room = activeRoom(window);
  const ProductActiveRoomCollisionState& collision = activeRoomCollision(window);
  const ProductActiveRoomCollisionFreshnessResult& freshness =
      activeRoomCollisionFreshness(window);

  const ActiveRoomReceiptContext context{
      window,
      room,
      collision,
      freshness,
  };

  for (const ActiveRoomReceiptFieldRow& row : kActiveRoomReceiptFields) {
    row.append(receipt, context, row.key);
  }
}

}  // namespace iggy3d
