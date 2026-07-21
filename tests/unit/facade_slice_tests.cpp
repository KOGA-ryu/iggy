// ASSET-BLD-1 phase 4 in-engine composition fixture proof.
//
// Loads fixtures/worlds/facade_slice.iggy3d.save through the REAL save codec
// (openCreativeWorld), discovers the REAL asset catalog, and proves the
// building-closure tranche composes through normal placement paths:
//
//   * every phase-4 asset id resolves in the catalog (all 26, not just the
//     ones placed in the slice);
//   * the socketed edges (wide leaf -> wide frame, shutters + mullion ->
//     window frame, cap -> chimney stack) round-trip parentId +
//     attachmentSocket through the codec;
//   * each plug socket's world position coincides with its named receiver
//     socket's world position (multi-receiver frames resolve by name);
//   * the engine's attachment-snap resolver reproduces the aligned transform
//     for every pair.
//
// The fixture is authored by
// assets/creative/calibration/tools/make_facade_slice_fixture.py.

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

// Receiver lookup by socket NAME: window frames carry three receivers, so
// role alone is ambiguous there.
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
      iggy3d::openCreativeWorld({"fixtures/worlds", "facade_slice"});
  bool ok = expect(opened.accepted,
                   "facade_slice fixture loads via the real codec") &&
            expect(opened.objectCount == 22U, "slice has twenty-two objects");
  if (!ok) {
    std::cerr << "  open reason: " << opened.reasonCode
              << " objects: " << opened.objectCount << '\n';
    return 1;
  }
  const cr::CreativeDocument& document = opened.document;

  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  ok = ok && expect(catalog.failures.empty(),
                    "asset catalog discovers cleanly with the phase-4 kit");

  // every phase-4 asset resolves in the catalog, placed in the slice or not
  const std::string_view kPhase4Assets[] = {
      "architecture/openings/door_frame_wide",
      "architecture/openings/door_leaf_wide_closed",
      "architecture/openings/door_leaf_wide_open",
      "architecture/openings/window_frame_standard",
      "architecture/openings/window_frame_small",
      "architecture/openings/window_shutter_left_closed",
      "architecture/openings/window_shutter_left_open",
      "architecture/openings/window_shutter_right_closed",
      "architecture/openings/window_shutter_right_open",
      "architecture/openings/window_mullion_cross",
      "architecture/openings/window_sill_standard",
      "architecture/openings/window_lintel_standard",
      "architecture/traversal/stair_straight_3m_with_rails",
      "architecture/traversal/stair_rail_slope_3m",
      "architecture/traversal/stair_newel_post",
      "architecture/roof/ridge_cap_straight_2m",
      "architecture/roof/eave_trim_2m",
      "architecture/roof/eave_trim_4m",
      "architecture/roof/eave_outer_corner",
      "architecture/roof/fascia_end",
      "architecture/roof/gable_cap_4m",
      "architecture/roof/gutter_straight_2m",
      "architecture/roof/downspout_3m",
      "architecture/roof/downspout_outlet",
      "architecture/roof/chimney_stack_short",
      "architecture/roof/chimney_cap",
  };
  for (std::string_view assetId : kPhase4Assets) {
    ok = ok && expect(catalog.find(assetId) != nullptr,
                      "phase-4 asset catalogued");
  }

  // the composed slice references resolve as placed objects
  const std::string_view kPlacedAssets[] = {
      "calibration/human_gauge_1p8m",
      "architecture/openings/door_frame_wide",
      "architecture/openings/door_leaf_wide_closed",
      "architecture/openings/window_frame_standard",
      "architecture/openings/window_shutter_left_closed",
      "architecture/openings/window_shutter_right_closed",
      "architecture/openings/window_mullion_cross",
      "architecture/openings/window_sill_standard",
      "architecture/openings/window_lintel_standard",
      "architecture/roof/eave_trim_4m",
      "architecture/roof/eave_outer_corner",
      "architecture/roof/fascia_end",
      "architecture/roof/gutter_straight_2m",
      "architecture/roof/downspout_3m",
      "architecture/roof/downspout_outlet",
      "architecture/roof/chimney_stack_short",
      "architecture/roof/chimney_cap",
  };
  for (std::string_view assetId : kPlacedAssets) {
    ok = ok && expect(hasAsset(document, assetId), "slice asset placed");
  }

  // socket round trips: door, window closures, chimney cap
  ok = proveSocketPair(document, catalog, "Wide Door Frame", "Wide Door Leaf",
                       "architecture/openings/door_leaf_wide_closed",
                       "hinge.left") &&
       ok;
  ok = proveSocketPair(document, catalog, "Window West", "Window West Shutter L",
                       "architecture/openings/window_shutter_left_closed",
                       "shutter.left") &&
       ok;
  ok = proveSocketPair(document, catalog, "Window West", "Window West Shutter R",
                       "architecture/openings/window_shutter_right_closed",
                       "shutter.right") &&
       ok;
  ok = proveSocketPair(document, catalog, "Window West", "Window West Mullion",
                       "architecture/openings/window_mullion_cross",
                       "mullion") &&
       ok;
  ok = proveSocketPair(document, catalog, "Window East", "Window East Mullion",
                       "architecture/openings/window_mullion_cross",
                       "mullion") &&
       ok;
  ok = proveSocketPair(document, catalog, "Chimney Stack", "Chimney Cap",
                       "architecture/roof/chimney_cap", "cap") &&
       ok;

  if (ok) {
    std::cout << "facade_slice_tests passed\n";
  }
  return ok ? 0 : 1;
}
