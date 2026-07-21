// ASSET-CAL-1 in-engine composition fixture proof (Batch 0/1 seed).
//
// Loads the committed bay world fixtures/worlds/calibration_bay.iggy3d.save
// through the REAL save codec (openCreativeWorld), discovers the REAL asset
// catalog (discoverStaticMeshAssetCatalog), and proves the socket contract
// survived Blender -> glTF -> import -> save -> load:
//
//   * every bay asset id resolves in the catalog (the 4x4 m bay composes);
//   * the socketed edges (door leaf -> frame, plug -> receiver) round-trip
//     their parentId + attachmentSocket through the codec;
//   * geometric alignment holds in engine space: each plug socket's world
//     position coincides with its receiver socket's world position (Q6);
//   * the engine's own attachment-snap resolver reproduces that aligned
//     transform for both pairs (editor placement path agrees).
//
// The fixture is authored from the map_demo envelope; see
// assets/creative/calibration/README.md for the construction record.

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

// Prove one socket pair: the loaded edge round-tripped, the plug socket world
// coincides with the receiver socket world, and the engine snap resolver
// reproduces the aligned transform.
bool proveSocketPair(const cr::CreativeDocument& document,
                     const iggy3d::StaticMeshAssetCatalog& catalog,
                     std::string_view receiverName,
                     std::string_view plugName,
                     std::string_view plugAssetId,
                     std::string_view expectedSocket,
                     const char* label) {
  const cr::CreativeObject* receiver = byName(document, receiverName);
  const cr::CreativeObject* plug = byName(document, plugName);
  if (!expect(receiver != nullptr && plug != nullptr,
              "socket pair objects present")) {
    return false;
  }
  // 1. the socket edge survived save/load
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
      socketOf(receiverEntry, iggy3d::StaticMeshAttachmentSocketRole::Receiver);
  const iggy3d::StaticMeshAttachmentSocket* plugSocket =
      socketOf(plugEntry, iggy3d::StaticMeshAttachmentSocketRole::Plug);
  if (!expect(receiverSocket != nullptr && plugSocket != nullptr,
              "receiver + plug sockets imported from glb")) {
    return false;
  }
  // 2. compatibility families match after the round trip
  ok = ok && expect(receiverSocket->compatibility == plugSocket->compatibility,
                    "socket compatibility families match");
  // 3. geometric alignment in engine space (Q6)
  const cr::CreativeVec3 receiverWorld = worldSocket(*receiver, *receiverSocket);
  const cr::CreativeVec3 plugWorld = worldSocket(*plug, *plugSocket);
  ok = ok && near3(receiverWorld, plugWorld,
                   "plug socket coincides with receiver socket in engine space");

  // 4. the engine's own snap resolver reproduces the aligned transform. The
  // receiver socket is filled by the authored plug, so the resolver reports
  // Occupied (or Ready) with the aligned candidate transform.
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
  static_cast<void>(label);
  return ok;
}

}  // namespace

int main() {
  const iggy3d::CreativeWorldOpenResult opened =
      iggy3d::openCreativeWorld({"fixtures/worlds", "calibration_bay"});
  bool ok = expect(opened.accepted,
                   "calibration_bay fixture loads via the real codec") &&
            expect(opened.objectCount == 13U, "bay has thirteen objects");
  if (!ok) {
    std::cerr << "  open reason: " << opened.reasonCode << '\n';
    return 1;
  }
  const cr::CreativeDocument& document = opened.document;

  const iggy3d::StaticMeshAssetCatalog catalog =
      iggy3d::discoverStaticMeshAssetCatalog("assets/creative");
  ok = ok && expect(catalog.failures.empty(),
                    "asset catalog discovers cleanly with the new kit");

  // the 4x4 m bay composition resolves through the catalog
  const std::string_view kBayAssets[] = {
      "calibration/grid_1m_10x10",
      "calibration/storey_3m",
      "calibration/human_gauge_1p8m",
      "architecture/openings/door_frame_standard",
      "architecture/openings/door_leaf_standard_closed",
      "architecture/traversal/stair_straight_3m",
      "architecture/traversal/stair_landing_2x2m",
      "architecture/structural/railing_straight_2m",
      "architecture/roof/ridge_cap_straight_4m",
      "architecture/roof/ridge_cap_end",
      "calibration/socket_receiver",
      "calibration/socket_plug",
  };
  for (std::string_view assetId : kBayAssets) {
    ok = ok && expect(hasAsset(document, assetId) &&
                          catalog.find(assetId) != nullptr,
                      "bay asset present and catalogued");
  }

  // socket round trip: door leaf -> frame, and calibration plug -> receiver
  ok = proveSocketPair(document, catalog, "Bay Door Frame", "Bay Door Leaf",
                       "architecture/openings/door_leaf_standard_closed",
                       "hinge.left", "door") &&
       ok;
  ok = proveSocketPair(document, catalog, "Bay Socket Receiver",
                       "Bay Socket Plug", "calibration/socket_plug",
                       "receiver.main", "calibration") &&
       ok;

  if (ok) {
    std::cout << "calibration_bay_tests passed\n";
  }
  return ok ? 0 : 1;
}
