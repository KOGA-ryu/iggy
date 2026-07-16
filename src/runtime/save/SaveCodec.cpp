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
