#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
#include "runtime/inventory/InventoryState.hpp"
#include "runtime/objective/ObjectiveState.hpp"
#include "runtime/player/PlayerSlot.hpp"
#include "runtime/replay/CommandLog.hpp"
#include "runtime/session/SessionState.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {

inline constexpr std::uint32_t kSaveSchemaVersion = 3;
inline constexpr std::uint32_t kMinimumReadableSaveSchemaVersion = 1;
inline constexpr std::uint32_t kRuntimeSaveVersion = 1;
inline constexpr std::uint32_t kSaveCreativeDocumentWaypointDwellVersion = 9;
inline constexpr std::uint32_t kSaveCreativeDocumentSegmentSpeedVersion = 10;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainHeightVersion = 11;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainOperationVersion =
    12;
inline constexpr std::uint32_t kSaveCreativeDocumentDoorVersion = 13;
inline constexpr std::uint32_t kSaveCreativeDocumentWindowVersion = 14;
inline constexpr std::uint32_t kSaveCreativeDocumentPatternRecipeVersion = 15;
inline constexpr std::uint32_t
    kSaveCreativeDocumentMeasurementAnnotationVersion = 16;
inline constexpr std::uint32_t kSaveCreativeDocumentAssetIdentityVersion = 17;
inline constexpr std::uint32_t
    kSaveCreativeDocumentTerrainMaterialWeightsVersion = 18;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainGradeVersion = 19;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainPathVersion = 20;
inline constexpr std::uint32_t
    kSaveCreativeDocumentTerrainOperationProvenanceVersion = 21;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainRegionVersion = 22;
inline constexpr std::uint32_t
    kSaveCreativeDocumentTerrainGenerationIntentVersion = 23;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainStampVersion = 24;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainProfileVersion = 25;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainLandformVersion = 26;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainHardEdgeVersion = 27;
inline constexpr std::uint32_t kSaveCreativeDocumentTerrainRoadVersion = 28;
inline constexpr std::uint32_t
    kSaveCreativeDocumentTerrainWatercourseVersion = 29;
inline constexpr std::uint32_t kSaveCreativeDocumentPlayerSpawnVersion = 30;
inline constexpr std::uint32_t kSaveCreativeDocumentNpcSpawnVersion = 31;
inline constexpr std::uint32_t kSaveCreativeDocumentObjectiveVersion = 32;
inline constexpr std::uint32_t kSaveCreativeDocumentSectionVersion =
    kSaveCreativeDocumentObjectiveVersion;
inline constexpr std::uint32_t kSaveCreativeWorldLayoutSectionVersion = 1U;

struct SaveEnvelopeMetadata {
  std::uint32_t schemaVersion = kSaveSchemaVersion;
  std::uint32_t minimumReadableSchemaVersion = kMinimumReadableSaveSchemaVersion;
  std::uint32_t runtimeSaveVersion = kRuntimeSaveVersion;
  std::string packageId;
  std::string scenarioId;
  std::string createdByToolId;
  std::string saveId;
  std::string worldId;
  std::string worldTitle;
  std::string saveTitle;
  std::string saveType;
  std::string createdAtUtc;
  std::string savedAtUtc;
  std::uint64_t savedStateHash = 0;
  std::string savedStateHashHex;
};

struct SaveSessionSection {
  SessionLifecycle lifecycle = SessionLifecycle::Loading;
  SessionOutcome outcome = SessionOutcome::None;
  std::uint64_t currentTick = 0;
  CommandId nextCommandId = 1;
  std::uint64_t sessionSeed = 0;
  std::uint32_t sessionSchemaVersion = 1;
  std::uint32_t fixedTickRateHz = 20;
  float interactionRangeMeters = 1.500F;
  float movementDistanceMeters = 3.000F;
  float slowTimeScale = 0.250F;
  std::string packageId;
  std::string scenarioId;
};

struct SaveEntityRecord {
  EntityId id;
  std::string stableName;
  EntityKind kind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 localBounds;
  bool active = true;
  bool persistent = true;
  bool targetable = false;
  std::vector<TargetAction> targetActions;
  InteractionKind interactionKind = InteractionKind::None;
  InteractionEffectKind interactionPrimaryEffect = InteractionEffectKind::None;
  std::string interactionItemId;
  std::uint32_t interactionItemCount = 0;
  std::string interactionObjectiveId;
  std::string interactionRequiredItemId;
  std::uint32_t interactionRequiredItemCount = 0;
  bool interactionRepeatable = false;
  bool interactionDeactivateTargetOnSuccess = false;
};

struct SaveWorldSection {
  EntityId nextEntityId;
  std::vector<SaveEntityRecord> entities;
};

struct SaveAuthoredRoomSemanticsRecord {
  std::string materialId;
  std::vector<std::string> traversalTags;
  std::vector<std::string> gameplayTags;
  bool walkable = false;
  bool blocksActor = false;
  bool blocksProjectile = false;
};

struct SaveAuthoredRoomFloorRecord {
  std::string id;
  std::int32_t storyIndex = 0;
  Vec3 centerMeters;
  Vec3 sizeMeters = {1.0F, 0.10F, 1.0F};
  SaveAuthoredRoomSemanticsRecord semantics;
  bool locked = false;
  bool hidden = false;
};

struct SaveAuthoredRoomWallRecord {
  std::string id;
  std::int32_t storyIndex = 0;
  Vec3 startMeters;
  Vec3 endMeters;
  float bottomY = 0.0F;
  float heightMeters = 2.0F;
  float thicknessMeters = 0.20F;
  SaveAuthoredRoomSemanticsRecord semantics;
  bool locked = false;
  bool hidden = false;
};

struct SaveAuthoredRoomMarkerRecord {
  std::string id;
  std::string tag;
  std::string glyph;
  std::uint32_t row = 0;
  std::uint32_t column = 0;
  Vec3 positionMeters;
  std::uint32_t sourceLine = 1;
  std::uint32_t sourceColumn = 1;
};

struct SaveAuthoredRoomObjectRecord {
  std::string id;
  std::string assetId;
  std::int32_t storyIndex = 0;
  Vec3 positionMeters;
  Vec3 sizeMeters = {0.8F, 0.8F, 0.8F};
  float yawDegrees = 0.0F;
  SaveAuthoredRoomSemanticsRecord semantics;
  bool locked = false;
  bool hidden = false;
  std::string glyph;
  std::uint32_t row = 0;
  std::uint32_t column = 0;
  std::uint32_t sourceLine = 1;
  std::uint32_t sourceColumn = 1;
};

struct SaveAuthoredRoomSection {
  bool present = false;
  std::string id = "editable_room";
  std::uint32_t version = 1;
  std::string source = "iggy3d.editor";
  std::string sourceFile = "editable_room";
  std::string sourceSubset = "authoring";
  std::vector<SaveAuthoredRoomFloorRecord> floors;
  std::vector<SaveAuthoredRoomWallRecord> walls;
  std::vector<SaveAuthoredRoomObjectRecord> objects;
  std::vector<SaveAuthoredRoomMarkerRecord> markers;
};

struct SaveCreativeDocumentVec3Record {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct SaveCreativeDocumentPathPointRecord {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double dwellSeconds = 0.0;
  double outgoingSpeedMultiplier = 1.0;
};

struct SaveCreativeDocumentTransformRecord {
  SaveCreativeDocumentVec3Record position;
  // Intrinsic X-then-Y-then-Z Euler radians; key remains transform.rotation.
  SaveCreativeDocumentVec3Record rotation;
  SaveCreativeDocumentVec3Record scale{1.0, 1.0, 1.0};
};

struct SaveCreativeDocumentBoundsRecord {
  SaveCreativeDocumentVec3Record min;
  SaveCreativeDocumentVec3Record max;
};

struct SaveCreativeDocumentObjectRecord {
  std::uint64_t id = 0;
  std::string kind;
  std::string name;
  std::string assetId;
  std::uint64_t assetContentHash = 0U;
  std::string assetMaterialVariant;
  SaveCreativeDocumentTransformRecord transform;
  SaveCreativeDocumentBoundsRecord bounds;
  std::uint64_t layerId = 0;
  bool visible = true;
  bool locked = false;
  bool hasParent = false;
  std::uint64_t parentId = 0;
  std::string attachmentSocket;
  std::vector<std::string> tags;
  std::vector<SaveCreativeDocumentPathPointRecord> pathPoints;
  double movingPlatformSpeedMetersPerSecond = 1.5;
  std::string movingPlatformTraversalMode = "PingPong";
  bool movingPlatformStartsActive = true;
  std::string doorLeafArrangement = "Single";
  std::string doorHingeSide = "MinimumEdge";
  std::string doorSwingSide = "PositiveNormal";
  std::string doorInitialState = "Closed";
  bool doorGameplayLocked = false;
  double doorTransitionSeconds = 0.35;
  std::string windowInsertKind = "Glazing";
  std::string playerSpawnProfileId = "default";
  std::string playerSpawnGroup = "default";
  double playerSpawnValidationRadiusMeters = 0.45;
  std::uint16_t playerSpawnFallbackPriority = 0U;
  std::string npcBehaviorProfileId;
  std::string npcTeam = "ActorDefault";
  std::uint16_t npcHitPoints = 0U;
  double npcInitialAlertLevel = 0.0;
  std::string npcSpawnPolicy = "AtPlayStart";
  std::string lootItemId;
  std::uint32_t lootItemCount = 1U;
  bool lootDeactivateOnCollect = true;
  std::string exitRequiredItemId;
  std::uint32_t exitRequiredItemCount = 0U;
};

struct SaveCreativeDocumentLogicLinkRecord {
  std::uint64_t sourceObjectId = 0;
  std::uint64_t targetObjectId = 0;
  std::string action;
};

struct SaveCreativeDocumentVoxelCellRecord {
  std::uint16_t localIndex = 0;
  std::string material;
};

struct SaveCreativeDocumentVoxelChunkRecord {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t z = 0;
  std::vector<SaveCreativeDocumentVoxelCellRecord> cells;
};

struct SaveCreativeDocumentTerrainControlRecord {
  std::int32_t x = 0;
  std::int32_t z = 0;
  std::uint16_t heightCells = 1;
  std::uint16_t radiusCells = 1;
};

struct SaveCreativeDocumentTerrainHeightFieldRecord {
  bool present = false;
  std::int32_t minimumX = 0;
  std::int32_t minimumZ = 0;
  std::uint16_t widthCells = 0;
  std::uint16_t depthCells = 0;
  std::vector<std::uint16_t> heights;
};

struct SaveCreativeDocumentTerrainPathPointRecord {
  std::uint32_t id = 0U;
  std::int32_t x = 0;
  std::int32_t z = 0;
  std::uint16_t heightCells = 4U;
  std::uint16_t halfWidthCells = 1U;
  std::uint16_t amplitudeCells = 0U;
  std::int32_t bankPermille = 0;
};

struct SaveCreativeDocumentTerrainProtectedRegionRecord {
  std::int32_t minimumX = 0;
  std::int32_t minimumZ = 0;
  std::uint16_t widthCells = 1U;
  std::uint16_t depthCells = 1U;
  std::string mask = "Rectangle";
};

struct SaveCreativeDocumentTerrainWatercourseCrossingRecord {
  std::uint32_t id = 0U;
  std::uint32_t pointId = 0U;
  std::uint16_t bankClearanceCells = 1U;
  std::uint16_t deckClearanceCells = 1U;
  std::uint16_t approachLengthCells = 2U;
};

struct SaveCreativeDocumentTerrainMaterialRecord;

struct SaveCreativeDocumentTerrainOperationRecord {
  std::uint64_t id = 0U;
  bool enabled = true;
  std::string owner = "Manual";
  std::string sourceKey;
  std::string operationKind = "GeneratedTerrain";
  std::uint32_t generationVersion = 1U;
  std::string generatorKind;
  std::uint64_t seed = 1U;
  std::int32_t minimumX = 0;
  std::int32_t minimumZ = 0;
  std::uint16_t widthCells = 0U;
  std::uint16_t depthCells = 0U;
  std::uint16_t baseHeightCells = 8U;
  std::uint16_t reliefCells = 6U;
  double horizontalScaleCells = 24.0;
  std::uint8_t octaveCount = 5U;
  double persistence = 0.5;
  double lacunarity = 2.0;
  double slopeDamping = 0.35;
  bool paintMaterials = true;
  std::string biomeIntent = "Temperate";
  std::string lowlandMaterial = "Grass";
  std::string highlandMaterial = "Stone";
  std::uint16_t materialTransitionHeightCells = 11U;
  std::uint32_t compositionVersion = 1U;
  std::string mask;
  std::string mode;
  std::uint16_t featherCells = 4U;
  std::vector<SaveCreativeDocumentTerrainProtectedRegionRecord>
      protectedRegions;
  std::uint32_t regionVersion = 1U;
  std::int32_t regionMinimumX = 0;
  std::int32_t regionMinimumZ = 0;
  std::uint16_t regionWidthCells = 1U;
  std::uint16_t regionDepthCells = 1U;
  std::string regionMask = "Rectangle";
  std::string regionMode = "Raise";
  std::uint16_t regionAmountCells = 1U;
  std::uint16_t regionTargetHeightCells = 4U;
  std::uint16_t regionNoiseReliefCells = 4U;
  double regionNoiseScaleCells = 12.0;
  std::uint16_t regionFeatherCells = 0U;
  std::uint64_t regionSeed = 1U;
  std::uint32_t gradeVersion = 1U;
  std::int32_t gradeStartX = 0;
  std::int32_t gradeStartZ = 0;
  std::int32_t gradeEndX = 1;
  std::int32_t gradeEndZ = 0;
  std::uint16_t gradeStartHeightCells = 4U;
  std::uint16_t gradeEndHeightCells = 4U;
  std::uint16_t gradeHalfWidthCells = 2U;
  std::int32_t gradeCrossSlopePermille = 0;
  std::uint16_t gradeFalloffCells = 2U;
  std::uint32_t pathVersion = 1U;
  std::string pathKind = "ROAD";
  std::string pathElevation = "GRADE";
  std::string pathCurve = "LINEAR";
  std::string pathCrossSection = "FLAT";
  std::string pathStartJoin = "OPEN";
  std::string pathEndJoin = "OPEN";
  std::uint16_t pathFalloffCells = 2U;
  bool pathPaintSurface = true;
  std::string pathMaterial = "Dirt";
  std::uint16_t pathRoadShoulderWidthCells = 0U;
  std::uint16_t pathRoadMaximumGradePermille = 0U;
  std::uint8_t pathRoadEdgeTreatment = 0U;
  double pathRoadEdgeWidthMeters = 0.15;
  double pathRoadEdgeHeightMeters = 0.15;
  std::uint8_t pathRoadEdgeMaterial = 3U;
  std::uint16_t pathWatercourseBankSlopeCells = 0U;
  std::uint8_t pathWatercourseDrainageDirection = 0U;
  std::uint8_t pathWatercourseSurfacePolicy = 0U;
  std::uint16_t pathWatercourseSurfaceInsetCells = 1U;
  std::uint32_t pathWatercourseNextCrossingId = 1U;
  std::vector<SaveCreativeDocumentTerrainWatercourseCrossingRecord>
      pathWatercourseCrossings;
  std::uint32_t pathNextPointId = 1U;
  std::vector<SaveCreativeDocumentTerrainPathPointRecord> pathPoints;
  std::uint32_t stampRecipeVersion = 1U;
  std::uint32_t stampVersion = 2U;
  std::string stampAssetId;
  std::string stampLabel;
  std::uint64_t stampAssetVersion = 1U;
  std::uint64_t stampSourceDocumentId = 0U;
  std::uint64_t stampSourceRevision = 0U;
  std::uint64_t stampContentSignature = 0U;
  std::int32_t stampSourceMinimumX = 0;
  std::int32_t stampSourceMinimumZ = 0;
  std::uint16_t stampMinimumHeightCells = 0U;
  SaveCreativeDocumentTerrainHeightFieldRecord stampHeightField;
  std::vector<SaveCreativeDocumentTerrainMaterialRecord> stampMaterials;
  std::int32_t stampTargetMinimumX = 0;
  std::int32_t stampTargetMinimumZ = 0;
  std::uint8_t stampQuarterTurns = 0U;
  bool stampMirrorX = false;
  bool stampMirrorZ = false;
  std::string stampMode = "MERGE";
  std::string stampElevation = "SURFACE";
  std::int16_t stampManualHeightOffsetCells = 0;
  std::uint32_t profileVersion = 1U;
  std::string profileKind = "HILL";
  std::string profileBlend = "SET";
  std::string profileRodPolicy = "FILL";
  std::int32_t profileCenterX = 0;
  std::int32_t profileCenterZ = 0;
  std::uint16_t profileBaseHeightCells = 4U;
  std::uint16_t profileRadiusCells = 4U;
  std::uint16_t profileAmplitudeCells = 4U;
  std::uint16_t profileSpacingCells = 1U;
  std::string profileDirection = "+X";
  std::uint8_t profileFrequency = 1U;
  std::uint64_t profileSeed = 0U;
  std::uint32_t landformVersion = 1U;
  std::string landformKind = "PLATEAU";
  std::int32_t landformMinimumX = 0;
  std::int32_t landformMinimumZ = 0;
  std::uint16_t landformWidthCells = 8U;
  std::uint16_t landformDepthCells = 8U;
  std::uint16_t landformBaseHeightCells = 1U;
  std::uint16_t landformTargetHeightCells = 4U;
  std::uint8_t landformTerraceCount = 4U;
  std::string landformDirection = "POSITIVE_X";
  std::string landformEdge = "SLOPE";
  std::uint16_t landformEdgeWidthCells = 2U;
  std::uint16_t landformFeatherCells = 0U;
  bool landformPaintSurface = true;
  std::string landformMaterial = "Grass";
  std::string landformErosion = "CLEAN";
  std::uint16_t landformErosionReliefCells = 0U;
  std::uint64_t landformSeed = 1U;
};

struct SaveCreativeDocumentPatternRecipeRecord {
  std::uint64_t id = 0U;
  std::string kind;
  std::vector<std::uint64_t> sourceObjectIds;
  std::vector<std::uint64_t> generatedObjectIds;
  std::string linearDirection;
  std::string linearCopyCount;
  std::string linearSpacing;
  double linearCellSize = 1.0;
  std::uint64_t linearMaxGeneratedObjects = 512U;
  SaveCreativeDocumentVec3Record radialPivot;
  std::string radialAxis;
  std::string radialInstanceCount;
  std::string radialSweep;
  std::uint64_t radialMaxGeneratedObjects = 512U;
  std::string scatterObjectKind;
  std::string scatterAssetId;
  std::uint64_t scatterAssetContentHash = 0U;
  std::string scatterAssetMaterialVariant;
  SaveCreativeDocumentBoundsRecord scatterAssetSourceBounds;
  std::vector<SaveCreativeDocumentVec3Record> scatterPaintCenters;
  struct Exclusion {
    SaveCreativeDocumentVec3Record center;
    double radiusMeters = 1.0;
  };
  std::vector<Exclusion> scatterExclusions;
  std::string scatterMask;
  std::string scatterYaw;
  double scatterBaseYawRadians = 0.0;
  double scatterRadiusMeters = 4.0;
  double scatterSpacingMeters = 2.0;
  double scatterDensityFraction = 0.65;
  double scatterScaleVariation = 0.10;
  double scatterMaximumSlopeRadians = 0.5235987755982988;
  bool scatterProjectToTerrainSurface = false;
  bool scatterAvoidCollisions = true;
  std::uint64_t scatterSeed = 0U;
  std::uint64_t scatterMaxGeneratedObjects = 512U;
};

struct SaveCreativeDocumentMeasurementAnnotationPointRecord {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  std::string snapKind;
};

struct SaveCreativeDocumentMeasurementAnnotationRecord {
  std::uint64_t id = 0U;
  std::string name;
  std::string mode;
  std::string axis;
  bool closePath = false;
  std::vector<SaveCreativeDocumentMeasurementAnnotationPointRecord> points;
};

struct SaveCreativeDocumentTerrainMaterialRecord {
  std::int32_t x = 0;
  std::int32_t z = 0;
  std::string material;
  bool hasWeights = false;
  std::uint16_t grassWeight = 0U;
  std::uint16_t dirtWeight = 0U;
  std::uint16_t stoneWeight = 0U;
  std::uint16_t sandWeight = 0U;
};

struct SaveCreativeDocumentTerrainHardEdgeRecord {
  std::int32_t firstX = 0;
  std::int32_t firstZ = 0;
  std::int32_t secondX = 0;
  std::int32_t secondZ = 0;
};

struct SaveCreativeDocumentSection {
  bool present = false;
  std::uint32_t version = kSaveCreativeDocumentSectionVersion;
  std::uint64_t documentId = 0;
  std::string name;
  std::string units = "Meters";
  SaveCreativeDocumentVec3Record gridOrigin;
  double cellSizeMeters = 1.0;
  std::uint32_t gridWidth = 0;
  std::uint32_t gridHeight = 0;
  std::uint32_t gridDepth = 0;
  std::string snapMode = "Disabled";
  std::uint32_t snapAxes = 0;
  double snapStepX = 1.0;
  double snapStepY = 1.0;
  double snapStepZ = 1.0;
  double snapOriginX = 0.0;
  double snapOriginY = 0.0;
  double snapOriginZ = 0.0;
  SaveCreativeDocumentBoundsRecord worldBounds;
  std::uint64_t nextObjectId = 1;
  std::vector<SaveCreativeDocumentObjectRecord> objects;
  std::vector<SaveCreativeDocumentLogicLinkRecord> logicLinks;
  std::vector<SaveCreativeDocumentVoxelChunkRecord> voxelChunks;
  std::vector<SaveCreativeDocumentTerrainControlRecord> terrainControls;
  SaveCreativeDocumentTerrainHeightFieldRecord terrainHeightField;
  std::vector<SaveCreativeDocumentTerrainHardEdgeRecord> terrainHardEdges;
  std::uint32_t terrainOperationStackVersion = 1U;
  std::uint64_t nextTerrainOperationId = 1U;
  SaveCreativeDocumentTerrainHeightFieldRecord terrainOperationBaseHeightField;
  std::vector<SaveCreativeDocumentTerrainHardEdgeRecord>
      terrainOperationBaseHardEdges;
  std::vector<SaveCreativeDocumentTerrainMaterialRecord>
      terrainOperationBaseMaterials;
  std::vector<SaveCreativeDocumentTerrainOperationRecord> terrainOperations;
  std::uint32_t patternRecipeStoreVersion = 2U;
  std::uint64_t nextPatternRecipeId = 1U;
  std::vector<SaveCreativeDocumentPatternRecipeRecord> patternRecipes;
  std::uint32_t measurementAnnotationStoreVersion = 1U;
  std::uint64_t nextMeasurementAnnotationId = 1U;
  std::vector<SaveCreativeDocumentMeasurementAnnotationRecord>
      measurementAnnotations;
  std::vector<SaveCreativeDocumentTerrainMaterialRecord> terrainMaterials;
};

// The 2D semantic authoring source is encoded by the Creative world-layout
// codec. Keeping it inside the same envelope makes document + source one
// atomic durable write without coupling the runtime save layer to Creative
// layout record types.
struct SaveCreativeWorldLayoutSection {
  bool present = false;
  std::uint32_t version = kSaveCreativeWorldLayoutSectionVersion;
  std::string encodedText;
};

struct SavePlayerSlotRecord {
  PlayerSlotId slotId = kInvalidPlayerSlotId;
  PlayerSlotKind kind = PlayerSlotKind::Unknown;
  EntityId controlledActor;
  std::string stableName;
};

struct SavePlayerSection {
  std::vector<SavePlayerSlotRecord> slots;
};

struct SaveClockSection {
  ClockMode mode = ClockMode::Normal;
  ClockMode previousUnpausedMode = ClockMode::Normal;
  float previousUnpausedTimeScale = 1.0F;
  std::uint64_t tickIndex = 0;
  std::uint32_t fixedTickRateHz = 20;
  float timeScale = 1.0F;
};

struct SaveCameraSection {
  CameraMode activeMode = CameraMode::ThirdPerson;
  CameraMode previousRealtimeMode = CameraMode::ThirdPerson;
  EntityId targetEntity;
  Vec3 targetPoint;
  bool targetHasPoint = false;
  float yawDegrees = 0.0F;
  float pitchDegrees = 0.0F;
  float orbitDistance = 8.0F;
};

struct SaveCommandRecord {
  CommandId commandId = kInvalidCommandId;
  CommandSequence sequence = kInvalidCommandSequence;
  CommandKind kind = CommandKind::None;
  CommandSource source = CommandSource::Unknown;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  EntityId actor;
  bool hasTargetEntity = false;
  EntityId targetEntity;
  bool hasTargetPoint = false;
  Vec3 targetPoint;
  CommandId retrySourceCommandId = kInvalidCommandId;
  std::int32_t attackDamage = 0;
  CommandAbilityKind ability = CommandAbilityKind::None;
  Vec3 abilityDirection;
  CommandTick issuedTick = kInvalidCommandTick;
  CommandTick scheduledTick = kInvalidCommandTick;
  CommandAdmissionStatus admission = CommandAdmissionStatus::Pending;
  CommandRejectionReason rejection = CommandRejectionReason::None;
};

struct SaveCommandLogSection {
  CommandLogResetPolicy resetPolicy = CommandLogResetPolicy::Clear;
  CommandSequence nextSequence = 1;
  std::uint64_t epoch = 0;
  std::vector<SaveCommandRecord> records;
};

struct SaveAbilityActorRecord {
  EntityId actor;
  std::uint32_t arcaneFocus = 0;
  CommandTick arcaneBoltReadyTick = 0;
  CommandTick arcaneFocusNextRechargeTick = 0;
};

struct SaveAbilitySection {
  std::vector<SaveAbilityActorRecord> actors;
};

struct SaveInventoryStackRecord {
  std::string itemId;
  std::uint32_t count = 0;
};

struct SavePlayerInventoryRecord {
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  std::vector<SaveInventoryStackRecord> stacks;
};

struct SaveInventorySection {
  std::vector<SavePlayerInventoryRecord> players;
};

struct SaveCombatantRecord {
  EntityId entity;
  std::uint32_t factionId = 0;
  std::int32_t hitPoints = 0;
  std::int32_t maxHitPoints = 0;
  bool defeated = false;
};

struct SaveCombatSection {
  std::vector<SaveCombatantRecord> combatants;
};

struct SaveAiActorRecord {
  EntityId actor;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t deterministicPolicy = 0;
  bool enabled = true;
  std::string behaviorProfileId = "default";
  EntityId target;
  AiBehaviorKind behavior = AiBehaviorKind::Idle;
  AiIntentKind lastIntent = AiIntentKind::None;
  std::uint32_t cooldownTicksRemaining = 0;
  bool hasHomePosition = false;
  Vec3 homePosition;
  std::string homeStableName;
  float leashRadiusMeters = 0.0F;
  float returnRadiusMeters = 0.0F;
  float homeToleranceMeters = 0.0F;
  // Patrol route + cursor (a2 commit 1). Defaults match AiActorState (empty route, Loop, 0, true).
  std::vector<Vec3> patrolWaypoints;
  PatrolMode patrolMode = PatrolMode::Loop;
  std::uint32_t patrolTargetIndex = 0;
  bool patrolForward = true;
  // Alert FSM + last-known memory + facing (a2 commit 2). Defaults match AiActorState.
  float alertLevel = 0.0F;
  std::uint64_t lastRiseTick = 0;
  std::uint8_t maxAlertIndexThisEngagement = 0;
  std::uint64_t graceUntilTick = 0;
  float graceThreshold = 0.0F;
  std::uint32_t graceCount = 0;
  Vec3 lastKnownTargetPosition{};
  std::uint64_t lastKnownTargetTick = 0;
  bool hasLastKnownTarget = false;
  std::uint32_t investigateDwellTicks = 0;
  Vec3 facingDirection{0.0F, 0.0F, 1.0F};
};

struct SaveAiSection {
  std::vector<SaveAiActorRecord> actors;
};

struct SaveObjectiveRecord {
  std::string objectiveId;
  ObjectiveStatus status = ObjectiveStatus::Inactive;
  ObjectiveConditionKind conditionKind = ObjectiveConditionKind::None;
  PlayerSlotId conditionPlayerSlot = kInvalidPlayerSlotId;
  std::string conditionItemId;
  std::uint32_t conditionItemCount = 0;
};

struct SaveObjectiveSection {
  std::vector<SaveObjectiveRecord> objectives;
};

struct SaveEnvelope {
  SaveEnvelopeMetadata metadata;
  SaveSessionSection session;
  SaveWorldSection world;
  SaveAuthoredRoomSection authoredRoom;
  SaveCreativeDocumentSection creativeDocument;
  SaveCreativeWorldLayoutSection creativeWorldLayout;
  SavePlayerSection players;
  SaveClockSection clock;
  SaveCameraSection camera;
  SaveAbilitySection abilities;
  SaveCommandLogSection commandLog;
  SaveInventorySection inventory;
  SaveCombatSection combat;
  SaveAiSection ai;
  SaveObjectiveSection objectives;
};

}  // namespace iggy3d
