#include "runtime/save/SaveCodec.hpp"

#include <array>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "runtime/save/SaveCodecEnumTables.hpp"
#include "runtime/save/SaveCodecFormat.hpp"

namespace iggy3d {

namespace {

std::string escapeString(const std::string& value) {
  std::string out;
  for (char c : value) {
    switch (c) {
      case '%':
        out += "%25";
        break;
      case '\n':
        out += "%0A";
        break;
      case '\r':
        out += "%0D";
        break;
      case '=':
        out += "%3D";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

std::string formatFloat(float value) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(3) << value;
  return out.str();
}

std::string formatVec3(Vec3 value) {
  return formatFloat(value.x) + "," + formatFloat(value.y) + "," + formatFloat(value.z);
}

// Shortest round-trip float text (std::to_chars). Used ONLY for the AI stealth-state fields
// (a2): unlike the fixed-3-decimal formatFloat, this loses no precision, so e.g. a partially
// drained alertLevel reloads to the EXACT value and post-load band-crossing ticks match an
// unreloaded run (the read side, std::strtof, is already lossless). Do NOT use for the legacy
// fields — they stay on formatFloat so their bytes are unchanged.
std::string formatFloatLossless(float value) {
  std::array<char, 32> buffer{};
  const std::to_chars_result result =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  return std::string(buffer.data(), result.ptr);
}

std::string formatVec3Lossless(Vec3 value) {
  return formatFloatLossless(value.x) + "," + formatFloatLossless(value.y) + "," +
         formatFloatLossless(value.z);
}

std::string formatDoubleLossless(double value) {
  std::array<char, 64> buffer{};
  const std::to_chars_result result =
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
  if (result.ec == std::errc{}) {
    return std::string(buffer.data(), result.ptr);
  }
  std::ostringstream out;
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
  return out.str();
}

std::string formatCreativeVec3(const SaveCreativeDocumentVec3Record& value) {
  return formatDoubleLossless(value.x) + "," + formatDoubleLossless(value.y) + "," +
         formatDoubleLossless(value.z);
}

template <typename T>
std::string unsignedText(T value) {
  return std::to_string(static_cast<std::uint64_t>(value));
}

class Writer {
public:
  explicit Writer(const SaveEnvelope& envelope) : envelope_(envelope) {}

  SaveEncodeResult finish() {
    out_ << save_codec_detail::kEnvelopeHeader << '\n';
    writeMetadata();
    writeSession();
    writeWorld();
    writeAuthoredRoom();
    writeCreativeDocument();
    writeCreativeWorldLayout();
    writePlayers();
    writeClock();
    writeCamera();
    writeAbilities();
    writeCommandLog();
    writeInventory();
    writeCombat();
    writeAi();
    writeObjectives();
    SaveEncodeResult result;
    result.encodedText = out_.str();
    result.savedStateHash = envelope_.metadata.savedStateHash;
    return result;
  }

private:
  void line(const std::string& key, const std::string& value) {
    out_ << key << '=' << value << '\n';
  }
  void lineString(const std::string& key, const std::string& value) {
    line(key, escapeString(value));
  }
  void lineBool(const std::string& key, bool value) {
    line(key, value ? "true" : "false");
  }
  template <typename Enum>
  void lineEnum(const std::string& key, Enum value) {
    line(key, save_codec_detail::enumText(value));
  }
  void lineCreativeVec3(const std::string& key,
                        const SaveCreativeDocumentVec3Record& value) {
    line(key, formatCreativeVec3(value));
  }

  void writeMetadata() {
    line("metadata.schemaVersion", unsignedText(envelope_.metadata.schemaVersion));
    line("metadata.minimumReadableSchemaVersion", unsignedText(envelope_.metadata.minimumReadableSchemaVersion));
    line("metadata.runtimeSaveVersion", unsignedText(envelope_.metadata.runtimeSaveVersion));
    lineString("metadata.packageId", envelope_.metadata.packageId);
    lineString("metadata.scenarioId", envelope_.metadata.scenarioId);
    lineString("metadata.createdByToolId", envelope_.metadata.createdByToolId);
    lineString("metadata.saveId", envelope_.metadata.saveId);
    lineString("metadata.worldId", envelope_.metadata.worldId);
    lineString("metadata.worldTitle", envelope_.metadata.worldTitle);
    lineString("metadata.saveTitle", envelope_.metadata.saveTitle);
    lineString("metadata.saveType", envelope_.metadata.saveType);
    lineString("metadata.createdAtUtc", envelope_.metadata.createdAtUtc);
    lineString("metadata.savedAtUtc", envelope_.metadata.savedAtUtc);
    line("metadata.savedStateHash", unsignedText(envelope_.metadata.savedStateHash));
    lineString("metadata.savedStateHashHex", envelope_.metadata.savedStateHashHex);
  }

  void writeSession() {
    lineEnum("session.lifecycle", envelope_.session.lifecycle);
    lineEnum("session.outcome", envelope_.session.outcome);
    line("session.currentTick", unsignedText(envelope_.session.currentTick));
    line("session.nextCommandId", unsignedText(envelope_.session.nextCommandId));
    line("session.sessionSeed", unsignedText(envelope_.session.sessionSeed));
    line("session.sessionSchemaVersion", unsignedText(envelope_.session.sessionSchemaVersion));
    line("session.fixedTickRateHz", unsignedText(envelope_.session.fixedTickRateHz));
    line("session.interactionRangeMeters", formatFloat(envelope_.session.interactionRangeMeters));
    line("session.movementDistanceMeters", formatFloat(envelope_.session.movementDistanceMeters));
    line("session.slowTimeScale", formatFloat(envelope_.session.slowTimeScale));
    lineString("session.packageId", envelope_.session.packageId);
    lineString("session.scenarioId", envelope_.session.scenarioId);
  }

  void writeWorld() {
    line("world.nextEntityId", unsignedText(toUint64(envelope_.world.nextEntityId)));
    line("world.entity.count", unsignedText(envelope_.world.entities.size()));
    for (std::size_t index = 0; index < envelope_.world.entities.size(); ++index) {
      const SaveEntityRecord& entity = envelope_.world.entities[index];
      const std::string p = "world.entity." + std::to_string(index) + ".";
      line(p + "id", unsignedText(toUint64(entity.id)));
      lineString(p + "stableName", entity.stableName);
      lineEnum(p + "kind", entity.kind);
      line(p + "transform.position", formatVec3(entity.transform.position));
      line(p + "transform.rotation", formatVec3(entity.transform.rotationEulerRadians));
      line(p + "transform.scale", formatVec3(entity.transform.scale));
      line(p + "localBounds.min", formatVec3(entity.localBounds.min));
      line(p + "localBounds.max", formatVec3(entity.localBounds.max));
      lineBool(p + "active", entity.active);
      lineBool(p + "persistent", entity.persistent);
      lineBool(p + "targetable", entity.targetable);
      line(p + "targetAction.count", unsignedText(entity.targetActions.size()));
      for (std::size_t action = 0; action < entity.targetActions.size(); ++action) {
        lineEnum(p + "targetAction." + std::to_string(action), entity.targetActions[action]);
      }
      lineEnum(p + "interactionKind", entity.interactionKind);
      lineEnum(p + "interactionPrimaryEffect", entity.interactionPrimaryEffect);
      lineString(p + "interactionItemId", entity.interactionItemId);
      line(p + "interactionItemCount", unsignedText(entity.interactionItemCount));
      lineString(p + "interactionObjectiveId", entity.interactionObjectiveId);
      lineString(p + "interactionRequiredItemId", entity.interactionRequiredItemId);
      line(p + "interactionRequiredItemCount", unsignedText(entity.interactionRequiredItemCount));
      lineBool(p + "interactionRepeatable", entity.interactionRepeatable);
      lineBool(p + "interactionDeactivateTargetOnSuccess", entity.interactionDeactivateTargetOnSuccess);
    }
  }

  void writeAuthoredRoomSemantics(const std::string& p,
                                  const SaveAuthoredRoomSemanticsRecord& semantics) {
    lineString(p + "materialId", semantics.materialId);
    line(p + "traversalTag.count", unsignedText(semantics.traversalTags.size()));
    for (std::size_t index = 0; index < semantics.traversalTags.size(); ++index) {
      lineString(p + "traversalTag." + std::to_string(index), semantics.traversalTags[index]);
    }
    line(p + "gameplayTag.count", unsignedText(semantics.gameplayTags.size()));
    for (std::size_t index = 0; index < semantics.gameplayTags.size(); ++index) {
      lineString(p + "gameplayTag." + std::to_string(index), semantics.gameplayTags[index]);
    }
    lineBool(p + "walkable", semantics.walkable);
    lineBool(p + "blocksActor", semantics.blocksActor);
    lineBool(p + "blocksProjectile", semantics.blocksProjectile);
  }

  void writeAuthoredRoom() {
    if (!envelope_.authoredRoom.present) {
      return;
    }
    lineBool("authoredRoom.present", true);
    lineString("authoredRoom.id", envelope_.authoredRoom.id);
    line("authoredRoom.version", unsignedText(envelope_.authoredRoom.version));
    lineString("authoredRoom.source", envelope_.authoredRoom.source);
    lineString("authoredRoom.sourceFile", envelope_.authoredRoom.sourceFile);
    lineString("authoredRoom.sourceSubset", envelope_.authoredRoom.sourceSubset);
    line("authoredRoom.floor.count", unsignedText(envelope_.authoredRoom.floors.size()));
    for (std::size_t index = 0; index < envelope_.authoredRoom.floors.size(); ++index) {
      const SaveAuthoredRoomFloorRecord& floor = envelope_.authoredRoom.floors[index];
      const std::string p = "authoredRoom.floor." + std::to_string(index) + ".";
      lineString(p + "id", floor.id);
      line(p + "storyIndex", std::to_string(floor.storyIndex));
      line(p + "centerMeters", formatVec3(floor.centerMeters));
      line(p + "sizeMeters", formatVec3(floor.sizeMeters));
      writeAuthoredRoomSemantics(p + "semantics.", floor.semantics);
      lineBool(p + "locked", floor.locked);
      lineBool(p + "hidden", floor.hidden);
    }
    line("authoredRoom.wall.count", unsignedText(envelope_.authoredRoom.walls.size()));
    for (std::size_t index = 0; index < envelope_.authoredRoom.walls.size(); ++index) {
      const SaveAuthoredRoomWallRecord& wall = envelope_.authoredRoom.walls[index];
      const std::string p = "authoredRoom.wall." + std::to_string(index) + ".";
      lineString(p + "id", wall.id);
      line(p + "storyIndex", std::to_string(wall.storyIndex));
      line(p + "startMeters", formatVec3(wall.startMeters));
      line(p + "endMeters", formatVec3(wall.endMeters));
      line(p + "bottomY", formatFloat(wall.bottomY));
      line(p + "heightMeters", formatFloat(wall.heightMeters));
      line(p + "thicknessMeters", formatFloat(wall.thicknessMeters));
      writeAuthoredRoomSemantics(p + "semantics.", wall.semantics);
      lineBool(p + "locked", wall.locked);
      lineBool(p + "hidden", wall.hidden);
    }
    line("authoredRoom.object.count", unsignedText(envelope_.authoredRoom.objects.size()));
    for (std::size_t index = 0; index < envelope_.authoredRoom.objects.size(); ++index) {
      const SaveAuthoredRoomObjectRecord& object = envelope_.authoredRoom.objects[index];
      const std::string p = "authoredRoom.object." + std::to_string(index) + ".";
      lineString(p + "id", object.id);
      lineString(p + "assetId", object.assetId);
      line(p + "storyIndex", std::to_string(object.storyIndex));
      line(p + "positionMeters", formatVec3(object.positionMeters));
      line(p + "sizeMeters", formatVec3(object.sizeMeters));
      line(p + "yawDegrees", formatFloat(object.yawDegrees));
      writeAuthoredRoomSemantics(p + "semantics.", object.semantics);
      lineBool(p + "locked", object.locked);
      lineBool(p + "hidden", object.hidden);
      lineString(p + "glyph", object.glyph);
      line(p + "row", unsignedText(object.row));
      line(p + "column", unsignedText(object.column));
      line(p + "sourceLine", unsignedText(object.sourceLine));
      line(p + "sourceColumn", unsignedText(object.sourceColumn));
    }
    line("authoredRoom.marker.count", unsignedText(envelope_.authoredRoom.markers.size()));
    for (std::size_t index = 0; index < envelope_.authoredRoom.markers.size(); ++index) {
      const SaveAuthoredRoomMarkerRecord& marker = envelope_.authoredRoom.markers[index];
      const std::string p = "authoredRoom.marker." + std::to_string(index) + ".";
      lineString(p + "id", marker.id);
      lineString(p + "tag", marker.tag);
      lineString(p + "glyph", marker.glyph);
      line(p + "row", unsignedText(marker.row));
      line(p + "column", unsignedText(marker.column));
      line(p + "positionMeters", formatVec3(marker.positionMeters));
      line(p + "sourceLine", unsignedText(marker.sourceLine));
      line(p + "sourceColumn", unsignedText(marker.sourceColumn));
    }
  }

  void writeCreativeDocumentObjectTags(
      const std::string& prefix,
      const SaveCreativeDocumentObjectRecord& object) {
    line(prefix + "tag.count", unsignedText(object.tags.size()));
    for (std::size_t tag = 0; tag < object.tags.size(); ++tag) {
      lineString(prefix + "tag." + std::to_string(tag), object.tags[tag]);
    }
  }

  void writeCreativeDocumentObjectPathPoints(
      const std::string& prefix,
      const SaveCreativeDocumentObjectRecord& object) {
    if (object.pathPoints.empty()) {
      return;
    }
    line(prefix + "pathPoint.count", unsignedText(object.pathPoints.size()));
    for (std::size_t point = 0; point < object.pathPoints.size(); ++point) {
      const SaveCreativeDocumentPathPointRecord& pathPoint =
          object.pathPoints[point];
      lineCreativeVec3(prefix + "pathPoint." + std::to_string(point) + ".position",
                       {pathPoint.x, pathPoint.y, pathPoint.z});
      line(prefix + "pathPoint." + std::to_string(point) + ".dwellSeconds",
           formatDoubleLossless(pathPoint.dwellSeconds));
      line(prefix + "pathPoint." + std::to_string(point) +
               ".outgoingSpeedMultiplier",
           formatDoubleLossless(pathPoint.outgoingSpeedMultiplier));
    }
  }

  void writeCreativeDocumentObject(
      const std::string& prefix,
      const SaveCreativeDocumentObjectRecord& object) {
    line(prefix + "id", unsignedText(object.id));
    lineString(prefix + "kind", object.kind);
    lineString(prefix + "name", object.name);
    if (!object.assetId.empty()) {
      lineString(prefix + "assetId", object.assetId);
      if (object.assetContentHash != 0U) {
        line(prefix + "assetContentHash",
             unsignedText(object.assetContentHash));
      }
      if (!object.assetMaterialVariant.empty()) {
        lineString(prefix + "assetMaterialVariant",
                   object.assetMaterialVariant);
      }
    }
    lineCreativeVec3(prefix + "transform.position", object.transform.position);
    lineCreativeVec3(prefix + "transform.rotation", object.transform.rotation);
    lineCreativeVec3(prefix + "transform.scale", object.transform.scale);
    lineCreativeVec3(prefix + "bounds.min", object.bounds.min);
    lineCreativeVec3(prefix + "bounds.max", object.bounds.max);
    line(prefix + "layerId", unsignedText(object.layerId));
    lineBool(prefix + "visible", object.visible);
    lineBool(prefix + "locked", object.locked);
    lineBool(prefix + "hasParent", object.hasParent);
    line(prefix + "parentId", unsignedText(object.parentId));
    if (!object.attachmentSocket.empty()) {
      lineString(prefix + "attachmentSocket", object.attachmentSocket);
    }
    writeCreativeDocumentObjectTags(prefix, object);
    writeCreativeDocumentObjectPathPoints(prefix, object);
    if (object.kind == "MovingPlatform") {
      line(prefix + "movingPlatform.speedMetersPerSecond",
           formatDoubleLossless(
               object.movingPlatformSpeedMetersPerSecond));
      lineString(prefix + "movingPlatform.traversalMode",
                 object.movingPlatformTraversalMode);
      lineBool(prefix + "movingPlatform.startsActive",
               object.movingPlatformStartsActive);
    }
    if (object.kind == "Door") {
      lineString(prefix + "door.leafArrangement",
                 object.doorLeafArrangement);
      lineString(prefix + "door.hingeSide", object.doorHingeSide);
      lineString(prefix + "door.swingSide", object.doorSwingSide);
      lineString(prefix + "door.initialState", object.doorInitialState);
      lineBool(prefix + "door.gameplayLocked", object.doorGameplayLocked);
      line(prefix + "door.transitionSeconds",
           formatDoubleLossless(object.doorTransitionSeconds));
    }
    if (object.kind == "Window") {
      lineString(prefix + "window.insertKind", object.windowInsertKind);
    }
    if (object.kind == "SpawnPoint") {
      lineString(prefix + "playerSpawn.profileId",
                 object.playerSpawnProfileId);
      lineString(prefix + "playerSpawn.group", object.playerSpawnGroup);
      line(prefix + "playerSpawn.validationRadiusMeters",
           formatDoubleLossless(object.playerSpawnValidationRadiusMeters));
      line(prefix + "playerSpawn.fallbackPriority",
           unsignedText(object.playerSpawnFallbackPriority));
    }
  }

  void writeCreativeTerrainHeightField(
      const std::string& prefix,
      const SaveCreativeDocumentTerrainHeightFieldRecord& heightField) {
    if (!heightField.present) {
      return;
    }
    lineBool(prefix + ".present", true);
    line(prefix + ".minimumX", std::to_string(heightField.minimumX));
    line(prefix + ".minimumZ", std::to_string(heightField.minimumZ));
    line(prefix + ".widthCells", unsignedText(heightField.widthCells));
    line(prefix + ".depthCells", unsignedText(heightField.depthCells));
    line(prefix + ".height.count", unsignedText(heightField.heights.size()));
    for (std::size_t index = 0U; index < heightField.heights.size(); ++index) {
      line(prefix + ".height." + std::to_string(index),
           unsignedText(heightField.heights[index]));
    }
  }

  void writeCreativeDocument() {
    if (!envelope_.creativeDocument.present) {
      return;
    }
    const SaveCreativeDocumentSection& section = envelope_.creativeDocument;
    lineBool("creativeDocument.present", true);
    line("creativeDocument.version", unsignedText(section.version));
    line("creativeDocument.documentId", unsignedText(section.documentId));
    lineString("creativeDocument.name", section.name);
    lineString("creativeDocument.units", section.units);
    lineCreativeVec3("creativeDocument.grid.origin", section.gridOrigin);
    line("creativeDocument.grid.cellSizeMeters", formatDoubleLossless(section.cellSizeMeters));
    line("creativeDocument.grid.width", unsignedText(section.gridWidth));
    line("creativeDocument.grid.height", unsignedText(section.gridHeight));
    line("creativeDocument.grid.depth", unsignedText(section.gridDepth));
    lineString("creativeDocument.snap.mode", section.snapMode);
    line("creativeDocument.snap.axes", unsignedText(section.snapAxes));
    line("creativeDocument.snap.stepX", formatDoubleLossless(section.snapStepX));
    line("creativeDocument.snap.stepY", formatDoubleLossless(section.snapStepY));
    line("creativeDocument.snap.stepZ", formatDoubleLossless(section.snapStepZ));
    line("creativeDocument.snap.originX", formatDoubleLossless(section.snapOriginX));
    line("creativeDocument.snap.originY", formatDoubleLossless(section.snapOriginY));
    line("creativeDocument.snap.originZ", formatDoubleLossless(section.snapOriginZ));
    lineCreativeVec3("creativeDocument.worldBounds.min", section.worldBounds.min);
    lineCreativeVec3("creativeDocument.worldBounds.max", section.worldBounds.max);
    line("creativeDocument.nextObjectId", unsignedText(section.nextObjectId));
    line("creativeDocument.object.count", unsignedText(section.objects.size()));
    for (std::size_t index = 0; index < section.objects.size(); ++index) {
      const SaveCreativeDocumentObjectRecord& object = section.objects[index];
      const std::string p = "creativeDocument.object." + std::to_string(index) + ".";
      writeCreativeDocumentObject(p, object);
    }
    line("creativeDocument.logicLink.count",
         unsignedText(section.logicLinks.size()));
    for (std::size_t index = 0; index < section.logicLinks.size(); ++index) {
      const SaveCreativeDocumentLogicLinkRecord& link =
          section.logicLinks[index];
      const std::string p = "creativeDocument.logicLink." +
                            std::to_string(index) + ".";
      line(p + "sourceObjectId", unsignedText(link.sourceObjectId));
      line(p + "targetObjectId", unsignedText(link.targetObjectId));
      lineString(p + "action", link.action);
    }
    line("creativeDocument.voxelChunk.count",
         unsignedText(section.voxelChunks.size()));
    for (std::size_t chunkIndex = 0;
         chunkIndex < section.voxelChunks.size(); ++chunkIndex) {
      const SaveCreativeDocumentVoxelChunkRecord& chunk =
          section.voxelChunks[chunkIndex];
      const std::string p = "creativeDocument.voxelChunk." +
                            std::to_string(chunkIndex) + ".";
      line(p + "x", std::to_string(chunk.x));
      line(p + "y", std::to_string(chunk.y));
      line(p + "z", std::to_string(chunk.z));
      line(p + "cell.count", unsignedText(chunk.cells.size()));
      for (std::size_t cellIndex = 0; cellIndex < chunk.cells.size();
           ++cellIndex) {
        const SaveCreativeDocumentVoxelCellRecord& cell =
            chunk.cells[cellIndex];
        const std::string cellPrefix =
            p + "cell." + std::to_string(cellIndex) + ".";
        line(cellPrefix + "localIndex", unsignedText(cell.localIndex));
        lineString(cellPrefix + "material", cell.material);
      }
    }
    line("creativeDocument.terrainControl.count",
         unsignedText(section.terrainControls.size()));
    for (std::size_t index = 0; index < section.terrainControls.size(); ++index) {
      const SaveCreativeDocumentTerrainControlRecord& control =
          section.terrainControls[index];
      const std::string p = "creativeDocument.terrainControl." +
                            std::to_string(index) + ".";
      line(p + "x", std::to_string(control.x));
      line(p + "z", std::to_string(control.z));
      line(p + "heightCells", unsignedText(control.heightCells));
      line(p + "radiusCells", unsignedText(control.radiusCells));
    }
    writeCreativeTerrainHeightField("creativeDocument.terrainHeightField",
                                    section.terrainHeightField);
    if (section.version >= kSaveCreativeDocumentTerrainHardEdgeVersion) {
      line("creativeDocument.terrainHardEdge.count",
           unsignedText(section.terrainHardEdges.size()));
      for (std::size_t index = 0U; index < section.terrainHardEdges.size();
           ++index) {
        const SaveCreativeDocumentTerrainHardEdgeRecord& edge =
            section.terrainHardEdges[index];
        const std::string prefix = "creativeDocument.terrainHardEdge." +
                                   std::to_string(index) + ".";
        line(prefix + "firstX", std::to_string(edge.firstX));
        line(prefix + "firstZ", std::to_string(edge.firstZ));
        line(prefix + "secondX", std::to_string(edge.secondX));
        line(prefix + "secondZ", std::to_string(edge.secondZ));
      }
    }
    line("creativeDocument.terrainOperation.version",
         unsignedText(section.terrainOperationStackVersion));
    line("creativeDocument.terrainOperation.nextId",
         unsignedText(section.nextTerrainOperationId));
    writeCreativeTerrainHeightField(
        "creativeDocument.terrainOperation.baseHeightField",
        section.terrainOperationBaseHeightField);
    if (section.version >= kSaveCreativeDocumentTerrainHardEdgeVersion) {
      line("creativeDocument.terrainOperation.baseHardEdge.count",
           unsignedText(section.terrainOperationBaseHardEdges.size()));
      for (std::size_t index = 0U;
           index < section.terrainOperationBaseHardEdges.size(); ++index) {
        const SaveCreativeDocumentTerrainHardEdgeRecord& edge =
            section.terrainOperationBaseHardEdges[index];
        const std::string prefix =
            "creativeDocument.terrainOperation.baseHardEdge." +
            std::to_string(index) + ".";
        line(prefix + "firstX", std::to_string(edge.firstX));
        line(prefix + "firstZ", std::to_string(edge.firstZ));
        line(prefix + "secondX", std::to_string(edge.secondX));
        line(prefix + "secondZ", std::to_string(edge.secondZ));
      }
    }
    if (!section.terrainOperations.empty() ||
        !section.terrainOperationBaseMaterials.empty()) {
      line("creativeDocument.terrainOperation.baseMaterial.count",
           unsignedText(section.terrainOperationBaseMaterials.size()));
      for (std::size_t index = 0U;
           index < section.terrainOperationBaseMaterials.size(); ++index) {
        const SaveCreativeDocumentTerrainMaterialRecord& material =
            section.terrainOperationBaseMaterials[index];
        const std::string prefix =
            "creativeDocument.terrainOperation.baseMaterial." +
            std::to_string(index) + ".";
        line(prefix + "x", std::to_string(material.x));
        line(prefix + "z", std::to_string(material.z));
        lineString(prefix + "material", material.material);
        lineBool(prefix + "weightsPresent", material.hasWeights);
        if (material.hasWeights) {
          line(prefix + "grassWeight", unsignedText(material.grassWeight));
          line(prefix + "dirtWeight", unsignedText(material.dirtWeight));
          line(prefix + "stoneWeight", unsignedText(material.stoneWeight));
          line(prefix + "sandWeight", unsignedText(material.sandWeight));
        }
      }
    }
    line("creativeDocument.terrainOperation.count",
         unsignedText(section.terrainOperations.size()));
    for (std::size_t index = 0U; index < section.terrainOperations.size();
         ++index) {
      const SaveCreativeDocumentTerrainOperationRecord& operation =
          section.terrainOperations[index];
      const std::string prefix = "creativeDocument.terrainOperation." +
                                 std::to_string(index) + ".";
      line(prefix + "id", unsignedText(operation.id));
      lineBool(prefix + "enabled", operation.enabled);
      if (section.version >=
          kSaveCreativeDocumentTerrainOperationProvenanceVersion) {
        lineString(prefix + "owner", operation.owner);
        lineString(prefix + "sourceKey", operation.sourceKey);
      }
      lineString(prefix + "kind", operation.operationKind);
      line(prefix + "generation.version",
           unsignedText(operation.generationVersion));
      lineString(prefix + "generation.kind", operation.generatorKind);
      line(prefix + "generation.seed", unsignedText(operation.seed));
      line(prefix + "generation.minimumX",
           std::to_string(operation.minimumX));
      line(prefix + "generation.minimumZ",
           std::to_string(operation.minimumZ));
      line(prefix + "generation.widthCells",
           unsignedText(operation.widthCells));
      line(prefix + "generation.depthCells",
           unsignedText(operation.depthCells));
      line(prefix + "generation.baseHeightCells",
           unsignedText(operation.baseHeightCells));
      line(prefix + "generation.reliefCells",
           unsignedText(operation.reliefCells));
      line(prefix + "generation.horizontalScaleCells",
           formatDoubleLossless(operation.horizontalScaleCells));
      line(prefix + "generation.octaveCount",
           unsignedText(operation.octaveCount));
      line(prefix + "generation.persistence",
           formatDoubleLossless(operation.persistence));
      line(prefix + "generation.lacunarity",
           formatDoubleLossless(operation.lacunarity));
      line(prefix + "generation.slopeDamping",
           formatDoubleLossless(operation.slopeDamping));
      if (section.version >=
          kSaveCreativeDocumentTerrainGenerationIntentVersion) {
        lineBool(prefix + "generation.paintMaterials",
                 operation.paintMaterials);
        lineString(prefix + "generation.biomeIntent",
                   operation.biomeIntent);
        lineString(prefix + "generation.lowlandMaterial",
                   operation.lowlandMaterial);
        lineString(prefix + "generation.highlandMaterial",
                   operation.highlandMaterial);
        line(prefix + "generation.materialTransitionHeightCells",
             unsignedText(operation.materialTransitionHeightCells));
      }
      line(prefix + "composition.version",
           unsignedText(operation.compositionVersion));
      lineString(prefix + "composition.mask", operation.mask);
      lineString(prefix + "composition.mode", operation.mode);
      line(prefix + "composition.featherCells",
           unsignedText(operation.featherCells));
      if (section.version >=
          kSaveCreativeDocumentTerrainGenerationIntentVersion) {
        line(prefix + "composition.protectedRegion.count",
             unsignedText(operation.protectedRegions.size()));
        for (std::size_t protectedIndex = 0U;
             protectedIndex < operation.protectedRegions.size();
             ++protectedIndex) {
          const SaveCreativeDocumentTerrainProtectedRegionRecord& region =
              operation.protectedRegions[protectedIndex];
          const std::string protectedPrefix =
              prefix + "composition.protectedRegion." +
              std::to_string(protectedIndex) + ".";
          line(protectedPrefix + "minimumX", std::to_string(region.minimumX));
          line(protectedPrefix + "minimumZ", std::to_string(region.minimumZ));
          line(protectedPrefix + "widthCells",
               unsignedText(region.widthCells));
          line(protectedPrefix + "depthCells",
               unsignedText(region.depthCells));
          lineString(protectedPrefix + "mask", region.mask);
        }
      }
      if (section.version >= kSaveCreativeDocumentTerrainRegionVersion) {
        line(prefix + "region.version", unsignedText(operation.regionVersion));
        line(prefix + "region.minimumX",
             std::to_string(operation.regionMinimumX));
        line(prefix + "region.minimumZ",
             std::to_string(operation.regionMinimumZ));
        line(prefix + "region.widthCells",
             unsignedText(operation.regionWidthCells));
        line(prefix + "region.depthCells",
             unsignedText(operation.regionDepthCells));
        lineString(prefix + "region.mask", operation.regionMask);
        lineString(prefix + "region.mode", operation.regionMode);
        line(prefix + "region.amountCells",
             unsignedText(operation.regionAmountCells));
        line(prefix + "region.targetHeightCells",
             unsignedText(operation.regionTargetHeightCells));
        line(prefix + "region.noiseReliefCells",
             unsignedText(operation.regionNoiseReliefCells));
        line(prefix + "region.noiseScaleCells",
             formatDoubleLossless(operation.regionNoiseScaleCells));
        line(prefix + "region.featherCells",
             unsignedText(operation.regionFeatherCells));
        line(prefix + "region.seed", unsignedText(operation.regionSeed));
      }
      line(prefix + "grade.version", unsignedText(operation.gradeVersion));
      line(prefix + "grade.startX", std::to_string(operation.gradeStartX));
      line(prefix + "grade.startZ", std::to_string(operation.gradeStartZ));
      line(prefix + "grade.endX", std::to_string(operation.gradeEndX));
      line(prefix + "grade.endZ", std::to_string(operation.gradeEndZ));
      line(prefix + "grade.startHeightCells",
           unsignedText(operation.gradeStartHeightCells));
      line(prefix + "grade.endHeightCells",
           unsignedText(operation.gradeEndHeightCells));
      line(prefix + "grade.halfWidthCells",
           unsignedText(operation.gradeHalfWidthCells));
      line(prefix + "grade.crossSlopePermille",
           std::to_string(operation.gradeCrossSlopePermille));
      line(prefix + "grade.falloffCells",
           unsignedText(operation.gradeFalloffCells));
      line(prefix + "path.version", unsignedText(operation.pathVersion));
      lineString(prefix + "path.kind", operation.pathKind);
      lineString(prefix + "path.elevation", operation.pathElevation);
      lineString(prefix + "path.curve", operation.pathCurve);
      lineString(prefix + "path.crossSection", operation.pathCrossSection);
      lineString(prefix + "path.startJoin", operation.pathStartJoin);
      lineString(prefix + "path.endJoin", operation.pathEndJoin);
      line(prefix + "path.falloffCells",
           unsignedText(operation.pathFalloffCells));
      lineBool(prefix + "path.paintSurface", operation.pathPaintSurface);
      lineString(prefix + "path.material", operation.pathMaterial);
      if (section.version >= kSaveCreativeDocumentTerrainRoadVersion) {
        line(prefix + "path.road.shoulderWidthCells",
             unsignedText(operation.pathRoadShoulderWidthCells));
        line(prefix + "path.road.maximumGradePermille",
             unsignedText(operation.pathRoadMaximumGradePermille));
        line(prefix + "path.road.edgeTreatment",
             unsignedText(operation.pathRoadEdgeTreatment));
        line(prefix + "path.road.edgeWidthMeters",
             formatDoubleLossless(operation.pathRoadEdgeWidthMeters));
        line(prefix + "path.road.edgeHeightMeters",
             formatDoubleLossless(operation.pathRoadEdgeHeightMeters));
        line(prefix + "path.road.edgeMaterial",
             unsignedText(operation.pathRoadEdgeMaterial));
      }
      if (section.version >=
          kSaveCreativeDocumentTerrainWatercourseVersion) {
        line(prefix + "path.watercourse.bankSlopeCells",
             unsignedText(operation.pathWatercourseBankSlopeCells));
        line(prefix + "path.watercourse.drainageDirection",
             unsignedText(operation.pathWatercourseDrainageDirection));
        line(prefix + "path.watercourse.surfacePolicy",
             unsignedText(operation.pathWatercourseSurfacePolicy));
        line(prefix + "path.watercourse.surfaceInsetCells",
             unsignedText(operation.pathWatercourseSurfaceInsetCells));
        line(prefix + "path.watercourse.nextCrossingId",
             unsignedText(operation.pathWatercourseNextCrossingId));
        line(prefix + "path.watercourse.crossing.count",
             unsignedText(operation.pathWatercourseCrossings.size()));
        for (std::size_t crossingIndex = 0U;
             crossingIndex < operation.pathWatercourseCrossings.size();
             ++crossingIndex) {
          const SaveCreativeDocumentTerrainWatercourseCrossingRecord& crossing =
              operation.pathWatercourseCrossings[crossingIndex];
          const std::string crossingPrefix =
              prefix + "path.watercourse.crossing." +
              std::to_string(crossingIndex) + ".";
          line(crossingPrefix + "id", unsignedText(crossing.id));
          line(crossingPrefix + "pointId", unsignedText(crossing.pointId));
          line(crossingPrefix + "bankClearanceCells",
               unsignedText(crossing.bankClearanceCells));
          line(crossingPrefix + "deckClearanceCells",
               unsignedText(crossing.deckClearanceCells));
          line(crossingPrefix + "approachLengthCells",
               unsignedText(crossing.approachLengthCells));
        }
      }
      line(prefix + "path.nextPointId",
           unsignedText(operation.pathNextPointId));
      line(prefix + "path.point.count",
           unsignedText(operation.pathPoints.size()));
      for (std::size_t pointIndex = 0U;
           pointIndex < operation.pathPoints.size(); ++pointIndex) {
        const SaveCreativeDocumentTerrainPathPointRecord& point =
            operation.pathPoints[pointIndex];
        const std::string pointPrefix =
            prefix + "path.point." + std::to_string(pointIndex) + ".";
        line(pointPrefix + "id", unsignedText(point.id));
        line(pointPrefix + "x", std::to_string(point.x));
        line(pointPrefix + "z", std::to_string(point.z));
        line(pointPrefix + "heightCells", unsignedText(point.heightCells));
        line(pointPrefix + "halfWidthCells",
             unsignedText(point.halfWidthCells));
        line(pointPrefix + "amplitudeCells",
             unsignedText(point.amplitudeCells));
        line(pointPrefix + "bankPermille",
             std::to_string(point.bankPermille));
      }
      if (section.version >= kSaveCreativeDocumentTerrainStampVersion) {
        line(prefix + "stamp.recipeVersion",
             unsignedText(operation.stampRecipeVersion));
        line(prefix + "stamp.version", unsignedText(operation.stampVersion));
        lineString(prefix + "stamp.assetId", operation.stampAssetId);
        lineString(prefix + "stamp.label", operation.stampLabel);
        line(prefix + "stamp.assetVersion",
             unsignedText(operation.stampAssetVersion));
        line(prefix + "stamp.sourceDocumentId",
             unsignedText(operation.stampSourceDocumentId));
        line(prefix + "stamp.sourceRevision",
             unsignedText(operation.stampSourceRevision));
        line(prefix + "stamp.contentSignature",
             unsignedText(operation.stampContentSignature));
        line(prefix + "stamp.sourceMinimumX",
             std::to_string(operation.stampSourceMinimumX));
        line(prefix + "stamp.sourceMinimumZ",
             std::to_string(operation.stampSourceMinimumZ));
        line(prefix + "stamp.minimumHeightCells",
             unsignedText(operation.stampMinimumHeightCells));
        writeCreativeTerrainHeightField(prefix + "stamp.height",
                                        operation.stampHeightField);
        line(prefix + "stamp.material.count",
             unsignedText(operation.stampMaterials.size()));
        for (std::size_t materialIndex = 0U;
             materialIndex < operation.stampMaterials.size();
             ++materialIndex) {
          const SaveCreativeDocumentTerrainMaterialRecord& material =
              operation.stampMaterials[materialIndex];
          const std::string materialPrefix =
              prefix + "stamp.material." + std::to_string(materialIndex) + ".";
          line(materialPrefix + "x", std::to_string(material.x));
          line(materialPrefix + "z", std::to_string(material.z));
          lineString(materialPrefix + "material", material.material);
          lineBool(materialPrefix + "weightsPresent", material.hasWeights);
          if (material.hasWeights) {
            line(materialPrefix + "grassWeight",
                 unsignedText(material.grassWeight));
            line(materialPrefix + "dirtWeight",
                 unsignedText(material.dirtWeight));
            line(materialPrefix + "stoneWeight",
                 unsignedText(material.stoneWeight));
            line(materialPrefix + "sandWeight",
                 unsignedText(material.sandWeight));
          }
        }
        line(prefix + "stamp.targetMinimumX",
             std::to_string(operation.stampTargetMinimumX));
        line(prefix + "stamp.targetMinimumZ",
             std::to_string(operation.stampTargetMinimumZ));
        line(prefix + "stamp.quarterTurns",
             unsignedText(operation.stampQuarterTurns));
        lineBool(prefix + "stamp.mirrorX", operation.stampMirrorX);
        lineBool(prefix + "stamp.mirrorZ", operation.stampMirrorZ);
        lineString(prefix + "stamp.mode", operation.stampMode);
        lineString(prefix + "stamp.elevation", operation.stampElevation);
        line(prefix + "stamp.manualHeightOffsetCells",
             std::to_string(operation.stampManualHeightOffsetCells));
      }
      if (section.version >= kSaveCreativeDocumentTerrainProfileVersion) {
        line(prefix + "profile.version",
             unsignedText(operation.profileVersion));
        lineString(prefix + "profile.kind", operation.profileKind);
        lineString(prefix + "profile.blend", operation.profileBlend);
        lineString(prefix + "profile.rodPolicy", operation.profileRodPolicy);
        line(prefix + "profile.centerX",
             std::to_string(operation.profileCenterX));
        line(prefix + "profile.centerZ",
             std::to_string(operation.profileCenterZ));
        line(prefix + "profile.baseHeightCells",
             unsignedText(operation.profileBaseHeightCells));
        line(prefix + "profile.radiusCells",
             unsignedText(operation.profileRadiusCells));
        line(prefix + "profile.amplitudeCells",
             unsignedText(operation.profileAmplitudeCells));
        line(prefix + "profile.spacingCells",
             unsignedText(operation.profileSpacingCells));
        lineString(prefix + "profile.direction", operation.profileDirection);
        line(prefix + "profile.frequency",
             unsignedText(operation.profileFrequency));
        line(prefix + "profile.seed", unsignedText(operation.profileSeed));
      }
      if (section.version >= kSaveCreativeDocumentTerrainLandformVersion) {
        line(prefix + "landform.version",
             unsignedText(operation.landformVersion));
        lineString(prefix + "landform.kind", operation.landformKind);
        line(prefix + "landform.minimumX",
             std::to_string(operation.landformMinimumX));
        line(prefix + "landform.minimumZ",
             std::to_string(operation.landformMinimumZ));
        line(prefix + "landform.widthCells",
             unsignedText(operation.landformWidthCells));
        line(prefix + "landform.depthCells",
             unsignedText(operation.landformDepthCells));
        line(prefix + "landform.baseHeightCells",
             unsignedText(operation.landformBaseHeightCells));
        line(prefix + "landform.targetHeightCells",
             unsignedText(operation.landformTargetHeightCells));
        line(prefix + "landform.terraceCount",
             unsignedText(operation.landformTerraceCount));
        lineString(prefix + "landform.direction",
                   operation.landformDirection);
        lineString(prefix + "landform.edge", operation.landformEdge);
        line(prefix + "landform.edgeWidthCells",
             unsignedText(operation.landformEdgeWidthCells));
        line(prefix + "landform.featherCells",
             unsignedText(operation.landformFeatherCells));
        lineBool(prefix + "landform.paintSurface",
                 operation.landformPaintSurface);
        lineString(prefix + "landform.material", operation.landformMaterial);
        lineString(prefix + "landform.erosion", operation.landformErosion);
        line(prefix + "landform.erosionReliefCells",
             unsignedText(operation.landformErosionReliefCells));
        line(prefix + "landform.seed", unsignedText(operation.landformSeed));
      }
    }
    line("creativeDocument.patternRecipe.version",
         unsignedText(section.patternRecipeStoreVersion));
    line("creativeDocument.patternRecipe.nextId",
         unsignedText(section.nextPatternRecipeId));
    line("creativeDocument.patternRecipe.count",
         unsignedText(section.patternRecipes.size()));
    for (std::size_t index = 0U; index < section.patternRecipes.size();
         ++index) {
      const SaveCreativeDocumentPatternRecipeRecord& recipe =
          section.patternRecipes[index];
      const std::string prefix = "creativeDocument.patternRecipe." +
                                 std::to_string(index) + ".";
      line(prefix + "id", unsignedText(recipe.id));
      lineString(prefix + "kind", recipe.kind);
      line(prefix + "source.count",
           unsignedText(recipe.sourceObjectIds.size()));
      for (std::size_t objectIndex = 0U;
           objectIndex < recipe.sourceObjectIds.size(); ++objectIndex) {
        line(prefix + "source." + std::to_string(objectIndex) + ".objectId",
             unsignedText(recipe.sourceObjectIds[objectIndex]));
      }
      line(prefix + "generated.count",
           unsignedText(recipe.generatedObjectIds.size()));
      for (std::size_t objectIndex = 0U;
           objectIndex < recipe.generatedObjectIds.size(); ++objectIndex) {
        line(prefix + "generated." + std::to_string(objectIndex) +
                 ".objectId",
             unsignedText(recipe.generatedObjectIds[objectIndex]));
      }
      lineString(prefix + "linear.direction", recipe.linearDirection);
      lineString(prefix + "linear.copyCount", recipe.linearCopyCount);
      lineString(prefix + "linear.spacing", recipe.linearSpacing);
      line(prefix + "linear.cellSize",
           formatDoubleLossless(recipe.linearCellSize));
      line(prefix + "linear.maxGeneratedObjects",
           unsignedText(recipe.linearMaxGeneratedObjects));
      line(prefix + "radial.pivot.x",
           formatDoubleLossless(recipe.radialPivot.x));
      line(prefix + "radial.pivot.y",
           formatDoubleLossless(recipe.radialPivot.y));
      line(prefix + "radial.pivot.z",
           formatDoubleLossless(recipe.radialPivot.z));
      lineString(prefix + "radial.axis", recipe.radialAxis);
      lineString(prefix + "radial.instanceCount",
                 recipe.radialInstanceCount);
      lineString(prefix + "radial.sweep", recipe.radialSweep);
      line(prefix + "radial.maxGeneratedObjects",
           unsignedText(recipe.radialMaxGeneratedObjects));
      if (section.patternRecipeStoreVersion >= 2U) {
        lineString(prefix + "scatter.objectKind", recipe.scatterObjectKind);
        lineString(prefix + "scatter.assetId", recipe.scatterAssetId);
        line(prefix + "scatter.assetContentHash",
             unsignedText(recipe.scatterAssetContentHash));
        lineString(prefix + "scatter.assetMaterialVariant",
                   recipe.scatterAssetMaterialVariant);
        line(prefix + "scatter.assetSourceBounds.min.x",
             formatDoubleLossless(recipe.scatterAssetSourceBounds.min.x));
        line(prefix + "scatter.assetSourceBounds.min.y",
             formatDoubleLossless(recipe.scatterAssetSourceBounds.min.y));
        line(prefix + "scatter.assetSourceBounds.min.z",
             formatDoubleLossless(recipe.scatterAssetSourceBounds.min.z));
        line(prefix + "scatter.assetSourceBounds.max.x",
             formatDoubleLossless(recipe.scatterAssetSourceBounds.max.x));
        line(prefix + "scatter.assetSourceBounds.max.y",
             formatDoubleLossless(recipe.scatterAssetSourceBounds.max.y));
        line(prefix + "scatter.assetSourceBounds.max.z",
             formatDoubleLossless(recipe.scatterAssetSourceBounds.max.z));
        line(prefix + "scatter.paintCenter.count",
             unsignedText(recipe.scatterPaintCenters.size()));
        for (std::size_t centerIndex = 0U;
             centerIndex < recipe.scatterPaintCenters.size(); ++centerIndex) {
          const SaveCreativeDocumentVec3Record& center =
              recipe.scatterPaintCenters[centerIndex];
          const std::string centerPrefix = prefix + "scatter.paintCenter." +
                                           std::to_string(centerIndex) + ".";
          line(centerPrefix + "x", formatDoubleLossless(center.x));
          line(centerPrefix + "y", formatDoubleLossless(center.y));
          line(centerPrefix + "z", formatDoubleLossless(center.z));
        }
        line(prefix + "scatter.exclusion.count",
             unsignedText(recipe.scatterExclusions.size()));
        for (std::size_t exclusionIndex = 0U;
             exclusionIndex < recipe.scatterExclusions.size();
             ++exclusionIndex) {
          const SaveCreativeDocumentPatternRecipeRecord::Exclusion& exclusion =
              recipe.scatterExclusions[exclusionIndex];
          const std::string exclusionPrefix =
              prefix + "scatter.exclusion." +
              std::to_string(exclusionIndex) + ".";
          line(exclusionPrefix + "center.x",
               formatDoubleLossless(exclusion.center.x));
          line(exclusionPrefix + "center.y",
               formatDoubleLossless(exclusion.center.y));
          line(exclusionPrefix + "center.z",
               formatDoubleLossless(exclusion.center.z));
          line(exclusionPrefix + "radiusMeters",
               formatDoubleLossless(exclusion.radiusMeters));
        }
        lineString(prefix + "scatter.mask", recipe.scatterMask);
        lineString(prefix + "scatter.yaw", recipe.scatterYaw);
        line(prefix + "scatter.baseYawRadians",
             formatDoubleLossless(recipe.scatterBaseYawRadians));
        line(prefix + "scatter.radiusMeters",
             formatDoubleLossless(recipe.scatterRadiusMeters));
        line(prefix + "scatter.spacingMeters",
             formatDoubleLossless(recipe.scatterSpacingMeters));
        line(prefix + "scatter.densityFraction",
             formatDoubleLossless(recipe.scatterDensityFraction));
        line(prefix + "scatter.scaleVariation",
             formatDoubleLossless(recipe.scatterScaleVariation));
        line(prefix + "scatter.maximumSlopeRadians",
             formatDoubleLossless(recipe.scatterMaximumSlopeRadians));
        lineBool(prefix + "scatter.projectToTerrainSurface",
                 recipe.scatterProjectToTerrainSurface);
        lineBool(prefix + "scatter.avoidCollisions",
                 recipe.scatterAvoidCollisions);
        line(prefix + "scatter.seed", unsignedText(recipe.scatterSeed));
        line(prefix + "scatter.maxGeneratedObjects",
             unsignedText(recipe.scatterMaxGeneratedObjects));
      }
    }
    line("creativeDocument.measurementAnnotation.version",
         unsignedText(section.measurementAnnotationStoreVersion));
    line("creativeDocument.measurementAnnotation.nextId",
         unsignedText(section.nextMeasurementAnnotationId));
    line("creativeDocument.measurementAnnotation.count",
         unsignedText(section.measurementAnnotations.size()));
    for (std::size_t annotationIndex = 0U;
         annotationIndex < section.measurementAnnotations.size();
         ++annotationIndex) {
      const SaveCreativeDocumentMeasurementAnnotationRecord& annotation =
          section.measurementAnnotations[annotationIndex];
      const std::string prefix = "creativeDocument.measurementAnnotation." +
                                 std::to_string(annotationIndex) + ".";
      line(prefix + "id", unsignedText(annotation.id));
      lineString(prefix + "name", annotation.name);
      lineString(prefix + "mode", annotation.mode);
      lineString(prefix + "axis", annotation.axis);
      lineBool(prefix + "closePath", annotation.closePath);
      line(prefix + "point.count", unsignedText(annotation.points.size()));
      for (std::size_t pointIndex = 0U;
           pointIndex < annotation.points.size(); ++pointIndex) {
        const SaveCreativeDocumentMeasurementAnnotationPointRecord& point =
            annotation.points[pointIndex];
        const std::string pointPrefix =
            prefix + "point." + std::to_string(pointIndex) + ".";
        line(pointPrefix + "x", formatDoubleLossless(point.x));
        line(pointPrefix + "y", formatDoubleLossless(point.y));
        line(pointPrefix + "z", formatDoubleLossless(point.z));
        lineString(pointPrefix + "snapKind", point.snapKind);
      }
    }
    line("creativeDocument.terrainMaterial.count",
         unsignedText(section.terrainMaterials.size()));
    for (std::size_t index = 0; index < section.terrainMaterials.size();
         ++index) {
      const SaveCreativeDocumentTerrainMaterialRecord& material =
          section.terrainMaterials[index];
      const std::string p = "creativeDocument.terrainMaterial." +
                            std::to_string(index) + ".";
      line(p + "x", std::to_string(material.x));
      line(p + "z", std::to_string(material.z));
      lineString(p + "material", material.material);
      lineBool(p + "weightsPresent", material.hasWeights);
      if (material.hasWeights) {
        line(p + "grassWeight", unsignedText(material.grassWeight));
        line(p + "dirtWeight", unsignedText(material.dirtWeight));
        line(p + "stoneWeight", unsignedText(material.stoneWeight));
        line(p + "sandWeight", unsignedText(material.sandWeight));
      }
    }
  }

  void writeCreativeWorldLayout() {
    if (!envelope_.creativeWorldLayout.present) {
      return;
    }
    const SaveCreativeWorldLayoutSection& section =
        envelope_.creativeWorldLayout;
    lineBool("creativeWorldLayout.present", true);
    line("creativeWorldLayout.version", unsignedText(section.version));
    lineString("creativeWorldLayout.encodedText", section.encodedText);
  }

  void writePlayers() {
    line("players.slot.count", unsignedText(envelope_.players.slots.size()));
    for (std::size_t index = 0; index < envelope_.players.slots.size(); ++index) {
      const SavePlayerSlotRecord& slot = envelope_.players.slots[index];
      const std::string p = "players.slot." + std::to_string(index) + ".";
      line(p + "slotId", unsignedText(slot.slotId));
      lineEnum(p + "kind", slot.kind);
      line(p + "controlledActor", unsignedText(toUint64(slot.controlledActor)));
      lineString(p + "stableName", slot.stableName);
    }
  }

  void writeClock() {
    lineEnum("clock.mode", envelope_.clock.mode);
    lineEnum("clock.previousUnpausedMode", envelope_.clock.previousUnpausedMode);
    line("clock.previousUnpausedTimeScale", formatFloat(envelope_.clock.previousUnpausedTimeScale));
    line("clock.tickIndex", unsignedText(envelope_.clock.tickIndex));
    line("clock.fixedTickRateHz", unsignedText(envelope_.clock.fixedTickRateHz));
    line("clock.timeScale", formatFloat(envelope_.clock.timeScale));
  }

  void writeCamera() {
    lineEnum("camera.activeMode", envelope_.camera.activeMode);
    lineEnum("camera.previousRealtimeMode", envelope_.camera.previousRealtimeMode);
    line("camera.targetEntity", unsignedText(toUint64(envelope_.camera.targetEntity)));
    line("camera.targetPoint", formatVec3(envelope_.camera.targetPoint));
    lineBool("camera.targetHasPoint", envelope_.camera.targetHasPoint);
    line("camera.yawDegrees", formatFloat(envelope_.camera.yawDegrees));
    line("camera.pitchDegrees", formatFloat(envelope_.camera.pitchDegrees));
    line("camera.orbitDistance", formatFloat(envelope_.camera.orbitDistance));
  }

  void writeAbilities() {
    line("abilities.actor.count", unsignedText(envelope_.abilities.actors.size()));
    for (std::size_t index = 0; index < envelope_.abilities.actors.size(); ++index) {
      const SaveAbilityActorRecord& actor = envelope_.abilities.actors[index];
      const std::string p = "abilities.actor." + std::to_string(index) + ".";
      line(p + "actor", unsignedText(toUint64(actor.actor)));
      line(p + "arcaneFocus", unsignedText(actor.arcaneFocus));
      line(p + "arcaneBoltReadyTick", unsignedText(actor.arcaneBoltReadyTick));
      line(p + "arcaneFocusNextRechargeTick",
           unsignedText(actor.arcaneFocusNextRechargeTick));
    }
  }

  void writeCommandLog() {
    lineEnum("commandLog.resetPolicy", envelope_.commandLog.resetPolicy);
    line("commandLog.nextSequence", unsignedText(envelope_.commandLog.nextSequence));
    line("commandLog.epoch", unsignedText(envelope_.commandLog.epoch));
    line("commandLog.record.count", unsignedText(envelope_.commandLog.records.size()));
    for (std::size_t index = 0; index < envelope_.commandLog.records.size(); ++index) {
      const SaveCommandRecord& record = envelope_.commandLog.records[index];
      const std::string p = "commandLog.record." + std::to_string(index) + ".";
      line(p + "commandId", unsignedText(record.commandId));
      line(p + "sequence", unsignedText(record.sequence));
      lineEnum(p + "kind", record.kind);
      lineEnum(p + "source", record.source);
      line(p + "playerSlot", unsignedText(record.playerSlot));
      line(p + "actor", unsignedText(toUint64(record.actor)));
      lineBool(p + "hasTargetEntity", record.hasTargetEntity);
      line(p + "targetEntity", unsignedText(toUint64(record.targetEntity)));
      lineBool(p + "hasTargetPoint", record.hasTargetPoint);
      line(p + "targetPoint", formatVec3(record.targetPoint));
      line(p + "retrySourceCommandId", unsignedText(record.retrySourceCommandId));
      line(p + "attackDamage", std::to_string(record.attackDamage));
      lineEnum(p + "ability", record.ability);
      line(p + "abilityDirection", formatVec3(record.abilityDirection));
      line(p + "issuedTick", unsignedText(record.issuedTick));
      line(p + "scheduledTick", unsignedText(record.scheduledTick));
      lineEnum(p + "admission", record.admission);
      lineEnum(p + "rejection", record.rejection);
    }
  }

  void writeInventory() {
    line("inventory.player.count", unsignedText(envelope_.inventory.players.size()));
    for (std::size_t index = 0; index < envelope_.inventory.players.size(); ++index) {
      const SavePlayerInventoryRecord& player = envelope_.inventory.players[index];
      const std::string p = "inventory.player." + std::to_string(index) + ".";
      line(p + "playerSlot", unsignedText(player.playerSlot));
      line(p + "stack.count", unsignedText(player.stacks.size()));
      for (std::size_t stack = 0; stack < player.stacks.size(); ++stack) {
        const std::string sp = p + "stack." + std::to_string(stack) + ".";
        lineString(sp + "itemId", player.stacks[stack].itemId);
        line(sp + "count", unsignedText(player.stacks[stack].count));
      }
    }
  }

  void writeCombat() {
    line("combat.combatant.count", unsignedText(envelope_.combat.combatants.size()));
    for (std::size_t index = 0; index < envelope_.combat.combatants.size(); ++index) {
      const SaveCombatantRecord& combatant = envelope_.combat.combatants[index];
      const std::string p = "combat.combatant." + std::to_string(index) + ".";
      line(p + "entity", unsignedText(toUint64(combatant.entity)));
      line(p + "factionId", unsignedText(combatant.factionId));
      line(p + "hitPoints", std::to_string(combatant.hitPoints));
      line(p + "maxHitPoints", std::to_string(combatant.maxHitPoints));
      lineBool(p + "defeated", combatant.defeated);
    }
  }

  void writeAi() {
    line("ai.actor.count", unsignedText(envelope_.ai.actors.size()));
    for (std::size_t index = 0; index < envelope_.ai.actors.size(); ++index) {
      const SaveAiActorRecord& actor = envelope_.ai.actors[index];
      const std::string p = "ai.actor." + std::to_string(index) + ".";
      line(p + "actor", unsignedText(toUint64(actor.actor)));
      line(p + "nextDecisionTick", unsignedText(actor.nextDecisionTick));
      line(p + "deterministicPolicy", unsignedText(actor.deterministicPolicy));
      lineBool(p + "enabled", actor.enabled);
      lineString(p + "behavior_profile_id", actor.behaviorProfileId);
      line(p + "target", unsignedText(toUint64(actor.target)));
      lineEnum(p + "behavior", actor.behavior);
      lineEnum(p + "lastIntent", actor.lastIntent);
      line(p + "cooldownTicksRemaining", unsignedText(actor.cooldownTicksRemaining));
      lineBool(p + "hasHomePosition", actor.hasHomePosition);
      line(p + "homePosition", formatVec3(actor.homePosition));
      lineString(p + "homeStableName", actor.homeStableName);
      line(p + "leashRadiusMeters", formatFloat(actor.leashRadiusMeters));
      line(p + "returnRadiusMeters", formatFloat(actor.returnRadiusMeters));
      line(p + "homeToleranceMeters", formatFloat(actor.homeToleranceMeters));
      // Patrol route + cursor (a2 commit 1). Waypoints use the lossless float encoder.
      line(p + "patrolWaypoint.count", unsignedText(actor.patrolWaypoints.size()));
      for (std::size_t w = 0; w < actor.patrolWaypoints.size(); ++w) {
        line(p + "patrolWaypoint." + std::to_string(w),
             formatVec3Lossless(actor.patrolWaypoints[w]));
      }
      lineEnum(p + "patrolMode", actor.patrolMode);
      line(p + "patrolTargetIndex", unsignedText(actor.patrolTargetIndex));
      lineBool(p + "patrolForward", actor.patrolForward);
      // Alert FSM + last-known memory + facing (a2 commit 2). Floats/Vec3s lossless.
      line(p + "alertLevel", formatFloatLossless(actor.alertLevel));
      line(p + "lastRiseTick", unsignedText(actor.lastRiseTick));
      line(p + "maxAlertIndexThisEngagement", unsignedText(actor.maxAlertIndexThisEngagement));
      line(p + "graceUntilTick", unsignedText(actor.graceUntilTick));
      line(p + "graceThreshold", formatFloatLossless(actor.graceThreshold));
      line(p + "graceCount", unsignedText(actor.graceCount));
      line(p + "lastKnownTargetPosition", formatVec3Lossless(actor.lastKnownTargetPosition));
      line(p + "lastKnownTargetTick", unsignedText(actor.lastKnownTargetTick));
      lineBool(p + "hasLastKnownTarget", actor.hasLastKnownTarget);
      line(p + "investigateDwellTicks", unsignedText(actor.investigateDwellTicks));
      line(p + "facingDirection", formatVec3Lossless(actor.facingDirection));
    }
  }

  void writeObjectives() {
    line("objectives.record.count", unsignedText(envelope_.objectives.objectives.size()));
    for (std::size_t index = 0; index < envelope_.objectives.objectives.size(); ++index) {
      const SaveObjectiveRecord& objective = envelope_.objectives.objectives[index];
      const std::string p = "objectives.record." + std::to_string(index) + ".";
      lineString(p + "objectiveId", objective.objectiveId);
      lineEnum(p + "status", objective.status);
      lineEnum(p + "conditionKind", objective.conditionKind);
      line(p + "conditionPlayerSlot", unsignedText(objective.conditionPlayerSlot));
      lineString(p + "conditionItemId", objective.conditionItemId);
      line(p + "conditionItemCount", unsignedText(objective.conditionItemCount));
    }
  }

  const SaveEnvelope& envelope_;
  std::ostringstream out_;
};

}  // namespace

SaveEncodeResult encodeSaveEnvelope(const SaveEnvelope& envelope) {
  return Writer(envelope).finish();
}

}  // namespace iggy3d
