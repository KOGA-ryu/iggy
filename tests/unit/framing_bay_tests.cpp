// ASSET-BLD-2 in-engine composition fixture proof.
//
// Loads fixtures/worlds/framing_bay.iggy3d.save through the REAL save codec,
// discovers the REAL asset catalog, and proves the structural tranche
// composes through normal placement paths:
//
//   * every BLD-2 asset id resolves in the catalog (all 24);
//   * the mirrored double-door leaves attach to their two NAMED receivers on
//     one frame;
//   * the iron bars mount in the standard window frame's mullion receiver
//     (alternate insert of the mullion family);
//   * the column chain base -> column -> cap round-trips a two-deep socket
//     parent chain, each edge aligned and resolver-reproduced.
//
// The fixture is authored by
// assets/creative/calibration/tools/make_framing_bay_fixture.py.

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

// World socket position for an axis-aligned (rotation 0) placed object.
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
      iggy3d::openCreativeWorld({"fixtures/worlds", "framing_bay"});
  bool ok = expect(opened.accepted,
                   "framing_bay fixture loads via the real codec") &&
            expect(opened.objectCount == 20U, "bay has twenty objects");
  if (!ok) {
    std::cerr << "  open reason: " << opened.reasonCode
              << " objects: " << opened.objectCount << '\n';
    return 1;
  }
  const cr::CreativeDocument& document = opened.document;

  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  ok = ok && expect(catalog.failures.empty(),
                    "asset catalog discovers cleanly with the BLD-2 kit");

  const std::string_view kBld2Assets[] = {
      "architecture/openings/door_frame_double",
      "architecture/openings/door_leaf_double_left_closed",
      "architecture/openings/door_leaf_double_left_open",
      "architecture/openings/door_leaf_double_right_closed",
      "architecture/openings/door_leaf_double_right_open",
      "architecture/openings/window_frame_wide",
      "architecture/openings/window_frame_tall",
      "architecture/openings/window_bars_standard",
      "architecture/structural/foundation_plinth_straight_2m",
      "architecture/structural/foundation_plinth_straight_4m",
      "architecture/structural/foundation_plinth_inner_corner",
      "architecture/structural/foundation_plinth_outer_corner",
      "architecture/structural/foundation_plinth_end",
      "architecture/structural/post_square_0p3x3m",
      "architecture/structural/post_square_0p5x3m",
      "architecture/structural/column_round_0p5x3m",
      "architecture/structural/column_round_base",
      "architecture/structural/column_round_cap",
      "architecture/structural/beam_2m",
      "architecture/structural/beam_4m",
      "architecture/structural/beam_6m",
      "architecture/structural/beam_end_cap",
      "architecture/structural/brace_left",
      "architecture/structural/brace_right",
  };
  for (std::string_view assetId : kBld2Assets) {
    ok = ok && expect(catalog.find(assetId) != nullptr,
                      "BLD-2 asset catalogued");
  }

  const std::string_view kPlacedAssets[] = {
      "calibration/human_gauge_1p8m",
      "architecture/structural/foundation_plinth_straight_4m",
      "architecture/structural/foundation_plinth_end",
      "architecture/structural/post_square_0p3x3m",
      "architecture/structural/beam_4m",
      "architecture/structural/beam_end_cap",
      "architecture/structural/brace_left",
      "architecture/structural/brace_right",
      "architecture/openings/door_frame_double",
      "architecture/openings/door_leaf_double_left_closed",
      "architecture/openings/door_leaf_double_right_closed",
      "architecture/openings/window_frame_wide",
      "architecture/openings/window_frame_tall",
      "architecture/openings/window_frame_standard",
      "architecture/openings/window_bars_standard",
      "architecture/structural/column_round_base",
      "architecture/structural/column_round_0p5x3m",
      "architecture/structural/column_round_cap",
  };
  for (std::string_view assetId : kPlacedAssets) {
    ok = ok && expect(hasAsset(document, assetId), "bay asset placed");
  }

  // mirrored leaves on one frame's two named receivers
  ok = proveSocketPair(document, catalog, "Double Door Frame",
                       "Double Leaf Left",
                       "architecture/openings/door_leaf_double_left_closed",
                       "hinge.left") &&
       ok;
  ok = proveSocketPair(document, catalog, "Double Door Frame",
                       "Double Leaf Right",
                       "architecture/openings/door_leaf_double_right_closed",
                       "hinge.right") &&
       ok;
  // alternate insert of the mullion family
  ok = proveSocketPair(document, catalog, "Window Barred", "Window Bars",
                       "architecture/openings/window_bars_standard",
                       "mullion") &&
       ok;
  // two-deep chain: base -> column -> cap
  ok = proveSocketPair(document, catalog, "Column Base", "Column",
                       "architecture/structural/column_round_0p5x3m",
                       "column.base") &&
       ok;
  ok = proveSocketPair(document, catalog, "Column", "Column Cap",
                       "architecture/structural/column_round_cap",
                       "column.cap") &&
       ok;

  if (ok) {
    std::cout << "framing_bay_tests passed\n";
  }
  return ok ? 0 : 1;
}
