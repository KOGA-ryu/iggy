

#include "app/iggy3d/creative/mutation/MutationApply.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace iggy3d::creative {
namespace {

[[nodiscard]] CreativeMutationApplyReceipt makeNoChangeReceipt(
    const CreativeObject& object,
    CreativeMutationKind mutationKind,
    std::string message) {
    return makeMutationApplyReceipt(
        CreativeMutationApplyStatus::NoChange,
        object,
        mutationKind,
        0,
        false,
        true,
        std::move(message));
}

[[nodiscard]] CreativeMutationApplyReceipt makeAppliedReceipt(
    const CreativeObject& object,
    CreativeMutationKind mutationKind,
    std::string message) {
    return makeMutationApplyReceipt(
        CreativeMutationApplyStatus::Applied,
        object,
        mutationKind,
        dirtyFlagsForMutation(object.kind, mutationKind),
        true,
        true,
        std::move(message));
}

[[nodiscard]] CreativeMutationApplyReceipt makeFutureStorageNoChangeReceipt(
    const CreativeObject& object,
    CreativeMutationKind mutationKind) {
    return makeNoChangeReceipt(object, mutationKind, "mutation has no stored object field yet");
}

[[nodiscard]] bool sameVec3(CreativeVec3 lhs, CreativeVec3 rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool sameTransform(const CreativeTransform& lhs, const CreativeTransform& rhs) noexcept {
    return sameVec3(lhs.position, rhs.position) && sameVec3(lhs.rotation, rhs.rotation) && sameVec3(lhs.scale, rhs.scale);
}

[[nodiscard]] bool sameBounds(const CreativeBounds& lhs, const CreativeBounds& rhs) noexcept {
    return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

[[nodiscard]] bool samePathPoints(const std::vector<CreativePathPoint>& lhs, const std::vector<CreativePathPoint>& rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (!sameVec3(lhs[index].position, rhs[index].position)) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool finiteVec3(CreativeVec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool validPathPoints(const std::vector<CreativePathPoint>& pathPoints) noexcept {
    if (pathPoints.size() < 2U) {
        return false;
    }

    return std::all_of(pathPoints.begin(), pathPoints.end(), [](const CreativePathPoint& point) {
        return finiteVec3(point.position);
    });
}

[[nodiscard]] bool validLineEndpoints(const std::vector<CreativePathPoint>& pathPoints) noexcept {
    return pathPoints.size() == 2U &&
           std::all_of(pathPoints.begin(), pathPoints.end(), [](const CreativePathPoint& point) {
               return finiteVec3(point.position);
           });
}

[[nodiscard]] bool objectStoresLineEndpoints(const CreativeObjectDescriptor& descriptor) noexcept {
    return descriptor.shapeKind == CreativeObjectShapeKind::Line &&
           descriptor.projectionProfile == CreativeSpatialProjectionProfile::LinkProjection &&
           !descriptor.hasBounds;
}

[[nodiscard]] CreativeVec3 boundsSize(const CreativeBounds& bounds) noexcept {
    return CreativeVec3{
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z,
    };
}

[[nodiscard]] CreativeBounds resizeBoundsFromMin(const CreativeBounds& bounds, CreativeVec3 size) noexcept {
    return CreativeBounds{
        bounds.min,
        CreativeVec3{
            bounds.min.x + size.x,
            bounds.min.y + size.y,
            bounds.min.z + size.z,
        },
    };
}

[[nodiscard]] CreativeBounds stretchBoundsMax(const CreativeBounds& bounds, CreativeVec3 delta) noexcept {
    return CreativeBounds{
        bounds.min,
        CreativeVec3{
            bounds.max.x + delta.x,
            bounds.max.y + delta.y,
            bounds.max.z + delta.z,
        },
    };
}

[[nodiscard]] CreativeBounds translateBounds(const CreativeBounds& bounds, CreativeVec3 delta) noexcept {
    return CreativeBounds{
        CreativeVec3{
            bounds.min.x + delta.x,
            bounds.min.y + delta.y,
            bounds.min.z + delta.z,
        },
        CreativeVec3{
            bounds.max.x + delta.x,
            bounds.max.y + delta.y,
            bounds.max.z + delta.z,
        },
    };
}

[[nodiscard]] bool hasTag(const CreativeObject& object, const std::string& tag) {
    return std::find(object.tags.begin(), object.tags.end(), tag) != object.tags.end();
}

[[nodiscard]] CreativeMutationApplyReceipt applyRotateMutationAs(
    CreativeObject& object,
    CreativeMutationKind mutationKind,
    const RotateMutation& mutation) {
    if (sameVec3(object.transform.rotation, mutation.rotation)) {
        return makeNoChangeReceipt(object, mutationKind, "object rotation already matches requested value");
    }

    object.transform.rotation = mutation.rotation;
    return makeAppliedReceipt(object, mutationKind, "object rotated");
}

[[nodiscard]] CreativeMutationApplyReceipt applyResizeMutationAs(
    CreativeObject& object,
    CreativeMutationKind mutationKind,
    CreativeVec3 size) {
    if (sameVec3(boundsSize(object.bounds), size)) {
        return makeNoChangeReceipt(object, mutationKind, "object bounds size already matches requested value");
    }

    object.bounds = resizeBoundsFromMin(object.bounds, size);
    return makeAppliedReceipt(object, mutationKind, "object resized");
}

[[nodiscard]] CreativeMutationApplyReceipt applySetBoundsMutationAs(
    CreativeObject& object,
    CreativeMutationKind mutationKind,
    const SetBoundsMutation& mutation) {
    if (sameBounds(object.bounds, mutation.bounds)) {
        return makeNoChangeReceipt(object, mutationKind, "object bounds already match requested bounds");
    }

    object.bounds = mutation.bounds;
    return makeAppliedReceipt(object, mutationKind, "object bounds changed");
}

[[nodiscard]] CreativeMutationApplyReceipt applyPayloadToObject(
    CreativeObject& object,
    CreativeMutationKind mutationKind,
    const CreativeMutationPayload& payload) {
    const auto& value = payload.value;

    switch (mutationKind) {
    case CreativeMutationKind::Unknown:
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::InvalidMutation, "unknown mutation cannot be applied");

    case CreativeMutationKind::Rename:
        return applyRenameMutation(object, std::get<RenameMutation>(value));
    case CreativeMutationKind::SetVisible:
        return applyVisibilityMutation(object, std::get<VisibilityMutation>(value));
    case CreativeMutationKind::SetLocked:
        return applyLockMutation(object, std::get<LockMutation>(value));

    case CreativeMutationKind::Move:
        return applyMoveMutation(object, std::get<MoveMutation>(value));
    case CreativeMutationKind::Rotate:
        return applyRotateMutation(object, std::get<RotateMutation>(value));
    case CreativeMutationKind::SetSpawnFacing:
        return applyRotateMutationAs(object, mutationKind, std::get<RotateMutation>(value));
    case CreativeMutationKind::Scale:
        return applyScaleMutation(object, std::get<ScaleMutation>(value));
    case CreativeMutationKind::SetTransform:
        return applySetTransformMutation(object, std::get<SetTransformMutation>(value));

    case CreativeMutationKind::Resize:
        return applyResizeMutation(object, std::get<ResizeMutation>(value));
    case CreativeMutationKind::Stretch:
        return applyStretchMutation(object, std::get<StretchMutation>(value));
    case CreativeMutationKind::SetBounds:
        return applySetBoundsMutation(object, std::get<SetBoundsMutation>(value));
    case CreativeMutationKind::SetTriggerShape:
        return applySetBoundsMutationAs(object, mutationKind, std::get<SetBoundsMutation>(value));

    case CreativeMutationKind::SetHeight:
    case CreativeMutationKind::SetRadius:
    case CreativeMutationKind::SetThickness:
    case CreativeMutationKind::SetLength:
    case CreativeMutationKind::SetWidth:
    case CreativeMutationKind::SetDepth:
    case CreativeMutationKind::SetBlueprintOpacity:
    case CreativeMutationKind::SetLightIntensity:
    case CreativeMutationKind::SetLightRadius:
    case CreativeMutationKind::SetLightConeAngle:
    case CreativeMutationKind::SetAudioRadius:
    case CreativeMutationKind::SetNavCost:
    case CreativeMutationKind::SetDistanceValue:
    case CreativeMutationKind::SetSpeedValue:
    case CreativeMutationKind::SetTimingWindow:
    case CreativeMutationKind::SetDangerLevel:
        return applyScalarMutation(object, mutationKind, std::get<ScalarMutation>(value));

    case CreativeMutationKind::SetParent:
        return applySetParentMutation(object, std::get<SetParentMutation>(value));
    case CreativeMutationKind::ClearParent:
        return applyClearParentMutation(object, mutationKind);
    case CreativeMutationKind::AttachTo:
        return applyAttachMutation(object, mutationKind, std::get<AttachToMutation>(value));
    case CreativeMutationKind::DetachFrom:
        return applyClearParentMutation(object, mutationKind);
    case CreativeMutationKind::LinkTarget:
    case CreativeMutationKind::SetEventRelayTarget:
    case CreativeMutationKind::SetCameraTarget:
    case CreativeMutationKind::SetCameraRail:
        return applyLinkMutation(object, mutationKind, std::get<LinkTargetMutation>(value));
    case CreativeMutationKind::UnlinkTarget:
        return makeFutureStorageNoChangeReceipt(object, mutationKind);
    case CreativeMutationKind::SetSocket:
        return applySocketMutation(object, mutationKind, std::get<SetSocketMutation>(value));
    case CreativeMutationKind::ClearSocket:
        return makeFutureStorageNoChangeReceipt(object, mutationKind);

    case CreativeMutationKind::AssignLayer:
        return applyAssignLayerMutation(object, std::get<AssignLayerMutation>(value));
    case CreativeMutationKind::AddTag:
        return applyAddTagMutation(object, std::get<TagMutation>(value));
    case CreativeMutationKind::RemoveTag:
        return applyRemoveTagMutation(object, std::get<TagMutation>(value));
    case CreativeMutationKind::ClearTags:
        return applyClearTagsMutation(object, mutationKind);

    case CreativeMutationKind::EditText:
    case CreativeMutationKind::SetLabel:
    case CreativeMutationKind::SetNotes:
    case CreativeMutationKind::SetTriggerEvent:
    case CreativeMutationKind::SetCondition:
    case CreativeMutationKind::SetSpawnerProfile:
    case CreativeMutationKind::SetDespawnRule:
    case CreativeMutationKind::SetCheckpointId:
    case CreativeMutationKind::SetTestLaneKind:
    case CreativeMutationKind::SetProbeKind:
    case CreativeMutationKind::SetExpectedResult:
    case CreativeMutationKind::SetEnemyProfile:
    case CreativeMutationKind::SetNpcProfile:
    case CreativeMutationKind::SetResourceKind:
    case CreativeMutationKind::SetLootTable:
    case CreativeMutationKind::SetQuestId:
    case CreativeMutationKind::SetDialogueId:
    case CreativeMutationKind::SetSafeZoneRule:
    case CreativeMutationKind::SetJumpArc:
    case CreativeMutationKind::SetClimbRule:
    case CreativeMutationKind::SetWallRunRule:
    case CreativeMutationKind::SetSlideRule:
    case CreativeMutationKind::SetMusicCue:
        if (std::holds_alternative<TextMutation>(value)) {
            return applyTextMutation(object, mutationKind, std::get<TextMutation>(value));
        }
        return applyStringIdMutation(object, mutationKind, std::get<StringIdMutation>(value));

    case CreativeMutationKind::SetPatrolRoute:
        if (std::holds_alternative<PathPointsMutation>(value)) {
            return applyPathPointsMutation(object, mutationKind, std::get<PathPointsMutation>(value));
        }

        if (std::holds_alternative<TextMutation>(value)) {
            return applyTextMutation(object, mutationKind, std::get<TextMutation>(value));
        }

        return applyStringIdMutation(object, mutationKind, std::get<StringIdMutation>(value));

    case CreativeMutationKind::SetReferenceSource:
        return applyReferenceSourceMutation(object, mutationKind, std::get<ReferenceSourceMutation>(value));
    case CreativeMutationKind::SetLightColor:
        return applyColorMutation(object, mutationKind, std::get<ColorMutation>(value));
    case CreativeMutationKind::SetAudioSource:
        return applyAudioSourceMutation(object, mutationKind, std::get<AudioSourceMutation>(value));
    }

    return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::InvalidMutation, "mutation dispatch fell through");
}

} // namespace

std::string_view toString(CreativeMutationApplyStatus status) noexcept {
    switch (status) {
    case CreativeMutationApplyStatus::Unknown: return "Unknown";
    case CreativeMutationApplyStatus::Applied: return "Applied";
    case CreativeMutationApplyStatus::NoChange: return "NoChange";
    case CreativeMutationApplyStatus::Rejected: return "Rejected";
    case CreativeMutationApplyStatus::MissingPayload: return "MissingPayload";
    case CreativeMutationApplyStatus::WrongPayload: return "WrongPayload";
    case CreativeMutationApplyStatus::UnsupportedMutation: return "UnsupportedMutation";
    case CreativeMutationApplyStatus::LockedObject: return "LockedObject";
    case CreativeMutationApplyStatus::InvalidObject: return "InvalidObject";
    case CreativeMutationApplyStatus::InvalidMutation: return "InvalidMutation";
    }

    return "Unknown";
}

bool mutationApplySucceeded(CreativeMutationApplyStatus status) noexcept {
    return status == CreativeMutationApplyStatus::Applied || status == CreativeMutationApplyStatus::NoChange;
}

bool mutationApplyFailed(CreativeMutationApplyStatus status) noexcept {
    return !mutationApplySucceeded(status);
}

bool mutationApplyChanged(CreativeMutationApplyStatus status) noexcept {
    return status == CreativeMutationApplyStatus::Applied;
}

CreativeMutationApplyReceipt makeMutationApplyReceipt(
    CreativeMutationApplyStatus status,
    const CreativeObject& object,
    CreativeMutationKind mutationKind,
    CreativeObjectDirtyFlags dirtyFlags,
    bool changed,
    bool allowed,
    std::string message) {
    return CreativeMutationApplyReceipt{
        status,
        object.id,
        object.kind,
        mutationKind,
        dirtyFlags,
        changed,
        allowed,
        std::move(message),
    };
}

CreativeMutationApplyReceipt rejectMutation(
    const CreativeObject& object,
    CreativeMutationKind mutationKind,
    CreativeMutationApplyStatus status,
    std::string message) {
    return makeMutationApplyReceipt(status, object, mutationKind, 0, false, false, std::move(message));
}

CreativeMutationApplyReceipt applyMutation(
    CreativeObject& object,
    const CreativeMutationRequest& request,
    const CreativeMutationApplyOptions& options) {
    if (request.objectId != 0 && object.id != request.objectId) {
        return rejectMutation(object, request.kind, CreativeMutationApplyStatus::InvalidObject, "mutation request object id does not match target object");
    }

    return applyMutation(object, request.kind, request.payload, options);
}

CreativeMutationApplyReceipt applyMutation(
    CreativeObject& object,
    CreativeMutationKind mutationKind,
    const CreativeMutationPayload& payload,
    const CreativeMutationApplyOptions& options) {
    if (object.kind == CreativeObjectKind::Unknown || object.id == 0) {
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::InvalidObject, "cannot mutate invalid object");
    }

    if (mutationKind == CreativeMutationKind::Unknown) {
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::InvalidMutation, "cannot apply unknown mutation");
    }

    if (options.rejectLockedObjects && object.locked && mutationKind != CreativeMutationKind::SetLocked) {
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::LockedObject, "object is locked");
    }

    if (options.validateDescriptorRules && !descriptorAllowsMutation(object.kind, mutationKind)) {
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::UnsupportedMutation, "object kind does not allow this mutation");
    }

    if (options.validatePayloadShape) {
        if (requiresPayload(mutationKind) && std::holds_alternative<std::monostate>(payload.value)) {
            return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::MissingPayload, "mutation requires payload");
        }

        if (!payloadMatchesMutation(mutationKind, payload)) {
            return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::WrongPayload, "mutation payload does not match mutation kind");
        }
    }

    auto receipt = applyPayloadToObject(object, mutationKind, payload);
    if (!options.allowNoChange && receipt.status == CreativeMutationApplyStatus::NoChange) {
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::Rejected, "no-change mutation rejected by apply options");
    }

    return receipt;
}

CreativeMutationApplyReceipt applyRenameMutation(CreativeObject& object, const RenameMutation& mutation) {
    if (object.name == mutation.name) {
        return makeNoChangeReceipt(object, CreativeMutationKind::Rename, "object name already matches requested name");
    }

    object.name = mutation.name;
    return makeAppliedReceipt(object, CreativeMutationKind::Rename, "object renamed");
}

CreativeMutationApplyReceipt applyVisibilityMutation(CreativeObject& object, const VisibilityMutation& mutation) {
    if (object.visible == mutation.visible) {
        return makeNoChangeReceipt(object, CreativeMutationKind::SetVisible, "object visibility already matches requested value");
    }

    object.visible = mutation.visible;
    return makeAppliedReceipt(object, CreativeMutationKind::SetVisible, "object visibility changed");
}

CreativeMutationApplyReceipt applyLockMutation(CreativeObject& object, const LockMutation& mutation) {
    if (object.locked == mutation.locked) {
        return makeNoChangeReceipt(object, CreativeMutationKind::SetLocked, "object lock state already matches requested value");
    }

    object.locked = mutation.locked;
    return makeAppliedReceipt(object, CreativeMutationKind::SetLocked, "object lock state changed");
}

CreativeMutationApplyReceipt applyMoveMutation(CreativeObject& object, const MoveMutation& mutation) {
    // TD-2: kinds without a transform but with bounds (Room) move by corner
    // anchor — bounds.min lands on the requested position and the transform
    // stays untouched.
    if (!objectHasTransform(object.kind) && objectHasBounds(object.kind)) {
        if (sameVec3(object.bounds.min, mutation.position)) {
            return makeNoChangeReceipt(object, CreativeMutationKind::Move, "object position already matches requested value");
        }

        const CreativeVec3 cornerDelta{
            mutation.position.x - object.bounds.min.x,
            mutation.position.y - object.bounds.min.y,
            mutation.position.z - object.bounds.min.z,
        };

        object.bounds = translateBounds(object.bounds, cornerDelta);
        return makeAppliedReceipt(object, CreativeMutationKind::Move, "object moved");
    }

    if (sameVec3(object.transform.position, mutation.position)) {
        return makeNoChangeReceipt(object, CreativeMutationKind::Move, "object position already matches requested value");
    }

    const CreativeVec3 oldPosition = object.transform.position;
    const CreativeVec3 delta{
        mutation.position.x - oldPosition.x,
        mutation.position.y - oldPosition.y,
        mutation.position.z - oldPosition.z,
    };

    object.transform.position = mutation.position;
    if (objectHasBounds(object.kind)) {
        object.bounds = translateBounds(object.bounds, delta);
    }
    return makeAppliedReceipt(object, CreativeMutationKind::Move, "object moved");
}

CreativeMutationApplyReceipt applyRotateMutation(CreativeObject& object, const RotateMutation& mutation) {
    return applyRotateMutationAs(object, CreativeMutationKind::Rotate, mutation);
}

CreativeMutationApplyReceipt applyScaleMutation(CreativeObject& object, const ScaleMutation& mutation) {
    if (sameVec3(object.transform.scale, mutation.scale)) {
        return makeNoChangeReceipt(object, CreativeMutationKind::Scale, "object scale already matches requested value");
    }

    object.transform.scale = mutation.scale;
    return makeAppliedReceipt(object, CreativeMutationKind::Scale, "object scaled");
}

CreativeMutationApplyReceipt applySetTransformMutation(CreativeObject& object, const SetTransformMutation& mutation) {
    if (sameTransform(object.transform, mutation.transform)) {
        return makeNoChangeReceipt(object, CreativeMutationKind::SetTransform, "object transform already matches requested value");
    }

    object.transform = mutation.transform;
    return makeAppliedReceipt(object, CreativeMutationKind::SetTransform, "object transform changed");
}

CreativeMutationApplyReceipt applyResizeMutation(CreativeObject& object, const ResizeMutation& mutation) {
    return applyResizeMutationAs(object, CreativeMutationKind::Resize, mutation.size);
}

CreativeMutationApplyReceipt applyStretchMutation(CreativeObject& object, const StretchMutation& mutation) {
    if (sameVec3(mutation.delta, CreativeVec3{})) {
        return makeNoChangeReceipt(object, CreativeMutationKind::Stretch, "stretch delta is zero");
    }

    object.bounds = stretchBoundsMax(object.bounds, mutation.delta);
    return makeAppliedReceipt(object, CreativeMutationKind::Stretch, "object bounds stretched");
}

CreativeMutationApplyReceipt applySetBoundsMutation(CreativeObject& object, const SetBoundsMutation& mutation) {
    return applySetBoundsMutationAs(object, CreativeMutationKind::SetBounds, mutation);
}

CreativeMutationApplyReceipt applyScalarMutation(CreativeObject& object, CreativeMutationKind mutationKind, const ScalarMutation& mutation) {
    switch (mutationKind) {
    case CreativeMutationKind::SetHeight: {
        const auto size = boundsSize(object.bounds);
        return applyResizeMutationAs(object, mutationKind, CreativeVec3{size.x, mutation.value, size.z});
    }
    case CreativeMutationKind::SetLength: {
        const auto size = boundsSize(object.bounds);
        return applyResizeMutationAs(object, mutationKind, CreativeVec3{mutation.value, size.y, size.z});
    }
    case CreativeMutationKind::SetWidth: {
        const auto size = boundsSize(object.bounds);
        return applyResizeMutationAs(object, mutationKind, CreativeVec3{mutation.value, size.y, size.z});
    }
    case CreativeMutationKind::SetDepth: {
        const auto size = boundsSize(object.bounds);
        return applyResizeMutationAs(object, mutationKind, CreativeVec3{size.x, size.y, mutation.value});
    }
    default:
        return makeFutureStorageNoChangeReceipt(object, mutationKind);
    }
}

CreativeMutationApplyReceipt applySetParentMutation(CreativeObject& object, const SetParentMutation& mutation) {
    if (object.parentId.has_value() && object.parentId.value() == mutation.parentId) {
        return makeNoChangeReceipt(object, CreativeMutationKind::SetParent, "object parent already matches requested parent");
    }

    object.parentId = mutation.parentId;
    return makeAppliedReceipt(object, CreativeMutationKind::SetParent, "object parent changed");
}

CreativeMutationApplyReceipt applyClearParentMutation(CreativeObject& object, CreativeMutationKind mutationKind) {
    if (!object.parentId.has_value()) {
        return makeNoChangeReceipt(object, mutationKind, "object has no parent to clear");
    }

    object.parentId.reset();
    return makeAppliedReceipt(object, mutationKind, "object parent cleared");
}

CreativeMutationApplyReceipt applyAssignLayerMutation(CreativeObject& object, const AssignLayerMutation& mutation) {
    if (object.layerId == mutation.layerId) {
        return makeNoChangeReceipt(object, CreativeMutationKind::AssignLayer, "object layer already matches requested layer");
    }

    object.layerId = mutation.layerId;
    return makeAppliedReceipt(object, CreativeMutationKind::AssignLayer, "object layer changed");
}

CreativeMutationApplyReceipt applyAddTagMutation(CreativeObject& object, const TagMutation& mutation) {
    if (hasTag(object, mutation.tag)) {
        return makeNoChangeReceipt(object, CreativeMutationKind::AddTag, "object already has requested tag");
    }

    object.tags.push_back(mutation.tag);
    return makeAppliedReceipt(object, CreativeMutationKind::AddTag, "object tag added");
}

CreativeMutationApplyReceipt applyRemoveTagMutation(CreativeObject& object, const TagMutation& mutation) {
    const auto originalSize = object.tags.size();
    object.tags.erase(std::remove(object.tags.begin(), object.tags.end(), mutation.tag), object.tags.end());

    if (object.tags.size() == originalSize) {
        return makeNoChangeReceipt(object, CreativeMutationKind::RemoveTag, "object did not have requested tag");
    }

    return makeAppliedReceipt(object, CreativeMutationKind::RemoveTag, "object tag removed");
}

CreativeMutationApplyReceipt applyClearTagsMutation(CreativeObject& object, CreativeMutationKind mutationKind) {
    if (object.tags.empty()) {
        return makeNoChangeReceipt(object, mutationKind, "object has no tags to clear");
    }

    object.tags.clear();
    return makeAppliedReceipt(object, mutationKind, "object tags cleared");
}

CreativeMutationApplyReceipt applyAttachMutation(CreativeObject& object, CreativeMutationKind mutationKind, const AttachToMutation& mutation) {
    if (object.parentId.has_value() && object.parentId.value() == mutation.targetId) {
        return makeNoChangeReceipt(object, mutationKind, "object parent already matches requested parent");
    }

    object.parentId = mutation.targetId;
    return makeAppliedReceipt(object, mutationKind, "object parent changed");
}

CreativeMutationApplyReceipt applyLinkMutation(CreativeObject& object, CreativeMutationKind mutationKind, const LinkTargetMutation& mutation) {
    (void)mutation;
    return makeFutureStorageNoChangeReceipt(object, mutationKind);
}

CreativeMutationApplyReceipt applySocketMutation(CreativeObject& object, CreativeMutationKind mutationKind, const SetSocketMutation& mutation) {
    (void)mutation;
    return makeFutureStorageNoChangeReceipt(object, mutationKind);
}

CreativeMutationApplyReceipt applyTextMutation(CreativeObject& object, CreativeMutationKind mutationKind, const TextMutation& mutation) {
    (void)mutation;
    return makeFutureStorageNoChangeReceipt(object, mutationKind);
}

CreativeMutationApplyReceipt applyPathPointsMutation(CreativeObject& object, CreativeMutationKind mutationKind, const PathPointsMutation& mutation) {
    const CreativeObjectDescriptor& descriptor = describeObject(object.kind);
    const bool storesPath = descriptor.shapeKind == CreativeObjectShapeKind::Path;
    const bool storesLineEndpoints = objectStoresLineEndpoints(descriptor);
    if (!storesPath && !storesLineEndpoints) {
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::UnsupportedMutation, "object kind does not store path points");
    }

    if (storesLineEndpoints && !validLineEndpoints(mutation.pathPoints)) {
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::Rejected, "line endpoints are invalid");
    }

    if (storesPath && !validPathPoints(mutation.pathPoints)) {
        return rejectMutation(object, mutationKind, CreativeMutationApplyStatus::Rejected, "path points are invalid");
    }

    if (samePathPoints(object.pathPoints, mutation.pathPoints)) {
        return makeNoChangeReceipt(
            object,
            mutationKind,
            storesLineEndpoints ? "object line endpoints already match requested endpoints"
                                : "object path points already match requested path");
    }

    object.pathPoints = mutation.pathPoints;
    return makeAppliedReceipt(
        object,
        mutationKind,
        storesLineEndpoints ? "object line endpoints changed" : "object path points changed");
}

CreativeMutationApplyReceipt applyReferenceSourceMutation(CreativeObject& object, CreativeMutationKind mutationKind, const ReferenceSourceMutation& mutation) {
    (void)mutation;
    return makeFutureStorageNoChangeReceipt(object, mutationKind);
}

CreativeMutationApplyReceipt applyColorMutation(CreativeObject& object, CreativeMutationKind mutationKind, const ColorMutation& mutation) {
    (void)mutation;
    return makeFutureStorageNoChangeReceipt(object, mutationKind);
}

CreativeMutationApplyReceipt applyAudioSourceMutation(CreativeObject& object, CreativeMutationKind mutationKind, const AudioSourceMutation& mutation) {
    (void)mutation;
    return makeFutureStorageNoChangeReceipt(object, mutationKind);
}

CreativeMutationApplyReceipt applyStringIdMutation(CreativeObject& object, CreativeMutationKind mutationKind, const StringIdMutation& mutation) {
    (void)mutation;
    return makeFutureStorageNoChangeReceipt(object, mutationKind);
}

CreativeMutationApplyReceipt applyObjectKindMutation(CreativeObject& object, CreativeMutationKind mutationKind, const ObjectKindMutation& mutation) {
    (void)mutation;
    return makeFutureStorageNoChangeReceipt(object, mutationKind);
}

} // namespace iggy3d::creative
