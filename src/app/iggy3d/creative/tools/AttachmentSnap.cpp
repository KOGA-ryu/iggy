#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/recipes/RampRecipe.hpp"
#include "app/iggy3d/creative/recipes/StairRecipe.hpp"
#include "app/iggy3d/creative/recipes/StructuralRoofRecipe.hpp"

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeVec3 add(CreativeVec3 lhs,
                               CreativeVec3 rhs) noexcept {
  return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

[[nodiscard]] CreativeVec3 subtract(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
  return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

[[nodiscard]] CreativeVec3 multiply(CreativeVec3 lhs,
                                    CreativeVec3 rhs) noexcept {
  return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

[[nodiscard]] CreativeVec3 toCreative(Vec3 value) noexcept {
  return {static_cast<double>(value.x), static_cast<double>(value.y),
          static_cast<double>(value.z)};
}

struct WorldAttachmentSocketFrame {
  CreativeVec3 position{};
  CreativeVec3 forward{};
  CreativeVec3 up{};
  bool positioned = false;
  bool oriented = false;
};

struct AttachmentSocketView {
  std::string_view name;
  std::string_view compatibility;
  StaticMeshAttachmentSocketRole role =
      StaticMeshAttachmentSocketRole::Invalid;
  CreativeVec3 position{};
  CreativeVec3 forward{0.0, 0.0, 1.0};
  CreativeVec3 up{0.0, 1.0, 0.0};
};

[[nodiscard]] AttachmentSocketView socketView(
    const StaticMeshAttachmentSocket& socket) noexcept {
  return {socket.name, socket.compatibility, socket.role,
          toCreative(socket.position), toCreative(socket.forward),
          toCreative(socket.up)};
}

enum class TargetAttachmentSocketStatus : std::uint8_t {
  Ready,
  AssetMissing,
  InvalidGeometry,
};

struct TargetAttachmentSocketSet {
  const std::vector<StaticMeshAttachmentSocket>* imported = nullptr;
  std::array<AttachmentSocketView,
             std::max(kCreativeStructuralRoofDrainageSocketCapacity,
                      std::max(kCreativeStairSocketCapacity,
                               kCreativeRampSocketCapacity))>
      generated{};
  std::size_t generatedCount = 0U;
  TargetAttachmentSocketStatus status =
      TargetAttachmentSocketStatus::AssetMissing;

  [[nodiscard]] std::size_t size() const noexcept {
    return imported != nullptr ? imported->size() : generatedCount;
  }

  [[nodiscard]] AttachmentSocketView at(std::size_t index) const noexcept {
    return imported != nullptr ? socketView((*imported)[index])
                               : generated[index];
  }
};

[[nodiscard]] TargetAttachmentSocketSet targetAttachmentSockets(
    const CreativeObject& target,
    const StaticMeshAssetCatalog& catalog) noexcept {
  TargetAttachmentSocketSet sockets;
  if (const StaticMeshAssetCatalogEntry* asset = catalog.find(target.assetId);
      asset != nullptr) {
    sockets.imported = &asset->attachmentSockets;
    sockets.status = TargetAttachmentSocketStatus::Ready;
    return sockets;
  }
  if (target.kind == CreativeObjectKind::Stair) {
    CreativeStairRecipeRequest request;
    request.authoredBounds = target.bounds;
    request.transform = target.transform;
    request.availableHeadroomMeters = kCreativeStairMinimumHeadroomMeters;
    const CreativeStairRecipeResult stair = planCreativeStair(request);
    if (!stair.accepted || stair.socketCount > sockets.generated.size()) {
      sockets.status = TargetAttachmentSocketStatus::InvalidGeometry;
      return sockets;
    }
    for (std::size_t index = 0U; index < stair.socketCount; ++index) {
      const CreativeStairSocketPlan& source = stair.sockets[index];
      sockets.generated[index] = {
          creativeStairSocketName(source.kind),
          creativeStairSocketCompatibility(source.kind),
          StaticMeshAttachmentSocketRole::Receiver,
          source.localPosition,
          source.forward,
          source.up,
      };
    }
    sockets.generatedCount = stair.socketCount;
    sockets.status = TargetAttachmentSocketStatus::Ready;
    return sockets;
  }
  if (target.kind == CreativeObjectKind::Ramp) {
    CreativeRampRecipeRequest request;
    request.authoredBounds = target.bounds;
    request.transform = target.transform;
    request.availableHeadroomMeters = kCreativeRampMinimumHeadroomMeters;
    CreativeStructuralMaterial material = CreativeStructuralMaterial::Blockout;
    if (parseCreativeStructuralMaterialTag(target.tags, material)) {
      request.material = material;
    }
    const CreativeRampRecipeResult ramp = planCreativeRamp(request);
    if (!ramp.accepted || ramp.socketCount > sockets.generated.size()) {
      sockets.status = TargetAttachmentSocketStatus::InvalidGeometry;
      return sockets;
    }
    for (std::size_t index = 0U; index < ramp.socketCount; ++index) {
      const CreativeRampSocketPlan& source = ramp.sockets[index];
      sockets.generated[index] = {
          creativeRampSocketName(source.kind),
          creativeRampSocketCompatibility(source.kind),
          StaticMeshAttachmentSocketRole::Receiver,
          source.localPosition,
          source.forward,
          source.up,
      };
    }
    sockets.generatedCount = ramp.socketCount;
    sockets.status = TargetAttachmentSocketStatus::Ready;
    return sockets;
  }
  if (target.kind == CreativeObjectKind::Roof ||
      target.kind == CreativeObjectKind::RoofSlope ||
      target.kind == CreativeObjectKind::HipRoof) {
    const CreativeStructuralRoofPartSocketResult roof =
        planCreativeStructuralRoofPartSockets(
            target.kind, target.bounds, target.transform);
    if (!roof.accepted || roof.socketCount > sockets.generated.size()) {
      sockets.status = TargetAttachmentSocketStatus::InvalidGeometry;
      return sockets;
    }
    for (std::size_t index = 0U; index < roof.socketCount; ++index) {
      const CreativeStructuralRoofPartSocketPlan& source =
          roof.sockets[index];
      sockets.generated[index] = {
          creativeStructuralRoofDrainageSocketName(source.edge),
          creativeStructuralRoofDrainageCompatibility(),
          StaticMeshAttachmentSocketRole::Receiver,
          source.localPosition,
          source.forward,
          source.up,
      };
    }
    sockets.generatedCount = roof.socketCount;
    sockets.status = TargetAttachmentSocketStatus::Ready;
  }
  return sockets;
}

[[nodiscard]] double squaredLength(CreativeVec3 value) noexcept {
  return value.x * value.x + value.y * value.y + value.z * value.z;
}

[[nodiscard]] bool horizontalFrame(CreativeVec3 forward,
                                   CreativeVec3 up) noexcept {
  constexpr double kAxisTolerance = 1.0e-4;
  const double forwardLengthSquared = squaredLength(forward);
  const double upLengthSquared = squaredLength(up);
  return isFiniteCreativeVec3(forward) && isFiniteCreativeVec3(up) &&
         forwardLengthSquared > kAxisTolerance &&
         upLengthSquared > kAxisTolerance &&
         std::fabs(forward.y) <= 0.001 && std::fabs(up.x) <= 0.001 &&
         std::fabs(up.z) <= 0.001 && up.y > 0.999;
}

[[nodiscard]] double yawForForward(CreativeVec3 forward) noexcept {
  return std::atan2(forward.x, forward.z);
}

[[nodiscard]] WorldAttachmentSocketFrame worldAttachmentSocketFrame(
    const AttachmentSocketView& socket,
    const CreativeTransform& targetTransform) noexcept {
  WorldAttachmentSocketFrame result;
  const CreativeVec3 localPosition =
      multiply(socket.position, targetTransform.scale);
  result.position = add(
      targetTransform.position,
      rotateCreativeVectorEulerXyz(
          localPosition, targetTransform.rotationEulerRadians));
  result.forward = rotateCreativeVectorEulerXyz(
      socket.forward, targetTransform.rotationEulerRadians);
  result.up = rotateCreativeVectorEulerXyz(
      socket.up, targetTransform.rotationEulerRadians);
  result.positioned = isFiniteCreativeVec3(result.position);
  result.oriented = result.positioned &&
                    horizontalFrame(result.forward, result.up);
  return result;
}

[[nodiscard]] double normalizeRadians(double radians) noexcept {
  constexpr double kTwoPi = 2.0 * std::numbers::pi;
  radians = std::remainder(radians, kTwoPi);
  return radians == -std::numbers::pi ? std::numbers::pi : radians;
}

[[nodiscard]] bool socketOccupied(const CreativeDocument& document,
                                  CreativeObjectId parentId,
                                  std::string_view socket,
                                  CreativeObjectId ignoredObjectId =
                                      kInvalidObjectId) noexcept {
  return std::any_of(
      document.objects().begin(), document.objects().end(),
      [parentId, socket, ignoredObjectId](const CreativeObject& object) {
        return object.id != ignoredObjectId && object.parentId == parentId &&
               object.attachmentSocket == socket;
      });
}

[[nodiscard]] bool betterReceiverCandidate(
    double distanceSquared,
    std::string_view socket,
    double bestDistanceSquared,
    std::string_view bestSocket,
    bool hasBest) noexcept {
  constexpr double kTieEpsilon = 1.0e-12;
  return !hasBest || distanceSquared < bestDistanceSquared - kTieEpsilon ||
         (std::fabs(distanceSquared - bestDistanceSquared) <= kTieEpsilon &&
          socket < bestSocket);
}

[[nodiscard]] bool betterCandidate(double distanceSquared,
                                   std::string_view targetSocket,
                                   std::string_view sourceSocket,
                                   double bestDistanceSquared,
                                   std::string_view bestTargetSocket,
                                   std::string_view bestSourceSocket,
                                   bool hasBest) noexcept {
  constexpr double kTieEpsilon = 1.0e-12;
  if (!hasBest || distanceSquared < bestDistanceSquared - kTieEpsilon) {
    return true;
  }
  if (std::fabs(distanceSquared - bestDistanceSquared) > kTieEpsilon) {
    return false;
  }
  return targetSocket < bestTargetSocket ||
         (targetSocket == bestTargetSocket && sourceSocket < bestSourceSocket);
}

}  // namespace

std::string_view toString(CreativeAttachmentSnapStatus status) noexcept {
  switch (status) {
    case CreativeAttachmentSnapStatus::NotRequested:
      return "creative_attachment_not_requested";
    case CreativeAttachmentSnapStatus::InvalidRequest:
      return "creative_attachment_request_invalid";
    case CreativeAttachmentSnapStatus::SourceAssetMissing:
      return "creative_attachment_source_asset_missing";
    case CreativeAttachmentSnapStatus::SourcePlugMissing:
      return "creative_attachment_source_plug_missing";
    case CreativeAttachmentSnapStatus::TargetObjectMissing:
      return "creative_attachment_target_object_missing";
    case CreativeAttachmentSnapStatus::TargetAssetMissing:
      return "creative_attachment_target_asset_missing";
    case CreativeAttachmentSnapStatus::TargetReceiverMissing:
      return "creative_attachment_target_receiver_missing";
    case CreativeAttachmentSnapStatus::AimedSocketUnavailable:
      return "creative_attachment_aimed_socket_unavailable";
    case CreativeAttachmentSnapStatus::NoCompatibleSocket:
      return "creative_attachment_socket_incompatible";
    case CreativeAttachmentSnapStatus::OutsideRadius:
      return "creative_attachment_socket_out_of_range";
    case CreativeAttachmentSnapStatus::Occupied:
      return "creative_attachment_socket_occupied";
    case CreativeAttachmentSnapStatus::Ready:
      return "creative_attachment_ready";
  }
  return "creative_attachment_status_invalid";
}

CreativeAttachmentSnapResult resolveCreativeAttachmentSnap(
    const CreativeAttachmentSnapRequest& request) noexcept {
  CreativeAttachmentSnapResult result;
  if (request.document == nullptr || request.assetCatalog == nullptr ||
      request.sourceAssetId.empty() ||
      request.targetObjectId == kInvalidObjectId ||
      !isFiniteCreativeVec3(request.aimPoint) ||
      !isPositiveCreativeVec3(request.sourceScale) ||
      !std::isfinite(request.maxDistanceMeters) ||
      request.maxDistanceMeters <= 0.0 ||
      !std::isfinite(request.aimedSocketRadiusMeters) ||
      request.aimedSocketRadiusMeters <= 0.0 ||
      request.selectionMode >= CreativeAttachmentSnapSelectionMode::Count) {
    result.status = CreativeAttachmentSnapStatus::InvalidRequest;
    return result;
  }

  const StaticMeshAssetCatalogEntry* source =
      request.assetCatalog->find(request.sourceAssetId);
  if (source == nullptr) {
    result.status = CreativeAttachmentSnapStatus::SourceAssetMissing;
    return result;
  }
  result.sourcePlugCount = static_cast<std::size_t>(std::count_if(
      source->attachmentSockets.begin(), source->attachmentSockets.end(),
      [](const StaticMeshAttachmentSocket& socket) {
        return socket.role == StaticMeshAttachmentSocketRole::Plug;
      }));
  if (result.sourcePlugCount == 0U) {
    result.status = CreativeAttachmentSnapStatus::SourcePlugMissing;
    return result;
  }

  const CreativeObject* target =
      request.document->findObject(request.targetObjectId);
  if (target == nullptr) {
    result.status = CreativeAttachmentSnapStatus::TargetObjectMissing;
    return result;
  }
  const TargetAttachmentSocketSet targetSockets =
      targetAttachmentSockets(*target, *request.assetCatalog);
  if (targetSockets.status == TargetAttachmentSocketStatus::AssetMissing) {
    result.status = CreativeAttachmentSnapStatus::TargetAssetMissing;
    return result;
  }
  if (targetSockets.status ==
      TargetAttachmentSocketStatus::InvalidGeometry) {
    result.status = CreativeAttachmentSnapStatus::InvalidRequest;
    return result;
  }
  for (std::size_t index = 0U; index < targetSockets.size(); ++index) {
    result.targetReceiverCount +=
        targetSockets.at(index).role ==
                StaticMeshAttachmentSocketRole::Receiver
            ? 1U
            : 0U;
  }
  if (result.targetReceiverCount == 0U) {
    result.status = CreativeAttachmentSnapStatus::TargetReceiverMissing;
    return result;
  }
  if (!isFiniteCreativeVec3(target->transform.position) ||
      !isFiniteCreativeVec3(target->transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(target->transform.scale)) {
    result.status = CreativeAttachmentSnapStatus::InvalidRequest;
    return result;
  }

  std::size_t aimedReceiverSetIndex = targetSockets.size();
  if (request.selectionMode ==
      CreativeAttachmentSnapSelectionMode::AimedSocket) {
    const double aimedRadiusSquared = request.aimedSocketRadiusMeters *
                                      request.aimedSocketRadiusMeters;
    bool hasAimedReceiver = false;
    double bestAimedDistanceSquared = 0.0;
    std::string_view bestAimedSocket;
    std::size_t receiverOrdinal = 0U;
    for (std::size_t receiverIndex = 0U;
         receiverIndex < targetSockets.size(); ++receiverIndex) {
      const AttachmentSocketView receiver = targetSockets.at(receiverIndex);
      if (receiver.role != StaticMeshAttachmentSocketRole::Receiver) {
        continue;
      }
      const WorldAttachmentSocketFrame frame =
          worldAttachmentSocketFrame(receiver, target->transform);
      const double distanceSquared =
          squaredLength(subtract(frame.position, request.aimPoint));
      if (frame.oriented && std::isfinite(distanceSquared) &&
          distanceSquared <= aimedRadiusSquared &&
          betterReceiverCandidate(distanceSquared, receiver.name,
                                  bestAimedDistanceSquared, bestAimedSocket,
                                  hasAimedReceiver)) {
        hasAimedReceiver = true;
        aimedReceiverSetIndex = receiverIndex;
        bestAimedDistanceSquared = distanceSquared;
        bestAimedSocket = receiver.name;
        result.selectedReceiverIndex = receiverOrdinal;
      }
      ++receiverOrdinal;
    }
    if (!hasAimedReceiver) {
      result.status = CreativeAttachmentSnapStatus::AimedSocketUnavailable;
      return result;
    }
  }

  const double maxDistanceSquared =
      request.maxDistanceMeters * request.maxDistanceMeters;
  bool compatibleInRange = false;
  bool hasBest = false;
  double bestDistanceSquared = 0.0;
  CreativeTransform bestTransform;
  std::string_view bestSourceSocket;
  std::string_view bestTargetSocket;
  std::string_view bestCompatibility;
  CreativeVec3 bestTargetForward;
  CreativeVec3 bestTargetUp;
  std::size_t bestReceiverIndex = 0U;
  bool hasOccupied = false;
  double occupiedDistanceSquared = 0.0;
  CreativeTransform occupiedTransform;
  std::string_view occupiedSourceSocket;
  std::string_view occupiedTargetSocket;
  std::string_view occupiedCompatibility;
  CreativeVec3 occupiedTargetForward;
  CreativeVec3 occupiedTargetUp;
  std::size_t occupiedReceiverIndex = 0U;

  std::size_t receiverOrdinal = 0U;
  for (std::size_t receiverIndex = 0U;
       receiverIndex < targetSockets.size(); ++receiverIndex) {
    const AttachmentSocketView receiver = targetSockets.at(receiverIndex);
    if (receiver.role != StaticMeshAttachmentSocketRole::Receiver) {
      continue;
    }
    const std::size_t currentReceiverOrdinal = receiverOrdinal++;
    if (request.selectionMode ==
            CreativeAttachmentSnapSelectionMode::AimedSocket &&
        receiverIndex != aimedReceiverSetIndex) {
      continue;
    }
    const WorldAttachmentSocketFrame targetFrame =
        worldAttachmentSocketFrame(receiver, target->transform);
    if (!targetFrame.oriented) {
      continue;
    }
    const CreativeVec3 aimDelta =
        subtract(targetFrame.position, request.aimPoint);
    const double distanceSquared = squaredLength(aimDelta);
    if (request.selectionMode ==
        CreativeAttachmentSnapSelectionMode::AimedSocket) {
      result.receiverSelected = true;
      result.targetObjectId = target->id;
      result.targetSocket = receiver.name;
      result.compatibility = receiver.compatibility;
      result.targetForward = targetFrame.forward;
      result.targetUp = targetFrame.up;
    }

    for (const StaticMeshAttachmentSocket& plug : source->attachmentSockets) {
      if (plug.role != StaticMeshAttachmentSocketRole::Plug ||
          plug.compatibility != receiver.compatibility) {
        continue;
      }
      ++result.compatiblePairCount;
      if (!std::isfinite(distanceSquared) ||
          distanceSquared > maxDistanceSquared) {
        continue;
      }
      compatibleInRange = true;
      const CreativeVec3 sourceForward = toCreative(plug.forward);
      const CreativeVec3 sourceUp = toCreative(plug.up);
      if (!horizontalFrame(sourceForward, sourceUp)) {
        continue;
      }
      const double yaw = normalizeRadians(
          yawForForward(targetFrame.forward) - yawForForward(sourceForward));
      const CreativeVec3 sourceSocketOffset =
          rotateCreativeVectorEulerXyz(
              multiply(toCreative(plug.position), request.sourceScale),
              {0.0, yaw, 0.0});
      CreativeTransform transform;
      transform.position = subtract(targetFrame.position, sourceSocketOffset);
      transform.rotationEulerRadians = {0.0, yaw, 0.0};
      transform.scale = request.sourceScale;
      if (!isFiniteCreativeVec3(transform.position) ||
          !std::isfinite(yaw)) {
        continue;
      }
      if (socketOccupied(*request.document, target->id, receiver.name,
                         request.ignoredOccupantObjectId)) {
        ++result.occupiedPairCount;
        if (betterCandidate(
                distanceSquared, receiver.name, plug.name,
                occupiedDistanceSquared,
                occupiedTargetSocket, occupiedSourceSocket,
                hasOccupied)) {
          hasOccupied = true;
          occupiedDistanceSquared = distanceSquared;
          occupiedTransform = transform;
          occupiedSourceSocket = plug.name;
          occupiedTargetSocket = receiver.name;
          occupiedCompatibility = receiver.compatibility;
          occupiedTargetForward = targetFrame.forward;
          occupiedTargetUp = targetFrame.up;
          occupiedReceiverIndex = currentReceiverOrdinal;
        }
        continue;
      }
      if (betterCandidate(distanceSquared, receiver.name, plug.name,
                          bestDistanceSquared,
                          bestTargetSocket, bestSourceSocket,
                          hasBest)) {
        hasBest = true;
        bestDistanceSquared = distanceSquared;
        bestTransform = transform;
        bestSourceSocket = plug.name;
        bestTargetSocket = receiver.name;
        bestCompatibility = receiver.compatibility;
        bestTargetForward = targetFrame.forward;
        bestTargetUp = targetFrame.up;
        bestReceiverIndex = currentReceiverOrdinal;
      }
    }
  }

  if (hasBest) {
    result.status = CreativeAttachmentSnapStatus::Ready;
    result.transform = bestTransform;
    result.targetObjectId = target->id;
    result.sourceSocket = bestSourceSocket;
    result.targetSocket = bestTargetSocket;
    result.compatibility = bestCompatibility;
    result.targetForward = bestTargetForward;
    result.targetUp = bestTargetUp;
    result.selectedReceiverIndex = bestReceiverIndex;
    result.receiverSelected = true;
    result.distanceMeters = std::sqrt(bestDistanceSquared);
    result.positioned = true;
    result.snapped = true;
    return result;
  }
  if (result.compatiblePairCount == 0U) {
    result.status = CreativeAttachmentSnapStatus::NoCompatibleSocket;
  } else if (!compatibleInRange) {
    result.status = CreativeAttachmentSnapStatus::OutsideRadius;
  } else if (hasOccupied) {
    result.status = CreativeAttachmentSnapStatus::Occupied;
    result.transform = occupiedTransform;
    result.targetObjectId = target->id;
    result.sourceSocket = occupiedSourceSocket;
    result.targetSocket = occupiedTargetSocket;
    result.compatibility = occupiedCompatibility;
    result.targetForward = occupiedTargetForward;
    result.targetUp = occupiedTargetUp;
    result.selectedReceiverIndex = occupiedReceiverIndex;
    result.receiverSelected = true;
    result.distanceMeters = std::sqrt(occupiedDistanceSquared);
    result.positioned = true;
  } else {
    result.status = CreativeAttachmentSnapStatus::InvalidRequest;
  }
  return result;
}

CreativeAttachmentSocketMarkerFrame buildCreativeAttachmentSocketMarkers(
    const CreativeAttachmentSocketMarkerRequest& request) noexcept {
  CreativeAttachmentSocketMarkerFrame frame;
  if (request.document == nullptr || request.assetCatalog == nullptr ||
      request.sourceAssetId.empty() ||
      request.targetObjectId == kInvalidObjectId ||
      !isFiniteCreativeVec3(request.aimPoint) ||
      !std::isfinite(request.maxDistanceMeters) ||
      request.maxDistanceMeters <= 0.0 ||
      !std::isfinite(request.aimedSocketRadiusMeters) ||
      request.aimedSocketRadiusMeters <= 0.0 ||
      request.selectionMode >= CreativeAttachmentSnapSelectionMode::Count) {
    frame.status = CreativeAttachmentSocketMarkerStatus::InvalidRequest;
    return frame;
  }

  const StaticMeshAssetCatalogEntry* source =
      request.assetCatalog->find(request.sourceAssetId);
  if (source == nullptr) {
    frame.status = CreativeAttachmentSocketMarkerStatus::SourceAssetMissing;
    return frame;
  }
  const bool hasPlug = std::any_of(
      source->attachmentSockets.begin(), source->attachmentSockets.end(),
      [](const StaticMeshAttachmentSocket& socket) {
        return socket.role == StaticMeshAttachmentSocketRole::Plug;
      });
  if (!hasPlug) {
    frame.status = CreativeAttachmentSocketMarkerStatus::SourcePlugMissing;
    return frame;
  }

  const CreativeObject* target =
      request.document->findObject(request.targetObjectId);
  if (target == nullptr) {
    frame.status = CreativeAttachmentSocketMarkerStatus::TargetObjectMissing;
    return frame;
  }
  const TargetAttachmentSocketSet targetSockets =
      targetAttachmentSockets(*target, *request.assetCatalog);
  if (targetSockets.status == TargetAttachmentSocketStatus::AssetMissing) {
    frame.status = CreativeAttachmentSocketMarkerStatus::TargetAssetMissing;
    return frame;
  }
  if (targetSockets.status ==
      TargetAttachmentSocketStatus::InvalidGeometry) {
    frame.status = CreativeAttachmentSocketMarkerStatus::InvalidRequest;
    return frame;
  }
  if (!isFiniteCreativeVec3(target->transform.position) ||
      !isFiniteCreativeVec3(target->transform.rotationEulerRadians) ||
      !isPositiveCreativeVec3(target->transform.scale)) {
    frame.status = CreativeAttachmentSocketMarkerStatus::InvalidRequest;
    return frame;
  }

  std::size_t aimedReceiverSetIndex = targetSockets.size();
  if (request.selectionMode ==
      CreativeAttachmentSnapSelectionMode::AimedSocket) {
    const double aimedRadiusSquared = request.aimedSocketRadiusMeters *
                                      request.aimedSocketRadiusMeters;
    bool hasAimedReceiver = false;
    double bestAimedDistanceSquared = 0.0;
    std::string_view bestAimedSocket;
    for (std::size_t receiverIndex = 0U;
         receiverIndex < targetSockets.size(); ++receiverIndex) {
      const AttachmentSocketView receiver = targetSockets.at(receiverIndex);
      if (receiver.role != StaticMeshAttachmentSocketRole::Receiver) {
        continue;
      }
      const WorldAttachmentSocketFrame candidate =
          worldAttachmentSocketFrame(receiver, target->transform);
      const double distanceSquared =
          squaredLength(subtract(candidate.position, request.aimPoint));
      if (candidate.oriented && std::isfinite(distanceSquared) &&
          distanceSquared <= aimedRadiusSquared &&
          betterReceiverCandidate(distanceSquared, receiver.name,
                                  bestAimedDistanceSquared, bestAimedSocket,
                                  hasAimedReceiver)) {
        hasAimedReceiver = true;
        aimedReceiverSetIndex = receiverIndex;
        bestAimedDistanceSquared = distanceSquared;
        bestAimedSocket = receiver.name;
      }
    }
  }

  for (std::size_t receiverIndex = 0U;
       receiverIndex < targetSockets.size(); ++receiverIndex) {
    const AttachmentSocketView receiver = targetSockets.at(receiverIndex);
    if (receiver.role != StaticMeshAttachmentSocketRole::Receiver) {
      continue;
    }
    ++frame.receiverCount;
    const WorldAttachmentSocketFrame targetFrame =
        worldAttachmentSocketFrame(receiver, target->transform);
    if (!targetFrame.positioned) {
      ++frame.skippedCount;
      continue;
    }

    bool compatible = false;
    if (targetFrame.oriented) {
      compatible = std::any_of(
          source->attachmentSockets.begin(), source->attachmentSockets.end(),
          [&receiver](const StaticMeshAttachmentSocket& plug) {
            return plug.role == StaticMeshAttachmentSocketRole::Plug &&
                   plug.compatibility == receiver.compatibility &&
                   horizontalFrame(toCreative(plug.forward),
                                   toCreative(plug.up));
          });
    }
    const CreativeVec3 aimDelta =
        subtract(targetFrame.position, request.aimPoint);
    const double distanceSquared = squaredLength(aimDelta);
    const double maxDistanceSquared =
        request.maxDistanceMeters * request.maxDistanceMeters;
    const bool inRange = std::isfinite(distanceSquared) &&
                         std::isfinite(maxDistanceSquared) &&
                         distanceSquared <= maxDistanceSquared;

    if (frame.markerCount == frame.markers.size()) {
      frame.truncated = true;
      ++frame.skippedCount;
      continue;
    }
    CreativeAttachmentSocketMarker& marker =
        frame.markers[frame.markerCount++];
    marker.worldPosition = targetFrame.position;
    marker.worldForward = targetFrame.forward;
    marker.worldUp = targetFrame.up;
    marker.targetObjectId = target->id;
    marker.targetSocket = receiver.name;
    marker.compatibility = receiver.compatibility;
    marker.state = CreativeAttachmentSocketMarkerState::Incompatible;
    marker.selected = receiverIndex == aimedReceiverSetIndex;
    if (compatible) {
      marker.state =
          !inRange
              ? CreativeAttachmentSocketMarkerState::OutOfRange
              : socketOccupied(*request.document, target->id, receiver.name,
                               request.ignoredOccupantObjectId)
                    ? CreativeAttachmentSocketMarkerState::Occupied
                    : CreativeAttachmentSocketMarkerState::Available;
    }
  }

  if (frame.receiverCount == 0U) {
    frame.status =
        CreativeAttachmentSocketMarkerStatus::TargetReceiverMissing;
    return frame;
  }
  frame.status = CreativeAttachmentSocketMarkerStatus::Built;
  frame.accepted = true;
  return frame;
}

}  // namespace iggy3d::creative
