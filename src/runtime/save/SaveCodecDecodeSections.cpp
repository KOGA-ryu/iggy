#include "runtime/save/SaveCodecReader.hpp"

namespace iggy3d::save_codec_detail {

// branch-gate-relocation: BG-1238 from=src/runtime/save/SaveCodec.cpp
void Reader::readMetadata() {
  readUnsigned("metadata.schemaVersion", envelope_.metadata.schemaVersion);
  readUnsigned("metadata.minimumReadableSchemaVersion", envelope_.metadata.minimumReadableSchemaVersion);
  readUnsigned("metadata.runtimeSaveVersion", envelope_.metadata.runtimeSaveVersion);
  readString("metadata.packageId", envelope_.metadata.packageId);
  readString("metadata.scenarioId", envelope_.metadata.scenarioId);
  readString("metadata.createdByToolId", envelope_.metadata.createdByToolId);
  readOptionalString("metadata.saveId", envelope_.metadata.saveId);
  readOptionalString("metadata.worldId", envelope_.metadata.worldId);
  readOptionalString("metadata.worldTitle", envelope_.metadata.worldTitle);
  readOptionalString("metadata.saveTitle", envelope_.metadata.saveTitle);
  readOptionalString("metadata.saveType", envelope_.metadata.saveType);
  readOptionalString("metadata.createdAtUtc", envelope_.metadata.createdAtUtc);
  readOptionalString("metadata.savedAtUtc", envelope_.metadata.savedAtUtc);
  readUnsigned("metadata.savedStateHash", envelope_.metadata.savedStateHash);
  readString("metadata.savedStateHashHex", envelope_.metadata.savedStateHashHex);
}

void Reader::readSession() {
  readEnum("session.lifecycle", envelope_.session.lifecycle);
  readEnum("session.outcome", envelope_.session.outcome);
  readUnsigned("session.currentTick", envelope_.session.currentTick);
  readUnsigned("session.nextCommandId", envelope_.session.nextCommandId, SaveCodecStatus::InvalidId);
  readUnsigned("session.sessionSeed", envelope_.session.sessionSeed);
  readUnsigned("session.sessionSchemaVersion", envelope_.session.sessionSchemaVersion);
  readUnsigned("session.fixedTickRateHz", envelope_.session.fixedTickRateHz);
  readFloat("session.interactionRangeMeters", envelope_.session.interactionRangeMeters);
  readFloat("session.movementDistanceMeters", envelope_.session.movementDistanceMeters);
  readFloat("session.slowTimeScale", envelope_.session.slowTimeScale);
  readString("session.packageId", envelope_.session.packageId);
  readString("session.scenarioId", envelope_.session.scenarioId);
}

void Reader::readWorld() {
  readEntityId("world.nextEntityId", envelope_.world.nextEntityId);
  std::uint64_t count = 0;
  readUnsigned("world.entity.count", count);
  envelope_.world.entities.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < envelope_.world.entities.size(); ++index) {
    SaveEntityRecord& entity = envelope_.world.entities[index];
    const std::string p = "world.entity." + std::to_string(index) + ".";
    readEntityId(p + "id", entity.id);
    readString(p + "stableName", entity.stableName);
    readEnum(p + "kind", entity.kind);
    readVec3(p + "transform.position", entity.transform.position);
    readVec3(p + "transform.rotation", entity.transform.rotationEulerRadians);
    readVec3(p + "transform.scale", entity.transform.scale);
    readVec3(p + "localBounds.min", entity.localBounds.min);
    readVec3(p + "localBounds.max", entity.localBounds.max);
    readBool(p + "active", entity.active);
    readBool(p + "persistent", entity.persistent);
    readBool(p + "targetable", entity.targetable);
    std::uint64_t actionCount = 0;
    readUnsigned(p + "targetAction.count", actionCount);
    entity.targetActions.resize(static_cast<std::size_t>(actionCount));
    for (std::size_t action = 0; action < entity.targetActions.size(); ++action) {
      readEnum(p + "targetAction." + std::to_string(action), entity.targetActions[action]);
    }
    readEnum(p + "interactionKind", entity.interactionKind);
    readEnum(p + "interactionPrimaryEffect", entity.interactionPrimaryEffect);
    readString(p + "interactionItemId", entity.interactionItemId);
    readUnsigned(p + "interactionItemCount", entity.interactionItemCount);
    readString(p + "interactionObjectiveId", entity.interactionObjectiveId);
    if (nextKeyIs(p + "interactionRequiredItemId")) {
      readString(p + "interactionRequiredItemId", entity.interactionRequiredItemId);
      readUnsigned(p + "interactionRequiredItemCount", entity.interactionRequiredItemCount);
    }
    readBool(p + "interactionRepeatable", entity.interactionRepeatable);
    readBool(p + "interactionDeactivateTargetOnSuccess", entity.interactionDeactivateTargetOnSuccess);
  }
}

void Reader::readStringVector(const std::string& countKey,
                      const std::string& itemPrefix,
                      std::vector<std::string>& out) {
  std::uint64_t count = 0;
  readUnsigned(countKey, count);
  out.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < out.size(); ++index) {
    readString(itemPrefix + std::to_string(index), out[index]);
  }
}

void Reader::readOptionalCreativePathPointVector(
    const std::string& countKey,
    const std::string& itemPrefix,
    std::vector<SaveCreativeDocumentPathPointRecord>& out) {
  if (!nextKeyIs(countKey)) {
    out.clear();
    return;
  }
  std::uint64_t count = 0;
  readUnsigned(countKey, count);
  out.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < out.size(); ++index) {
    SaveCreativeDocumentVec3Record position;
    readCreativeVec3(itemPrefix + std::to_string(index) + ".position",
                     position);
    out[index].x = position.x;
    out[index].y = position.y;
    out[index].z = position.z;
    const std::string dwellKey =
        itemPrefix + std::to_string(index) + ".dwellSeconds";
    if (envelope_.creativeDocument.version >=
            kSaveCreativeDocumentWaypointDwellVersion ||
        nextKeyIs(dwellKey)) {
      readDouble(dwellKey, out[index].dwellSeconds);
    }
    const std::string speedKey =
        itemPrefix + std::to_string(index) + ".outgoingSpeedMultiplier";
    if (envelope_.creativeDocument.version >=
            kSaveCreativeDocumentSegmentSpeedVersion ||
        nextKeyIs(speedKey)) {
      readDouble(speedKey, out[index].outgoingSpeedMultiplier);
    }
  }
}

void Reader::readAuthoredRoomSemantics(const std::string& p,
                               SaveAuthoredRoomSemanticsRecord& semantics) {
  readString(p + "materialId", semantics.materialId);
  readStringVector(p + "traversalTag.count", p + "traversalTag.",
                   semantics.traversalTags);
  readStringVector(p + "gameplayTag.count", p + "gameplayTag.", semantics.gameplayTags);
  readBool(p + "walkable", semantics.walkable);
  readBool(p + "blocksActor", semantics.blocksActor);
  readBool(p + "blocksProjectile", semantics.blocksProjectile);
}

void Reader::readAuthoredRoom() {
  if (!nextKeyIs("authoredRoom.present")) {
    return;
  }
  readBool("authoredRoom.present", envelope_.authoredRoom.present);
  if (!envelope_.authoredRoom.present) {
    return;
  }
  readString("authoredRoom.id", envelope_.authoredRoom.id);
  readUnsigned("authoredRoom.version", envelope_.authoredRoom.version);
  readString("authoredRoom.source", envelope_.authoredRoom.source);
  readString("authoredRoom.sourceFile", envelope_.authoredRoom.sourceFile);
  readString("authoredRoom.sourceSubset", envelope_.authoredRoom.sourceSubset);
  std::uint64_t floorCount = 0;
  readUnsigned("authoredRoom.floor.count", floorCount);
  envelope_.authoredRoom.floors.resize(static_cast<std::size_t>(floorCount));
  for (std::size_t index = 0; index < envelope_.authoredRoom.floors.size(); ++index) {
    SaveAuthoredRoomFloorRecord& floor = envelope_.authoredRoom.floors[index];
    const std::string p = "authoredRoom.floor." + std::to_string(index) + ".";
    readString(p + "id", floor.id);
    readI32(p + "storyIndex", floor.storyIndex);
    readVec3(p + "centerMeters", floor.centerMeters);
    readVec3(p + "sizeMeters", floor.sizeMeters);
    readAuthoredRoomSemantics(p + "semantics.", floor.semantics);
    readBool(p + "locked", floor.locked);
    readBool(p + "hidden", floor.hidden);
  }
  std::uint64_t wallCount = 0;
  readUnsigned("authoredRoom.wall.count", wallCount);
  envelope_.authoredRoom.walls.resize(static_cast<std::size_t>(wallCount));
  for (std::size_t index = 0; index < envelope_.authoredRoom.walls.size(); ++index) {
    SaveAuthoredRoomWallRecord& wall = envelope_.authoredRoom.walls[index];
    const std::string p = "authoredRoom.wall." + std::to_string(index) + ".";
    readString(p + "id", wall.id);
    readI32(p + "storyIndex", wall.storyIndex);
    readVec3(p + "startMeters", wall.startMeters);
    readVec3(p + "endMeters", wall.endMeters);
    readFloat(p + "bottomY", wall.bottomY);
    readFloat(p + "heightMeters", wall.heightMeters);
    readFloat(p + "thicknessMeters", wall.thicknessMeters);
    readAuthoredRoomSemantics(p + "semantics.", wall.semantics);
    readBool(p + "locked", wall.locked);
    readBool(p + "hidden", wall.hidden);
  }
  // branch-gate: BG-1135
  if (nextKeyIs("authoredRoom.object.count")) {
    std::uint64_t objectCount = 0;
    readUnsigned("authoredRoom.object.count", objectCount);
    envelope_.authoredRoom.objects.resize(static_cast<std::size_t>(objectCount));
    for (std::size_t index = 0; index < envelope_.authoredRoom.objects.size(); ++index) {
      SaveAuthoredRoomObjectRecord& object = envelope_.authoredRoom.objects[index];
      const std::string p = "authoredRoom.object." + std::to_string(index) + ".";
      readString(p + "id", object.id);
      readString(p + "assetId", object.assetId);
      readI32(p + "storyIndex", object.storyIndex);
      readVec3(p + "positionMeters", object.positionMeters);
      readVec3(p + "sizeMeters", object.sizeMeters);
      readFloat(p + "yawDegrees", object.yawDegrees);
      readAuthoredRoomSemantics(p + "semantics.", object.semantics);
      readBool(p + "locked", object.locked);
      readBool(p + "hidden", object.hidden);
      readString(p + "glyph", object.glyph);
      readUnsigned(p + "row", object.row);
      readUnsigned(p + "column", object.column);
      readUnsigned(p + "sourceLine", object.sourceLine);
      readUnsigned(p + "sourceColumn", object.sourceColumn);
    }
  }
  if (!nextKeyIs("authoredRoom.marker.count")) {
    return;
  }
  std::uint64_t markerCount = 0;
  readUnsigned("authoredRoom.marker.count", markerCount);
  envelope_.authoredRoom.markers.resize(static_cast<std::size_t>(markerCount));
  for (std::size_t index = 0; index < envelope_.authoredRoom.markers.size(); ++index) {
    SaveAuthoredRoomMarkerRecord& marker = envelope_.authoredRoom.markers[index];
    const std::string p = "authoredRoom.marker." + std::to_string(index) + ".";
    readString(p + "id", marker.id);
    readString(p + "tag", marker.tag);
    readString(p + "glyph", marker.glyph);
    readUnsigned(p + "row", marker.row);
    readUnsigned(p + "column", marker.column);
    readVec3(p + "positionMeters", marker.positionMeters);
    readUnsigned(p + "sourceLine", marker.sourceLine);
    readUnsigned(p + "sourceColumn", marker.sourceColumn);
  }
}

void Reader::readCreativeDocumentObject(const std::string& prefix,
                                SaveCreativeDocumentObjectRecord& object) {
  readUnsigned(prefix + "id", object.id);
  readString(prefix + "kind", object.kind);
  readString(prefix + "name", object.name);
  if (nextKeyIs(prefix + "assetId")) {
    readString(prefix + "assetId", object.assetId);
  }
  readCreativeVec3(prefix + "transform.position",
                   object.transform.position);
  readCreativeVec3(prefix + "transform.rotation",
                   object.transform.rotation);
  readCreativeVec3(prefix + "transform.scale", object.transform.scale);
  readCreativeVec3(prefix + "bounds.min", object.bounds.min);
  readCreativeVec3(prefix + "bounds.max", object.bounds.max);
  readUnsigned(prefix + "layerId", object.layerId);
  readBool(prefix + "visible", object.visible);
  readBool(prefix + "locked", object.locked);
  readBool(prefix + "hasParent", object.hasParent);
  readUnsigned(prefix + "parentId", object.parentId);
  if (nextKeyIs(prefix + "attachmentSocket")) {
    readString(prefix + "attachmentSocket", object.attachmentSocket);
  }
  readStringVector(prefix + "tag.count", prefix + "tag.", object.tags);
  readOptionalCreativePathPointVector(prefix + "pathPoint.count",
                                      prefix + "pathPoint.",
                                      object.pathPoints);
  if (nextKeyIs(prefix + "movingPlatform.speedMetersPerSecond")) {
    readDouble(prefix + "movingPlatform.speedMetersPerSecond",
               object.movingPlatformSpeedMetersPerSecond);
    readString(prefix + "movingPlatform.traversalMode",
               object.movingPlatformTraversalMode);
    readBool(prefix + "movingPlatform.startsActive",
             object.movingPlatformStartsActive);
  }
}

void Reader::readCreativeDocument() {
  if (!nextKeyIs("creativeDocument.present")) {
    return;
  }
  SaveCreativeDocumentSection& section = envelope_.creativeDocument;
  readBool("creativeDocument.present", section.present);
  if (!section.present) {
    return;
  }
  readUnsigned("creativeDocument.version", section.version);
  readUnsigned("creativeDocument.documentId", section.documentId);
  readString("creativeDocument.name", section.name);
  readString("creativeDocument.units", section.units);
  readCreativeVec3("creativeDocument.grid.origin", section.gridOrigin);
  readDouble("creativeDocument.grid.cellSizeMeters", section.cellSizeMeters);
  readUnsigned("creativeDocument.grid.width", section.gridWidth);
  readUnsigned("creativeDocument.grid.height", section.gridHeight);
  readUnsigned("creativeDocument.grid.depth", section.gridDepth);
  readString("creativeDocument.snap.mode", section.snapMode);
  readUnsigned("creativeDocument.snap.axes", section.snapAxes);
  readDouble("creativeDocument.snap.stepX", section.snapStepX);
  readDouble("creativeDocument.snap.stepY", section.snapStepY);
  readDouble("creativeDocument.snap.stepZ", section.snapStepZ);
  readDouble("creativeDocument.snap.originX", section.snapOriginX);
  readDouble("creativeDocument.snap.originY", section.snapOriginY);
  readDouble("creativeDocument.snap.originZ", section.snapOriginZ);
  readCreativeVec3("creativeDocument.worldBounds.min", section.worldBounds.min);
  readCreativeVec3("creativeDocument.worldBounds.max", section.worldBounds.max);
  readUnsigned("creativeDocument.nextObjectId", section.nextObjectId);
  std::uint64_t objectCount = 0;
  readUnsigned("creativeDocument.object.count", objectCount);
  section.objects.resize(static_cast<std::size_t>(objectCount));
  for (std::size_t index = 0; index < section.objects.size(); ++index) {
    SaveCreativeDocumentObjectRecord& object = section.objects[index];
    const std::string p = "creativeDocument.object." + std::to_string(index) + ".";
    readCreativeDocumentObject(p, object);
  }
  if (nextKeyIs("creativeDocument.logicLink.count")) {
    constexpr std::uint64_t kMaxCreativeLogicLinkCount = 1'048'576U;
    std::uint64_t linkCount = 0U;
    readUnsigned("creativeDocument.logicLink.count", linkCount);
    if (linkCount > kMaxCreativeLogicLinkCount) {
      result_ = fail(SaveCodecStatus::InvalidNumber,
                     "creativeDocument.logicLink.count", index_,
                     "logic link count exceeds limit");
      return;
    }
    section.logicLinks.resize(static_cast<std::size_t>(linkCount));
    for (std::size_t linkIndex = 0U;
         linkIndex < section.logicLinks.size(); ++linkIndex) {
      SaveCreativeDocumentLogicLinkRecord& link =
          section.logicLinks[linkIndex];
      const std::string p = "creativeDocument.logicLink." +
                            std::to_string(linkIndex) + ".";
      readUnsigned(p + "sourceObjectId", link.sourceObjectId);
      readUnsigned(p + "targetObjectId", link.targetObjectId);
      readString(p + "action", link.action);
    }
  }
  if (nextKeyIs("creativeDocument.voxelChunk.count")) {
    constexpr std::uint64_t kMaxCreativeVoxelChunkCount = 1'048'576U;
    constexpr std::uint64_t kMaxCreativeVoxelCellCount = 16'777'216U;
    constexpr std::uint64_t kMaxCreativeVoxelCellsPerChunk = 4096U;
    std::uint64_t chunkCount = 0;
    readUnsigned("creativeDocument.voxelChunk.count", chunkCount);
    if (chunkCount > kMaxCreativeVoxelChunkCount) {
      result_ = fail(SaveCodecStatus::InvalidNumber,
                     "creativeDocument.voxelChunk.count", index_,
                     "voxel chunk count exceeds limit");
      return;
    }
    section.voxelChunks.resize(static_cast<std::size_t>(chunkCount));
    std::uint64_t totalCellCount = 0;
    for (std::size_t chunkIndex = 0;
         chunkIndex < section.voxelChunks.size(); ++chunkIndex) {
      SaveCreativeDocumentVoxelChunkRecord& chunk =
          section.voxelChunks[chunkIndex];
      const std::string p = "creativeDocument.voxelChunk." +
                            std::to_string(chunkIndex) + ".";
      readI32(p + "x", chunk.x);
      readI32(p + "y", chunk.y);
      readI32(p + "z", chunk.z);
      std::uint64_t cellCount = 0;
      readUnsigned(p + "cell.count", cellCount);
      if (cellCount == 0U ||
          cellCount > kMaxCreativeVoxelCellsPerChunk ||
          totalCellCount > kMaxCreativeVoxelCellCount - cellCount) {
        result_ = fail(SaveCodecStatus::InvalidNumber, p + "cell.count",
                       index_, "voxel cell count exceeds limit");
        return;
      }
      totalCellCount += cellCount;
      chunk.cells.resize(static_cast<std::size_t>(cellCount));
      for (std::size_t cellIndex = 0; cellIndex < chunk.cells.size();
           ++cellIndex) {
        SaveCreativeDocumentVoxelCellRecord& cell = chunk.cells[cellIndex];
        const std::string cellPrefix =
            p + "cell." + std::to_string(cellIndex) + ".";
        readUnsigned(cellPrefix + "localIndex", cell.localIndex);
        readString(cellPrefix + "material", cell.material);
      }
    }
  }
  if (nextKeyIs("creativeDocument.terrainControl.count")) {
    std::uint64_t controlCount = 0;
    readUnsigned("creativeDocument.terrainControl.count", controlCount);
    if (controlCount > 256U) {
      result_ = fail(SaveCodecStatus::InvalidNumber,
                     "creativeDocument.terrainControl.count", index_,
                     "terrain control count exceeds limit");
      return;
    }
    section.terrainControls.resize(static_cast<std::size_t>(controlCount));
    for (std::size_t controlIndex = 0;
         controlIndex < section.terrainControls.size(); ++controlIndex) {
      SaveCreativeDocumentTerrainControlRecord& control =
          section.terrainControls[controlIndex];
      const std::string p = "creativeDocument.terrainControl." +
                            std::to_string(controlIndex) + ".";
      readI32(p + "x", control.x);
      readI32(p + "z", control.z);
      readUnsigned(p + "heightCells", control.heightCells);
      readUnsigned(p + "radiusCells", control.radiusCells);
    }
  }
  if (nextKeyIs("creativeDocument.terrainMaterial.count")) {
    std::uint64_t materialCount = 0U;
    readUnsigned("creativeDocument.terrainMaterial.count", materialCount);
    if (materialCount > 8192U) {
      result_ = fail(SaveCodecStatus::InvalidNumber,
                     "creativeDocument.terrainMaterial.count", index_,
                     "terrain material count exceeds limit");
      return;
    }
    section.terrainMaterials.resize(static_cast<std::size_t>(materialCount));
    for (std::size_t materialIndex = 0U;
         materialIndex < section.terrainMaterials.size(); ++materialIndex) {
      SaveCreativeDocumentTerrainMaterialRecord& material =
          section.terrainMaterials[materialIndex];
      const std::string p = "creativeDocument.terrainMaterial." +
                            std::to_string(materialIndex) + ".";
      readI32(p + "x", material.x);
      readI32(p + "z", material.z);
      readString(p + "material", material.material);
    }
  }
}

void Reader::readCreativeWorldLayout() {
  if (!nextKeyIs("creativeWorldLayout.present")) {
    return;
  }
  SaveCreativeWorldLayoutSection& section = envelope_.creativeWorldLayout;
  readBool("creativeWorldLayout.present", section.present);
  if (!section.present) {
    return;
  }
  readUnsigned("creativeWorldLayout.version", section.version);
  readString("creativeWorldLayout.encodedText", section.encodedText);
  constexpr std::size_t kMaxEncodedWorldLayoutBytes = 8U * 1024U * 1024U;
  if (section.encodedText.size() > kMaxEncodedWorldLayoutBytes) {
    result_ = fail(SaveCodecStatus::InvalidNumber,
                   "creativeWorldLayout.encodedText", index_,
                   "world layout source exceeds limit");
  }
}

void Reader::readPlayers() {
  std::uint64_t count = 0;
  readUnsigned("players.slot.count", count);
  envelope_.players.slots.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < envelope_.players.slots.size(); ++index) {
    SavePlayerSlotRecord& slot = envelope_.players.slots[index];
    const std::string p = "players.slot." + std::to_string(index) + ".";
    readUnsigned(p + "slotId", slot.slotId, SaveCodecStatus::InvalidId);
    readEnum(p + "kind", slot.kind);
    readEntityId(p + "controlledActor", slot.controlledActor);
    readString(p + "stableName", slot.stableName);
  }
}

void Reader::readClock() {
  readEnum("clock.mode", envelope_.clock.mode);
  readEnum("clock.previousUnpausedMode", envelope_.clock.previousUnpausedMode);
  readFloat("clock.previousUnpausedTimeScale", envelope_.clock.previousUnpausedTimeScale);
  readUnsigned("clock.tickIndex", envelope_.clock.tickIndex);
  readUnsigned("clock.fixedTickRateHz", envelope_.clock.fixedTickRateHz);
  readFloat("clock.timeScale", envelope_.clock.timeScale);
}

void Reader::readCamera() {
  readEnum("camera.activeMode", envelope_.camera.activeMode);
  readEnum("camera.previousRealtimeMode", envelope_.camera.previousRealtimeMode);
  readEntityId("camera.targetEntity", envelope_.camera.targetEntity);
  readVec3("camera.targetPoint", envelope_.camera.targetPoint);
  readBool("camera.targetHasPoint", envelope_.camera.targetHasPoint);
  readFloat("camera.yawDegrees", envelope_.camera.yawDegrees);
  readFloat("camera.pitchDegrees", envelope_.camera.pitchDegrees);
  readFloat("camera.orbitDistance", envelope_.camera.orbitDistance);
}

void Reader::readAbilities() {
  if (!nextKeyIs("abilities.actor.count")) {
    return;
  }
  std::uint64_t count = 0;
  readUnsigned("abilities.actor.count", count);
  envelope_.abilities.actors.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < envelope_.abilities.actors.size(); ++index) {
    SaveAbilityActorRecord& actor = envelope_.abilities.actors[index];
    const std::string p = "abilities.actor." + std::to_string(index) + ".";
    readEntityId(p + "actor", actor.actor);
    readUnsigned(p + "arcaneFocus", actor.arcaneFocus);
    readUnsigned(p + "arcaneBoltReadyTick", actor.arcaneBoltReadyTick);
    if (nextKeyIs(p + "arcaneFocusNextRechargeTick")) {
      readUnsigned(p + "arcaneFocusNextRechargeTick",
                   actor.arcaneFocusNextRechargeTick);
    }
  }
}

void Reader::readCommandLog() {
  readEnum("commandLog.resetPolicy", envelope_.commandLog.resetPolicy);
  if (envelope_.commandLog.resetPolicy != CommandLogResetPolicy::Clear) {
    result_ = fail(SaveCodecStatus::InvalidEnum, "commandLog.resetPolicy", index_, "unsupported reset policy");
    return;
  }
  readUnsigned("commandLog.nextSequence", envelope_.commandLog.nextSequence, SaveCodecStatus::InvalidSequence);
  readUnsigned("commandLog.epoch", envelope_.commandLog.epoch);
  std::uint64_t count = 0;
  readUnsigned("commandLog.record.count", count);
  envelope_.commandLog.records.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < envelope_.commandLog.records.size(); ++index) {
    SaveCommandRecord& record = envelope_.commandLog.records[index];
    const std::string p = "commandLog.record." + std::to_string(index) + ".";
    readUnsigned(p + "commandId", record.commandId, SaveCodecStatus::InvalidId);
    readUnsigned(p + "sequence", record.sequence, SaveCodecStatus::InvalidSequence);
    readEnum(p + "kind", record.kind);
    readEnum(p + "source", record.source);
    readUnsigned(p + "playerSlot", record.playerSlot, SaveCodecStatus::InvalidId);
    readEntityId(p + "actor", record.actor);
    readBool(p + "hasTargetEntity", record.hasTargetEntity);
    readEntityId(p + "targetEntity", record.targetEntity);
    readBool(p + "hasTargetPoint", record.hasTargetPoint);
    readVec3(p + "targetPoint", record.targetPoint);
    readUnsigned(p + "retrySourceCommandId", record.retrySourceCommandId, SaveCodecStatus::InvalidId);
    readOptionalI32(p + "attackDamage", record.attackDamage);
    if (nextKeyIs(p + "ability")) {
      readEnum(p + "ability", record.ability);
      readVec3(p + "abilityDirection", record.abilityDirection);
    }
    readUnsigned(p + "issuedTick", record.issuedTick);
    readUnsigned(p + "scheduledTick", record.scheduledTick);
    readEnum(p + "admission", record.admission);
    readEnum(p + "rejection", record.rejection);
  }
}

void Reader::readInventory() {
  std::uint64_t count = 0;
  readUnsigned("inventory.player.count", count);
  envelope_.inventory.players.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < envelope_.inventory.players.size(); ++index) {
    SavePlayerInventoryRecord& player = envelope_.inventory.players[index];
    const std::string p = "inventory.player." + std::to_string(index) + ".";
    readUnsigned(p + "playerSlot", player.playerSlot, SaveCodecStatus::InvalidId);
    std::uint64_t stackCount = 0;
    readUnsigned(p + "stack.count", stackCount);
    player.stacks.resize(static_cast<std::size_t>(stackCount));
    for (std::size_t stack = 0; stack < player.stacks.size(); ++stack) {
      const std::string sp = p + "stack." + std::to_string(stack) + ".";
      readString(sp + "itemId", player.stacks[stack].itemId);
      readUnsigned(sp + "count", player.stacks[stack].count);
    }
  }
}

void Reader::readCombat() {
  std::uint64_t count = 0;
  readUnsigned("combat.combatant.count", count);
  envelope_.combat.combatants.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < envelope_.combat.combatants.size(); ++index) {
    SaveCombatantRecord& combatant = envelope_.combat.combatants[index];
    const std::string p = "combat.combatant." + std::to_string(index) + ".";
    readEntityId(p + "entity", combatant.entity);
    readUnsigned(p + "factionId", combatant.factionId);
    readI32(p + "hitPoints", combatant.hitPoints);
    readI32(p + "maxHitPoints", combatant.maxHitPoints);
    readBool(p + "defeated", combatant.defeated);
  }
}

void Reader::readAi() {
  std::uint64_t count = 0;
  readUnsigned("ai.actor.count", count);
  envelope_.ai.actors.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < envelope_.ai.actors.size(); ++index) {
    SaveAiActorRecord& actor = envelope_.ai.actors[index];
    const std::string p = "ai.actor." + std::to_string(index) + ".";
    readEntityId(p + "actor", actor.actor);
    readUnsigned(p + "nextDecisionTick", actor.nextDecisionTick);
    readUnsigned(p + "deterministicPolicy", actor.deterministicPolicy);
    readBool(p + "enabled", actor.enabled);
    if (nextKeyIs(p + "behavior_profile_id")) {
      readString(p + "behavior_profile_id", actor.behaviorProfileId);
    }
    if (actor.behaviorProfileId.empty()) {
      actor.behaviorProfileId = "default";
    }
    if (nextKeyIs(p + "target")) {
      readEntityId(p + "target", actor.target);
    }
    if (nextKeyIs(p + "behavior")) {
      readEnum(p + "behavior", actor.behavior);
    }
    if (nextKeyIs(p + "lastIntent")) {
      readEnum(p + "lastIntent", actor.lastIntent);
    }
    if (nextKeyIs(p + "cooldownTicksRemaining")) {
      readUnsigned(p + "cooldownTicksRemaining", actor.cooldownTicksRemaining);
    }
    if (nextKeyIs(p + "hasHomePosition")) {
      readBool(p + "hasHomePosition", actor.hasHomePosition);
    }
    if (nextKeyIs(p + "homePosition")) {
      readVec3(p + "homePosition", actor.homePosition);
    }
    if (nextKeyIs(p + "homeStableName")) {
      readString(p + "homeStableName", actor.homeStableName);
    }
    if (nextKeyIs(p + "leashRadiusMeters")) {
      readFloat(p + "leashRadiusMeters", actor.leashRadiusMeters);
    }
    if (nextKeyIs(p + "returnRadiusMeters")) {
      readFloat(p + "returnRadiusMeters", actor.returnRadiusMeters);
    }
    if (nextKeyIs(p + "homeToleranceMeters")) {
      readFloat(p + "homeToleranceMeters", actor.homeToleranceMeters);
    }
    // Patrol route + cursor (a2 commit 1). Each key guarded by nextKeyIs so an old envelope
    // missing them decodes to the record defaults (empty route, Loop, 0, true).
    if (nextKeyIs(p + "patrolWaypoint.count")) {
      std::uint64_t waypointCount = 0;
      readUnsigned(p + "patrolWaypoint.count", waypointCount);
      actor.patrolWaypoints.resize(static_cast<std::size_t>(waypointCount));
      for (std::size_t w = 0; w < actor.patrolWaypoints.size(); ++w) {
        readVec3(p + "patrolWaypoint." + std::to_string(w), actor.patrolWaypoints[w]);
      }
    }
    if (nextKeyIs(p + "patrolMode")) {
      readEnum(p + "patrolMode", actor.patrolMode);
    }
    if (nextKeyIs(p + "patrolTargetIndex")) {
      readUnsigned(p + "patrolTargetIndex", actor.patrolTargetIndex);
    }
    if (nextKeyIs(p + "patrolForward")) {
      readBool(p + "patrolForward", actor.patrolForward);
    }
    // Alert FSM + last-known memory + facing (a2 commit 2). Each guarded so an old envelope
    // missing them decodes to the record defaults (fresh, amnesiac-but-valid guard).
    if (nextKeyIs(p + "alertLevel")) {
      readFloat(p + "alertLevel", actor.alertLevel);
    }
    if (nextKeyIs(p + "lastRiseTick")) {
      readUnsigned(p + "lastRiseTick", actor.lastRiseTick);
    }
    if (nextKeyIs(p + "maxAlertIndexThisEngagement")) {
      readUnsigned(p + "maxAlertIndexThisEngagement", actor.maxAlertIndexThisEngagement);
    }
    if (nextKeyIs(p + "graceUntilTick")) {
      readUnsigned(p + "graceUntilTick", actor.graceUntilTick);
    }
    if (nextKeyIs(p + "graceThreshold")) {
      readFloat(p + "graceThreshold", actor.graceThreshold);
    }
    if (nextKeyIs(p + "graceCount")) {
      readUnsigned(p + "graceCount", actor.graceCount);
    }
    if (nextKeyIs(p + "lastKnownTargetPosition")) {
      readVec3(p + "lastKnownTargetPosition", actor.lastKnownTargetPosition);
    }
    if (nextKeyIs(p + "lastKnownTargetTick")) {
      readUnsigned(p + "lastKnownTargetTick", actor.lastKnownTargetTick);
    }
    if (nextKeyIs(p + "hasLastKnownTarget")) {
      readBool(p + "hasLastKnownTarget", actor.hasLastKnownTarget);
    }
    if (nextKeyIs(p + "investigateDwellTicks")) {
      readUnsigned(p + "investigateDwellTicks", actor.investigateDwellTicks);
    }
    if (nextKeyIs(p + "facingDirection")) {
      readVec3(p + "facingDirection", actor.facingDirection);
    }
  }
}

void Reader::readObjectives() {
  std::uint64_t count = 0;
  readUnsigned("objectives.record.count", count);
  envelope_.objectives.objectives.resize(static_cast<std::size_t>(count));
  for (std::size_t index = 0; index < envelope_.objectives.objectives.size(); ++index) {
    SaveObjectiveRecord& objective = envelope_.objectives.objectives[index];
    const std::string p = "objectives.record." + std::to_string(index) + ".";
    readString(p + "objectiveId", objective.objectiveId);
    readEnum(p + "status", objective.status);
    readEnum(p + "conditionKind", objective.conditionKind);
    readUnsigned(p + "conditionPlayerSlot", objective.conditionPlayerSlot, SaveCodecStatus::InvalidId);
    readString(p + "conditionItemId", objective.conditionItemId);
    readUnsigned(p + "conditionItemCount", objective.conditionItemCount);
  }
}

}  // namespace iggy3d::save_codec_detail
