#include "EditorDesktopWidgets.hpp"

#include <array>
#include <cstdint>
#include <string>

#include "EditorVolume.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void resetVolumeFeedback(CreativeEditorVolumeState& state) noexcept {
  state.lastReceipt = {};
  invalidateCreativeEditorVolumeOperationPreview(state);
}

void appendNudgeButton(const char* label,
                       cr::CreativeGridCoord3 delta,
                       CreativeEditorVolumeState& state) {
  if (ImGui::SmallButton(label) &&
      cr::moveCreativeVolumeSelection(state.selection, delta)) {
    resetVolumeFeedback(state);
  }
}

void appendGrowButton(const char* label,
                      cr::CreativeVolumeFace face,
                      CreativeEditorVolumeState& state) {
  if (ImGui::SmallButton(label) &&
      cr::resizeCreativeVolumeSelectionFace(state.selection, face, 1)) {
    resetVolumeFeedback(state);
  }
}

}  // namespace

void appendCreativeDesktopVolumeInspector(
    CreativeEditorState& editor,
    const cr::CreativeDocument& document,
    std::span<const cr::CreativeObjectId> selectedObjectIds) {
  CreativeEditorVolumeState& volume = editor.volume;
  if (!volume.active) {
    return;
  }

  ImGui::SeparatorText("Volume region");
  static_cast<void>(creativeDesktopInputTextStdString("Name",
                                                       &volume.regionName));

  ImGui::BeginDisabled(selectedObjectIds.empty());
  if (ImGui::Button("Fit to selected content")) {
    const cr::CreativeGridSettings grid = document.gridSettings();
    const double cellSize = volume.selection.cellSize;
    if (cr::fitCreativeVolumeSelectionToObjects(
            document, selectedObjectIds, cellSize, grid.origin,
            volume.selection)) {
      resetVolumeFeedback(volume);
    }
  }
  ImGui::EndDisabled();

  const cr::CreativeVolumeRegionFacts initialFacts =
      cr::inspectCreativeVolumeRegion(volume.selection);
  if (!initialFacts.valid) {
    ImGui::TextDisabled("Set two corners in the viewport to define this region.");
    return;
  }

  std::array minimum{
      initialFacts.exclusiveBounds.min.x,
      initialFacts.exclusiveBounds.min.y,
      initialFacts.exclusiveBounds.min.z,
  };
  std::array maximumExclusive{
      initialFacts.exclusiveBounds.max.x,
      initialFacts.exclusiveBounds.max.y,
      initialFacts.exclusiveBounds.max.z,
  };
  const bool minimumChanged = ImGui::InputInt3("Min (inclusive)", minimum.data());
  const bool maximumChanged =
      ImGui::InputInt3("Max (exclusive)", maximumExclusive.data());
  if (minimumChanged || maximumChanged) {
    const cr::CreativeGridBounds3 edited{
        {minimum[0], minimum[1], minimum[2]},
        {maximumExclusive[0], maximumExclusive[1], maximumExclusive[2]},
    };
    if (cr::setCreativeVolumeSelectionGridBounds(volume.selection, edited)) {
      resetVolumeFeedback(volume);
    }
  }

  ImGui::TextUnformatted("Move one cell");
  appendNudgeButton("-X", {-1, 0, 0}, volume);
  ImGui::SameLine();
  appendNudgeButton("+X", {1, 0, 0}, volume);
  ImGui::SameLine();
  appendNudgeButton("-Y", {0, -1, 0}, volume);
  ImGui::SameLine();
  appendNudgeButton("+Y", {0, 1, 0}, volume);
  ImGui::SameLine();
  appendNudgeButton("-Z", {0, 0, -1}, volume);
  ImGui::SameLine();
  appendNudgeButton("+Z", {0, 0, 1}, volume);

  ImGui::TextUnformatted("Grow one face");
  appendGrowButton("Grow -X", cr::CreativeVolumeFace::NegativeX, volume);
  ImGui::SameLine();
  appendGrowButton("Grow +X", cr::CreativeVolumeFace::PositiveX, volume);
  ImGui::SameLine();
  appendGrowButton("Grow -Y", cr::CreativeVolumeFace::NegativeY, volume);
  ImGui::SameLine();
  appendGrowButton("Grow +Y", cr::CreativeVolumeFace::PositiveY, volume);
  ImGui::SameLine();
  appendGrowButton("Grow -Z", cr::CreativeVolumeFace::NegativeZ, volume);
  ImGui::SameLine();
  appendGrowButton("Grow +Z", cr::CreativeVolumeFace::PositiveZ, volume);

  const cr::CreativeVolumeRegionFacts facts =
      cr::inspectCreativeVolumeRegion(volume.selection);
  ImGui::Text("Inclusive max: %d, %d, %d", facts.inclusiveMaximum.x,
              facts.inclusiveMaximum.y, facts.inclusiveMaximum.z);
  ImGui::Text("Size: %d x %d x %d | %llu cells", facts.dimensions.x,
              facts.dimensions.y, facts.dimensions.z,
              static_cast<unsigned long long>(facts.cellCount));

  const cr::CreativeVolumeOperationReceipt& preview =
      refreshCreativeEditorVolumeOperationPreview(
          volume, document, volume.selection, editor.placeBrush,
          editor.toolSettings);
  const std::uint64_t changed =
      cr::creativeVolumeChangedMemberCount(preview);
  ImGui::Text("%s: %llu changes",
              std::string(cr::toString(volume.operation)).c_str(),
              static_cast<unsigned long long>(changed));
  ImGui::TextDisabled("%s", std::string(cr::toString(preview.status)).c_str());
}

}  // namespace iggy3d_creative_app
