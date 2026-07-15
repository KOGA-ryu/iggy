#pragma once

#include "app/iggy3d/creative/document/DocumentSnap.hpp"
#include "app/iggy3d/creative/document/LogicLink.hpp"
#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/document/TerrainField.hpp"
#include "app/iggy3d/creative/document/TerrainMaterialField.hpp"
#include "app/iggy3d/creative/document/VoxelField.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace iggy3d::creative {

using CreativeDocumentId = std::uint64_t;

inline constexpr CreativeDocumentId kInvalidDocumentId = 0;

enum class CreativeDocumentCreateStatus : std::uint8_t {
  Unknown,
  InvalidDocument,
  InvalidKind,
  Created,
  Rejected,
};

enum class CreativeDocumentRemoveStatus : std::uint8_t {
  Unknown,
  InvalidDocument,
  InvalidObjectId,
  MissingObject,
  LockedObject,
  ParentHasChildren,
  Removed,
};

enum class CreativeDocumentRestoreStatus : std::uint8_t {
  Unknown,
  InvalidDocument,
  InvalidDocumentId,
  InvalidSettings,
  InvalidObject,
  InvalidLogicLink,
  DuplicateObjectId,
  InvalidVoxelField,
  InvalidTerrainField,
  InvalidTerrainMaterialField,
  InvalidNextObjectId,
  Restored,
};

enum class CreativeUnits : std::uint8_t {
  Meters,
};

struct CreativeGridSettings {
  CreativeVec3 origin;
  double cellSizeMeters = 1.0;
  CreativeGridSize3 size;
};

struct CreativeDocumentCreateRequest {
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  std::string name;
  std::string assetId;
  CreativeTransform transform;
  bool hasTransformOverride = false;
  CreativeBounds bounds;
  bool hasBoundsOverride = false;
  CreativeLayerId layerId = kDefaultLayerId;
  bool hasLayerOverride = false;
  bool visible = true;
  bool hasVisibleOverride = false;
  bool locked = false;
  bool hasLockedOverride = false;
  std::vector<std::string> tags;
  std::optional<CreativeObjectId> parentId = std::nullopt;
  std::string attachmentSocket;
  bool hasPathOverride = false;
  std::vector<CreativePathPoint> pathPoints;
};

struct CreativeDocumentCreateReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool objectCreated = false;
  CreativeDocumentCreateStatus status = CreativeDocumentCreateStatus::Unknown;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  std::string objectName;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  CreativeObjectDirtyFlags creationDirtyFlags = 0;
  std::string_view message = "document_create_not_requested";
  std::string_view reasonCode = "document_create_not_requested";
};

struct CreativeDocumentRemoveRequest {
  CreativeObjectId objectId = kInvalidObjectId;
};

struct CreativeDocumentRemoveReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  bool objectRemoved = false;
  CreativeDocumentRemoveStatus status = CreativeDocumentRemoveStatus::Unknown;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  std::string objectName;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t removedLogicLinkCount = 0;
  CreativeObjectDirtyFlags removalDirtyFlags = 0;
  std::string_view message = "document_remove_not_requested";
  std::string_view reasonCode = "document_remove_not_requested";
};

struct CreativeDocumentRestoreRequest {
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::string name;
  CreativeUnits units = CreativeUnits::Meters;
  CreativeGridSettings gridSettings;
  CreativeDocumentSnapSettings snapSettings =
      makeDefaultCreativeDocumentSnapSettings();
  CreativeBounds worldBounds;
  CreativeObjectId nextObjectId = 1;
  std::vector<CreativeObject> objects;
  std::vector<CreativeLogicLink> logicLinks;
  CreativeVoxelField voxelField;
  CreativeTerrainField terrainField;
  CreativeTerrainMaterialField terrainMaterialField;
};

struct CreativeDocumentRestoreReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeDocumentRestoreStatus status =
      CreativeDocumentRestoreStatus::Unknown;
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  std::uint64_t logicLinkCount = 0;
  std::uint64_t voxelCellCount = 0;
  std::uint64_t terrainControlCount = 0;
  std::uint64_t terrainMaterialOverrideCount = 0;
  CreativeObjectId nextObjectId = kInvalidObjectId;
  std::string_view message = "document_restore_not_requested";
  std::string_view reasonCode = "document_restore_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeDocumentCreateStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDocumentRemoveStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDocumentRestoreStatus status) noexcept;

[[nodiscard]] std::string_view validateCreativeObjectParentGraph(
    std::span<const CreativeObject> objects);

// CreativeDocument is the authored content container for one creative work.
// It owns the durable content truth: document identity, revision, and later the
// placed creative objects. It must not own editor tools, UI state, render state,
// filesystem paths, async jobs, or command routing.
//
// Activation policy:
// - Keep the complete intended shape visible in this file.
// - Only enable the currently tested slice.
// - Future fields and functions remain commented until their slice is tested.
class CreativeDocument {
 public:
  CreativeDocument() = default;

  [[nodiscard]] static CreativeDocument create(std::string name);

  [[nodiscard]] bool isValid() const noexcept;
  [[nodiscard]] CreativeDocumentId id() const noexcept;
  [[nodiscard]] std::string_view name() const noexcept;
  [[nodiscard]] std::uint64_t revision() const noexcept;
  [[nodiscard]] CreativeObjectDirtyFlags dirtyFlags() const noexcept;
  [[nodiscard]] CreativeObjectDirtyFlags drainDirtyFlags() noexcept;
  [[nodiscard]] CreativeUnits units() const noexcept;
  [[nodiscard]] CreativeGridSettings gridSettings() const noexcept;
  [[nodiscard]] CreativeDocumentSnapSettings documentSnapSettings()
      const noexcept;
  [[nodiscard]] CreativeBounds worldBounds() const noexcept;
  [[nodiscard]] CreativeObjectId nextObjectId() const noexcept;

  [[nodiscard]] bool assignId(CreativeDocumentId id) noexcept;
  bool setUnits(CreativeUnits units);
  bool setGridSettings(CreativeGridSettings settings);
  bool setDocumentSnapSettings(CreativeDocumentSnapSettings settings);
  bool setWorldBounds(CreativeBounds bounds);
  bool rename(std::string nextName);
  void reset();

  [[nodiscard]] std::uint64_t objectCount() const noexcept;
  [[nodiscard]] bool containsObject(CreativeObjectId id) const noexcept;
  [[nodiscard]] const CreativeObject* findObject(
      CreativeObjectId id) const noexcept;
  [[nodiscard]] CreativeObject* findObject(CreativeObjectId id) noexcept;
  [[nodiscard]] std::span<const CreativeObject> objects() const noexcept;
  [[nodiscard]] std::span<const CreativeLogicLink> logicLinks() const noexcept;
  [[nodiscard]] const CreativeLogicLink* findLogicLink(
      CreativeObjectId sourceObjectId,
      CreativeObjectId targetObjectId) const noexcept;
  [[nodiscard]] const CreativeVoxelField& voxelField() const noexcept;
  [[nodiscard]] const CreativeTerrainField& terrainField() const noexcept;
  [[nodiscard]] const CreativeTerrainMaterialField& terrainMaterialField()
      const noexcept;

  [[nodiscard]] CreativeDocumentCreateReceipt createObject(
      const CreativeDocumentCreateRequest& request);
  [[nodiscard]] CreativeDocumentRemoveReceipt removeDocumentObject(
      const CreativeDocumentRemoveRequest& request);
  [[nodiscard]] CreativeDocumentRemoveReceipt removeDocumentObject(
      CreativeObjectId id);
  [[nodiscard]] CreativeLogicLinkMutationReceipt setLogicLink(
      const CreativeLogicLinkMutationRequest& request);
  [[nodiscard]] CreativeLogicLinkMutationReceipt removeLogicLink(
      CreativeObjectId sourceObjectId,
      CreativeObjectId targetObjectId);
  void markObjectMutationChanged(CreativeObjectDirtyFlags dirtyFlags = 0) noexcept;
  [[nodiscard]] CreativeVoxelMutationReceipt applyVoxelEdits(
      std::span<const CreativeVoxelEdit> edits);
  [[nodiscard]] CreativeTerrainMutationReceipt applyTerrainControlEdits(
      std::span<const CreativeTerrainControlEdit> edits);
  [[nodiscard]] CreativeTerrainMaterialMutationReceipt applyTerrainMaterialEdits(
      std::span<const CreativeTerrainMaterialEdit> edits);
  [[nodiscard]] CreativeDocumentRestoreReceipt restoreForLoad(
      const CreativeDocumentRestoreRequest& request);

  // Future slice: authored spatial content.
  // [[nodiscard]] bool setObjectTransform(CreativeObjectId id, CreativeTransform transform);
  // [[nodiscard]] bool setObjectBounds(CreativeObjectId id, CreativeBounds bounds);

  // Future slice: layers and organization.
  // [[nodiscard]] CreativeLayerId createLayer(std::string name);
  // [[nodiscard]] bool renameLayer(CreativeLayerId id, std::string nextName);
  // [[nodiscard]] bool removeLayer(CreativeLayerId id);
  // [[nodiscard]] bool moveObjectToLayer(CreativeObjectId objectId, CreativeLayerId layerId);

  // Future slice: tags and notes.
  // [[nodiscard]] bool addTag(CreativeObjectId id, std::string tag);
  // [[nodiscard]] bool removeTag(CreativeObjectId id, std::string_view tag);
  // [[nodiscard]] std::string_view notes() const noexcept;
  // bool setNotes(std::string notes);

 private:
  [[nodiscard]] CreativeObjectId appendObject(CreativeObject object);
  [[nodiscard]] std::size_t eraseLogicLinksForObject(
      CreativeObjectId objectId) noexcept;
  void markContentChanged() noexcept;
  void markDirty(CreativeObjectDirtyFlags dirtyFlags) noexcept;

  bool valid_{true};
  CreativeDocumentId id_{kInvalidDocumentId};
  std::string name_{};
  std::uint64_t revision_{0};
  CreativeObjectDirtyFlags dirtyFlags_{0};

  std::vector<CreativeObject> objects_{};
  std::unordered_map<CreativeObjectId, std::size_t> objectIndex_{};
  std::vector<CreativeLogicLink> logicLinks_{};
  CreativeObjectId nextObjectId_{1};
  CreativeVoxelField voxelField_{};
  CreativeTerrainField terrainField_{};
  CreativeTerrainMaterialField terrainMaterialField_{};

  CreativeUnits units_{CreativeUnits::Meters};
  CreativeGridSettings gridSettings_{};
  CreativeDocumentSnapSettings snapSettings_{
      makeDefaultCreativeDocumentSnapSettings()};
  CreativeBounds worldBounds_{};

  // Future slice: organization.
  // std::vector<CreativeLayer> layers_{};
  // CreativeLayerId nextLayerId_{1};
  // std::vector<std::string> tagRegistry_{};
  // std::string notes_{};
};

}  // namespace iggy3d::creative
