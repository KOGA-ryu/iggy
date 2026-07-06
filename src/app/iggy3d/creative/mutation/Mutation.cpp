

#include "app/iggy3d/creative/mutation/Mutation.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace iggy3d::creative {
namespace {

[[nodiscard]] bool containsMutation(const std::vector<CreativeMutationKind>& mutations, CreativeMutationKind kind) noexcept {
    return std::find(mutations.begin(), mutations.end(), kind) != mutations.end();
}

[[nodiscard]] std::vector<CreativeMutationKind> commonIdentityMutations() {
    return {
        CreativeMutationKind::Rename,
        CreativeMutationKind::SetVisible,
        CreativeMutationKind::SetLocked,
        CreativeMutationKind::AssignLayer,
        CreativeMutationKind::AddTag,
        CreativeMutationKind::RemoveTag,
        CreativeMutationKind::ClearTags,
    };
}

void appendTransformMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Move);
    mutations.push_back(CreativeMutationKind::Rotate);
    mutations.push_back(CreativeMutationKind::Scale);
    mutations.push_back(CreativeMutationKind::SetTransform);
}

void appendBoxShapeMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::Resize);
    mutations.push_back(CreativeMutationKind::Stretch);
    mutations.push_back(CreativeMutationKind::SetBounds);
    mutations.push_back(CreativeMutationKind::SetHeight);
    mutations.push_back(CreativeMutationKind::SetLength);
    mutations.push_back(CreativeMutationKind::SetWidth);
    mutations.push_back(CreativeMutationKind::SetDepth);
}

void appendRelationshipMutations(std::vector<CreativeMutationKind>& mutations) {
    mutations.push_back(CreativeMutationKind::SetParent);
    mutations.push_back(CreativeMutationKind::ClearParent);
    mutations.push_back(CreativeMutationKind::AttachTo);
    mutations.push_back(CreativeMutationKind::DetachFrom);
    mutations.push_back(CreativeMutationKind::LinkTarget);
    mutations.push_back(CreativeMutationKind::UnlinkTarget);
    mutations.push_back(CreativeMutationKind::SetSocket);
    mutations.push_back(CreativeMutationKind::ClearSocket);
}

enum class CreativeMutationPayloadKind {
    NoPayload,
    Rename,
    Visibility,
    Lock,
    Move,
    Rotate,
    Scale,
    SetTransform,
    Resize,
    Stretch,
    SetBounds,
    Scalar,
    SetParent,
    AttachTo,
    LinkTarget,
    SetSocket,
    AssignLayer,
    Tag,
    TextOrStringId,
    ReferenceSource,
    Color,
    AudioSource,
    PathPointsOrLegacyText
};

struct CreativeMutationMetadataRow {
    CreativeMutationKind kind{CreativeMutationKind::Unknown};
    std::string_view name{"Unknown"};
    CreativeMutationCategory category{CreativeMutationCategory::Unknown};
    CreativeMutationStoragePolicy storagePolicy{CreativeMutationStoragePolicy::Unknown};
    bool changesGeometry{false};
    bool changesRelationships{false};
    bool changesRuntimeMeaning{false};
    CreativeMutationPayloadKind payloadKind{CreativeMutationPayloadKind::NoPayload};
};

[[nodiscard]] constexpr bool mutationCategoryChangesGeometry(CreativeMutationCategory category) noexcept {
    return category == CreativeMutationCategory::Transform || category == CreativeMutationCategory::Shape;
}

[[nodiscard]] constexpr bool mutationCategoryChangesRelationships(CreativeMutationCategory category) noexcept {
    return category == CreativeMutationCategory::Relationship;
}

[[nodiscard]] constexpr bool mutationCategoryChangesRuntimeMeaning(CreativeMutationCategory category) noexcept {
    return category == CreativeMutationCategory::Logic || category == CreativeMutationCategory::Navigation ||
           category == CreativeMutationCategory::Testing || category == CreativeMutationCategory::Sensory ||
           category == CreativeMutationCategory::Gameplay;
}

[[nodiscard]] constexpr CreativeMutationMetadataRow mutationMetadata(
    CreativeMutationKind kind,
    std::string_view name,
    CreativeMutationCategory category,
    CreativeMutationPayloadKind payloadKind,
    bool alsoChangesGeometry = false,
    bool alsoChangesRelationships = false,
    CreativeMutationStoragePolicy storagePolicy = CreativeMutationStoragePolicy::StoredObject) noexcept {
    return CreativeMutationMetadataRow{
        kind,
        name,
        category,
        storagePolicy,
        mutationCategoryChangesGeometry(category) || alsoChangesGeometry,
        mutationCategoryChangesRelationships(category) || alsoChangesRelationships,
        mutationCategoryChangesRuntimeMeaning(category),
        payloadKind,
    };
}

constexpr std::array kCreativeMutationMetadataRows{
    mutationMetadata(CreativeMutationKind::Unknown, "Unknown", CreativeMutationCategory::Unknown,
                     CreativeMutationPayloadKind::NoPayload, false, false, CreativeMutationStoragePolicy::Unknown),

    mutationMetadata(CreativeMutationKind::Rename, "Rename", CreativeMutationCategory::Identity,
                     CreativeMutationPayloadKind::Rename),
    mutationMetadata(CreativeMutationKind::SetVisible, "SetVisible", CreativeMutationCategory::Identity,
                     CreativeMutationPayloadKind::Visibility),
    mutationMetadata(CreativeMutationKind::SetLocked, "SetLocked", CreativeMutationCategory::Identity,
                     CreativeMutationPayloadKind::Lock),

    mutationMetadata(CreativeMutationKind::Move, "Move", CreativeMutationCategory::Transform,
                     CreativeMutationPayloadKind::Move),
    mutationMetadata(CreativeMutationKind::Rotate, "Rotate", CreativeMutationCategory::Transform,
                     CreativeMutationPayloadKind::Rotate),
    mutationMetadata(CreativeMutationKind::Scale, "Scale", CreativeMutationCategory::Transform,
                     CreativeMutationPayloadKind::Scale),
    mutationMetadata(CreativeMutationKind::SetTransform, "SetTransform", CreativeMutationCategory::Transform,
                     CreativeMutationPayloadKind::SetTransform),

    mutationMetadata(CreativeMutationKind::Resize, "Resize", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::Resize),
    mutationMetadata(CreativeMutationKind::Stretch, "Stretch", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::Stretch),
    mutationMetadata(CreativeMutationKind::SetBounds, "SetBounds", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::SetBounds),
    mutationMetadata(CreativeMutationKind::SetHeight, "SetHeight", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::Scalar),
    mutationMetadata(CreativeMutationKind::SetRadius, "SetRadius", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetThickness, "SetThickness", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetLength, "SetLength", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::Scalar),
    mutationMetadata(CreativeMutationKind::SetWidth, "SetWidth", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::Scalar),
    mutationMetadata(CreativeMutationKind::SetDepth, "SetDepth", CreativeMutationCategory::Shape,
                     CreativeMutationPayloadKind::Scalar),

    mutationMetadata(CreativeMutationKind::SetParent, "SetParent", CreativeMutationCategory::Relationship,
                     CreativeMutationPayloadKind::SetParent),
    mutationMetadata(CreativeMutationKind::ClearParent, "ClearParent", CreativeMutationCategory::Relationship,
                     CreativeMutationPayloadKind::NoPayload),
    mutationMetadata(CreativeMutationKind::AttachTo, "AttachTo", CreativeMutationCategory::Relationship,
                     CreativeMutationPayloadKind::AttachTo),
    mutationMetadata(CreativeMutationKind::DetachFrom, "DetachFrom", CreativeMutationCategory::Relationship,
                     CreativeMutationPayloadKind::NoPayload),
    mutationMetadata(CreativeMutationKind::LinkTarget, "LinkTarget", CreativeMutationCategory::Relationship,
                     CreativeMutationPayloadKind::LinkTarget, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::UnlinkTarget, "UnlinkTarget", CreativeMutationCategory::Relationship,
                     CreativeMutationPayloadKind::NoPayload, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetSocket, "SetSocket", CreativeMutationCategory::Relationship,
                     CreativeMutationPayloadKind::SetSocket, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::ClearSocket, "ClearSocket", CreativeMutationCategory::Relationship,
                     CreativeMutationPayloadKind::NoPayload, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),

    mutationMetadata(CreativeMutationKind::AssignLayer, "AssignLayer", CreativeMutationCategory::Organization,
                     CreativeMutationPayloadKind::AssignLayer),
    mutationMetadata(CreativeMutationKind::AddTag, "AddTag", CreativeMutationCategory::Organization,
                     CreativeMutationPayloadKind::Tag),
    mutationMetadata(CreativeMutationKind::RemoveTag, "RemoveTag", CreativeMutationCategory::Organization,
                     CreativeMutationPayloadKind::Tag),
    mutationMetadata(CreativeMutationKind::ClearTags, "ClearTags", CreativeMutationCategory::Organization,
                     CreativeMutationPayloadKind::NoPayload),

    mutationMetadata(CreativeMutationKind::EditText, "EditText", CreativeMutationCategory::Content,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetLabel, "SetLabel", CreativeMutationCategory::Content,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetNotes, "SetNotes", CreativeMutationCategory::Content,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetReferenceSource, "SetReferenceSource", CreativeMutationCategory::Content,
                     CreativeMutationPayloadKind::ReferenceSource, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetBlueprintOpacity, "SetBlueprintOpacity",
                     CreativeMutationCategory::Content, CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),

    mutationMetadata(CreativeMutationKind::SetTriggerShape, "SetTriggerShape", CreativeMutationCategory::Logic,
                     CreativeMutationPayloadKind::SetBounds, true),
    mutationMetadata(CreativeMutationKind::SetTriggerEvent, "SetTriggerEvent", CreativeMutationCategory::Logic,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetCondition, "SetCondition", CreativeMutationCategory::Logic,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetEventRelayTarget, "SetEventRelayTarget",
                     CreativeMutationCategory::Logic, CreativeMutationPayloadKind::LinkTarget, false, true,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetSpawnerProfile, "SetSpawnerProfile", CreativeMutationCategory::Logic,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetDespawnRule, "SetDespawnRule", CreativeMutationCategory::Logic,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),

    mutationMetadata(CreativeMutationKind::SetSpawnFacing, "SetSpawnFacing", CreativeMutationCategory::Navigation,
                     CreativeMutationPayloadKind::Rotate),
    mutationMetadata(CreativeMutationKind::SetCheckpointId, "SetCheckpointId", CreativeMutationCategory::Navigation,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetNavCost, "SetNavCost", CreativeMutationCategory::Navigation,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetPatrolRoute, "SetPatrolRoute", CreativeMutationCategory::Navigation,
                     CreativeMutationPayloadKind::PathPointsOrLegacyText, false, false,
                     CreativeMutationStoragePolicy::PayloadDependent),
    mutationMetadata(CreativeMutationKind::SetJumpArc, "SetJumpArc", CreativeMutationCategory::Navigation,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetClimbRule, "SetClimbRule", CreativeMutationCategory::Navigation,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetWallRunRule, "SetWallRunRule", CreativeMutationCategory::Navigation,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetSlideRule, "SetSlideRule", CreativeMutationCategory::Navigation,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),

    mutationMetadata(CreativeMutationKind::SetTestLaneKind, "SetTestLaneKind", CreativeMutationCategory::Testing,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetDistanceValue, "SetDistanceValue", CreativeMutationCategory::Testing,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetSpeedValue, "SetSpeedValue", CreativeMutationCategory::Testing,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetTimingWindow, "SetTimingWindow", CreativeMutationCategory::Testing,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetProbeKind, "SetProbeKind", CreativeMutationCategory::Testing,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetExpectedResult, "SetExpectedResult", CreativeMutationCategory::Testing,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),

    mutationMetadata(CreativeMutationKind::SetLightColor, "SetLightColor", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::Color, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetLightIntensity, "SetLightIntensity", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetLightRadius, "SetLightRadius", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetLightConeAngle, "SetLightConeAngle", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetAudioRadius, "SetAudioRadius", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetAudioSource, "SetAudioSource", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::AudioSource, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetMusicCue, "SetMusicCue", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetCameraTarget, "SetCameraTarget", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::LinkTarget, false, true,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetCameraRail, "SetCameraRail", CreativeMutationCategory::Sensory,
                     CreativeMutationPayloadKind::LinkTarget, false, true,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),

    mutationMetadata(CreativeMutationKind::SetEnemyProfile, "SetEnemyProfile", CreativeMutationCategory::Gameplay,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetNpcProfile, "SetNpcProfile", CreativeMutationCategory::Gameplay,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetResourceKind, "SetResourceKind", CreativeMutationCategory::Gameplay,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetLootTable, "SetLootTable", CreativeMutationCategory::Gameplay,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetQuestId, "SetQuestId", CreativeMutationCategory::Gameplay,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetDialogueId, "SetDialogueId", CreativeMutationCategory::Gameplay,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetDangerLevel, "SetDangerLevel", CreativeMutationCategory::Gameplay,
                     CreativeMutationPayloadKind::Scalar, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
    mutationMetadata(CreativeMutationKind::SetSafeZoneRule, "SetSafeZoneRule", CreativeMutationCategory::Gameplay,
                     CreativeMutationPayloadKind::TextOrStringId, false, false,
                     CreativeMutationStoragePolicy::FutureStoragePlaceholder),
};

[[nodiscard]] const CreativeMutationMetadataRow* metadataFor(CreativeMutationKind kind) noexcept {
    const auto it = std::find_if(kCreativeMutationMetadataRows.begin(), kCreativeMutationMetadataRows.end(),
                                 [kind](const CreativeMutationMetadataRow& row) {
                                     return row.kind == kind;
                                 });
    return it != kCreativeMutationMetadataRows.end() ? &(*it) : nullptr;
}

[[nodiscard]] bool payloadMatchesKind(CreativeMutationPayloadKind payloadKind,
                                      const CreativeMutationPayload& payload) noexcept {
    const auto& value = payload.value;

    switch (payloadKind) {
    case CreativeMutationPayloadKind::NoPayload:
        return std::holds_alternative<std::monostate>(value);
    case CreativeMutationPayloadKind::Rename:
        return std::holds_alternative<RenameMutation>(value);
    case CreativeMutationPayloadKind::Visibility:
        return std::holds_alternative<VisibilityMutation>(value);
    case CreativeMutationPayloadKind::Lock:
        return std::holds_alternative<LockMutation>(value);
    case CreativeMutationPayloadKind::Move:
        return std::holds_alternative<MoveMutation>(value);
    case CreativeMutationPayloadKind::Rotate:
        return std::holds_alternative<RotateMutation>(value);
    case CreativeMutationPayloadKind::Scale:
        return std::holds_alternative<ScaleMutation>(value);
    case CreativeMutationPayloadKind::SetTransform:
        return std::holds_alternative<SetTransformMutation>(value);
    case CreativeMutationPayloadKind::Resize:
        return std::holds_alternative<ResizeMutation>(value);
    case CreativeMutationPayloadKind::Stretch:
        return std::holds_alternative<StretchMutation>(value);
    case CreativeMutationPayloadKind::SetBounds:
        return std::holds_alternative<SetBoundsMutation>(value);
    case CreativeMutationPayloadKind::Scalar:
        return std::holds_alternative<ScalarMutation>(value);
    case CreativeMutationPayloadKind::SetParent:
        return std::holds_alternative<SetParentMutation>(value);
    case CreativeMutationPayloadKind::AttachTo:
        return std::holds_alternative<AttachToMutation>(value);
    case CreativeMutationPayloadKind::LinkTarget:
        return std::holds_alternative<LinkTargetMutation>(value);
    case CreativeMutationPayloadKind::SetSocket:
        return std::holds_alternative<SetSocketMutation>(value);
    case CreativeMutationPayloadKind::AssignLayer:
        return std::holds_alternative<AssignLayerMutation>(value);
    case CreativeMutationPayloadKind::Tag:
        return std::holds_alternative<TagMutation>(value);
    case CreativeMutationPayloadKind::TextOrStringId:
        return std::holds_alternative<TextMutation>(value) || std::holds_alternative<StringIdMutation>(value);
    case CreativeMutationPayloadKind::ReferenceSource:
        return std::holds_alternative<ReferenceSourceMutation>(value);
    case CreativeMutationPayloadKind::Color:
        return std::holds_alternative<ColorMutation>(value);
    case CreativeMutationPayloadKind::AudioSource:
        return std::holds_alternative<AudioSourceMutation>(value);
    case CreativeMutationPayloadKind::PathPointsOrLegacyText:
        return std::holds_alternative<PathPointsMutation>(value) || std::holds_alternative<TextMutation>(value) ||
               std::holds_alternative<StringIdMutation>(value);
    }

    return false;
}

} // namespace

std::string_view toString(CreativeMutationCategory category) noexcept {
    switch (category) {
    case CreativeMutationCategory::Unknown: return "Unknown";
    case CreativeMutationCategory::Identity: return "Identity";
    case CreativeMutationCategory::Transform: return "Transform";
    case CreativeMutationCategory::Shape: return "Shape";
    case CreativeMutationCategory::Relationship: return "Relationship";
    case CreativeMutationCategory::Organization: return "Organization";
    case CreativeMutationCategory::Content: return "Content";
    case CreativeMutationCategory::Logic: return "Logic";
    case CreativeMutationCategory::Navigation: return "Navigation";
    case CreativeMutationCategory::Testing: return "Testing";
    case CreativeMutationCategory::Sensory: return "Sensory";
    case CreativeMutationCategory::Gameplay: return "Gameplay";
    }

    return "Unknown";
}

std::string_view toString(CreativeMutationKind kind) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    return metadata != nullptr ? metadata->name : "Unknown";
}

std::string_view toString(CreativeMutationStoragePolicy policy) noexcept {
    switch (policy) {
    case CreativeMutationStoragePolicy::Unknown: return "Unknown";
    case CreativeMutationStoragePolicy::StoredObject: return "StoredObject";
    case CreativeMutationStoragePolicy::FutureStoragePlaceholder: return "FutureStoragePlaceholder";
    case CreativeMutationStoragePolicy::PayloadDependent: return "PayloadDependent";
    }

    return "Unknown";
}

CreativeMutationCategory categoryOf(CreativeMutationKind kind) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    return metadata != nullptr ? metadata->category : CreativeMutationCategory::Unknown;
}

CreativeMutationDescriptor describeMutation(CreativeMutationKind kind) noexcept {
    return CreativeMutationDescriptor{
        kind,
        categoryOf(kind),
        mutationStoragePolicy(kind),
        toString(kind),
        "authored creative object mutation",
        mutationChangesGeometry(kind),
        mutationChangesRelationships(kind),
        mutationChangesRuntimeMeaning(kind),
    };
}

bool isIdentityMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Identity;
}

bool isTransformMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Transform;
}

bool isShapeMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Shape;
}

bool isRelationshipMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Relationship;
}

bool isOrganizationMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Organization;
}

bool isContentMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Content;
}

bool isLogicMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Logic;
}

bool isNavigationMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Navigation;
}

bool isTestingMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Testing;
}

bool isSensoryMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Sensory;
}

bool isGameplayMutation(CreativeMutationKind kind) noexcept {
    return categoryOf(kind) == CreativeMutationCategory::Gameplay;
}

bool mutationChangesGeometry(CreativeMutationKind kind) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    return metadata != nullptr && metadata->changesGeometry;
}

bool mutationChangesRelationships(CreativeMutationKind kind) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    return metadata != nullptr && metadata->changesRelationships;
}

bool mutationChangesRuntimeMeaning(CreativeMutationKind kind) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    return metadata != nullptr && metadata->changesRuntimeMeaning;
}

CreativeMutationStoragePolicy mutationStoragePolicy(CreativeMutationKind kind) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    return metadata != nullptr ? metadata->storagePolicy : CreativeMutationStoragePolicy::Unknown;
}

CreativeMutationStoragePolicy mutationPayloadStoragePolicy(
    CreativeMutationKind kind,
    const CreativeMutationPayload& payload) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    if (metadata == nullptr || !payloadMatchesKind(metadata->payloadKind, payload)) {
        return CreativeMutationStoragePolicy::Unknown;
    }

    if (metadata->storagePolicy != CreativeMutationStoragePolicy::PayloadDependent) {
        return metadata->storagePolicy;
    }

    if (kind == CreativeMutationKind::SetPatrolRoute &&
        std::holds_alternative<PathPointsMutation>(payload.value)) {
        return CreativeMutationStoragePolicy::StoredObject;
    }

    if (kind == CreativeMutationKind::SetPatrolRoute &&
        (std::holds_alternative<TextMutation>(payload.value) ||
         std::holds_alternative<StringIdMutation>(payload.value))) {
        return CreativeMutationStoragePolicy::FutureStoragePlaceholder;
    }

    return CreativeMutationStoragePolicy::Unknown;
}

bool mutationHasStoredObjectEffect(CreativeMutationKind kind) noexcept {
    const CreativeMutationStoragePolicy policy = mutationStoragePolicy(kind);
    return policy == CreativeMutationStoragePolicy::StoredObject ||
           policy == CreativeMutationStoragePolicy::PayloadDependent;
}

bool mutationPayloadHasStoredObjectEffect(
    CreativeMutationKind kind,
    const CreativeMutationPayload& payload) noexcept {
    return mutationPayloadStoragePolicy(kind, payload) == CreativeMutationStoragePolicy::StoredObject;
}

std::vector<CreativeMutationKind> allowedMutations(CreativeObjectKind objectKind) {
    auto mutations = commonIdentityMutations();

    switch (objectKind) {
    case CreativeObjectKind::Unknown:
    case CreativeObjectKind::Count:
        return {};

    case CreativeObjectKind::Room:
        // TD-2: no-transform kinds with bounds move by corner anchor, so Move
        // is a real verb for Room even though it stores no transform.
        mutations.push_back(CreativeMutationKind::Move);
        appendBoxShapeMutations(mutations);
        return mutations;

    case CreativeObjectKind::Wall:
    case CreativeObjectKind::Floor:
    case CreativeObjectKind::Ceiling:
    case CreativeObjectKind::Roof:
    case CreativeObjectKind::Stair:
    case CreativeObjectKind::Ramp:
    case CreativeObjectKind::Platform:
    case CreativeObjectKind::MovingPlatform:
    case CreativeObjectKind::Column:
    case CreativeObjectKind::Pillar:
    case CreativeObjectKind::Beam:
    case CreativeObjectKind::Arch:
    case CreativeObjectKind::Fence:
    case CreativeObjectKind::Railing:
    case CreativeObjectKind::Bridge:
    case CreativeObjectKind::Ladder:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        appendRelationshipMutations(mutations);
        return mutations;

    case CreativeObjectKind::Door:
    case CreativeObjectKind::Window:
    case CreativeObjectKind::Socket:
    case CreativeObjectKind::AttachmentPoint:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        appendRelationshipMutations(mutations);
        return mutations;

    case CreativeObjectKind::TerrainPatch:
    case CreativeObjectKind::WaterVolume:
    case CreativeObjectKind::LavaVolume:
    case CreativeObjectKind::Pit:
    case CreativeObjectKind::Slope:
    case CreativeObjectKind::Cliff:
    case CreativeObjectKind::CaveOpening:
    case CreativeObjectKind::BoundaryVolume:
    case CreativeObjectKind::KillPlane:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetRadius);
        mutations.push_back(CreativeMutationKind::SetThickness);
        return mutations;

    case CreativeObjectKind::SpawnPoint:
    case CreativeObjectKind::ExitPoint:
    case CreativeObjectKind::EntrancePoint:
    case CreativeObjectKind::Checkpoint:
    case CreativeObjectKind::CoverPoint:
    case CreativeObjectKind::PatrolNode:
    case CreativeObjectKind::EnemySpawn:
    case CreativeObjectKind::NpcSpawn:
    case CreativeObjectKind::InterestPoint:
    case CreativeObjectKind::ResourceNode:
    case CreativeObjectKind::LootPoint:
    case CreativeObjectKind::QuestMarker:
    case CreativeObjectKind::DialogueMarker:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::Rotate);
        mutations.push_back(CreativeMutationKind::SetSpawnFacing);
        mutations.push_back(CreativeMutationKind::SetCheckpointId);
        return mutations;

    case CreativeObjectKind::NavRegion:
    case CreativeObjectKind::AlertZone:
    case CreativeObjectKind::SafeZone:
    case CreativeObjectKind::DangerZone:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetNavCost);
        mutations.push_back(CreativeMutationKind::SetDangerLevel);
        mutations.push_back(CreativeMutationKind::SetSafeZoneRule);
        return mutations;

    case CreativeObjectKind::NavLink:
    case CreativeObjectKind::JumpLink:
    case CreativeObjectKind::ClimbLink:
    case CreativeObjectKind::PatrolRoute:
        appendRelationshipMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetNavCost);
        mutations.push_back(CreativeMutationKind::SetJumpArc);
        mutations.push_back(CreativeMutationKind::SetClimbRule);
        mutations.push_back(CreativeMutationKind::SetPatrolRoute);
        return mutations;

    case CreativeObjectKind::WallRunSurface:
    case CreativeObjectKind::SlideSurface:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetWallRunRule);
        mutations.push_back(CreativeMutationKind::SetSlideRule);
        return mutations;

    case CreativeObjectKind::TriggerZone:
    case CreativeObjectKind::Switch:
    case CreativeObjectKind::Lever:
    case CreativeObjectKind::PressurePlate:
    case CreativeObjectKind::Button:
    case CreativeObjectKind::ConditionGate:
    case CreativeObjectKind::EventRelay:
    case CreativeObjectKind::Spawner:
    case CreativeObjectKind::DespawnZone:
    case CreativeObjectKind::ScriptMarker:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        appendRelationshipMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetTriggerShape);
        mutations.push_back(CreativeMutationKind::SetTriggerEvent);
        mutations.push_back(CreativeMutationKind::SetCondition);
        mutations.push_back(CreativeMutationKind::SetEventRelayTarget);
        mutations.push_back(CreativeMutationKind::SetSpawnerProfile);
        mutations.push_back(CreativeMutationKind::SetDespawnRule);
        return mutations;

    case CreativeObjectKind::TestLane:
    case CreativeObjectKind::DistanceMarker:
    case CreativeObjectKind::SpeedMarker:
    case CreativeObjectKind::JumpTarget:
    case CreativeObjectKind::CoyoteTimeLedge:
    case CreativeObjectKind::FallShaft:
    case CreativeObjectKind::CollisionProbe:
    case CreativeObjectKind::PhysicsProbe:
    case CreativeObjectKind::TimingGate:
    case CreativeObjectKind::TestStart:
    case CreativeObjectKind::TestEnd:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetTestLaneKind);
        mutations.push_back(CreativeMutationKind::SetDistanceValue);
        mutations.push_back(CreativeMutationKind::SetSpeedValue);
        mutations.push_back(CreativeMutationKind::SetTimingWindow);
        mutations.push_back(CreativeMutationKind::SetProbeKind);
        mutations.push_back(CreativeMutationKind::SetExpectedResult);
        return mutations;

    case CreativeObjectKind::Prop:
    case CreativeObjectKind::Decal:
    case CreativeObjectKind::Sign:
    case CreativeObjectKind::Banner:
    case CreativeObjectKind::FoliagePatch:
    case CreativeObjectKind::Rock:
    case CreativeObjectKind::Crate:
    case CreativeObjectKind::Barrel:
    case CreativeObjectKind::Furniture:
    case CreativeObjectKind::Decoration:
    case CreativeObjectKind::Group:
    case CreativeObjectKind::PrefabInstance:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        appendRelationshipMutations(mutations);
        return mutations;

    case CreativeObjectKind::PointLight:
    case CreativeObjectKind::SpotLight:
    case CreativeObjectKind::AreaLight:
    case CreativeObjectKind::AmbientZone:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::Rotate);
        mutations.push_back(CreativeMutationKind::SetLightColor);
        mutations.push_back(CreativeMutationKind::SetLightIntensity);
        mutations.push_back(CreativeMutationKind::SetLightRadius);
        mutations.push_back(CreativeMutationKind::SetLightConeAngle);
        return mutations;

    case CreativeObjectKind::ReverbZone:
    case CreativeObjectKind::SoundEmitter:
    case CreativeObjectKind::MusicZone:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::SetAudioRadius);
        mutations.push_back(CreativeMutationKind::SetAudioSource);
        mutations.push_back(CreativeMutationKind::SetMusicCue);
        return mutations;

    case CreativeObjectKind::CameraMarker:
    case CreativeObjectKind::CameraTarget:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::Rotate);
        mutations.push_back(CreativeMutationKind::SetCameraTarget);
        return mutations;

    case CreativeObjectKind::CameraRail:
    case CreativeObjectKind::CutsceneMarker:
        appendTransformMutations(mutations);
        appendRelationshipMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetCameraTarget);
        mutations.push_back(CreativeMutationKind::SetCameraRail);
        return mutations;

    case CreativeObjectKind::Note:
    case CreativeObjectKind::Label:
    case CreativeObjectKind::Comment:
        mutations.push_back(CreativeMutationKind::Move);
        mutations.push_back(CreativeMutationKind::EditText);
        mutations.push_back(CreativeMutationKind::SetLabel);
        mutations.push_back(CreativeMutationKind::SetNotes);
        return mutations;

    case CreativeObjectKind::MeasurementMarker:
    case CreativeObjectKind::MeasurementLine:
    case CreativeObjectKind::MeasurementBox:
    case CreativeObjectKind::GridAnchor:
    case CreativeObjectKind::SnapAnchor:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetDistanceValue);
        return mutations;

    case CreativeObjectKind::ReferenceImage:
    case CreativeObjectKind::BlueprintOverlay:
        appendTransformMutations(mutations);
        appendBoxShapeMutations(mutations);
        mutations.push_back(CreativeMutationKind::SetReferenceSource);
        mutations.push_back(CreativeMutationKind::SetBlueprintOpacity);
        return mutations;
    }

    return mutations;
}

bool canMutate(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept {
    const auto mutations = allowedMutations(objectKind);
    return containsMutation(mutations, mutationKind);
}

bool requiresPayload(CreativeMutationKind kind) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    return metadata != nullptr && metadata->payloadKind != CreativeMutationPayloadKind::NoPayload;
}

bool payloadMatchesMutation(CreativeMutationKind kind, const CreativeMutationPayload& payload) noexcept {
    const CreativeMutationMetadataRow* metadata = metadataFor(kind);
    return metadata != nullptr && payloadMatchesKind(metadata->payloadKind, payload);
}

CreativeMutationPayload makeRenamePayload(std::string name) {
    return CreativeMutationPayload{RenameMutation{std::move(name)}};
}

CreativeMutationPayload makeVisibilityPayload(bool visible) {
    return CreativeMutationPayload{VisibilityMutation{visible}};
}

CreativeMutationPayload makeLockPayload(bool locked) {
    return CreativeMutationPayload{LockMutation{locked}};
}

CreativeMutationPayload makeMovePayload(CreativeVec3 position) {
    return CreativeMutationPayload{MoveMutation{position}};
}

CreativeMutationPayload makeRotatePayload(CreativeVec3 rotation) {
    return CreativeMutationPayload{RotateMutation{rotation}};
}

CreativeMutationPayload makeScalePayload(CreativeVec3 scale) {
    return CreativeMutationPayload{ScaleMutation{scale}};
}

CreativeMutationPayload makeTransformPayload(CreativeTransform transform) {
    return CreativeMutationPayload{SetTransformMutation{transform}};
}

CreativeMutationPayload makeResizePayload(CreativeVec3 size) {
    return CreativeMutationPayload{ResizeMutation{size}};
}

CreativeMutationPayload makeStretchPayload(CreativeVec3 delta) {
    return CreativeMutationPayload{StretchMutation{delta}};
}

CreativeMutationPayload makeBoundsPayload(CreativeBounds bounds) {
    return CreativeMutationPayload{SetBoundsMutation{bounds}};
}

CreativeMutationPayload makeScalarPayload(double value) {
    return CreativeMutationPayload{ScalarMutation{value}};
}

CreativeMutationPayload makeParentPayload(CreativeObjectId parentId) {
    return CreativeMutationPayload{SetParentMutation{parentId}};
}

CreativeMutationPayload makeAttachPayload(CreativeObjectId targetId, std::string socket) {
    return CreativeMutationPayload{AttachToMutation{targetId, std::move(socket)}};
}

CreativeMutationPayload makeLinkPayload(CreativeObjectId targetId) {
    return CreativeMutationPayload{LinkTargetMutation{targetId}};
}

CreativeMutationPayload makeSocketPayload(std::string socket) {
    return CreativeMutationPayload{SetSocketMutation{std::move(socket)}};
}

CreativeMutationPayload makeLayerPayload(CreativeLayerId layerId) {
    return CreativeMutationPayload{AssignLayerMutation{layerId}};
}

CreativeMutationPayload makeTagPayload(std::string tag) {
    return CreativeMutationPayload{TagMutation{std::move(tag)}};
}

CreativeMutationPayload makeTextPayload(std::string text) {
    return CreativeMutationPayload{TextMutation{std::move(text)}};
}

CreativeMutationPayload makeReferenceSourcePayload(std::string source) {
    return CreativeMutationPayload{ReferenceSourceMutation{std::move(source)}};
}

CreativeMutationPayload makeColorPayload(CreativeRgba color) {
    return CreativeMutationPayload{ColorMutation{color}};
}

CreativeMutationPayload makeAudioSourcePayload(std::string source) {
    return CreativeMutationPayload{AudioSourceMutation{std::move(source)}};
}

CreativeMutationPayload makeStringIdPayload(std::string id) {
    return CreativeMutationPayload{StringIdMutation{std::move(id)}};
}

CreativeMutationPayload makePathPointsPayload(std::vector<CreativePathPoint> pathPoints) {
    return CreativeMutationPayload{PathPointsMutation{std::move(pathPoints)}};
}

CreativeMutationPayload makeObjectKindPayload(CreativeObjectKind kind) {
    return CreativeMutationPayload{ObjectKindMutation{kind}};
}

} // namespace iggy3d::creative
