#include "app/iggy3d/creative/tools/AttachmentSnap.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "app/iggy3d/creative/Geometry.hpp"

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

[[nodiscard]] double normalizeRadians(double radians) noexcept {
  constexpr double kTwoPi = 2.0 * std::numbers::pi;
  radians = std::remainder(radians, kTwoPi);
  return radians == -std::numbers::pi ? std::numbers::pi : radians;
}

[[nodiscard]] bool socketOccupied(const CreativeDocument& document,
                                  CreativeObjectId parentId,
                                  std::string_view socket) noexcept {
  return std::any_of(
      document.objects().begin(), document.objects().end(),
      [parentId, socket](const CreativeObject& object) {
        return object.parentId == parentId && object.attachmentSocket == socket;
      });
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

CreativeAttachmentSnapResult resolveCreativeAttachmentSnap(
    const CreativeAttachmentSnapRequest& request) noexcept {
  CreativeAttachmentSnapResult result;
  if (request.document == nullptr || request.assetCatalog == nullptr ||
      request.sourceAssetId.empty() ||
      request.targetObjectId == kInvalidObjectId ||
      !isFiniteCreativeVec3(request.aimPoint) ||
      !isPositiveCreativeVec3(request.sourceScale) ||
      !std::isfinite(request.maxDistanceMeters) ||
      request.maxDistanceMeters <= 0.0) {
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
  const StaticMeshAssetCatalogEntry* targetAsset =
      request.assetCatalog->find(target->assetId);
  if (targetAsset == nullptr) {
    result.status = CreativeAttachmentSnapStatus::TargetAssetMissing;
    return result;
  }
  result.targetReceiverCount = static_cast<std::size_t>(std::count_if(
      targetAsset->attachmentSockets.begin(),
      targetAsset->attachmentSockets.end(),
      [](const StaticMeshAttachmentSocket& socket) {
        return socket.role == StaticMeshAttachmentSocketRole::Receiver;
      }));
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

  const double maxDistanceSquared =
      request.maxDistanceMeters * request.maxDistanceMeters;
  bool compatibleInRange = false;
  bool hasBest = false;
  double bestDistanceSquared = 0.0;
  CreativeTransform bestTransform;
  const StaticMeshAttachmentSocket* bestSource = nullptr;
  const StaticMeshAttachmentSocket* bestTarget = nullptr;
  bool hasOccupied = false;
  double occupiedDistanceSquared = 0.0;
  CreativeTransform occupiedTransform;
  const StaticMeshAttachmentSocket* occupiedSource = nullptr;
  const StaticMeshAttachmentSocket* occupiedTarget = nullptr;

  for (const StaticMeshAttachmentSocket& receiver :
       targetAsset->attachmentSockets) {
    if (receiver.role != StaticMeshAttachmentSocketRole::Receiver) {
      continue;
    }
    const CreativeVec3 targetLocalPosition = multiply(
        toCreative(receiver.position), target->transform.scale);
    const CreativeVec3 targetWorldPosition = add(
        target->transform.position,
        rotateCreativeVectorEulerXyz(
            targetLocalPosition, target->transform.rotationEulerRadians));
    const CreativeVec3 targetWorldForward = rotateCreativeVectorEulerXyz(
        toCreative(receiver.forward), target->transform.rotationEulerRadians);
    const CreativeVec3 targetWorldUp = rotateCreativeVectorEulerXyz(
        toCreative(receiver.up), target->transform.rotationEulerRadians);
    if (!horizontalFrame(targetWorldForward, targetWorldUp) ||
        !isFiniteCreativeVec3(targetWorldPosition)) {
      continue;
    }
    const CreativeVec3 aimDelta =
        subtract(targetWorldPosition, request.aimPoint);
    const double distanceSquared = squaredLength(aimDelta);

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
          yawForForward(targetWorldForward) - yawForForward(sourceForward));
      const CreativeVec3 sourceSocketOffset =
          rotateCreativeVectorEulerXyz(
              multiply(toCreative(plug.position), request.sourceScale),
              {0.0, yaw, 0.0});
      CreativeTransform transform;
      transform.position = subtract(targetWorldPosition, sourceSocketOffset);
      transform.rotationEulerRadians = {0.0, yaw, 0.0};
      transform.scale = request.sourceScale;
      if (!isFiniteCreativeVec3(transform.position) ||
          !std::isfinite(yaw)) {
        continue;
      }
      if (socketOccupied(*request.document, target->id, receiver.name)) {
        ++result.occupiedPairCount;
        if (betterCandidate(
                distanceSquared, receiver.name, plug.name,
                occupiedDistanceSquared,
                occupiedTarget == nullptr ? std::string_view{}
                                          : occupiedTarget->name,
                occupiedSource == nullptr ? std::string_view{}
                                          : occupiedSource->name,
                hasOccupied)) {
          hasOccupied = true;
          occupiedDistanceSquared = distanceSquared;
          occupiedTransform = transform;
          occupiedSource = &plug;
          occupiedTarget = &receiver;
        }
        continue;
      }
      if (betterCandidate(distanceSquared, receiver.name, plug.name,
                          bestDistanceSquared,
                          bestTarget == nullptr ? std::string_view{}
                                                : bestTarget->name,
                          bestSource == nullptr ? std::string_view{}
                                                : bestSource->name,
                          hasBest)) {
        hasBest = true;
        bestDistanceSquared = distanceSquared;
        bestTransform = transform;
        bestSource = &plug;
        bestTarget = &receiver;
      }
    }
  }

  if (hasBest) {
    result.status = CreativeAttachmentSnapStatus::Ready;
    result.transform = bestTransform;
    result.targetObjectId = target->id;
    result.sourceSocket = bestSource->name;
    result.targetSocket = bestTarget->name;
    result.compatibility = bestTarget->compatibility;
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
    result.sourceSocket = occupiedSource->name;
    result.targetSocket = occupiedTarget->name;
    result.compatibility = occupiedTarget->compatibility;
    result.distanceMeters = std::sqrt(occupiedDistanceSquared);
    result.positioned = true;
  } else {
    result.status = CreativeAttachmentSnapStatus::InvalidRequest;
  }
  return result;
}

}  // namespace iggy3d::creative
