#pragma once

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
  void markContentChanged() noexcept;

  bool valid_{true};
  std::string name_{};
  std::uint64_t revision_{0};

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
