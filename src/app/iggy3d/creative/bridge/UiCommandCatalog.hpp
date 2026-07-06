#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace iggy3d {

enum class ProductCreativeUiCommandKind : std::uint8_t {
  None,
  ToggleSelectedObjectVisibility,
  ToggleSelectedObjectLocked,
  SetActiveTool,
  CreateObject,
  RebuildRoom,
  UndoLastDocumentChange,
  DeleteSelectedObject,
  GenerateSelectedRoomShell,
  RemoveSelectedRoomShell,
};

struct ProductCreativeUiCommandCatalogEntry {
  std::string_view semanticId;
  std::string_view rowId;
  std::string_view label;
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
  creative::Tool tool = creative::Tool::Select;
  creative::CreativeObjectKind objectKind =
      creative::CreativeObjectKind::Unknown;
};

struct ProductCreativeUiCommandKindMetadata {
  ProductCreativeUiCommandKind commandKind =
      ProductCreativeUiCommandKind::None;
  std::string_view receiptName;
  bool expectsHandler = false;
};

inline constexpr std::array<ProductCreativeUiCommandCatalogEntry, 11>
    kProductCreativeUiCommandCatalog = {{
        {"creative.row.tools.tool_select",
         "tool_select",
         "Select",
         ProductCreativeUiCommandKind::SetActiveTool,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.tools.tool_move",
         "tool_move",
         "Move",
         ProductCreativeUiCommandKind::SetActiveTool,
         creative::Tool::Move,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.tools.tool_measure",
         "tool_measure",
         "Measure",
         ProductCreativeUiCommandKind::SetActiveTool,
         creative::Tool::Measure,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.tools.tool_navigate",
         "tool_navigate",
         "Navigate",
         ProductCreativeUiCommandKind::SetActiveTool,
         creative::Tool::Navigate,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.tools.rebuild_room",
         "rebuild_room",
         "Rebuild Room",
         ProductCreativeUiCommandKind::RebuildRoom,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.tools.undo",
         "undo",
         "Undo",
         ProductCreativeUiCommandKind::UndoLastDocumentChange,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.selection.inspector_visible",
         "inspector_visible",
         "Inspector Visible",
         ProductCreativeUiCommandKind::ToggleSelectedObjectVisibility,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.selection.inspector_locked",
         "inspector_locked",
         "Inspector Locked",
         ProductCreativeUiCommandKind::ToggleSelectedObjectLocked,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.selection.delete_selected",
         "delete_selected",
         "Delete Selected",
         ProductCreativeUiCommandKind::DeleteSelectedObject,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.selection.generate_room_shell",
         "generate_room_shell",
         "Generate Room Shell",
         ProductCreativeUiCommandKind::GenerateSelectedRoomShell,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
        {"creative.row.selection.remove_room_shell",
         "remove_room_shell",
         "Remove Room Shell",
         ProductCreativeUiCommandKind::RemoveSelectedRoomShell,
         creative::Tool::Select,
         creative::CreativeObjectKind::Unknown},
    }};

inline constexpr std::array<ProductCreativeUiCommandCatalogEntry, 2>
    kProductCreativeUiCreatePalette = {{
        {"creative.row.create.create_room",
         "create_room",
         "Create Room",
         ProductCreativeUiCommandKind::CreateObject,
         creative::Tool::Select,
         creative::CreativeObjectKind::Room},
        {"creative.row.create.create_crate",
         "create_crate",
         "Create Crate",
         ProductCreativeUiCommandKind::CreateObject,
         creative::Tool::Select,
         creative::CreativeObjectKind::Crate},
    }};

inline constexpr std::array<ProductCreativeUiCommandKindMetadata, 10>
    kProductCreativeUiCommandKindMetadata = {{
        {ProductCreativeUiCommandKind::None, "none", false},
        {ProductCreativeUiCommandKind::ToggleSelectedObjectVisibility,
         "toggle_selected_object_visibility",
         true},
        {ProductCreativeUiCommandKind::ToggleSelectedObjectLocked,
         "toggle_selected_object_locked",
         true},
        {ProductCreativeUiCommandKind::SetActiveTool,
         "set_active_tool",
         true},
        {ProductCreativeUiCommandKind::CreateObject,
         "create_object",
         true},
        {ProductCreativeUiCommandKind::RebuildRoom,
         "rebuild_room",
         true},
        {ProductCreativeUiCommandKind::UndoLastDocumentChange,
         "undo_last_document_change",
         true},
        {ProductCreativeUiCommandKind::DeleteSelectedObject,
         "delete_selected_object",
         true},
        {ProductCreativeUiCommandKind::GenerateSelectedRoomShell,
         "generate_selected_room_shell",
         true},
        {ProductCreativeUiCommandKind::RemoveSelectedRoomShell,
         "remove_selected_room_shell",
         true},
    }};

[[nodiscard]] inline std::span<const ProductCreativeUiCommandCatalogEntry>
productCreativeUiCommandCatalog() noexcept {
  return kProductCreativeUiCommandCatalog;
}

[[nodiscard]] inline std::span<const ProductCreativeUiCommandCatalogEntry>
productCreativeUiCreatePalette() noexcept {
  return kProductCreativeUiCreatePalette;
}

[[nodiscard]] inline std::span<const ProductCreativeUiCommandKindMetadata>
productCreativeUiCommandKindMetadataCatalog() noexcept {
  return kProductCreativeUiCommandKindMetadata;
}

[[nodiscard]] inline bool productCreativeUiCreatePaletteEntryAllowed(
    const ProductCreativeUiCommandCatalogEntry& entry) noexcept {
  if (entry.commandKind != ProductCreativeUiCommandKind::CreateObject) {
    return false;
  }

  const creative::CreativeObjectDescriptor& descriptor =
      creative::describeObject(entry.objectKind);
  if (descriptor.kind == creative::CreativeObjectKind::Unknown ||
      descriptor.kind != entry.objectKind || descriptor.isEditorOnly) {
    return false;
  }

  if (descriptor.profile == creative::CreativeObjectProfile::RoomContainer) {
    return descriptor.hasBounds && descriptor.canOwnChildren;
  }

  return creative::descriptorShowsInAuthoringBrushPalette(descriptor) &&
         descriptor.hasTransform && descriptor.hasBounds;
}

[[nodiscard]] inline const ProductCreativeUiCommandCatalogEntry*
findProductCreativeUiCommandBySemanticId(std::string_view semanticId) noexcept {
  for (const ProductCreativeUiCommandCatalogEntry& entry :
       kProductCreativeUiCommandCatalog) {
    if (entry.semanticId == semanticId) {
      return &entry;
    }
  }
  for (const ProductCreativeUiCommandCatalogEntry& entry :
       kProductCreativeUiCreatePalette) {
    if (entry.semanticId == semanticId &&
        productCreativeUiCreatePaletteEntryAllowed(entry)) {
      return &entry;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const ProductCreativeUiCommandCatalogEntry*
findFirstProductCreativeUiCommandRowByKind(
    ProductCreativeUiCommandKind kind) noexcept {
  for (const ProductCreativeUiCommandCatalogEntry& entry :
       kProductCreativeUiCommandCatalog) {
    if (entry.commandKind == kind) {
      return &entry;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const ProductCreativeUiCommandKindMetadata*
findProductCreativeUiCommandKindMetadata(
    ProductCreativeUiCommandKind kind) noexcept {
  for (const ProductCreativeUiCommandKindMetadata& metadata :
       kProductCreativeUiCommandKindMetadata) {
    if (metadata.commandKind == kind) {
      return &metadata;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::string_view productCreativeUiCommandKindReceiptName(
    ProductCreativeUiCommandKind kind) noexcept {
  if (const ProductCreativeUiCommandKindMetadata* metadata =
          findProductCreativeUiCommandKindMetadata(kind);
      metadata != nullptr) {
    return metadata->receiptName;
  }
  return "unknown";
}

[[nodiscard]] inline bool productCreativeUiCommandKindExpectsHandler(
    ProductCreativeUiCommandKind kind) noexcept {
  if (const ProductCreativeUiCommandKindMetadata* metadata =
          findProductCreativeUiCommandKindMetadata(kind);
      metadata != nullptr) {
    return metadata->expectsHandler;
  }
  return false;
}

}  // namespace iggy3d
