#include "app/iggy3d/save/CreativeDocumentSection.hpp"

#include <limits>
#include <optional>
#include <span>
#include <utility>

namespace iggy3d {
namespace {

void setStatus(ProductCreativeDocumentSectionReceipt& receipt,
               ProductCreativeDocumentSectionStatus status,
               std::string_view reason) noexcept {
  receipt.status = status;
  receipt.message = reason;
  receipt.reasonCode = reason;
}

[[nodiscard]] SaveCreativeDocumentVec3Record toSaveVec3(
    creative::CreativeVec3 value) noexcept {
  return {value.x, value.y, value.z};
}

[[nodiscard]] creative::CreativeVec3 toCreativeVec3(
    SaveCreativeDocumentVec3Record value) noexcept {
  return {value.x, value.y, value.z};
}

[[nodiscard]] SaveCreativeDocumentTransformRecord toSaveTransform(
    creative::CreativeTransform transform) noexcept {
  SaveCreativeDocumentTransformRecord record;
  record.position = toSaveVec3(transform.position);
  record.rotation = toSaveVec3(transform.rotation);
  record.scale = toSaveVec3(transform.scale);
  return record;
}

[[nodiscard]] creative::CreativeTransform toCreativeTransform(
    SaveCreativeDocumentTransformRecord record) noexcept {
  creative::CreativeTransform transform;
  transform.position = toCreativeVec3(record.position);
  transform.rotation = toCreativeVec3(record.rotation);
  transform.scale = toCreativeVec3(record.scale);
  return transform;
}

[[nodiscard]] SaveCreativeDocumentBoundsRecord toSaveBounds(
    creative::CreativeBounds bounds) noexcept {
  return {toSaveVec3(bounds.min), toSaveVec3(bounds.max)};
}

[[nodiscard]] creative::CreativeBounds toCreativeBounds(
    SaveCreativeDocumentBoundsRecord record) noexcept {
  return {toCreativeVec3(record.min), toCreativeVec3(record.max)};
}

[[nodiscard]] std::string_view toSaveUnits(
    creative::CreativeUnits units) noexcept {
  switch (units) {
    case creative::CreativeUnits::Meters:
      return "Meters";
  }
  return {};
}

[[nodiscard]] bool parseUnits(std::string_view value,
                              creative::CreativeUnits& out) noexcept {
  if (value == "Meters") {
    out = creative::CreativeUnits::Meters;
    return true;
  }
  return false;
}

[[nodiscard]] std::string_view toSaveSnapMode(
    creative::CreativeDocumentSnapMode mode) noexcept {
  switch (mode) {
    case creative::CreativeDocumentSnapMode::Disabled:
      return "Disabled";
    case creative::CreativeDocumentSnapMode::Grid:
      return "Grid";
  }
  return {};
}

[[nodiscard]] bool parseSnapMode(
    std::string_view value,
    creative::CreativeDocumentSnapMode& out) noexcept {
  if (value == "Disabled") {
    out = creative::CreativeDocumentSnapMode::Disabled;
    return true;
  }
  if (value == "Grid") {
    out = creative::CreativeDocumentSnapMode::Grid;
    return true;
  }
  return false;
}

[[nodiscard]] bool toCreativeGridSize(
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t depth,
    creative::CreativeGridSize3& out) noexcept {
  constexpr auto maxValue =
      static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
  if (width > maxValue || height > maxValue || depth > maxValue) {
    return false;
  }
  out.width = static_cast<std::int32_t>(width);
  out.height = static_cast<std::int32_t>(height);
  out.depth = static_cast<std::int32_t>(depth);
  return true;
}

[[nodiscard]] bool toSaveGridDimension(std::int32_t value,
                                       std::uint32_t& out) noexcept {
  if (value < 0) {
    return false;
  }
  out = static_cast<std::uint32_t>(value);
  return true;
}

[[nodiscard]] bool toSaveGridSettings(
    creative::CreativeGridSettings settings,
    SaveCreativeDocumentSection& section) noexcept {
  section.gridOrigin = toSaveVec3(settings.origin);
  section.cellSizeMeters = settings.cellSizeMeters;
  return toSaveGridDimension(settings.size.width, section.gridWidth) &&
         toSaveGridDimension(settings.size.height, section.gridHeight) &&
         toSaveGridDimension(settings.size.depth, section.gridDepth);
}

[[nodiscard]] bool toCreativeGridSettings(
    const SaveCreativeDocumentSection& section,
    creative::CreativeGridSettings& out) noexcept {
  out.origin = toCreativeVec3(section.gridOrigin);
  out.cellSizeMeters = section.cellSizeMeters;
  return toCreativeGridSize(section.gridWidth,
                            section.gridHeight,
                            section.gridDepth,
                            out.size);
}

[[nodiscard]] bool toSaveSnapSettings(
    creative::CreativeDocumentSnapSettings settings,
    SaveCreativeDocumentSection& section) noexcept {
  const std::string_view snapMode = toSaveSnapMode(settings.mode);
  if (snapMode.empty()) {
    return false;
  }
  section.snapMode = std::string{snapMode};
  section.snapAxes = settings.axes;
  section.snapStepX = settings.stepX;
  section.snapStepY = settings.stepY;
  section.snapStepZ = settings.stepZ;
  section.snapOriginX = settings.originX;
  section.snapOriginY = settings.originY;
  section.snapOriginZ = settings.originZ;
  return true;
}

[[nodiscard]] bool toCreativeSnapSettings(
    const SaveCreativeDocumentSection& section,
    creative::CreativeDocumentSnapSettings& out) noexcept {
  if (!parseSnapMode(section.snapMode, out.mode) ||
      section.snapAxes > std::numeric_limits<
                             creative::CreativeDocumentSnapAxisMask>::max()) {
    return false;
  }
  out.axes =
      static_cast<creative::CreativeDocumentSnapAxisMask>(section.snapAxes);
  out.stepX = section.snapStepX;
  out.stepY = section.snapStepY;
  out.stepZ = section.snapStepZ;
  out.originX = section.snapOriginX;
  out.originY = section.snapOriginY;
  out.originZ = section.snapOriginZ;
  return creative::isValidCreativeDocumentSnapSettings(out);
}

[[nodiscard]] bool parseObjectKind(std::string_view value,
                                   creative::CreativeObjectKind& out) noexcept {
  if (value.empty()) {
    return false;
  }
  for (const creative::CreativeObjectDescriptor& descriptor :
       creative::allObjectDescriptors()) {
    if (descriptor.kind != creative::CreativeObjectKind::Unknown &&
        descriptor.name == value) {
      out = descriptor.kind;
      return true;
    }
  }
  return false;
}

[[nodiscard]] SaveCreativeDocumentObjectRecord toSaveObject(
    const creative::CreativeObject& object) {
  SaveCreativeDocumentObjectRecord record;
  record.id = object.id;
  record.kind = std::string{creative::toString(object.kind)};
  record.name = object.name;
  record.transform = toSaveTransform(object.transform);
  record.bounds = toSaveBounds(object.bounds);
  record.layerId = object.layerId;
  record.visible = object.visible;
  record.locked = object.locked;
  record.hasParent = object.parentId.has_value();
  record.parentId = object.parentId.value_or(creative::kInvalidObjectId);
  record.tags = object.tags;
  return record;
}

[[nodiscard]] bool toCreativeObject(
    const SaveCreativeDocumentObjectRecord& record,
    creative::CreativeObject& out) noexcept {
  creative::CreativeObjectKind kind = creative::CreativeObjectKind::Unknown;
  if (!parseObjectKind(record.kind, kind)) {
    return false;
  }
  out.id = record.id;
  out.kind = kind;
  out.name = record.name;
  out.transform = toCreativeTransform(record.transform);
  out.bounds = toCreativeBounds(record.bounds);
  out.layerId = record.layerId;
  out.visible = record.visible;
  out.locked = record.locked;
  out.parentId = record.hasParent
                     ? std::optional<creative::CreativeObjectId>{record.parentId}
                     : std::nullopt;
  out.tags = record.tags;
  return true;
}

[[nodiscard]] bool objectIdsAreUniqueAndNextIdIsValid(
    std::span<const SaveCreativeDocumentObjectRecord> objects,
    creative::CreativeObjectId nextObjectId,
    ProductCreativeDocumentSectionReceipt& receipt) {
  creative::CreativeObjectId maxObjectId = creative::kInvalidObjectId;
  for (std::size_t index = 0; index < objects.size(); ++index) {
    const creative::CreativeObjectId id = objects[index].id;
    if (id == creative::kInvalidObjectId) {
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidObject,
                "invalid_object");
      return false;
    }
    for (std::size_t prior = 0; prior < index; ++prior) {
      if (objects[prior].id == id) {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::DuplicateObjectId,
                  "duplicate_object_id");
        return false;
      }
    }
    if (id > maxObjectId) {
      maxObjectId = id;
    }
  }

  if (nextObjectId == creative::kInvalidObjectId ||
      nextObjectId <= maxObjectId) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidNextObjectId,
              "invalid_next_object_id");
    return false;
  }
  return true;
}

void mirrorRestoreFailure(ProductCreativeDocumentSectionReceipt& receipt,
                          creative::CreativeDocumentRestoreStatus status,
                          std::string_view reason) noexcept {
  switch (status) {
    case creative::CreativeDocumentRestoreStatus::InvalidDocument:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidDocument,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidDocumentId:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidDocumentId,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidSettings:
      if (reason == "invalid_grid_settings") {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidGrid,
                  reason);
      } else if (reason == "invalid_document_snap_settings") {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidSnap,
                  reason);
      } else if (reason == "invalid_world_bounds") {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidWorldBounds,
                  reason);
      } else if (reason == "invalid_units") {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidUnits,
                  reason);
      } else {
        setStatus(receipt,
                  ProductCreativeDocumentSectionStatus::InvalidDocument,
                  reason);
      }
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidObject:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidObject,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::DuplicateObjectId:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::DuplicateObjectId,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::InvalidNextObjectId:
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidNextObjectId,
                reason);
      return;
    case creative::CreativeDocumentRestoreStatus::Unknown:
    case creative::CreativeDocumentRestoreStatus::Restored:
      break;
  }
  setStatus(receipt,
            ProductCreativeDocumentSectionStatus::InvalidDocument,
            reason);
}

}  // namespace

std::string_view toString(
    ProductCreativeDocumentSectionStatus status) noexcept {
  switch (status) {
    case ProductCreativeDocumentSectionStatus::Unknown:
      return "Unknown";
    case ProductCreativeDocumentSectionStatus::MissingSection:
      return "MissingSection";
    case ProductCreativeDocumentSectionStatus::InvalidDocument:
      return "InvalidDocument";
    case ProductCreativeDocumentSectionStatus::InvalidDocumentId:
      return "InvalidDocumentId";
    case ProductCreativeDocumentSectionStatus::InvalidUnits:
      return "InvalidUnits";
    case ProductCreativeDocumentSectionStatus::InvalidGrid:
      return "InvalidGrid";
    case ProductCreativeDocumentSectionStatus::InvalidSnap:
      return "InvalidSnap";
    case ProductCreativeDocumentSectionStatus::InvalidWorldBounds:
      return "InvalidWorldBounds";
    case ProductCreativeDocumentSectionStatus::InvalidObject:
      return "InvalidObject";
    case ProductCreativeDocumentSectionStatus::InvalidObjectKind:
      return "InvalidObjectKind";
    case ProductCreativeDocumentSectionStatus::DuplicateObjectId:
      return "DuplicateObjectId";
    case ProductCreativeDocumentSectionStatus::InvalidNextObjectId:
      return "InvalidNextObjectId";
    case ProductCreativeDocumentSectionStatus::Converted:
      return "Converted";
  }
  return "Unknown";
}

ProductCreativeDocumentSectionBuildResult buildSaveCreativeDocumentSection(
    const creative::CreativeDocument& document) {
  ProductCreativeDocumentSectionBuildResult result;
  ProductCreativeDocumentSectionReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.documentId = document.id();
  receipt.objectCount = document.objectCount();
  receipt.nextObjectId = document.nextObjectId();

  if (!document.isValid()) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidDocument,
              "invalid_document");
    return result;
  }

  if (document.id() == creative::kInvalidDocumentId) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidDocumentId,
              "invalid_document_id");
    return result;
  }

  const std::string_view units = toSaveUnits(document.units());
  if (units.empty()) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidUnits,
              "invalid_units");
    return result;
  }

  result.section.present = true;
  result.section.version = 1;
  result.section.documentId = document.id();
  result.section.name = std::string{document.name()};
  result.section.units = std::string{units};
  if (!toSaveGridSettings(document.gridSettings(), result.section)) {
    result.section = {};
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidGrid,
              "invalid_grid_settings");
    return result;
  }
  if (!toSaveSnapSettings(document.documentSnapSettings(), result.section)) {
    result.section = {};
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidSnap,
              "invalid_document_snap_settings");
    return result;
  }
  result.section.worldBounds = toSaveBounds(document.worldBounds());
  result.section.nextObjectId = document.nextObjectId();
  result.section.objects.reserve(document.objects().size());
  for (const creative::CreativeObject& object : document.objects()) {
    result.section.objects.push_back(toSaveObject(object));
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = ProductCreativeDocumentSectionStatus::Converted;
  receipt.objectCount = result.section.objects.size();
  receipt.nextObjectId = result.section.nextObjectId;
  receipt.message = "creative_document_section_converted";
  receipt.reasonCode = "creative_document_section_converted";
  return result;
}

ProductCreativeDocumentSectionRestoreResult restoreCreativeDocumentFromSaveSection(
    const SaveCreativeDocumentSection& section) {
  ProductCreativeDocumentSectionRestoreResult result;
  ProductCreativeDocumentSectionReceipt& receipt = result.receipt;
  receipt.requested = true;
  receipt.documentId = section.documentId;
  receipt.objectCount = section.objects.size();
  receipt.nextObjectId = section.nextObjectId;

  if (!section.present) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::MissingSection,
              "missing_creative_document_section");
    return result;
  }

  creative::CreativeDocumentRestoreRequest request;
  request.documentId = section.documentId;
  request.name = section.name;
  if (!parseUnits(section.units, request.units)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidUnits,
              "invalid_units");
    return result;
  }
  if (!toCreativeGridSettings(section, request.gridSettings)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidGrid,
              "invalid_grid_settings");
    return result;
  }
  if (!toCreativeSnapSettings(section, request.snapSettings)) {
    setStatus(receipt,
              ProductCreativeDocumentSectionStatus::InvalidSnap,
              "invalid_document_snap_settings");
    return result;
  }
  request.worldBounds = toCreativeBounds(section.worldBounds);
  request.nextObjectId = section.nextObjectId;

  if (!objectIdsAreUniqueAndNextIdIsValid(section.objects,
                                          section.nextObjectId,
                                          receipt)) {
    return result;
  }

  request.objects.reserve(section.objects.size());
  for (const SaveCreativeDocumentObjectRecord& objectRecord : section.objects) {
    creative::CreativeObject object;
    if (!toCreativeObject(objectRecord, object)) {
      setStatus(receipt,
                ProductCreativeDocumentSectionStatus::InvalidObjectKind,
                "invalid_object_kind");
      return result;
    }
    request.objects.push_back(std::move(object));
  }

  const creative::CreativeDocumentRestoreReceipt restored =
      result.document.restoreForLoad(request);
  if (!restored.accepted) {
    mirrorRestoreFailure(receipt, restored.status, restored.reasonCode);
    return result;
  }

  receipt.accepted = true;
  receipt.changed = true;
  receipt.status = ProductCreativeDocumentSectionStatus::Converted;
  receipt.message = "creative_document_section_converted";
  receipt.reasonCode = "creative_document_section_converted";
  return result;
}

}  // namespace iggy3d
