// THE MAP DEMO template ("map_demo"): a seeded, deterministic 80x80m stealth
// map authored through the real document chain, composing the
// stealth_blockout kit (cover + clamber traversal) with patrol guards.
//
// Layout (X east, Z south, Y up; all placements axis-aligned):
//   - COURTYARD  west  (x -38..-8): open plaza, scattered kit cover, player
//     spawn at the west edge, two looping patrol guards + a gate watch post.
//   - WAREHOUSE  northeast (x -2..38, z -38..-2): three aisles between long
//     walls, one patrol guard per aisle, kit crates/low walls inside.
//     Clamber bypass #1: an elevated platform walkway at 1.5m hugging the
//     north edge -- reachable ONLY by clamber (rise > autoStep 0.35m).
//   - VERTICAL YARD southeast (x -2..38, z 2..38): giant-scale traversal
//     (giant stair, giant shelf), the clamber test ladder, one ground patrol
//     guard and a watch post guarding the objective.
//     Clamber bypass #2: giant stair (0.5m risers -- every step is a
//     clamber) onto an elevated 3.0m walkway running to the objective drop.
//   - Objective: a LootPoint pickup deep in the vertical yard, reachable by
//     a ground sneaking path (courtyard gate -> warehouse aisles -> yard
//     gate) and by either clamber bypass.
//
// Moving guards are children of explicit PatrolRoute objects. Route points
// live on their owning route; runtime meaning is independent of document and
// room-bake order. Phase desync comes from rotated point orders and different
// route lengths.
// Determinism: a fixed-seed LCG supplies cover jitter; no wall clock, no
// std::random.

#include "app/iggy3d/creative/world/MapTemplate.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

namespace iggy3d::creative {
namespace map_demo_internal {
namespace {

constexpr std::uint32_t kMapDemoSeed = 20260717U;

struct DemoRng {
  std::uint32_t state = kMapDemoSeed;
  double next() {  // [0,1)
    state = state * 1664525U + 1013904223U;
    return static_cast<double>(state >> 8) /
           static_cast<double>(1U << 24);
  }
  double jitter(double amplitudeMeters) {
    return (next() - 0.5) * 2.0 * amplitudeMeters;
  }
};

struct KitPiece {
  std::string_view assetId;
  CreativeObjectKind kind;
  double halfX;
  double minZ;  // asset-local Z extent (origin at base center)
  double maxZ;
  double height;
};

// stealth_blockout catalog pieces (dims from the kit metrics sheet).
constexpr KitPiece kLowWall{"stealth_blockout/cover_low_wall_2x1p1",
                            CreativeObjectKind::Wall, 1.0, -0.2, 0.2, 1.1};
constexpr KitPiece kHighWall{"stealth_blockout/cover_high_wall_2x2",
                             CreativeObjectKind::Wall, 1.0, -0.2, 0.2, 2.0};
constexpr KitPiece kCrate{"stealth_blockout/cover_crate_1m",
                          CreativeObjectKind::Crate, 0.5, -0.5, 0.5, 1.0};
constexpr KitPiece kCrateHalf{"stealth_blockout/cover_crate_0p5",
                              CreativeObjectKind::Crate, 0.25, -0.25, 0.25,
                              0.5};
constexpr KitPiece kBarrel{"stealth_blockout/cover_barrel_0p6x1p1",
                           CreativeObjectKind::Barrel, 0.3, -0.3, 0.3, 1.1};
constexpr KitPiece kSandbag{"stealth_blockout/cover_sandbag_run_2x0p9",
                            CreativeObjectKind::Wall, 1.0, -0.3, 0.3, 0.9};
constexpr KitPiece kClamber0p6{"stealth_blockout/clamber_block_0p6",
                               CreativeObjectKind::Platform, 0.5, -0.5, 0.5,
                               0.6};
constexpr KitPiece kClamber1p0{"stealth_blockout/clamber_block_1p0",
                               CreativeObjectKind::Platform, 0.5, -0.5, 0.5,
                               1.0};
constexpr KitPiece kClamber1p4{"stealth_blockout/clamber_block_1p4",
                               CreativeObjectKind::Platform, 0.5, -0.5, 0.5,
                               1.4};
constexpr KitPiece kClamber1p8{"stealth_blockout/clamber_block_1p8",
                               CreativeObjectKind::Platform, 0.5, -0.5, 0.5,
                               1.8};
constexpr KitPiece kClamberFail{"stealth_blockout/clamber_fail_2p0",
                                CreativeObjectKind::Platform, 0.5, -0.5, 0.5,
                                2.0};
constexpr KitPiece kPlatform{"stealth_blockout/platform_2x2x0p5",
                             CreativeObjectKind::Platform, 1.0, -1.0, 1.0,
                             0.5};
constexpr KitPiece kGiantStair{"stealth_blockout/giant_stair_4x6x3",
                               CreativeObjectKind::Stair, 2.0, -3.0, 3.0,
                               3.0};
constexpr KitPiece kGiantShelf{"stealth_blockout/giant_shelf_2x3p6",
                               CreativeObjectKind::Platform, 1.0, -1.2, 0.4,
                               3.6};
constexpr KitPiece kGuardPost{"stealth_blockout/guard_post_1p8",
                              CreativeObjectKind::Prop, 0.35, -0.35, 0.35,
                              1.8};

// The two clamber-only bypass walkways. The proof test reads these SAME
// constants back out of the authored document by object name.
constexpr double kBypass1WalkwayTopMeters = 1.5;
constexpr double kBypass2WalkwayTopMeters = 3.0;

CreativeDocumentCreateRequest boxRequest(CreativeObjectKind kind,
                                         std::string name,
                                         double minX, double minY,
                                         double minZ, double maxX,
                                         double maxY, double maxZ) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = {(minX + maxX) * 0.5, minY,
                                (minZ + maxZ) * 0.5};
  request.hasTransformOverride = true;
  request.bounds = {{minX, minY, minZ}, {maxX, maxY, maxZ}};
  request.hasBoundsOverride = true;
  return request;
}

// A stealth_blockout catalog instance with its base at (x, baseY, z).
CreativeDocumentCreateRequest kitRequest(const KitPiece& piece,
                                         std::string name, double x,
                                         double z, double baseY = 0.0) {
  CreativeDocumentCreateRequest request;
  request.kind = piece.kind;
  request.name = std::move(name);
  request.assetId = std::string(piece.assetId);
  request.transform.position = {x, baseY, z};
  request.hasTransformOverride = true;
  request.bounds = {{x - piece.halfX, baseY, z + piece.minZ},
                    {x + piece.halfX, baseY + piece.height, z + piece.maxZ}};
  request.hasBoundsOverride = true;
  return request;
}

CreativeDocumentCreateRequest markerRequest(CreativeObjectKind kind,
                                            std::string name, double x,
                                            double z, double y = 0.0) {
  CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.name = std::move(name);
  request.transform.position = {x, y, z};
  request.hasTransformOverride = true;
  return request;
}

// A route and its explicitly parented guard. `phase` rotates point order so no
// two guards start toward the same leg beat.
void appendPatrolGuard(std::vector<CreativeDocumentCreateRequest>& requests,
                       CreativeObjectId firstObjectId,
                       const std::string& label,
                       const std::vector<std::array<double, 2>>& nodes,
                       std::size_t phase) {
  const std::size_t count = nodes.size();
  const std::array<double, 2>& start = nodes[phase % count];
  CreativeDocumentCreateRequest route;
  route.kind = CreativeObjectKind::PatrolRoute;
  route.name = label + " Route";
  route.hasPathOverride = true;
  for (std::size_t i = 0; i < count; ++i) {
    const std::array<double, 2>& node = nodes[(phase + i) % count];
    route.pathPoints.push_back({{node[0], 0.0, node[1]}, 0.0, 1.0});
  }
  const CreativeObjectId routeObjectId =
      firstObjectId + static_cast<CreativeObjectId>(requests.size());
  requests.push_back(std::move(route));

  const std::array<double, 2>& next = nodes[(phase + 1U) % count];
  CreativeDocumentCreateRequest guard =
      markerRequest(CreativeObjectKind::NpcSpawn, label + " Guard",
                    start[0], start[1]);
  guard.parentId = routeObjectId;
  guard.transform.rotationEulerRadians.y =
      std::atan2(next[0] - start[0], -(next[1] - start[1]));
  requests.push_back(std::move(guard));
}

void appendCoverField(std::vector<CreativeDocumentCreateRequest>& requests,
                      DemoRng& rng, const std::string& label,
                      const KitPiece& piece, std::size_t count,
                      double minX, double maxX, double minZ, double maxZ,
                      double jitterMeters) {
  // Rough grid walk with seeded jitter: enough scatter to break sightlines,
  // deterministic across runs.
  const std::size_t columns = count >= 4U ? 3U : count;
  const double spanX = maxX - minX;
  const double spanZ = maxZ - minZ;
  const std::size_t rows = (count + columns - 1U) / columns;
  std::size_t placed = 0U;
  for (std::size_t row = 0; row < rows && placed < count; ++row) {
    for (std::size_t column = 0; column < columns && placed < count;
         ++column) {
      const double x = minX + spanX * (static_cast<double>(column) + 0.5) /
                                  static_cast<double>(columns) +
                       rng.jitter(jitterMeters);
      const double z = minZ + spanZ * (static_cast<double>(row) + 0.5) /
                                  static_cast<double>(rows) +
                       rng.jitter(jitterMeters);
      ++placed;
      requests.push_back(kitRequest(
          piece, label + " " + std::to_string(placed), x, z));
    }
  }
}

std::vector<CreativeDocumentCreateRequest> mapDemoObjects(
    CreativeObjectId firstObjectId) {
  std::vector<CreativeDocumentCreateRequest> requests;
  requests.reserve(160U);
  DemoRng rng;

  // ---- Ground + perimeter ------------------------------------------------
  requests.push_back(boxRequest(CreativeObjectKind::Floor, "Demo Ground",
                                -40.0, -0.5, -40.0, 40.0, 0.0, 40.0));
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Perimeter North",
                                -40.0, 0.0, -40.0, 40.0, 3.0, -39.5));
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Perimeter South",
                                -40.0, 0.0, 39.5, 40.0, 3.0, 40.0));
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Perimeter West",
                                -40.0, 0.0, -39.5, -39.5, 3.0, 39.5));
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Perimeter East",
                                39.5, 0.0, -39.5, 40.0, 3.0, 39.5));

  // ---- Zone dividers (with gates) ---------------------------------------
  // Courtyard | east zones divider at x = -6..-5, gates at z -22..-18 and
  // z 14..18.
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Divider West A",
                                -6.0, 0.0, -39.5, -5.0, 3.0, -22.0));
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Divider West B",
                                -6.0, 0.0, -18.0, -5.0, 3.0, 14.0));
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Divider West C",
                                -6.0, 0.0, 18.0, -5.0, 3.0, 39.5));
  // Warehouse | vertical yard divider at z = -1..0, gate at x 28..32.
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Divider South A",
                                -5.0, 0.0, -1.0, 28.0, 3.0, 0.0));
  requests.push_back(boxRequest(CreativeObjectKind::Wall, "Divider South B",
                                32.0, 0.0, -1.0, 39.5, 3.0, 0.0));

  // ---- COURTYARD (west) --------------------------------------------------
  requests.push_back(markerRequest(CreativeObjectKind::SpawnPoint,
                                   "Player Spawn", -36.0, 0.0));
  // Cover sits INSIDE or safely outside the patrol rectangles -- never on a
  // route leg (legs at z=-20/-6 and z=8/24, x lanes -28/-14, margin >= 1m
  // after jitter).
  appendCoverField(requests, rng, "Courtyard Crate", kCrate, 6U, -26.0,
                   -16.0, -17.5, -9.0, 1.2);
  appendCoverField(requests, rng, "Courtyard Barrel", kBarrel, 4U, -30.0,
                   -14.0, -2.0, 4.0, 1.0);
  appendCoverField(requests, rng, "Courtyard Low Wall", kLowWall, 4U, -26.0,
                   -16.0, 11.0, 21.0, 0.8);
  appendCoverField(requests, rng, "Courtyard Sandbag", kSandbag, 3U, -30.0,
                   -14.0, 30.0, 36.0, 0.8);
  appendCoverField(requests, rng, "Courtyard Half Crate", kCrateHalf, 2U,
                   -12.0, -8.0, -10.0, 2.0, 0.5);
  // Gate cover: break LOS through the two west gates.
  requests.push_back(kitRequest(kHighWall, "Gate Cover North", -9.0, -17.0));
  requests.push_back(kitRequest(kHighWall, "Gate Cover South", -9.0, 13.0));

  appendPatrolGuard(requests, firstObjectId, "Courtyard North",
                    {{-28.0, -20.0}, {-14.0, -20.0}, {-14.0, -6.0},
                     {-28.0, -6.0}},
                    0U);
  appendPatrolGuard(requests, firstObjectId, "Courtyard South",
                    {{-28.0, 8.0}, {-14.0, 8.0}, {-14.0, 24.0},
                     {-28.0, 24.0}},
                    2U);
  // Parentless stationary watch.
  requests.push_back(markerRequest(CreativeObjectKind::NpcSpawn,
                                   "Gate Watch", -10.0, -14.0));
  requests.push_back(
      kitRequest(kGuardPost, "Gate Watch Marker", -10.8, -14.0));

  // ---- WAREHOUSE (northeast) ---------------------------------------------
  // Aisle walls (native, 2.5m high) at z = -30 / -24 / -18 / -12.
  for (int wall = 0; wall < 4; ++wall) {
    const double z = -30.0 + 6.0 * wall;
    requests.push_back(boxRequest(
        CreativeObjectKind::Wall,
        "Aisle Wall " + std::to_string(wall + 1), 2.0, 0.0, z, 34.0, 2.5,
        z + 0.4));
  }
  // Aisle cover hugs the walls (single row) so the straight patrol legs at
  // z = -27 / -21 / -15 stay clear.
  appendCoverField(requests, rng, "Aisle1 Crate", kCrate, 4U, 6.0, 30.0,
                   -29.2, -28.8, 0.3);
  appendCoverField(requests, rng, "Aisle2 Low Wall", kLowWall, 4U, 6.0, 30.0,
                   -23.2, -22.8, 0.3);
  appendCoverField(requests, rng, "Aisle3 Crate", kCrate, 4U, 6.0, 30.0,
                   -13.2, -12.8, 0.3);
  appendCoverField(requests, rng, "Warehouse Apron Barrel", kBarrel, 4U, 4.0,
                   26.0, -10.0, -4.0, 1.0);

  appendPatrolGuard(requests, firstObjectId, "Aisle One",
                    {{6.0, -27.0}, {30.0, -27.0}}, 0U);
  appendPatrolGuard(requests, firstObjectId, "Aisle Two",
                    {{6.0, -21.0}, {30.0, -21.0}}, 1U);
  appendPatrolGuard(requests, firstObjectId, "Aisle Three",
                    {{6.0, -15.0}, {30.0, -15.0}}, 0U);

  // Clamber bypass #1: elevated walkway (top 1.5m) along the north edge,
  // above the aisle-one guard's beat. Only entry: clamber the 1.5m rise
  // (autoStep 0.35 refuses; the band (0.35, 1.80] accepts).
  for (int segment = 0; segment < 5; ++segment) {
    const double x = 6.0 + 4.0 * segment;
    requests.push_back(kitRequest(
        kPlatform, "Bypass1 Walkway " + std::to_string(segment + 1), x,
        -36.0, kBypass1WalkwayTopMeters - 0.5));
  }
  // On/off cushions so the drop reads as intended (drops are free).
  requests.push_back(
      kitRequest(kCrateHalf, "Bypass1 Landing", 24.0, -34.0));

  // ---- VERTICAL YARD (southeast) ------------------------------------------
  // Clamber test ladder at the yard entrance (tutorial row).
  requests.push_back(kitRequest(kClamber0p6, "Ladder 0p6", 2.0, 4.0));
  requests.push_back(kitRequest(kClamber1p0, "Ladder 1p0", 4.0, 4.0));
  requests.push_back(kitRequest(kClamber1p4, "Ladder 1p4", 6.0, 4.0));
  requests.push_back(kitRequest(kClamber1p8, "Ladder 1p8", 8.0, 4.0));
  requests.push_back(kitRequest(kClamberFail, "Ladder Fail 2p0", 10.0, 4.0));

  // Yard guard rectangle: (4,8)->(26,8)->(26,16)->(4,16); cover only in the
  // mid-band and east of the rectangle.
  appendCoverField(requests, rng, "Yard Crate", kCrate, 4U, 6.0, 24.0, 10.5,
                   13.5, 1.0);
  appendCoverField(requests, rng, "Yard Sandbag", kSandbag, 3U, 6.0, 26.0,
                   29.0, 34.0, 0.8);
  appendCoverField(requests, rng, "Yard Barrel", kBarrel, 3U, 30.0, 36.0,
                   6.0, 14.0, 0.8);

  // Clamber bypass #2: giant stair (0.5m risers -- every step is a clamber)
  // up to 3.0m, across the giant shelf and an elevated walkway to the
  // objective drop.
  // Chain: ground -> giant stair (low face z=17, top 3.0 at z=23) -> clamber
  // 0.6 onto the giant shelf top (3.6) -> drop 0.6 onto the walkway (3.0)
  // heading east -> drop at the end near the objective.
  requests.push_back(
      kitRequest(kGiantStair, "Giant Stair", 14.0, 20.0));
  requests.push_back(kitRequest(kGiantShelf, "Giant Shelf", 14.0, 24.6));
  for (int segment = 0; segment < 3; ++segment) {
    const double x = 16.0 + 4.0 * segment;
    requests.push_back(kitRequest(
        kPlatform, "Bypass2 Walkway " + std::to_string(segment + 1), x,
        26.0, kBypass2WalkwayTopMeters - 0.5));
  }

  appendPatrolGuard(requests, firstObjectId, "Yard",
                    {{4.0, 8.0}, {26.0, 8.0}, {26.0, 16.0}, {4.0, 16.0}},
                    3U);
  requests.push_back(markerRequest(CreativeObjectKind::NpcSpawn,
                                   "Objective Watch", 34.0, 26.0));
  requests.push_back(
      kitRequest(kGuardPost, "Objective Watch Marker", 36.2, 26.0));

  // The objective: a pickup deep in the yard, under the bypass-2 drop.
  requests.push_back(markerRequest(CreativeObjectKind::LootPoint,
                                   "Objective Cache", 30.0, 32.0));

  return requests;
}

}  // namespace
}  // namespace map_demo_internal

CreativeMapTemplateResult buildMapDemoMapTemplate(
    CreativeDocumentId documentId) {
  using map_demo_internal::mapDemoObjects;

  CreativeMapTemplateResult result;
  result.requested = true;
  result.templateId = std::string(kMapDemoTemplateId);
  const auto fail = [&result](CreativeMapTemplateStatus status,
                              std::string_view reasonCode) {
    result.status = status;
    result.reasonCode = std::string(reasonCode);
    result.accepted = false;
  };
  if (documentId == kInvalidDocumentId) {
    fail(CreativeMapTemplateStatus::InvalidDocumentId,
         "creative_map_template_document_id_invalid");
    return result;
  }

  Facade facade;
  CreativeDocument document = CreativeDocument::create("Map Demo");
  if (!document.assignId(documentId) ||
      !document.setGridSettings({{-40.0, 0.0, -40.0}, 1.0, {80, 20, 80}}) ||
      !document.setWorldBounds({{-40.0, -1.0, -40.0}, {40.0, 20.0, 40.0}}) ||
      !facade.installDocument(std::move(document)).accepted) {
    fail(CreativeMapTemplateStatus::DocumentSetupFailed,
         "creative_map_template_document_setup_failed");
    return result;
  }

  const std::vector<CreativeDocumentCreateRequest> objects =
      mapDemoObjects(facade.document().nextObjectId());
  const CreativeFacadeDocumentBatchCreateReceipt created =
      facade.createDocumentObjectsAtomically(objects);
  if (!created.accepted || !created.changed ||
      created.appliedCreateCount != objects.size()) {
    fail(CreativeMapTemplateStatus::ObjectBatchFailed,
         "creative_map_template_object_batch_failed");
    return result;
  }

  for (const CreativeObject& object : facade.document().objects()) {
    if (object.kind == CreativeObjectKind::Floor &&
        object.name == "Demo Ground") {
      result.primaryFloorObjectId = object.id;
      break;
    }
  }
  if (result.primaryFloorObjectId == kInvalidObjectId) {
    fail(CreativeMapTemplateStatus::ObjectBatchFailed,
         "creative_map_template_primary_floor_missing");
    return result;
  }

  result.document = facade.document();
  result.objectCount = result.document.objectCount();
  result.status = CreativeMapTemplateStatus::Ready;
  result.reasonCode = "creative_map_template_ready";
  result.accepted = true;
  return result;
}

}  // namespace iggy3d::creative
