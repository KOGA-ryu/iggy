
#pragma once

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

struct CreativeVec3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct CreativeTransform {
  CreativeVec3 position{};
  CreativeVec3 rotation{};
  CreativeVec3 scale{1.0, 1.0, 1.0};
};

struct CreativeBounds {
  CreativeVec3 min{};
  CreativeVec3 max{};
};

struct CreativePathPoint {
  CreativeVec3 position{};
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

  Count
};

struct CreativeObject {
  CreativeObjectId id{kInvalidObjectId};
  CreativeObjectKind kind{CreativeObjectKind::Unknown};
  std::string name{};

  CreativeTransform transform{};
  CreativeBounds bounds{};

  CreativeLayerId layerId{kDefaultLayerId};
  bool visible{true};
  bool locked{false};

  std::vector<std::string> tags{};
  std::optional<CreativeObjectId> parentId{};
  std::vector<CreativePathPoint> pathPoints{};
};

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
[[nodiscard]] std::string_view serializedObjectKindId(
    CreativeObjectKind kind) noexcept;
[[nodiscard]] bool parseSerializedObjectKindId(
    std::string_view value,
    CreativeObjectKind& out) noexcept;
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
