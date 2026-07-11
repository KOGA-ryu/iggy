

#pragma once

#include "app/iggy3d/creative/document/Object.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace iggy3d::creative {

// Mutation.hpp is the creative-mode verb vocabulary.
//
// Object.hpp defines the nouns: Room, Wall, SpawnPoint, TriggerZone, Door, and
// so on. Mutation.hpp defines the reusable verbs those nouns may accept.
//
// The design rule is simple:
// - Do not write rotate/move/rename separately for every object kind.
// - Define each mutation once.
// - Let each CreativeObjectKind opt into the mutations it supports.
// - Commands and documents apply these shared mutations after rule validation.
//
// This file must stay free of UI, rendering, filesystem, async work, mouse/tool
// state, and editor panel state. It describes authored content changes only.

using CreativeMutationId = std::uint64_t;

struct CreativeRgba {
    double r{1.0};
    double g{1.0};
    double b{1.0};
    double a{1.0};
};

enum class CreativeMutationCategory {
    Unknown,
    Identity,
    Transform,
    Shape,
    Relationship,
    Organization,
    Content,
    Logic,
    Navigation,
    Testing,
    Sensory,
    Gameplay
};

enum class CreativeMutationKind {
    Unknown,

    // Identity / common authored state
    Rename,
    SetVisible,
    SetLocked,

    // Transform
    Move,
    Rotate,
    Scale,
    SetTransform,

    // Shape / dimensions
    Resize,
    Stretch,
    SetBounds,
    SetHeight,
    SetRadius,
    SetThickness,
    SetLength,
    SetWidth,
    SetDepth,

    // Relationship / attachment
    SetParent,
    ClearParent,
    AttachTo,
    DetachFrom,
    LinkTarget,
    UnlinkTarget,
    SetSocket,
    ClearSocket,

    // Organization
    AssignLayer,
    AddTag,
    RemoveTag,
    ClearTags,

    // Content / authoring text
    EditText,
    SetLabel,
    SetNotes,
    SetReferenceSource,
    SetBlueprintOpacity,

    // Logic / event wiring
    SetTriggerShape,
    SetTriggerEvent,
    SetCondition,
    SetEventRelayTarget,
    SetSpawnerProfile,
    SetDespawnRule,

    // Navigation / movement authoring
    SetSpawnFacing,
    SetCheckpointId,
    SetNavCost,
    SetPatrolRoute,
    SetJumpArc,
    SetClimbRule,
    SetWallRunRule,
    SetSlideRule,

    // Testing / measurement lab
    SetTestLaneKind,
    SetDistanceValue,
    SetSpeedValue,
    SetTimingWindow,
    SetProbeKind,
    SetExpectedResult,

    // Sensory
    SetLightColor,
    SetLightIntensity,
    SetLightRadius,
    SetLightConeAngle,
    SetAudioRadius,
    SetAudioSource,
    SetMusicCue,
    SetCameraTarget,
    SetCameraRail,

    // Gameplay
    SetEnemyProfile,
    SetNpcProfile,
    SetResourceKind,
    SetLootTable,
    SetQuestId,
    SetDialogueId,
    SetDangerLevel,
    SetSafeZoneRule
};

enum class CreativeMutationStoragePolicy {
    Unknown,
    StoredObject,
    FutureStoragePlaceholder,
    PayloadDependent
};

struct RenameMutation {
    std::string name{};
};

struct VisibilityMutation {
    bool visible{true};
};

struct LockMutation {
    bool locked{false};
};

struct MoveMutation {
    CreativeVec3 position{};
};

struct RotateMutation {
    CreativeVec3 rotationEulerRadians{};
};

struct ScaleMutation {
    CreativeVec3 scale{1.0, 1.0, 1.0};
};

struct SetTransformMutation {
    CreativeTransform transform{};
};

struct ResizeMutation {
    CreativeVec3 size{};
};

struct StretchMutation {
    CreativeVec3 delta{};
};

struct SetBoundsMutation {
    CreativeBounds bounds{};
};

struct ScalarMutation {
    double value{0.0};
};

struct SetParentMutation {
    CreativeObjectId parentId{0};
};

struct AttachToMutation {
    CreativeObjectId targetId{0};
    std::string socket{};
};

struct LinkTargetMutation {
    CreativeObjectId targetId{0};
};

struct SetSocketMutation {
    std::string socket{};
};

struct AssignLayerMutation {
    CreativeLayerId layerId{0};
};

struct TagMutation {
    std::string tag{};
};

struct TextMutation {
    std::string text{};
};

struct ReferenceSourceMutation {
    std::string source{};
};

struct ColorMutation {
    CreativeRgba color{};
};

struct AudioSourceMutation {
    std::string source{};
};

struct StringIdMutation {
    std::string id{};
};

struct PathPointsMutation {
    std::vector<CreativePathPoint> pathPoints{};
};

struct ObjectKindMutation {
    CreativeObjectKind kind{CreativeObjectKind::Unknown};
};

struct CreativeMutationPayload {
    using Value = std::variant<
        std::monostate,
        RenameMutation,
        VisibilityMutation,
        LockMutation,
        MoveMutation,
        RotateMutation,
        ScaleMutation,
        SetTransformMutation,
        ResizeMutation,
        StretchMutation,
        SetBoundsMutation,
        ScalarMutation,
        SetParentMutation,
        AttachToMutation,
        LinkTargetMutation,
        SetSocketMutation,
        AssignLayerMutation,
        TagMutation,
        TextMutation,
        ReferenceSourceMutation,
        ColorMutation,
        AudioSourceMutation,
        StringIdMutation,
        PathPointsMutation,
        ObjectKindMutation>;

    Value value{};
};

struct CreativeMutationRequest {
    CreativeMutationId id{0};
    CreativeObjectId objectId{0};
    CreativeMutationKind kind{CreativeMutationKind::Unknown};
    CreativeMutationPayload payload{};
};

struct CreativeMutationRule {
    CreativeObjectKind objectKind{CreativeObjectKind::Unknown};
    CreativeMutationKind mutationKind{CreativeMutationKind::Unknown};
};

struct CreativeMutationDescriptor {
    CreativeMutationKind kind{CreativeMutationKind::Unknown};
    CreativeMutationCategory category{CreativeMutationCategory::Unknown};
    CreativeMutationStoragePolicy storagePolicy{CreativeMutationStoragePolicy::Unknown};
    std::string_view name{};
    std::string_view purpose{};
    bool changesGeometry{false};
    bool changesRelationships{false};
    bool changesRuntimeMeaning{false};
};

[[nodiscard]] std::string_view toString(CreativeMutationCategory category) noexcept;
[[nodiscard]] std::string_view toString(CreativeMutationKind kind) noexcept;
[[nodiscard]] std::string_view toString(CreativeMutationStoragePolicy policy) noexcept;
[[nodiscard]] CreativeMutationCategory categoryOf(CreativeMutationKind kind) noexcept;
[[nodiscard]] CreativeMutationDescriptor describeMutation(CreativeMutationKind kind) noexcept;

[[nodiscard]] bool isIdentityMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isTransformMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isShapeMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isRelationshipMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isOrganizationMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isContentMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isLogicMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isNavigationMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isTestingMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isSensoryMutation(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool isGameplayMutation(CreativeMutationKind kind) noexcept;

[[nodiscard]] bool mutationChangesGeometry(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool mutationChangesRelationships(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool mutationChangesRuntimeMeaning(CreativeMutationKind kind) noexcept;
[[nodiscard]] CreativeMutationStoragePolicy mutationStoragePolicy(CreativeMutationKind kind) noexcept;
[[nodiscard]] CreativeMutationStoragePolicy mutationPayloadStoragePolicy(
    CreativeMutationKind kind,
    const CreativeMutationPayload& payload) noexcept;
[[nodiscard]] bool mutationHasStoredObjectEffect(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool mutationPayloadHasStoredObjectEffect(
    CreativeMutationKind kind,
    const CreativeMutationPayload& payload) noexcept;

[[nodiscard]] bool canMutate(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept;
[[nodiscard]] std::vector<CreativeMutationKind> allowedMutations(CreativeObjectKind objectKind);

[[nodiscard]] bool requiresPayload(CreativeMutationKind kind) noexcept;
[[nodiscard]] bool payloadMatchesMutation(CreativeMutationKind kind, const CreativeMutationPayload& payload) noexcept;

[[nodiscard]] CreativeMutationPayload makeRenamePayload(std::string name);
[[nodiscard]] CreativeMutationPayload makeVisibilityPayload(bool visible);
[[nodiscard]] CreativeMutationPayload makeLockPayload(bool locked);
[[nodiscard]] CreativeMutationPayload makeMovePayload(CreativeVec3 position);
[[nodiscard]] CreativeMutationPayload makeRotatePayload(
    CreativeVec3 rotationEulerRadians);
[[nodiscard]] CreativeMutationPayload makeBoundsPayload(CreativeBounds bounds);
[[nodiscard]] CreativeMutationPayload makeScalarPayload(double value);
[[nodiscard]] CreativeMutationPayload makeParentPayload(CreativeObjectId parentId);
[[nodiscard]] CreativeMutationPayload makeAttachPayload(CreativeObjectId targetId, std::string socket);
[[nodiscard]] CreativeMutationPayload makeLinkPayload(CreativeObjectId targetId);
[[nodiscard]] CreativeMutationPayload makeTextPayload(std::string text);
[[nodiscard]] CreativeMutationPayload makeStringIdPayload(std::string id);
[[nodiscard]] CreativeMutationPayload makePathPointsPayload(std::vector<CreativePathPoint> pathPoints);

} // namespace iggy3d::creative
