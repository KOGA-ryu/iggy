#include "EditorWorldLayoutHierarchyPanel.hpp"

#include "EditorDesktopCommands.hpp"
#include "EditorDesktopUi.hpp"
#include "EditorWorldLayout.hpp"

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "imgui.h"

namespace iggy3d_creative_app {
namespace {

bool selected(const CreativeEditorWorldLayoutState& state,
              CreativeEditorWorldLayoutSelectionKind kind,
              std::size_t index) noexcept {
  return state.selection.kind == kind && state.selection.index == index;
}

[[nodiscard]] bool hasVisibleName(const char* name) noexcept {
  for (const char* cursor = name; *cursor != '\0'; ++cursor) {
    if (*cursor != ' ' && *cursor != '\t' && *cursor != '\r' &&
        *cursor != '\n') {
      return true;
    }
  }
  return false;
}

void refreshWorldLayoutHierarchyCaches(
    CreativeDesktopWorldLayoutHierarchyState& hierarchy,
    const CreativeEditorWorldLayoutState& state) {
  if (!hierarchy.cache.valid ||
      hierarchy.cache.model.sourceEpoch != state.sourceEpoch ||
      hierarchy.cache.model.layoutRevision != state.revision) {
    static_cast<void>(refreshCreativeEditorWorldLayoutHierarchy(
        hierarchy.cache, state.sourceEpoch, state.revision, state.source));
    hierarchy.filteredRowsValid = false;
  }
  const std::string query(hierarchy.searchBuffer.data());
  if (!hierarchy.filteredRowsValid || query != hierarchy.appliedQuery) {
    hierarchy.filteredRows = filterCreativeEditorWorldLayoutHierarchy(
        hierarchy.cache.model, query);
    hierarchy.appliedQuery = query;
    hierarchy.filteredRowsValid = true;
  }
}

[[nodiscard]] bool hierarchyRowSelected(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutHierarchyRow& row) noexcept {
  if (row.kind != CreativeEditorWorldLayoutHierarchyRowKind::Symbol) {
    return false;
  }
  switch (row.table) {
    case cr::CreativeWorldLayoutTable::Building:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Building,
                      row.sourceIndex);
    case cr::CreativeWorldLayoutTable::Level:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Level,
                      row.sourceIndex);
    case cr::CreativeWorldLayoutTable::Room:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Room,
                      row.sourceIndex);
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      return selected(
          state, CreativeEditorWorldLayoutSelectionKind::VerticalConnector,
          row.sourceIndex);
    case cr::CreativeWorldLayoutTable::Box:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Box,
                      row.sourceIndex);
    case cr::CreativeWorldLayoutTable::Wall:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Wall,
                      row.sourceIndex);
    case cr::CreativeWorldLayoutTable::Opening:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Opening,
                      row.sourceIndex);
    case cr::CreativeWorldLayoutTable::Object:
      return selected(state, CreativeEditorWorldLayoutSelectionKind::Object,
                      row.sourceIndex);
    case cr::CreativeWorldLayoutTable::TerrainProfile:
      return selected(
          state, CreativeEditorWorldLayoutSelectionKind::TerrainProfile,
          row.sourceIndex);
    case cr::CreativeWorldLayoutTable::TerrainPath:
      return selected(state,
                      CreativeEditorWorldLayoutSelectionKind::TerrainPath,
                      row.sourceIndex);
    case cr::CreativeWorldLayoutTable::None:
    case cr::CreativeWorldLayoutTable::TerrainPathPoint:
      return false;
  }
  return false;
}

void setPendingHierarchySource(
    CreativeDesktopWorldLayoutHierarchyState& hierarchy,
    const CreativeEditorWorldLayoutHierarchyRow& row) {
  hierarchy.pendingTable = row.table;
  hierarchy.pendingIndex = row.sourceIndex;
  hierarchy.pendingStableKey = row.stableKey;
  hierarchy.pendingLabel = row.label;
}

struct HierarchyPopupRequests {
  bool rename = false;
  bool remove = false;
};

void drawWorldLayoutHierarchyRow(
    CreativeDesktopWorldLayoutHierarchyState& hierarchy,
    const CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands,
    const std::vector<bool>& visible, std::size_t rowIndex,
    bool editingDisabled, bool filterActive,
    HierarchyPopupRequests& popupRequests) {
  if (rowIndex >= hierarchy.cache.model.rows.size() || !visible[rowIndex]) {
    return;
  }
  const CreativeEditorWorldLayoutHierarchyRow& row =
      hierarchy.cache.model.rows[rowIndex];
  bool hasVisibleChildren = false;
  for (std::size_t index = rowIndex + 1U; index < row.subtreeEnd; ++index) {
    if (visible[index]) {
      hasVisibleChildren = true;
      break;
    }
  }

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth |
                             ImGuiTreeNodeFlags_OpenOnArrow |
                             ImGuiTreeNodeFlags_OpenOnDoubleClick;
  if (!hasVisibleChildren) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  }
  if (hierarchyRowSelected(state, row)) {
    flags |= ImGuiTreeNodeFlags_Selected;
  }
  if (filterActive) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
  } else if (row.depth <= 1U) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  }

  ImGui::PushID(row.uiKey.c_str());
  const bool open = ImGui::TreeNodeEx(
      "##layout_source", flags, "%s%s%s%s",
      row.recovery == CreativeEditorWorldLayoutHierarchyRecovery::Unassigned
          ? "! "
          : "",
      row.label.c_str(),
      row.kind == CreativeEditorWorldLayoutHierarchyRowKind::Symbol ? "  "
                                                                    : "",
      row.kind == CreativeEditorWorldLayoutHierarchyRowKind::Symbol
          ? row.typeLabel.c_str()
          : "");

  if (row.kind == CreativeEditorWorldLayoutHierarchyRowKind::Symbol &&
      ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    commands.push(CreativeDesktopCommandId::WorldLayoutFocusSource,
                  CreativeDesktopWorldLayoutSourcePayload{
                      row.table, row.sourceIndex, row.stableKey});
  }
  if (row.kind == CreativeEditorWorldLayoutHierarchyRowKind::Symbol &&
      ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s\n%s", row.typeLabel.c_str(),
                      row.stableKey.c_str());
  }

  if (row.kind == CreativeEditorWorldLayoutHierarchyRowKind::Symbol &&
      ImGui::BeginPopupContextItem("##layout_source_actions")) {
    ImGui::BeginDisabled(editingDisabled);
    if (creativeEditorWorldLayoutSourceCanRename(row.table) &&
        ImGui::MenuItem("Rename...")) {
      setPendingHierarchySource(hierarchy, row);
      std::snprintf(hierarchy.renameBuffer.data(),
                    hierarchy.renameBuffer.size(), "%s", row.label.c_str());
      popupRequests.rename = true;
    }
    if (creativeEditorWorldLayoutSourceCanDuplicate(row.table) &&
        ImGui::MenuItem("Duplicate")) {
      commands.push(CreativeDesktopCommandId::WorldLayoutDuplicateSource,
                    CreativeDesktopWorldLayoutSourcePayload{
                        row.table, row.sourceIndex, row.stableKey});
    }
    if (creativeEditorWorldLayoutSourceCanDelete(row.table) &&
        ImGui::MenuItem("Delete...")) {
      setPendingHierarchySource(hierarchy, row);
      popupRequests.remove = true;
    }
    ImGui::EndDisabled();
    ImGui::EndPopup();
  }

  if (hasVisibleChildren && open) {
    std::size_t child = rowIndex + 1U;
    while (child < row.subtreeEnd) {
      if (hierarchy.cache.model.rows[child].parentRow == rowIndex) {
        drawWorldLayoutHierarchyRow(hierarchy, state, commands, visible, child,
                                    editingDisabled, filterActive,
                                    popupRequests);
        child = hierarchy.cache.model.rows[child].subtreeEnd;
      } else {
        ++child;
      }
    }
    ImGui::TreePop();
  }
  ImGui::PopID();
}

void drawWorldLayoutHierarchyModals(
    CreativeDesktopWorldLayoutHierarchyState& hierarchy,
    CreativeDesktopCommandFrame& commands,
    HierarchyPopupRequests popupRequests) {
  if (popupRequests.rename) {
    ImGui::OpenPopup("Rename layout source");
  }
  if (popupRequests.remove) {
    ImGui::OpenPopup("Delete layout source");
  }

  if (ImGui::BeginPopupModal("Rename layout source", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Rename %s", hierarchy.pendingLabel.c_str());
    ImGui::SetNextItemWidth(320.0F);
    const bool submitted = ImGui::InputText(
        "Name", hierarchy.renameBuffer.data(), hierarchy.renameBuffer.size(),
        ImGuiInputTextFlags_EnterReturnsTrue);
    const bool nameValid = hasVisibleName(hierarchy.renameBuffer.data());
    ImGui::BeginDisabled(!nameValid);
    const bool renameClicked = ImGui::Button("Rename");
    ImGui::EndDisabled();
    if (nameValid && (submitted || renameClicked)) {
      commands.push(
          CreativeDesktopCommandId::WorldLayoutRenameSource,
          CreativeDesktopWorldLayoutSourceRenamePayload{
              hierarchy.pendingTable, hierarchy.pendingIndex,
              hierarchy.pendingStableKey,
              std::string(hierarchy.renameBuffer.data())});
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  if (ImGui::BeginPopupModal("Delete layout source", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("Delete %s?", hierarchy.pendingLabel.c_str());
    ImGui::TextDisabled("Hosted or owned layout symbols may also be removed.");
    if (ImGui::Button("Delete")) {
      commands.push(CreativeDesktopCommandId::WorldLayoutDeleteSource,
                    CreativeDesktopWorldLayoutSourcePayload{
                        hierarchy.pendingTable, hierarchy.pendingIndex,
                        hierarchy.pendingStableKey});
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void drawWorldLayoutHierarchy(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands, bool editingDisabled) {
  CreativeDesktopWorldLayoutHierarchyState& hierarchy =
      desktopUi.worldLayoutHierarchy;
  refreshWorldLayoutHierarchyCaches(hierarchy, state);
  ImGui::SetNextItemWidth(-1.0F);
  if (ImGui::InputTextWithHint(
          "##world_layout_source_search", "Search name, type, or key",
          hierarchy.searchBuffer.data(), hierarchy.searchBuffer.size())) {
    hierarchy.filteredRowsValid = false;
    refreshWorldLayoutHierarchyCaches(hierarchy, state);
  }

  if (hierarchy.cache.model.recoveredSymbolCount > 0U) {
    ImGui::TextColored(
        ImVec4{1.0F, 0.78F, 0.22F, 1.0F}, "%llu unassigned",
        static_cast<unsigned long long>(
            hierarchy.cache.model.recoveredSymbolCount));
  }

  std::vector<bool> visible(hierarchy.cache.model.rows.size(), false);
  for (const std::size_t index : hierarchy.filteredRows) {
    if (index < visible.size()) {
      visible[index] = true;
    }
  }
  HierarchyPopupRequests popupRequests;
  ImGui::BeginChild("##world_layout_source_rows");
  if (hierarchy.filteredRows.empty()) {
    ImGui::TextDisabled(hierarchy.cache.model.rows.empty()
                            ? "No layout sources"
                            : "No matching sources");
  }
  std::size_t row = 0U;
  while (row < hierarchy.cache.model.rows.size()) {
    if (hierarchy.cache.model.rows[row].parentRow ==
        kInvalidCreativeEditorWorldLayoutHierarchyRow) {
      drawWorldLayoutHierarchyRow(hierarchy, state, commands, visible, row,
                                  editingDisabled,
                                  !hierarchy.appliedQuery.empty(),
                                  popupRequests);
      row = hierarchy.cache.model.rows[row].subtreeEnd;
    } else {
      ++row;
    }
  }
  ImGui::EndChild();
  drawWorldLayoutHierarchyModals(hierarchy, commands, popupRequests);
}

}  // namespace

void drawCreativeEditorWorldLayoutHierarchy(
    CreativeEditorDesktopUiState& desktopUi,
    const CreativeEditorWorldLayoutState& state,
    CreativeDesktopCommandFrame& commands,
    bool editingDisabled) {
  drawWorldLayoutHierarchy(desktopUi, state, commands, editingDisabled);
}

}  // namespace iggy3d_creative_app
