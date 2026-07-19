#include "app/iggy3d/creative/document/Document.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/DocumentInternal.hpp"

#include <utility>

namespace iggy3d::creative {

using document_internal::documentIdentityDirtyFlags;
using document_internal::documentSettingsDirtyFlags;
using document_internal::isValidGridSettings;
using document_internal::isValidUnits;
using document_internal::isValidWorldBounds;

namespace {

bool sameGridSize(CreativeGridSize3 lhs, CreativeGridSize3 rhs) noexcept {
  return lhs.width == rhs.width && lhs.height == rhs.height &&
         lhs.depth == rhs.depth;
}

bool sameGridSettings(CreativeGridSettings lhs,
                      CreativeGridSettings rhs) noexcept {
  return creativeVec3ExactlyEqual(lhs.origin, rhs.origin) &&
         lhs.cellSizeMeters == rhs.cellSizeMeters &&
         sameGridSize(lhs.size, rhs.size);
}

bool sameSnapSettings(CreativeDocumentSnapSettings lhs,
                      CreativeDocumentSnapSettings rhs) noexcept {
  return lhs.mode == rhs.mode && lhs.axes == rhs.axes &&
         lhs.stepX == rhs.stepX && lhs.stepY == rhs.stepY &&
         lhs.stepZ == rhs.stepZ && lhs.originX == rhs.originX &&
         lhs.originY == rhs.originY && lhs.originZ == rhs.originZ;
}

}  // namespace

CreativeDocument CreativeDocument::create(std::string name) {
  CreativeDocument document;
  document.valid_ = true;
  document.name_ = std::move(name);
  document.revision_ = 0;
  return document;
}

bool CreativeDocument::isValid() const noexcept {
  return valid_ && voxelField_.isValid() && terrainField_.isValid() &&
         terrainHeightField_.isValid() &&
         validateCreativeTerrainOperationStack(terrainOperationStack_) &&
         (!terrainOperationStack_.operations.empty() ||
          terrainOperationStack_.baseHeightField.cellCount() == 0U) &&
         terrainMaterialField_.isValid();
}

CreativeDocumentId CreativeDocument::id() const noexcept {
  return id_;
}

std::string_view CreativeDocument::name() const noexcept {
  return name_;
}

std::uint64_t CreativeDocument::revision() const noexcept {
  return revision_;
}

CreativeObjectDirtyFlags CreativeDocument::dirtyFlags() const noexcept {
  return dirtyFlags_;
}

CreativeObjectDirtyFlags CreativeDocument::drainDirtyFlags() noexcept {
  const CreativeObjectDirtyFlags drained = dirtyFlags_;
  dirtyFlags_ = 0;
  return drained;
}

CreativeUnits CreativeDocument::units() const noexcept {
  return units_;
}

CreativeGridSettings CreativeDocument::gridSettings() const noexcept {
  return gridSettings_;
}

CreativeDocumentSnapSettings CreativeDocument::documentSnapSettings()
    const noexcept {
  return snapSettings_;
}

CreativeBounds CreativeDocument::worldBounds() const noexcept {
  return worldBounds_;
}

CreativeObjectId CreativeDocument::nextObjectId() const noexcept {
  return nextObjectId_;
}

bool CreativeDocument::assignId(CreativeDocumentId id) noexcept {
  if (id == kInvalidDocumentId || id_ == id ||
      id_ != kInvalidDocumentId) {
    return false;
  }

  id_ = id;
  return true;
}

bool CreativeDocument::setUnits(CreativeUnits units) {
  if (!valid_ || !isValidUnits(units) || units_ == units) {
    return false;
  }

  units_ = units;
  markObjectMutationChanged(documentSettingsDirtyFlags());
  return true;
}

bool CreativeDocument::setGridSettings(CreativeGridSettings settings) {
  if (!valid_ || !isValidGridSettings(settings) ||
      sameGridSettings(gridSettings_, settings)) {
    return false;
  }

  gridSettings_ = settings;
  markObjectMutationChanged(documentSettingsDirtyFlags());
  return true;
}

bool CreativeDocument::setDocumentSnapSettings(
    CreativeDocumentSnapSettings settings) {
  if (!valid_ || !isValidCreativeDocumentSnapSettings(settings) ||
      sameSnapSettings(snapSettings_, settings)) {
    return false;
  }

  snapSettings_ = settings;
  markObjectMutationChanged(documentSettingsDirtyFlags());
  return true;
}

bool CreativeDocument::setWorldBounds(CreativeBounds bounds) {
  if (!valid_ || !isValidWorldBounds(bounds) ||
      creativeBoundsExactlyEqual(worldBounds_, bounds)) {
    return false;
  }

  worldBounds_ = bounds;
  markObjectMutationChanged(documentSettingsDirtyFlags());
  return true;
}

bool CreativeDocument::rename(std::string nextName) {
  if (!valid_ || name_ == nextName) {
    return false;
  }

  name_ = std::move(nextName);
  markObjectMutationChanged(documentIdentityDirtyFlags());
  return true;
}

void CreativeDocument::reset() {
  valid_ = true;
  id_ = kInvalidDocumentId;
  name_.clear();
  revision_ = 0;
  dirtyFlags_ = 0;

  objects_.clear();
  objectIndex_.clear();
  logicLinks_.clear();
  nextObjectId_ = 1;
  voxelField_.clear();
  terrainField_.clear();
  terrainHeightField_.clear();
  terrainOperationStack_ = {};
  terrainMaterialField_.clear();
  units_ = CreativeUnits::Meters;
  gridSettings_ = {};
  snapSettings_ = makeDefaultCreativeDocumentSnapSettings();
  worldBounds_ = {};

  // Future slice reset duties:
  // layers_.clear();
  // nextLayerId_ = 1;
  // tagRegistry_.clear();
  // notes_.clear();
}

std::uint64_t CreativeDocument::objectCount() const noexcept {
  return objects_.size();
}

bool CreativeDocument::containsObject(CreativeObjectId id) const noexcept {
  return objectIndex_.find(id) != objectIndex_.end();
}

const CreativeObject* CreativeDocument::findObject(
    CreativeObjectId id) const noexcept {
  const auto found = objectIndex_.find(id);
  if (found == objectIndex_.end()) {
    return nullptr;
  }
  return &objects_[found->second];
}

CreativeObject* CreativeDocument::findObject(CreativeObjectId id) noexcept {
  const auto found = objectIndex_.find(id);
  if (found == objectIndex_.end()) {
    return nullptr;
  }
  return &objects_[found->second];
}

std::span<const CreativeObject> CreativeDocument::objects() const noexcept {
  return objects_;
}

const CreativeVoxelField& CreativeDocument::voxelField() const noexcept {
  return voxelField_;
}

const CreativeTerrainField& CreativeDocument::terrainField() const noexcept {
  return terrainField_;
}

const CreativeTerrainHeightField& CreativeDocument::terrainHeightField()
    const noexcept {
  return terrainHeightField_;
}

const CreativeTerrainOperationStack&
CreativeDocument::terrainOperationStack() const noexcept {
  return terrainOperationStack_;
}

const CreativeTerrainMaterialField& CreativeDocument::terrainMaterialField()
    const noexcept {
  return terrainMaterialField_;
}

void CreativeDocument::markContentChanged() noexcept {
  if (valid_) {
    ++revision_;
  }
}

void CreativeDocument::markDirty(CreativeObjectDirtyFlags dirtyFlags) noexcept {
  dirtyFlags_ |= dirtyFlags;
}

void CreativeDocument::markObjectMutationChanged(
    CreativeObjectDirtyFlags dirtyFlags) noexcept {
  markContentChanged();
  markDirty(dirtyFlags);
}

CreativeVoxelMutationReceipt CreativeDocument::applyVoxelEdits(
    std::span<const CreativeVoxelEdit> edits) {
  if (!valid_) {
    CreativeVoxelMutationReceipt receipt;
    receipt.requested = true;
    receipt.attemptedCellCount = edits.size();
    receipt.status = CreativeVoxelMutationStatus::InvalidField;
    receipt.reasonCode = "creative_voxel_document_invalid";
    return receipt;
  }

  CreativeObjectDirtyFlags dirtyFlags = 0;
  for (const CreativeVoxelEdit& edit : edits) {
    const CreativeObjectKind oldMaterial = voxelField_.materialAt(edit.cell);
    if (oldMaterial != CreativeObjectKind::Unknown) {
      dirtyFlags |= dirtyFlagsForCreation(oldMaterial);
    }
    if (edit.material != CreativeObjectKind::Unknown &&
        edit.material != CreativeObjectKind::Count) {
      dirtyFlags |= dirtyFlagsForCreation(edit.material);
    }
  }

  CreativeVoxelMutationReceipt receipt = voxelField_.apply(edits);
  if (receipt.changed) {
    markObjectMutationChanged(dirtyFlags | documentSettingsDirtyFlags());
  }
  return receipt;
}

CreativeTerrainMutationReceipt CreativeDocument::applyTerrainControlEdits(
    std::span<const CreativeTerrainControlEdit> edits) {
  if (!valid_) {
    CreativeTerrainMutationReceipt receipt;
    receipt.requested = true;
    receipt.attemptedEditCount = edits.size();
    receipt.status = CreativeTerrainMutationStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_document_invalid";
    return receipt;
  }

  if (!terrainOperationStack_.operations.empty()) {
    CreativeTerrainField stagedTerrain = terrainField_;
    CreativeTerrainMutationReceipt receipt = stagedTerrain.apply(edits);
    if (!receipt.accepted || !receipt.changed) {
      return receipt;
    }
    const CreativeTerrainOperationReplayResult replay =
        replayCreativeTerrainOperations(stagedTerrain,
                                        terrainOperationStack_);
    if (!replay.receipt.accepted) {
      receipt.accepted = false;
      receipt.changed = false;
      receipt.status = CreativeTerrainMutationStatus::InvalidField;
      receipt.revisionAfter = receipt.revisionBefore;
      receipt.controlCountAfter = receipt.controlCountBefore;
      receipt.changedControlCount = 0U;
      receipt.reasonCode =
          "creative_terrain_operation_control_replay_rejected";
      return receipt;
    }
    terrainField_ = std::move(stagedTerrain);
    terrainHeightField_ = replay.heightField;
    markObjectMutationChanged(
        dirtyFlagsForCreation(CreativeObjectKind::TerrainPatch) |
        documentSettingsDirtyFlags());
    return receipt;
  }

  CreativeTerrainMutationReceipt receipt = terrainField_.apply(edits);
  if (receipt.changed) {
    markObjectMutationChanged(
        dirtyFlagsForCreation(CreativeObjectKind::TerrainPatch) |
        documentSettingsDirtyFlags());
  }
  return receipt;
}

CreativeTerrainHeightFieldReplaceReceipt
CreativeDocument::replaceTerrainHeightField(
    CreativeTerrainHeightFieldBounds bounds,
    std::span<const std::uint16_t> heights) {
  if (!valid_) {
    CreativeTerrainHeightFieldReplaceReceipt receipt;
    receipt.requested = true;
    receipt.cellCountBefore = terrainHeightField_.cellCount();
    receipt.cellCountAfter = receipt.cellCountBefore;
    receipt.status = CreativeTerrainHeightFieldReplaceStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_height_document_invalid";
    return receipt;
  }
  CreativeTerrainHeightFieldReplaceReceipt receipt =
      terrainHeightField_.replace(bounds, heights);
  if (receipt.changed) {
    terrainOperationStack_ = {};
    markObjectMutationChanged(
        dirtyFlagsForCreation(CreativeObjectKind::TerrainPatch) |
        documentSettingsDirtyFlags());
  }
  return receipt;
}

CreativeTerrainMaterialMutationReceipt
CreativeDocument::applyTerrainMaterialEdits(
    std::span<const CreativeTerrainMaterialEdit> edits) {
  if (!valid_) {
    CreativeTerrainMaterialMutationReceipt receipt;
    receipt.requested = true;
    receipt.attemptedEditCount = edits.size();
    receipt.status = CreativeTerrainMaterialMutationStatus::InvalidField;
    receipt.reasonCode = "creative_terrain_material_document_invalid";
    return receipt;
  }
  CreativeTerrainMaterialMutationReceipt receipt =
      terrainMaterialField_.apply(edits);
  if (receipt.changed) {
    markObjectMutationChanged(documentSettingsDirtyFlags());
  }
  return receipt;
}

}  // namespace iggy3d::creative
