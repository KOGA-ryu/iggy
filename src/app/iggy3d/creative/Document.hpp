#pragma once

#include "app/iggy3d/creative/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/Object.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace iggy3d::creative {

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
  Removed,
};

struct CreativeDocumentCreateRequest {
  CreativeObjectKind kind = CreativeObjectKind::Unknown;
  std::string name;
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
  CreativeObjectDirtyFlags removalDirtyFlags = 0;
  std::string_view message = "document_remove_not_requested";
  std::string_view reasonCode = "document_remove_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeDocumentCreateStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDocumentRemoveStatus status) noexcept;

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
  [[nodiscard]] std::string_view name() const noexcept;
  [[nodiscard]] std::uint64_t revision() const noexcept;
  [[nodiscard]] CreativeObjectDirtyFlags dirtyFlags() const noexcept;
  [[nodiscard]] CreativeObjectDirtyFlags drainDirtyFlags() noexcept;

  bool rename(std::string nextName);
  void reset();

  // Future slice: document identity.
  // [[nodiscard]] CreativeDocumentId id() const noexcept;

  [[nodiscard]] std::uint64_t objectCount() const noexcept;
  [[nodiscard]] bool containsObject(CreativeObjectId id) const noexcept;
  [[nodiscard]] const CreativeObject* findObject(
      CreativeObjectId id) const noexcept;
  [[nodiscard]] CreativeObject* findObject(CreativeObjectId id) noexcept;
  [[nodiscard]] std::span<const CreativeObject> objects() const noexcept;

  [[nodiscard]] CreativeDocumentCreateReceipt createObject(
      const CreativeDocumentCreateRequest& request);
  [[nodiscard]] CreativeDocumentRemoveReceipt removeDocumentObject(
      const CreativeDocumentRemoveRequest& request);
  [[nodiscard]] CreativeDocumentRemoveReceipt removeDocumentObject(
      CreativeObjectId id);
  [[nodiscard]] CreativeObjectId createRoom(
      std::string name,
      CreativeTransform transform = {},
      CreativeBounds bounds = {},
      CreativeLayerId layerId = kDefaultLayerId,
      bool visible = true,
      bool locked = false,
      std::vector<std::string> tags = {},
      std::optional<CreativeObjectId> parentId = std::nullopt);
  [[nodiscard]] bool renameObject(CreativeObjectId id, std::string nextName);
  [[nodiscard]] bool removeObject(CreativeObjectId id);
  void markObjectMutationChanged(CreativeObjectDirtyFlags dirtyFlags = 0) noexcept;

  // Future slice: authored spatial content.
  // [[nodiscard]] bool setObjectTransform(CreativeObjectId id, CreativeTransform transform);
  // [[nodiscard]] bool setObjectBounds(CreativeObjectId id, CreativeBounds bounds);

  // Future slice: document-level authoring settings.
  // [[nodiscard]] CreativeUnits units() const noexcept;
  // [[nodiscard]] CreativeGridSettings gridSettings() const noexcept;
  // [[nodiscard]] CreativeSnapSettings snapSettings() const noexcept;
  // [[nodiscard]] CreativeBounds worldBounds() const noexcept;
  // bool setUnits(CreativeUnits units);
  // bool setGridSettings(CreativeGridSettings settings);
  // bool setSnapSettings(CreativeSnapSettings settings);
  // bool setWorldBounds(CreativeBounds bounds);

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
  void markContentChanged() noexcept;
  void markDirty(CreativeObjectDirtyFlags dirtyFlags) noexcept;

  bool valid_{true};
  std::string name_{};
  std::uint64_t revision_{0};
  CreativeObjectDirtyFlags dirtyFlags_{0};

  // Future slice: stable document identity.
  // CreativeDocumentId id_{};

  std::vector<CreativeObject> objects_{};
  std::unordered_map<CreativeObjectId, std::size_t> objectIndex_{};
  CreativeObjectId nextObjectId_{1};

  // Future slice: document authoring settings.
  // CreativeUnits units_{CreativeUnits::Meters};
  // CreativeGridSettings gridSettings_{};
  // CreativeSnapSettings snapSettings_{};
  // CreativeBounds worldBounds_{};

  // Future slice: organization.
  // std::vector<CreativeLayer> layers_{};
  // CreativeLayerId nextLayerId_{1};
  // std::vector<std::string> tagRegistry_{};
  // std::string notes_{};
};

}  // namespace iggy3d::creative
