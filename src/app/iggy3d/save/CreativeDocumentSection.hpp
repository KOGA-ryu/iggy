#pragma once

#include "app/iggy3d/creative/Document.hpp"
#include "runtime/save/SaveEnvelope.hpp"

#include <cstdint>
#include <string_view>

namespace iggy3d {

enum class ProductCreativeDocumentSectionStatus : std::uint8_t {
  Unknown,
  MissingSection,
  InvalidDocument,
  InvalidDocumentId,
  InvalidUnits,
  InvalidGrid,
  InvalidSnap,
  InvalidWorldBounds,
  InvalidObject,
  InvalidObjectKind,
  DuplicateObjectId,
  InvalidNextObjectId,
  Converted,
};

struct ProductCreativeDocumentSectionReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  ProductCreativeDocumentSectionStatus status =
      ProductCreativeDocumentSectionStatus::Unknown;
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  creative::CreativeObjectId nextObjectId = creative::kInvalidObjectId;
  std::string_view message = "creative_document_section_not_requested";
  std::string_view reasonCode = "creative_document_section_not_requested";
};

struct ProductCreativeDocumentSectionBuildResult {
  SaveCreativeDocumentSection section;
  ProductCreativeDocumentSectionReceipt receipt;
};

struct ProductCreativeDocumentSectionRestoreResult {
  creative::CreativeDocument document;
  ProductCreativeDocumentSectionReceipt receipt;
};

[[nodiscard]] std::string_view toString(
    ProductCreativeDocumentSectionStatus status) noexcept;

[[nodiscard]] ProductCreativeDocumentSectionBuildResult
buildSaveCreativeDocumentSection(const creative::CreativeDocument& document);

[[nodiscard]] ProductCreativeDocumentSectionRestoreResult
restoreCreativeDocumentFromSaveSection(
    const SaveCreativeDocumentSection& section);

}  // namespace iggy3d
