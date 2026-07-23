
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

using CreativeObjectId = std::uint64_t;
using CreativeLayerId = std::uint64_t;

inline constexpr CreativeObjectId kInvalidObjectId = 0;
inline constexpr CreativeLayerId kDefaultLayerId = 0;
inline constexpr std::size_t kCreativeAttachmentSocketNameCapacity = 64U;
inline constexpr std::size_t kCreativeAssetMaterialVariantNameCapacity = 64U;
inline constexpr std::size_t kCreativePlayerProfileIdCapacity = 64U;
inline constexpr std::size_t kCreativeSpawnGroupCapacity = 64U;
inline constexpr double kCreativePlayerSpawnMinimumValidationRadiusMeters =
    0.30;
inline constexpr double kCreativePlayerSpawnMaximumValidationRadiusMeters =
    4.0;

struct CreativeVec3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct CreativeTransform {
  CreativeVec3 position{};
  // Intrinsic X-then-Y-then-Z Euler rotation, always stored in radians.
  CreativeVec3 rotationEulerRadians{};
  CreativeVec3 scale{1.0, 1.0, 1.0};
};

struct CreativeBounds {
  CreativeVec3 min{};
  CreativeVec3 max{};
};

// Stable semantic materials for procedural structural geometry. Imported
// assets retain their own material slots; these values let authored recipes
// request a deterministic fallback material without coupling the document to
// a renderer backend.
enum class CreativeStructuralMaterial : std::uint8_t {
  Blockout,
  Plaster,
  Timber,
  Stone,
  Brick,
  Count,
};

struct CreativePathPoint {
  CreativeVec3 position{};
  double dwellSeconds{0.0};
  // Speed for the segment leaving this point in authored path order. The
  // physical segment keeps this multiplier when traversed in reverse.
  double outgoingSpeedMultiplier{1.0};
};

inline constexpr std::size_t kCreativeMovingPlatformPathPointCapacity = 32U;
inline constexpr double kCreativePathPointMaximumDwellSeconds = 60.0;
inline constexpr double kCreativePathPointMinimumOutgoingSpeedMultiplier =
    0.25;
inline constexpr double kCreativePathPointMaximumOutgoingSpeedMultiplier =
    4.0;

enum class CreativeMovingPlatformTraversalMode : std::uint8_t {
  PingPong,
  Loop,
  Count,
};

struct CreativeMovingPlatformSettings {
  double speedMetersPerSecond{1.5};
  CreativeMovingPlatformTraversalMode traversalMode{
      CreativeMovingPlatformTraversalMode::PingPong};
  bool startsActive{true};

  bool operator==(const CreativeMovingPlatformSettings&) const = default;
};

enum class CreativeDoorLeafArrangement : std::uint8_t {
  Single,
  Double,
  Count,
};

// Hinge identity is stored against the canonical world-axis extent of the
// closed leaf. It therefore survives wall endpoint reversal and closed-state
// save/load without relying on a transient open pose.
enum class CreativeDoorHingeSide : std::uint8_t {
  MinimumEdge,
  MaximumEdge,
  Count,
};

enum class CreativeDoorSwingSide : std::uint8_t {
  NegativeNormal,
  PositiveNormal,
  Count,
};

enum class CreativeDoorInitialState : std::uint8_t {
  Closed,
  Open,
  Count,
};

struct CreativeDoorSettings {
  CreativeDoorLeafArrangement leafArrangement{
      CreativeDoorLeafArrangement::Single};
  CreativeDoorHingeSide hingeSide{CreativeDoorHingeSide::MinimumEdge};
  CreativeDoorSwingSide swingSide{CreativeDoorSwingSide::PositiveNormal};
  CreativeDoorInitialState initialState{CreativeDoorInitialState::Closed};
  bool gameplayLocked{false};
  double transitionSeconds{0.35};

  bool operator==(const CreativeDoorSettings&) const = default;
};

enum class CreativeWindowInsertKind : std::uint8_t {
  Glazing,
  PairedShutters,
  Count,
};

// Window settings describe the durable insert treatment. Frame, sill, and
// lintel geometry are derived by WindowRecipe from the hosted cutout.
struct CreativeWindowSettings {
  CreativeWindowInsertKind insertKind{CreativeWindowInsertKind::Glazing};

  bool operator==(const CreativeWindowSettings&) const = default;
};

// Authored player-entry policy. Facing remains the object's Y rotation so the
// standard transform tools and the play camera consume one orientation truth.
// Lower fallback priorities are preferred; object id breaks ties
// deterministically when validating legacy documents.
struct CreativePlayerSpawnSettings {
  std::string playerProfileId{"default"};
  std::string spawnGroup{"default"};
  double validationRadiusMeters{0.45};
  std::uint16_t fallbackPriority{0U};

  bool operator==(const CreativePlayerSpawnSettings&) const = default;
};

enum class CreativeObjectKind {
  Unknown,

  // Structural
  Room,
  Wall,
  Floor,
  Ceiling,
  Roof,
  Door,
  Window,
  Stair,
  Ramp,
  Platform,
  MovingPlatform,
  Column,
  Pillar,
  Beam,
  Arch,
  Fence,
  Railing,
  Bridge,
  Ladder,

  // Terrain / volumes
  TerrainPatch,
  WaterVolume,
  LavaVolume,
  Pit,
  Slope,
  Cliff,
  CaveOpening,
  BoundaryVolume,
  KillPlane,

  // Navigation / movement
  SpawnPoint,
  ExitPoint,
  EntrancePoint,
  Checkpoint,
  NavRegion,
  NavLink,
  JumpLink,
  ClimbLink,
  WallRunSurface,
  SlideSurface,
  CoverPoint,
  PatrolNode,

  // Logic
  TriggerZone,
  Switch,
  Lever,
  PressurePlate,
  Button,
  ConditionGate,
  EventRelay,
  Spawner,
  DespawnZone,
  ScriptMarker,

  // Testing
  TestLane,
  DistanceMarker,
  SpeedMarker,
  JumpTarget,
  CoyoteTimeLedge,
  FallShaft,
  CollisionProbe,
  PhysicsProbe,
  TimingGate,
  TestStart,
  TestEnd,

  // Visual dressing
  Prop,
  Decal,
  Sign,
  Banner,
  FoliagePatch,
  Rock,
  Crate,
  Barrel,
  Furniture,
  Decoration,

  // Light / sound / camera
  PointLight,
  SpotLight,
  AreaLight,
  AmbientZone,
  ReverbZone,
  SoundEmitter,
  MusicZone,
  CameraMarker,
  CameraRail,
  CameraTarget,
  CutsceneMarker,

  // Authoring / meta
  Note,
  Label,
  Comment,
  MeasurementMarker,
  MeasurementLine,
  MeasurementBox,
  GridAnchor,
  SnapAnchor,
  ReferenceImage,
  BlueprintOverlay,
  Group,
  PrefabInstance,
  Socket,
  AttachmentPoint,

  // Gameplay
  EnemySpawn,
  NpcSpawn,
  PatrolRoute,
  InterestPoint,
  AlertZone,
  SafeZone,
  DangerZone,
  ResourceNode,
  LootPoint,
  QuestMarker,
  DialogueMarker,

  // Appended to preserve every existing serialized object-kind value.
  GableRoof,
  // Semantic alias for new authored shed/gable panels. The legacy wire value
  // and old saved GableRoof objects remain valid.
  RoofSlope = GableRoof,
  HipRoof,

  Count
};

struct CreativeObject {
  CreativeObjectId id{kInvalidObjectId};
  CreativeObjectKind kind{CreativeObjectKind::Unknown};
  std::string name{};
  // Stable content key resolved by the render asset library. Empty keeps the
  // descriptor-backed generated mesh.
  std::string assetId{};
  // Exact imported-content revision selected when this instance was authored.
  // Zero is retained for legacy saves and authored composites without a GLB.
  std::uint64_t assetContentHash{0U};
  // Empty selects the asset's default material set.
  std::string assetMaterialVariant{};

  CreativeTransform transform{};
  CreativeBounds bounds{};

  CreativeLayerId layerId{kDefaultLayerId};
  bool visible{true};
  bool locked{false};

  std::vector<std::string> tags{};
  std::optional<CreativeObjectId> parentId{};
  // Name of the receiver on parentId. Empty means an ordinary hierarchy edge.
  std::string attachmentSocket{};
  std::vector<CreativePathPoint> pathPoints{};
  CreativeMovingPlatformSettings movingPlatform{};
  CreativeDoorSettings door{};
  CreativeWindowSettings window{};
  CreativePlayerSpawnSettings playerSpawn{};
};

struct CreativeTransformedBounds {
  std::array<CreativeVec3, 8> corners{};
  CreativeBounds worldBounds{};
  CreativeVec3 center{};
  CreativeVec3 size{};
  CreativeVec3 rotationEulerRadians{};
  bool valid{false};
};

// `authoredBounds` remain in document coordinates at identity rotation/scale.
// The transform position is the pivot used to derive live world geometry.
[[nodiscard]] CreativeTransformedBounds resolveCreativeTransformedBounds(
    CreativeBounds authoredBounds,
    CreativeTransform transform) noexcept;
[[nodiscard]] CreativeTransformedBounds resolveCreativeObjectBounds(
    const CreativeObject& object) noexcept;

[[nodiscard]] CreativeObject makeRoomObject(
    CreativeObjectId id,
    std::string name,
    CreativeTransform transform = {},
    CreativeBounds bounds = {},
    CreativeLayerId layerId = kDefaultLayerId,
    bool visible = true,
    bool locked = false,
    std::vector<std::string> tags = {},
    std::optional<CreativeObjectId> parentId = std::nullopt);

[[nodiscard]] std::string_view toString(CreativeObjectKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeStructuralMaterial material) noexcept;
[[nodiscard]] std::string creativeStructuralMaterialTag(
    CreativeStructuralMaterial material);
[[nodiscard]] bool parseCreativeStructuralMaterialTag(
    std::span<const std::string> tags,
    CreativeStructuralMaterial& material) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMovingPlatformTraversalMode mode) noexcept;
[[nodiscard]] bool parseCreativeMovingPlatformTraversalMode(
    std::string_view value,
    CreativeMovingPlatformTraversalMode& output) noexcept;
[[nodiscard]] bool isValidCreativeMovingPlatformSettings(
    const CreativeMovingPlatformSettings& settings) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDoorLeafArrangement arrangement) noexcept;
[[nodiscard]] std::string_view toString(CreativeDoorHingeSide side) noexcept;
[[nodiscard]] std::string_view toString(CreativeDoorSwingSide side) noexcept;
[[nodiscard]] std::string_view toString(CreativeDoorInitialState state) noexcept;
[[nodiscard]] bool parseCreativeDoorLeafArrangement(
    std::string_view value,
    CreativeDoorLeafArrangement& output) noexcept;
[[nodiscard]] bool parseCreativeDoorHingeSide(
    std::string_view value,
    CreativeDoorHingeSide& output) noexcept;
[[nodiscard]] bool parseCreativeDoorSwingSide(
    std::string_view value,
    CreativeDoorSwingSide& output) noexcept;
[[nodiscard]] bool parseCreativeDoorInitialState(
    std::string_view value,
    CreativeDoorInitialState& output) noexcept;
[[nodiscard]] bool isValidCreativeDoorSettings(
    const CreativeDoorSettings& settings) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWindowInsertKind kind) noexcept;
[[nodiscard]] bool parseCreativeWindowInsertKind(
    std::string_view value,
    CreativeWindowInsertKind& output) noexcept;
[[nodiscard]] bool isValidCreativeWindowSettings(
    const CreativeWindowSettings& settings) noexcept;
[[nodiscard]] bool isValidCreativePlayerProfileId(
    std::string_view value) noexcept;
[[nodiscard]] bool isValidCreativeSpawnGroup(std::string_view value) noexcept;
[[nodiscard]] bool isValidCreativePlayerSpawnSettings(
    const CreativePlayerSpawnSettings& settings) noexcept;
[[nodiscard]] bool isValidCreativePathPoint(
    const CreativePathPoint& point) noexcept;
[[nodiscard]] bool isValidCreativeMovingPlatformPath(
    std::span<const CreativePathPoint> pathPoints) noexcept;
[[nodiscard]] std::string_view serializedObjectKindId(
    CreativeObjectKind kind) noexcept;
[[nodiscard]] bool parseSerializedObjectKindId(
    std::string_view value,
    CreativeObjectKind& out) noexcept;
[[nodiscard]] bool validCreativeAttachmentSocketName(
    std::string_view value) noexcept;
[[nodiscard]] bool validCreativeAssetMaterialVariantName(
    std::string_view value) noexcept;
[[nodiscard]] std::span<const CreativeObjectKind> allCreativeObjectKinds()
    noexcept;
[[nodiscard]] bool isStructuralObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool isTerrainOrVolumeObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool isNavigationOrMovementObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool isLogicObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool isTestingObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool isVisualDressingObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool isLightSoundOrCameraObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool isAuthoringMetaObject(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool isGameplayObject(CreativeObjectKind kind) noexcept;

}  // namespace iggy3d::creative
