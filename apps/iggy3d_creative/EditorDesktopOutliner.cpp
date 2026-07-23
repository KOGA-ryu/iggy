#include "EditorDesktopWidgets.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "EditorDesktopModel.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Select.hpp"

// UI-4A Project panel. Renders the deterministic hierarchy the pure model
// projects and converts pure selection plans into typed desktop commands. It
// derives nothing itself and mutates nothing: every state change leaves as a
// command for the sole dispatcher.

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr float kRowIndentPixels = 12.0F;

[[nodiscard]] const char* recoveryTooltip(
    CreativeDesktopHierarchyRecovery recovery) noexcept {
  switch (recovery) {
    case CreativeDesktopHierarchyRecovery::MissingParent:
      return "Recovered root: this object's parent is not in the document.";
    case CreativeDesktopHierarchyRecovery::Cycle:
      return "Recovered root: this object's parent chain forms a cycle.";
    case CreativeDesktopHierarchyRecovery::DepthLimit:
      return "Nesting is deeper than the displayed depth limit; shown clamped.";
    case CreativeDesktopHierarchyRecovery::None:
      break;
  }
  return "";
}

void appendHoverTooltip(const char* text) {
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(text);
    ImGui::EndTooltip();
  }
}

// Refreshes the revision-owned hierarchy model and the query-owned filter. The
// hierarchy rebuilds only when the document id/revision moves; a query change
// re-filters the cached rows without rebuilding them.
void refreshOutlinerCaches(CreativeDesktopOutlinerState& state,
                           const cr::CreativeDocument& document) {
  if (!state.modelValid || state.model.documentId != document.id() ||
      state.model.documentRevision != document.revision()) {
    state.model = buildCreativeDesktopOutlinerModel(document);
    state.modelValid = true;
    state.filteredRowsValid = false;
  }
  const std::string query(state.searchBuffer.data());
  if (!state.filteredRowsValid || query != state.appliedQuery) {
    state.filteredRows = filterCreativeDesktopOutlinerRows(state.model, query);
    state.appliedQuery = query;
    state.filteredRowsValid = true;
  }
}

// The visibility/lock controls. They emit only their own flag command and must
// never also select the row, so they are drawn as separate items before the
// row's selectable. Absent during Play (document edits are read-only there).
void appendRowFlagControls(const CreativeDesktopOutlinerRow& row,
                           CreativeDesktopCommandFrame& commands) {
  const bool inheritedHidden = row.visible && !row.effectivelyVisible;
  const bool inheritedLocked = !row.locked && row.effectivelyLocked;
  if (ImGui::SmallButton(inheritedHidden ? "o*" : row.visible ? "o" : "-")) {
    commands.push(CreativeDesktopCommandId::SetObjectsVisible,
                  CreativeDesktopObjectFlagPayload{{row.objectId},
                                                   !row.visible});
  }
  appendHoverTooltip(inheritedHidden
                         ? "Locally visible; hidden by an ancestor"
                     : row.visible ? "Visible — click to hide"
                                   : "Hidden — click to show");
  ImGui::SameLine();
  if (ImGui::SmallButton(inheritedLocked ? "L*" : row.locked ? "L" : ".")) {
    commands.push(CreativeDesktopCommandId::SetObjectsLocked,
                  CreativeDesktopObjectFlagPayload{{row.objectId},
                                                   !row.locked});
  }
  appendHoverTooltip(inheritedLocked
                         ? "Unlocked locally; locked by an ancestor"
                     : row.locked ? "Locked — click to unlock"
                                  : "Unlocked — click to lock");
  ImGui::SameLine();
}

}  // namespace

CreativeDesktopLiveSelection creativeDesktopLiveSelection(
    const cr::CreativeAppState& appState) {
  CreativeDesktopLiveSelection selection;
  const cr::CreativeSelectionState& state = appState.facade.selectionState();
  if (state.selectedTarget.value != cr::kInvalidId) {
    selection.primaryObjectId =
        static_cast<cr::CreativeObjectId>(state.selectedTarget.value);
  }
  for (const cr::TargetRef& target : cr::selectedTargetList(state)) {
    if (target.value != cr::kInvalidId) {
      selection.objectIds.push_back(
          static_cast<cr::CreativeObjectId>(target.value));
    }
  }
  if (selection.objectIds.empty() &&
      selection.primaryObjectId != cr::kInvalidObjectId) {
    selection.objectIds.push_back(selection.primaryObjectId);
  }
  return selection;
}

void buildCreativeEditorDesktopOutlinerPanel(
    CreativeEditorDesktopUiState& desktopUi,
    const cr::CreativeAppState& appState,
    bool playModeActive,
    const CreativeEditorUiInputFrame& input,
    CreativeDesktopCommandFrame& commands) {
  CreativeDesktopOutlinerState& state = desktopUi.outliner;
  const cr::CreativeDocument& document = appState.facade.document();

  if (!ImGui::BeginTabBar("##project_tabs")) {
    return;
  }
  if (!ImGui::BeginTabItem("Scene")) {
    ImGui::EndTabBar();
    return;
  }

  refreshOutlinerCaches(state, document);

  ImGui::SetNextItemWidth(-1.0F);
  if (ImGui::InputTextWithHint("##outliner_search", "Search name, kind, or id",
                               state.searchBuffer.data(),
                               state.searchBuffer.size())) {
    state.filteredRowsValid = false;
    refreshOutlinerCaches(state, document);
  }

  if (state.model.recoveredRowCount > 0U) {
    ImGui::TextColored(ImVec4{1.0F, 0.82F, 0.25F, 1.0F},
                       "%llu recovered row(s)",
                       static_cast<unsigned long long>(
                           state.model.recoveredRowCount));
    appendHoverTooltip(
        "Some objects have a missing parent, a parent cycle, or nesting past "
        "the depth limit. They are shown as recovered roots.");
  }

  // Selection is read from live document truth every frame, never cached.
  const CreativeDesktopLiveSelection live = creativeDesktopLiveSelection(appState);

  std::vector<cr::CreativeObjectId> visibleIds;
  visibleIds.reserve(state.filteredRows.size());
  for (const std::size_t index : state.filteredRows) {
    visibleIds.push_back(state.model.rows[index].objectId);
  }

  ImGui::BeginChild("##outliner_rows");
  if (state.filteredRows.empty()) {
    ImGui::TextDisabled(document.objectCount() == 0U ? "No objects"
                                                     : "No matching objects");
  }
  for (const std::size_t index : state.filteredRows) {
    const CreativeDesktopOutlinerRow& row = state.model.rows[index];
    ImGui::PushID(
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(row.objectId)));
    const float indent = static_cast<float>(row.depth) * kRowIndentPixels;
    if (indent > 0.0F) {
      ImGui::Indent(indent);
    }

    // Play stays inspectable: rows still select, but the flag controls (which
    // mutate the document) are absent.
    if (!playModeActive) {
      appendRowFlagControls(row, commands);
    }
    if (row.recovery != CreativeDesktopHierarchyRecovery::None) {
      ImGui::TextColored(ImVec4{1.0F, 0.82F, 0.25F, 1.0F}, "!");
      appendHoverTooltip(recoveryTooltip(row.recovery));
      ImGui::SameLine();
    }

    const bool isPrimary = row.objectId == live.primaryObjectId;
    const bool isSelected =
        std::find(live.objectIds.begin(), live.objectIds.end(), row.objectId) !=
        live.objectIds.end();
    // The primary keeps the normal selection colour; secondary rows get a
    // quieter one so the two are visually distinct.
    const bool quietHighlight = isSelected && !isPrimary;
    if (quietHighlight) {
      const ImVec4 header = ImGui::GetStyleColorVec4(ImGuiCol_Header);
      ImGui::PushStyleColor(
          ImGuiCol_Header,
          ImVec4{header.x, header.y, header.z, header.w * 0.45F});
    }
    char label[192];
    std::snprintf(label, sizeof(label), "%s  %s  #%llu",
                  row.name.empty() ? "(unnamed)" : row.name.c_str(),
                  std::string(cr::toString(row.kind)).c_str(),
                  static_cast<unsigned long long>(row.objectId));
    if (ImGui::Selectable(label, isSelected || isPrimary)) {
      const CreativeDesktopSelectionPlan plan = planCreativeDesktopSelection(
          visibleIds, live.objectIds, live.primaryObjectId, row.objectId,
          state.selectionAnchor,
          creativeDesktopSelectionGestureFor(input.selectionAdditiveDown,
                                             input.selectionToggleDown));
      if (plan.accepted) {
        commands.push(CreativeDesktopCommandId::SelectObjects,
                      CreativeDesktopSelectPayload{plan.objectIds,
                                                   plan.primaryObjectId});
        state.selectionAnchor = plan.nextAnchorObjectId;
      }
    }
    if (ImGui::BeginPopupContextItem("##selection_scope")) {
      const auto appendHierarchySelection = [&](
          const char* label,
          CreativeDesktopHierarchySelectionScope scope) {
        const CreativeDesktopSelectionPlan plan =
            planCreativeDesktopHierarchySelection(state.model, row.objectId,
                                                   scope);
        if (ImGui::MenuItem(label, nullptr, false, plan.accepted)) {
          commands.push(CreativeDesktopCommandId::SelectObjects,
                        CreativeDesktopSelectPayload{plan.objectIds,
                                                     plan.primaryObjectId});
          state.selectionAnchor = plan.nextAnchorObjectId;
        }
      };
      appendHierarchySelection(
          "Select parent",
          CreativeDesktopHierarchySelectionScope::Parent);
      appendHierarchySelection(
          "Select direct children",
          CreativeDesktopHierarchySelectionScope::DirectChildren);
      appendHierarchySelection(
          "Select hierarchy",
          CreativeDesktopHierarchySelectionScope::Subtree);
      ImGui::EndPopup();
    }
    if (quietHighlight) {
      ImGui::PopStyleColor();
    }

    if (indent > 0.0F) {
      ImGui::Unindent(indent);
    }
    ImGui::PopID();
  }
  ImGui::EndChild();

  ImGui::EndTabItem();
  ImGui::EndTabBar();
}

}  // namespace iggy3d_creative_app
