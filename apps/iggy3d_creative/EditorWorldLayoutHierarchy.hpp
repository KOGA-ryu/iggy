#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/world/WorldLayout.hpp"

namespace iggy3d_creative_app {

inline constexpr std::size_t kInvalidCreativeEditorWorldLayoutHierarchyRow =
    static_cast<std::size_t>(-1);

enum class CreativeEditorWorldLayoutHierarchyRowKind : std::uint8_t {
  Group,
  Symbol,
};

enum class CreativeEditorWorldLayoutHierarchyRecovery : std::uint8_t {
  None,
  Unassigned,
};

struct CreativeEditorWorldLayoutHierarchyRow {
  CreativeEditorWorldLayoutHierarchyRowKind kind =
      CreativeEditorWorldLayoutHierarchyRowKind::Group;
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t sourceIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::size_t parentRow = kInvalidCreativeEditorWorldLayoutHierarchyRow;
  std::size_t subtreeEnd = 0U;
  std::uint32_t depth = 0U;
  bool hasChildren = false;
  CreativeEditorWorldLayoutHierarchyRecovery recovery =
      CreativeEditorWorldLayoutHierarchyRecovery::None;
  std::string label;
  std::string typeLabel;
  std::string stableKey;
  std::string uiKey;
};

struct CreativeEditorWorldLayoutHierarchyModel {
  std::uint64_t sourceEpoch = 0U;
  std::uint64_t layoutRevision = 0U;
  std::vector<CreativeEditorWorldLayoutHierarchyRow> rows;
  std::size_t sourceSymbolCount = 0U;
  std::size_t recoveredSymbolCount = 0U;
};

struct CreativeEditorWorldLayoutHierarchyCache {
  bool valid = false;
  std::uint64_t buildCount = 0U;
  CreativeEditorWorldLayoutHierarchyModel model;
};

// Projects the flat World Layout tables into one deterministic display tree.
// Every user-facing source symbol is emitted exactly once. Path control points
// remain implementation storage beneath their owning path and are not rows.
[[nodiscard]] CreativeEditorWorldLayoutHierarchyModel
buildCreativeEditorWorldLayoutHierarchy(
    std::uint64_t sourceEpoch,
    std::uint64_t layoutRevision,
    const iggy3d::creative::CreativeWorldLayout& layout);

[[nodiscard]] const CreativeEditorWorldLayoutHierarchyModel&
refreshCreativeEditorWorldLayoutHierarchy(
    CreativeEditorWorldLayoutHierarchyCache& cache,
    std::uint64_t sourceEpoch,
    std::uint64_t layoutRevision,
    const iggy3d::creative::CreativeWorldLayout& layout);

// Returns model-row indices in model order. Matching rows retain their full
// ancestor path; terms are ASCII case-insensitive and AND-combined.
[[nodiscard]] std::vector<std::size_t>
filterCreativeEditorWorldLayoutHierarchy(
    const CreativeEditorWorldLayoutHierarchyModel& model,
    std::string_view query);

}  // namespace iggy3d_creative_app
