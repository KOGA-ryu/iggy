

#pragma once

#include "app/iggy3d/creative/mutation/Mutation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d::creative {

// ObjectDescriptor is the creative-mode noun truth table.
//
// Object.hpp defines the raw object record.
// Mutation.hpp defines reusable verbs.
// ObjectDescriptor.hpp defines what each object kind means, which profile it
// belongs to, what systems it affects, and which mutation rules should apply.
//
// This is the CNC file for object-kind expansion. Future object kinds should be
// added here as table rows, not rediscovered through the repo.
//
// This file must not own editor state, UI state, rendering resources,
// filesystem state, async jobs, or command dispatch.

enum class CreativeObjectCategory {
    Unknown,
    Structural,
    TerrainOrVolume,
    NavigationOrMovement,
    Logic,
    Testing,
    VisualDressing,
    LightSoundOrCamera,
    AuthoringMeta,
    Gameplay
};

// Profiles are reusable behavior bundles. They are the shortcut that prevents
// writing one-off mutation rules for every object kind.
enum class CreativeObjectProfile {
    Unknown,

    // Large authored containers or room-like volumes.
    RoomContainer,

    // Box-like structural geometry: walls, floors, ceilings, beams, ramps.
    BoxStructural,

    // Attachments that usually depend on another object: doors, windows,
    // sockets, attachment points.
    Attachment,

    // Terrain or environmental volume: water, lava, pits, caves, boundaries.
    TerrainVolume,

    // Point marker with position/facing but usually no size.
    Marker,

    // Box or volume that represents area behavior.
    Volume,

    // Relationship/path object: nav links, jump links, patrol routes.
    LinkOrRoute,

    // Logic/event object: switch, lever, trigger, spawner, relay.
    LogicNode,

    // Movement testing instrument: wall run surfaces, jump targets, probes.
    MovementTest,

    // Visual-only authored object: prop, decal, sign, furniture.
    Dressing,

    // Light emitter or lighting zone.
    Light,

    // Audio emitter or audio zone.
    Audio,

    // Camera object or camera path helper.
    Camera,

    // Text/annotation authoring object.
    TextAnnotation,

    // Measurement/grid/reference helper.
    AuthoringHelper,

    // Gameplay semantic object: enemy spawn, quest marker, loot point.
    GameplayMarker
};

// Shape kinds are generic editable archetypes. They describe how an authored
// object should be manipulated by future generic tools; they are related to,
// but intentionally distinct from, spatial projection policy.
enum class CreativeObjectShapeKind {
    Unknown,
    Point,
    Line,
    BoxVolume,
    Surface,
    Path,
    MeshProxy,
};

enum class CreativeSpatialProjectionProfile {
    Unknown,
    NoProjection,
    PointProjection,
    BoxProjection,
    VolumeProjection,
    LineProjection,
    PathProjection,
    LinkProjection,
};

enum class CreativeSpatialOccupancyKind {
    Unknown,
    Structural,
    Collision,
    Navigation,
    Trigger,
    Gameplay,
    Light,
    Audio,
    Camera,
    Testing,
    Authoring,
};

enum class CreativeRuntimeAnchorSemantic {
    None,
    Spawn,
    Exit,
    Npc,
    Monster,
    Pickup,
    Light,
    Audio,
    Camera,
};

enum class CreativeAuthoringPaletteVisibility {
    Hidden,
    Brush,
};

enum class CreativeObjectDirtyFlag : std::uint64_t {
    None = 0,
    Identity = 1ull << 0,
    Transform = 1ull << 1,
    Bounds = 1ull << 2,
    Geometry = 1ull << 3,
    Collision = 1ull << 4,
    Navigation = 1ull << 5,
    Logic = 1ull << 6,
    Lighting = 1ull << 7,
    Audio = 1ull << 8,
    Camera = 1ull << 9,
    Gameplay = 1ull << 10,
    Testing = 1ull << 11,
    Preview = 1ull << 12,
    Serialization = 1ull << 13
};

using CreativeObjectDirtyFlags = std::uint64_t;

struct CreativeObjectDefaults {
    CreativeTransform transform{};
    CreativeBounds bounds{};
    CreativeLayerId layerId{0};
    bool visible{true};
    bool locked{false};
};

struct CreativeObjectDescriptor {
    CreativeObjectKind kind{CreativeObjectKind::Unknown};
    CreativeObjectCategory category{CreativeObjectCategory::Unknown};
    CreativeObjectProfile profile{CreativeObjectProfile::Unknown};
    CreativeObjectShapeKind shapeKind{CreativeObjectShapeKind::Unknown};
    CreativeSpatialProjectionProfile projectionProfile{
        CreativeSpatialProjectionProfile::Unknown};
    CreativeSpatialOccupancyKind occupancyKind{
        CreativeSpatialOccupancyKind::Unknown};
    CreativeRuntimeAnchorSemantic runtimeAnchorSemantic{
        CreativeRuntimeAnchorSemantic::None};
    CreativeAuthoringPaletteVisibility authoringPaletteVisibility{
        CreativeAuthoringPaletteVisibility::Hidden};

    std::string_view name{};
    std::string_view displayName{};
    std::string_view purpose{};

    CreativeObjectDirtyFlags creationDirtyFlags{0};
    CreativeObjectDefaults defaults{};

    bool hasTransform{true};
    bool hasBounds{false};
    bool canHaveParent{false};
    bool canOwnChildren{false};
    bool isRuntimeMeaningful{false};
    bool isEditorOnly{false};
};

[[nodiscard]] CreativeObjectDirtyFlags operator|(CreativeObjectDirtyFlag lhs, CreativeObjectDirtyFlag rhs) noexcept;
[[nodiscard]] CreativeObjectDirtyFlags operator|(CreativeObjectDirtyFlags lhs, CreativeObjectDirtyFlag rhs) noexcept;
[[nodiscard]] bool hasDirtyFlag(CreativeObjectDirtyFlags flags, CreativeObjectDirtyFlag flag) noexcept;

[[nodiscard]] std::string_view toString(CreativeObjectCategory category) noexcept;
[[nodiscard]] std::string_view toString(CreativeObjectProfile profile) noexcept;
[[nodiscard]] std::string_view toString(CreativeObjectShapeKind shapeKind) noexcept;
[[nodiscard]] std::string_view toString(CreativeRuntimeAnchorSemantic semantic) noexcept;
[[nodiscard]] std::string_view toString(CreativeObjectDirtyFlag flag) noexcept;

[[nodiscard]] CreativeObjectCategory categoryOf(CreativeObjectKind kind) noexcept;
[[nodiscard]] CreativeObjectProfile profileOf(CreativeObjectKind kind) noexcept;
[[nodiscard]] CreativeObjectShapeKind shapeKindForObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] const CreativeObjectDescriptor& describeObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] std::span<const CreativeObjectDescriptor> allObjectDescriptors() noexcept;

[[nodiscard]] CreativeObjectDirtyFlags dirtyFlagsForCreation(CreativeObjectKind kind) noexcept;
[[nodiscard]] CreativeObjectDirtyFlags dirtyFlagsForMutation(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept;

[[nodiscard]] bool objectHasTransform(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool objectHasBounds(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool objectCanHaveParent(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool objectCanOwnChildren(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool objectIsRuntimeMeaningful(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool objectIsEditorOnly(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool descriptorShowsInAuthoringBrushPalette(const CreativeObjectDescriptor& descriptor) noexcept;
[[nodiscard]] bool objectShowsInAuthoringBrushPalette(CreativeObjectKind kind) noexcept;

[[nodiscard]] bool objectUsesProfile(CreativeObjectKind kind, CreativeObjectProfile profile) noexcept;
[[nodiscard]] bool objectUsesCategory(CreativeObjectKind kind, CreativeObjectCategory category) noexcept;

[[nodiscard]] bool descriptorAllowsMutation(CreativeObjectKind objectKind, CreativeMutationKind mutationKind) noexcept;

} // namespace iggy3d::creative
