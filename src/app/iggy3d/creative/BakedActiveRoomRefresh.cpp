#include "app/iggy3d/creative/BakedActiveRoomRefresh.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "app/iggy3d/CreativeReasoningActivation.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"

namespace iggy3d {
namespace {

constexpr const char* kCreativeRoomBakeNoRenderableObjects =
    "creative_room_bake_no_renderable_objects";
constexpr const char* kProductCreativeBakedRoomClearedNoRenderableObjects =
    "product_creative_baked_room_cleared_no_renderable_objects";

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

void setCreativeBakedActiveRoomRefreshStatus(
    ProductCreativeBakedActiveRoomRefreshResult& result,
    std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

std::string fallbackString(std::string_view value, std::string_view fallback) {
  return value.empty() ? std::string(fallback) : std::string(value);
}

ProductActiveRoomState clearedCreativeBakedActiveRoom(
    const ProductCreativeBakedActiveRoomRefreshRequest& request) {
  ProductActiveRoomState activeRoom;
  activeRoom.status = kProductCreativeBakedRoomClearedNoRenderableObjects;
  activeRoom.reasonCode = kProductCreativeBakedRoomClearedNoRenderableObjects;
  activeRoom.source = "creative_room_bake";
  activeRoom.roomId = fallbackString(request.roomId, "creative_room");
  activeRoom.sourceName = fallbackString(request.sourceName, "iggy3d.creative");
  activeRoom.sourceSubset =
      fallbackString(request.sourceSubset, "creative_document_bake");
  activeRoom.room.id = activeRoom.roomId;
  activeRoom.room.version = 1;
  activeRoom.room.units = "m";
  activeRoom.room.source = "iggy3d.creative_document";
  activeRoom.room.sourceFile = activeRoom.sourceName;
  activeRoom.room.sourceSubset = activeRoom.sourceSubset;
  return activeRoom;
}

class ProductCreativeBakedRoomRefreshService {
 public:
  ProductCreativeBakedRoomRefreshService(
      const ProductCreativeBakedActiveRoomRefreshRequest& request,
      std::optional<Session>& activeSession,
      ProductAppWindowState& window,
      const creative::CreativeAppState& creativeApp)
      : request_(request),
        activeSession_(activeSession),
        window_(window),
        creativeApp_(creativeApp) {}

  ProductCreativeBakedActiveRoomRefreshResult execute() {
    const creative::CreativeDocument& document = creativeApp_.facade.document();
    result_.documentId = document.id();
    result_.objectCount = document.objectCount();

    if (!passesPreconditions(document)) {
      return result_;
    }

    const creative::CreativeRoomBakeResult bake = bakeDocument(document);
    mirrorBakeResult(bake);
    if (!bake.receipt.accepted) {
      return handleRejectedBake(document, bake);
    }

    return installBakedRoom(document, bake.room);
  }

 private:
  bool passesPreconditions(const creative::CreativeDocument& document) {
    if (window_.inputDevice.interactionMode != ProductInteractionMode::Creative) {
      setCreativeBakedActiveRoomRefreshStatus(
          result_,
          "product_creative_baked_room_inactive");
      return false;
    }
    if (!activeSession_.has_value()) {
      setCreativeBakedActiveRoomRefreshStatus(
          result_,
          "product_creative_baked_room_session_missing");
      return false;
    }
    if (document.id() == creative::kInvalidDocumentId || !document.isValid()) {
      setCreativeBakedActiveRoomRefreshStatus(
          result_,
          "product_creative_baked_room_document_invalid");
      return false;
    }
    return true;
  }

  creative::CreativeRoomBakeRequest bakeRequestFor(
      const creative::CreativeDocument& document) const {
    creative::CreativeRoomBakeRequest bakeRequest;
    bakeRequest.document = &document;
    bakeRequest.roomId = request_.roomId;
    bakeRequest.sourceName = request_.sourceName;
    bakeRequest.sourceSubset = request_.sourceSubset;
    bakeRequest.includeHidden = request_.includeHidden;
    return bakeRequest;
  }

  creative::CreativeRoomBakeResult bakeDocument(
      const creative::CreativeDocument& document) {
    const creative::CreativeRoomBakeRequest bakeRequest =
        bakeRequestFor(document);
    result_.bakeMeasured = true;
    result_.bakedDocumentRevision = document.revision();
    const auto bakeStarted = std::chrono::steady_clock::now();
    const creative::CreativeRoomBakeResult bake =
        creative::buildRoomAssetFromCreativeDocument(bakeRequest);
    result_.bakeElapsedMicroseconds = elapsedMicroseconds(bakeStarted);
    return bake;
  }

  void mirrorBakeResult(const creative::CreativeRoomBakeResult& bake) {
    result_.bakeReceipt = bake.receipt;
    result_.staticMeshCount =
        static_cast<std::uint64_t>(bake.room.staticMeshes.size());
    result_.anchorCount = static_cast<std::uint64_t>(bake.room.anchors.size());
    result_.spatialSurfaceCount =
        static_cast<std::uint64_t>(bake.room.spatialSurfaces.size());
    result_.staticMeshSourceCount =
        static_cast<std::uint64_t>(bake.staticMeshSources.size());
    result_.anchorSourceCount =
        static_cast<std::uint64_t>(bake.anchorSources.size());
    result_.spatialSurfaceSourceCount =
        static_cast<std::uint64_t>(bake.spatialSurfaceSources.size());
  }

  ProductCreativeBakedActiveRoomRefreshResult handleRejectedBake(
      const creative::CreativeDocument& document,
      const creative::CreativeRoomBakeResult& bake) {
    if (request_.clearOnNoRenderable &&
        bake.receipt.reasonCode == kCreativeRoomBakeNoRenderableObjects) {
      activeRoom(window_) = clearedCreativeBakedActiveRoom(request_);
      bumpActiveRoomRevision(window_);
      (void)ensureActiveRoomCollisionFresh(window_, &*activeSession_);

      result_.accepted = true;
      result_.clearedActiveRoom = true;
      mirrorActiveRoomState();
      setCreativeBakedActiveRoomRefreshStatus(
          result_,
          kProductCreativeBakedRoomClearedNoRenderableObjects);
      recordProductCreativeBakedRoomFresh(window_,
                                          document.id(),
                                          document.revision());
      return result_;
    }

    setCreativeBakedActiveRoomRefreshStatus(result_, bake.receipt.reasonCode);
    return result_;
  }

  ProductCreativeBakedActiveRoomRefreshResult installBakedRoom(
      const creative::CreativeDocument& document,
      const RoomAsset& room) {
    ProductActiveRoomState bakedActiveRoom = buildProductActiveRoomFromPackageRoom(
        room, "iggy3d.creative", "creative.document");

    activeRoom(window_) = std::move(bakedActiveRoom);
    bumpActiveRoomRevision(window_);
    (void)ensureActiveRoomCollisionFresh(window_, &*activeSession_);

    if (request_.activationHook) {
      request_.activationHook(*activeSession_,
                              activeRoom(window_).room,
                              document);
    }

    mirrorActiveRoomState();
    result_.accepted = true;
    setCreativeBakedActiveRoomRefreshStatus(
        result_,
        "product_creative_baked_room_refreshed");
    recordProductCreativeBakedRoomFresh(window_,
                                        document.id(),
                                        document.revision());
    return result_;
  }

  void mirrorActiveRoomState() {
    result_.activeRoomLoaded = activeRoom(window_).loaded;
    result_.activeRoomStatus = activeRoom(window_).status;
    result_.collisionReady = activeRoomCollision(window_).ready;
    result_.collisionQuerySurfaceCount =
        activeRoomCollision(window_).querySurfaceCount;
  }

  const ProductCreativeBakedActiveRoomRefreshRequest& request_;
  std::optional<Session>& activeSession_;
  ProductAppWindowState& window_;
  const creative::CreativeAppState& creativeApp_;
  ProductCreativeBakedActiveRoomRefreshResult result_;
};

}  // namespace

ProductCreativeBakedActiveRoomRefreshResult refreshProductCreativeBakedActiveRoom(
    const ProductCreativeBakedActiveRoomRefreshRequest& request,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    const creative::CreativeAppState& creativeApp) {
  // Default the activation hook to the reasoning-graph fill: build the L4 graph
  // from the baked room and install it, so the shipped stealth guard reasons
  // over the authored room instead of the empty-graph fallback. Callers may
  // still supply their own hook (tests do).
  ProductCreativeBakedActiveRoomRefreshRequest resolved = request;
  if (!resolved.activationHook) {
    resolved.activationHook = &activateCreativeReasoningGraph;
  }
  return ProductCreativeBakedRoomRefreshService{
      resolved,
      activeSession,
      window,
      creativeApp,
  }.execute();
}

}  // namespace iggy3d
