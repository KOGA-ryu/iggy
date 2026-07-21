// ASSET-BLD-3 / Batch 1 acceptance composition proof.
//
// proof/two_storey_estate (backlog Required Compositions): exterior facade,
// per-floor partitions, stairs, roof, balconies — composed exclusively from
// catalog assets through real placement records, loaded via the real codec.
// This test is the Batch 1 (Building Closure) gate: it proves assets from
// all three tranches (CAL-1 seed, BLD-1, BLD-2, BLD-3) close into one
// authored two-storey space with working socket edges on every family that
// defines one.
//
// The fixture is authored by
// assets/creative/calibration/tools/make_two_storey_estate_fixture.py.

#include <cmath>
#include <iostream>
#include <string_view>

#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"
#include "app/iggy3d/creative/world/WorldService.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace {

namespace cr = iggy3d::creative;

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const cr::CreativeObject* byName(const cr::CreativeDocument& document,
                                 std::string_view name) {
  for (const cr::CreativeObject& object : document.objects()) {
    if (object.name == name) {
      return &object;
    }
  }
  return nullptr;
}

bool hasAsset(const cr::CreativeDocument& document, std::string_view assetId) {
  for (const cr::CreativeObject& object : document.objects()) {
    if (object.assetId == assetId) {
      return true;
    }
  }
  return false;
}

const iggy3d::StaticMeshAttachmentSocket* socketNamed(
    const iggy3d::StaticMeshAssetCatalogEntry* entry, std::string_view name) {
  if (entry == nullptr) {
    return nullptr;
  }
  for (const iggy3d::StaticMeshAttachmentSocket& socket :
       entry->attachmentSockets) {
    if (socket.name == name) {
      return &socket;
    }
  }
  return nullptr;
}

const iggy3d::StaticMeshAttachmentSocket* socketOf(
    const iggy3d::StaticMeshAssetCatalogEntry* entry,
    iggy3d::StaticMeshAttachmentSocketRole role) {
  if (entry == nullptr) {
    return nullptr;
  }
  for (const iggy3d::StaticMeshAttachmentSocket& socket :
       entry->attachmentSockets) {
    if (socket.role == role) {
      return &socket;
    }
  }
  return nullptr;
}

cr::CreativeVec3 worldSocket(const cr::CreativeObject& object,
                             const iggy3d::StaticMeshAttachmentSocket& socket) {
  return {object.transform.position.x + static_cast<double>(socket.position.x),
          object.transform.position.y + static_cast<double>(socket.position.y),
          object.transform.position.z + static_cast<double>(socket.position.z)};
}

bool near3(cr::CreativeVec3 a, cr::CreativeVec3 b, const char* message) {
  const bool ok = std::fabs(a.x - b.x) < 1.0e-3 &&
                  std::fabs(a.y - b.y) < 1.0e-3 &&
                  std::fabs(a.z - b.z) < 1.0e-3;
  return expect(ok, message);
}

bool proveSocketPair(const cr::CreativeDocument& document,
                     const iggy3d::StaticMeshAssetCatalog& catalog,
                     std::string_view receiverName,
                     std::string_view plugName,
                     std::string_view plugAssetId,
                     std::string_view expectedSocket) {
  const cr::CreativeObject* receiver = byName(document, receiverName);
  const cr::CreativeObject* plug = byName(document, plugName);
  if (!expect(receiver != nullptr && plug != nullptr,
              "socket pair objects present")) {
    std::cerr << "  pair: " << receiverName << " / " << plugName << '\n';
    return false;
  }
  bool ok = expect(plug->parentId.has_value() &&
                       plug->parentId.value() == receiver->id,
                   "plug parented to receiver through the codec") &&
            expect(plug->attachmentSocket == expectedSocket,
                   "plug attachmentSocket round-tripped");

  const iggy3d::StaticMeshAssetCatalogEntry* receiverEntry =
      catalog.find(receiver->assetId);
  const iggy3d::StaticMeshAssetCatalogEntry* plugEntry =
      catalog.find(plug->assetId);
  const iggy3d::StaticMeshAttachmentSocket* receiverSocket =
      socketNamed(receiverEntry, expectedSocket);
  const iggy3d::StaticMeshAttachmentSocket* plugSocket =
      socketOf(plugEntry, iggy3d::StaticMeshAttachmentSocketRole::Plug);
  if (!expect(receiverSocket != nullptr && plugSocket != nullptr,
              "named receiver + plug sockets imported from glb")) {
    std::cerr << "  pair: " << receiverName << " / " << plugName << '\n';
    return false;
  }
  ok = ok && expect(receiverSocket->compatibility == plugSocket->compatibility,
                    "socket compatibility families match");
  const cr::CreativeVec3 receiverWorld = worldSocket(*receiver, *receiverSocket);
  const cr::CreativeVec3 plugWorld = worldSocket(*plug, *plugSocket);
  ok = ok && near3(receiverWorld, plugWorld,
                   "plug socket coincides with receiver socket in engine space");

  cr::CreativeAttachmentSnapRequest request;
  request.document = &document;
  request.assetCatalog = &catalog;
  request.sourceAssetId = plugAssetId;
  request.targetObjectId = receiver->id;
  request.aimPoint = receiverWorld;
  const cr::CreativeAttachmentSnapResult snap =
      cr::resolveCreativeAttachmentSnap(request);
  ok = ok &&
       expect(snap.compatiblePairCount >= 1U,
              "resolver finds a compatible plug/receiver pair") &&
       expect(snap.positioned &&
                  (snap.status == cr::CreativeAttachmentSnapStatus::Occupied ||
                   snap.status == cr::CreativeAttachmentSnapStatus::Ready),
              "resolver positions the plug on the receiver") &&
       near3(snap.transform.position, plug->transform.position,
             "resolver transform matches the authored plug placement");
  if (!ok) {
    std::cerr << "  pair: " << receiverName << " / " << plugName << '\n';
  }
  return ok;
}

}  // namespace

int main() {
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({"fixtures/worlds", "two_storey_estate"});
  bool ok = expect(opened.accepted,
                   "two_storey_estate fixture loads via the real codec") &&
            expect(opened.objectCount == 49U, "estate has forty-nine objects");
  if (!ok) {
    std::cerr << "  open reason: " << opened.reasonCode
              << " objects: " << opened.objectCount << '\n';
    return 1;
  }
  const cr::CreativeDocument& document = opened.document;

  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  ok = ok && expect(catalog.failures.empty(),
                    "asset catalog discovers cleanly with the BLD-3 kit");

  // every BLD-3 asset resolves in the catalog
  const std::string_view kBld3Assets[] = {
      "architecture/traversal/stair_quarter_turn_3m",
      "architecture/traversal/stair_landing_2x4m",
      "architecture/traversal/ramp_2x3x1p5m",
      "architecture/traversal/ramp_2x6x3m",
      "architecture/traversal/ramp_landing_2x2m",
      "architecture/structural/arch_1p2m",
      "architecture/structural/arch_2m",
      "architecture/structural/arch_4m",
      "architecture/structural/buttress_low",
      "architecture/structural/buttress_tall",
      "architecture/structural/wall_pier_0p5x3m",
      "architecture/structural/wall_pier_1x3m",
      "architecture/structural/parapet_straight_2m",
      "architecture/structural/parapet_straight_4m",
      "architecture/structural/parapet_inner_corner",
      "architecture/structural/parapet_outer_corner",
      "architecture/structural/parapet_end",
      "architecture/structural/balcony_deck_2x1p5m",
      "architecture/structural/balcony_deck_4x1p5m",
      "architecture/structural/balcony_bracket",
      "architecture/structural/railing_straight_1m",
      "architecture/structural/railing_corner",
      "architecture/structural/railing_end_post",
  };
  for (std::string_view assetId : kBld3Assets) {
    ok = ok && expect(catalog.find(assetId) != nullptr,
                      "BLD-3 asset catalogued");
  }

  // the estate composes assets from every Batch 1 tranche
  const std::string_view kPlacedAssets[] = {
      "calibration/human_gauge_1p8m",
      "architecture/structural/foundation_plinth_straight_4m",
      "architecture/structural/foundation_plinth_end",
      "architecture/structural/foundation_plinth_outer_corner",
      "architecture/structural/buttress_low",
      "architecture/structural/wall_pier_1x3m",
      "architecture/structural/wall_pier_0p5x3m",
      "architecture/openings/door_frame_double",
      "architecture/openings/door_leaf_double_left_closed",
      "architecture/openings/door_leaf_double_right_closed",
      "architecture/openings/window_frame_standard",
      "architecture/openings/window_bars_standard",
      "architecture/openings/window_sill_standard",
      "architecture/openings/window_lintel_standard",
      "architecture/openings/window_frame_wide",
      "architecture/openings/window_frame_tall",
      "architecture/openings/window_shutter_left_closed",
      "architecture/openings/window_shutter_right_closed",
      "architecture/structural/arch_2m",
      "architecture/structural/column_round_base",
      "architecture/structural/column_round_0p5x3m",
      "architecture/structural/column_round_cap",
      "architecture/traversal/stair_quarter_turn_3m",
      "architecture/traversal/stair_landing_2x4m",
      "architecture/structural/railing_corner",
      "architecture/structural/railing_straight_1m",
      "architecture/structural/railing_end_post",
      "architecture/openings/door_frame_wide",
      "architecture/openings/door_leaf_wide_closed",
      "architecture/structural/balcony_deck_2x1p5m",
      "architecture/structural/balcony_bracket",
      "architecture/structural/parapet_straight_4m",
      "architecture/structural/parapet_end",
      "architecture/structural/parapet_outer_corner",
      "architecture/roof/gable_cap_4m",
      "architecture/roof/ridge_cap_straight_4m",
      "architecture/roof/chimney_stack_short",
      "architecture/roof/chimney_cap",
  };
  for (std::string_view assetId : kPlacedAssets) {
    ok = ok && expect(hasAsset(document, assetId), "estate asset placed");
  }

  // socket edges across the composition
  ok = proveSocketPair(document, catalog, "Estate Door Frame",
                       "Estate Leaf Left",
                       "architecture/openings/door_leaf_double_left_closed",
                       "hinge.left") &&
       ok;
  ok = proveSocketPair(document, catalog, "Estate Door Frame",
                       "Estate Leaf Right",
                       "architecture/openings/door_leaf_double_right_closed",
                       "hinge.right") &&
       ok;
  ok = proveSocketPair(document, catalog, "Window Ground Barred",
                       "Ground Bars",
                       "architecture/openings/window_bars_standard",
                       "mullion") &&
       ok;
  ok = proveSocketPair(document, catalog, "Balcony Door Frame",
                       "Balcony Door Leaf",
                       "architecture/openings/door_leaf_wide_closed",
                       "hinge.left") &&
       ok;
  ok = proveSocketPair(document, catalog, "Column Base West", "Column West",
                       "architecture/structural/column_round_0p5x3m",
                       "column.base") &&
       ok;
  ok = proveSocketPair(document, catalog, "Column West", "Column Cap West",
                       "architecture/structural/column_round_cap",
                       "column.cap") &&
       ok;
  ok = proveSocketPair(document, catalog, "Estate Chimney",
                       "Estate Chimney Cap", "architecture/roof/chimney_cap",
                       "cap") &&
       ok;

  if (ok) {
    std::cout << "two_storey_estate_tests passed\n";
  }
  return ok ? 0 : 1;
}
