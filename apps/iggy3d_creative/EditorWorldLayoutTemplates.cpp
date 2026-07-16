#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {
namespace {

constexpr std::string_view kBuildingTemplatePrefix = "building_template_";
constexpr std::string_view kBuildingTemplateExtension = ".iwlt";

std::string templateIdForOrdinal(std::uint64_t ordinal) {
  std::ostringstream stream;
  stream << kBuildingTemplatePrefix << std::setw(4) << std::setfill('0')
         << ordinal;
  return stream.str();
}

std::uint64_t templateOrdinal(std::string_view templateId) noexcept {
  if (!templateId.starts_with(kBuildingTemplatePrefix)) {
    return 0U;
  }
  const std::string_view digits =
      templateId.substr(kBuildingTemplatePrefix.size());
  std::uint64_t ordinal = 0U;
  const auto parsed =
      std::from_chars(digits.data(), digits.data() + digits.size(), ordinal);
  return parsed.ec == std::errc{} && parsed.ptr == digits.data() + digits.size()
             ? ordinal
             : 0U;
}

bool templateIdExists(
    const CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    std::string_view templateId) {
  return std::any_of(library.templates.begin(), library.templates.end(),
                     [templateId](const auto& value) {
                       return value.templateId == templateId;
                     });
}

std::string nextTemplateId(
    const CreativeEditorWorldLayoutBuildingTemplateLibrary& library) {
  std::uint64_t ordinal = library.nextTemplateOrdinal;
  for (;;) {
    const std::string templateId = templateIdForOrdinal(ordinal);
    std::error_code error;
    const bool fileExists = std::filesystem::exists(
        library.root / (templateId + std::string(kBuildingTemplateExtension)),
        error);
    if (error) {
      return {};
    }
    if (!templateIdExists(library, templateId) && !fileExists) {
      return templateId;
    }
    if (ordinal == std::numeric_limits<std::uint64_t>::max()) {
      return {};
    }
    ++ordinal;
  }
}

bool readTemplateFile(const std::filesystem::path& path, std::string& output) {
  std::error_code error;
  const std::uintmax_t size = std::filesystem::file_size(path, error);
  if (error || size > cr::kCreativeWorldLayoutCodecMaxEncodedBytes) {
    return false;
  }
  output.resize(static_cast<std::size_t>(size));
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    output.clear();
    return false;
  }
  stream.read(output.data(), static_cast<std::streamsize>(output.size()));
  return stream.good() ||
         (stream.eof() &&
          stream.gcount() == static_cast<std::streamsize>(output.size()));
}

bool writeTemplateFileAtomically(const std::filesystem::path& path,
                                 std::string_view encodedText) {
  std::filesystem::path temporary = path;
  temporary += ".tmp";
  {
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    if (!stream) {
      return false;
    }
    stream.write(encodedText.data(),
                 static_cast<std::streamsize>(encodedText.size()));
    stream.flush();
    if (!stream) {
      std::error_code cleanupError;
      std::filesystem::remove(temporary, cleanupError);
      return false;
    }
  }
  std::error_code error;
  std::filesystem::rename(temporary, path, error);
  if (error) {
    std::error_code cleanupError;
    std::filesystem::remove(temporary, cleanupError);
    return false;
  }
  return true;
}

bool snapTemplateAnchor(CreativeEditorWorldLayoutPoint point,
                        cr::CreativeTerrainCoord2& output) noexcept {
  if (!detail::finiteWorldLayoutPoint(point)) {
    return false;
  }
  const double roundedX = std::round(point.x);
  const double roundedZ = std::round(point.z);
  if (roundedX < std::numeric_limits<std::int32_t>::min() ||
      roundedX > std::numeric_limits<std::int32_t>::max() ||
      roundedZ < std::numeric_limits<std::int32_t>::min() ||
      roundedZ > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  output = {static_cast<std::int32_t>(roundedX),
            static_cast<std::int32_t>(roundedZ)};
  return true;
}

CreativeEditorWorldLayoutEditReceipt rebuildTemplatePlacement(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeTerrainCoord2 anchor) {
  cr::CreativeWorldLayoutBuildingEditResult stamped =
      cr::stampCreativeWorldLayoutBuildingTemplate(
          state.source, state.buildingTemplatePlacement.orientedTemplate,
          {anchor, state.nextStableOrdinal, false});
  state.buildingTemplatePlacement.anchor = anchor;
  state.buildingTemplatePlacement.previewValid = stamped.accepted;
  state.buildingTemplatePlacement.reasonCode = stamped.reasonCode;
  if (!stamped.accepted) {
    state.buildingTemplatePlacement.candidate = {};
    state.buildingTemplatePlacement.previewBounds = {};
    state.buildingTemplatePlacement.resultBuildingIndex =
        cr::kInvalidCreativeWorldLayoutIndex;
    state.statusMessage = stamped.reasonCode;
    return {false, true, stamped.reasonCode};
  }
  const std::size_t resultBuildingIndex = stamped.resultBuildingIndex;
  cr::CreativeWorldLayoutBuildingBounds previewBounds;
  if (!cr::measureCreativeWorldLayoutBuildingBounds(
          stamped.edited, resultBuildingIndex, previewBounds)) {
    state.buildingTemplatePlacement = {};
    state.statusMessage = "building template preview is invalid";
    return {false, true,
            "creative_editor_world_layout_building_template_preview_invalid"};
  }
  state.buildingTemplatePlacement.previewBounds = previewBounds;
  state.buildingTemplatePlacement.resultBuildingIndex = resultBuildingIndex;
  state.buildingTemplatePlacement.nextStableOrdinal = stamped.nextStableOrdinal;
  state.buildingTemplatePlacement.candidate = std::move(stamped.edited);
  state.statusMessage = "click the canvas to place building";
  return {true, true,
          "creative_editor_world_layout_building_template_preview_ready"};
}

}  // namespace

CreativeEditorWorldLayoutBuildingTemplateLoadReceipt
loadCreativeEditorWorldLayoutBuildingTemplateLibrary(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const std::filesystem::path& creativeSaveRoot) {
  CreativeEditorWorldLayoutBuildingTemplateLoadReceipt receipt;
  receipt.requested = true;
  library = {};
  library.root = creativeSaveRoot / "world_layout_templates";
  std::error_code error;
  std::filesystem::create_directories(library.root, error);
  if (error) {
    receipt.reasonCode =
        "creative_editor_world_layout_building_template_root_create_failed";
    library.statusMessage = receipt.reasonCode;
    library.root.clear();
    return receipt;
  }

  std::vector<std::filesystem::path> paths;
  for (std::filesystem::directory_iterator iterator(library.root, error), end;
       !error && iterator != end; iterator.increment(error)) {
    std::error_code typeError;
    if (iterator->is_regular_file(typeError) && !typeError &&
        iterator->path().extension() == kBuildingTemplateExtension) {
      paths.push_back(iterator->path());
    }
  }
  if (error) {
    receipt.reasonCode =
        "creative_editor_world_layout_building_template_scan_failed";
    library.statusMessage = receipt.reasonCode;
    library.root.clear();
    return receipt;
  }
  std::sort(paths.begin(), paths.end());

  std::uint64_t maximumOrdinal = 0U;
  for (const std::filesystem::path& path : paths) {
    const std::string filenameId = path.stem().string();
    maximumOrdinal = std::max(maximumOrdinal, templateOrdinal(filenameId));
    std::string encodedText;
    if (!readTemplateFile(path, encodedText)) {
      ++receipt.rejectedCount;
      continue;
    }
    cr::CreativeWorldLayoutDecodeResult decoded =
        cr::decodeCreativeWorldLayout(encodedText);
    cr::CreativeWorldLayoutBuildingTemplateResult loaded =
        decoded.accepted ? cr::loadCreativeWorldLayoutBuildingTemplate(
                               std::move(decoded.layout))
                         : cr::CreativeWorldLayoutBuildingTemplateResult{};
    if (!decoded.accepted || !loaded.accepted ||
        loaded.value.templateId != filenameId ||
        templateIdExists(library, filenameId) ||
        library.templates.size() >=
            kCreativeEditorWorldLayoutBuildingTemplateCapacity) {
      ++receipt.rejectedCount;
      continue;
    }
    library.templates.push_back(std::move(loaded.value));
    ++receipt.loadedCount;
  }
  library.nextTemplateOrdinal =
      maximumOrdinal == std::numeric_limits<std::uint64_t>::max()
          ? maximumOrdinal
          : maximumOrdinal + 1U;
  if (!library.templates.empty()) {
    library.selectedIndex = 0U;
  }
  receipt.accepted = true;
  receipt.reasonCode =
      "creative_editor_world_layout_building_template_library_ready";
  library.statusMessage = receipt.reasonCode;
  return receipt;
}

CreativeEditorWorldLayoutEditReceipt
captureCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex,
    std::string label) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary& library =
      state.buildingTemplates;
  if (library.root.empty()) {
    library.statusMessage = "building template library is not loaded";
    return {false, false,
            "creative_editor_world_layout_building_template_library_missing"};
  }
  if (library.templates.size() >=
      kCreativeEditorWorldLayoutBuildingTemplateCapacity) {
    library.statusMessage = "building template library is full";
    return {false, false,
            "creative_editor_world_layout_building_template_capacity_reached"};
  }
  const std::string templateId = nextTemplateId(library);
  if (templateId.empty()) {
    library.statusMessage = "building template id space is exhausted";
    return {false, false,
            "creative_editor_world_layout_building_template_id_exhausted"};
  }
  cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          state.source, {buildingIndex, templateId, std::move(label)});
  if (!captured.accepted) {
    library.statusMessage = captured.reasonCode;
    state.statusMessage = "building template could not be saved";
    return {false, false, captured.reasonCode};
  }
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(captured.value.normalizedLayout);
  const std::filesystem::path path =
      library.root / (templateId + std::string(kBuildingTemplateExtension));
  if (!encoded.accepted ||
      !writeTemplateFileAtomically(path, encoded.encodedText)) {
    library.statusMessage =
        "creative_editor_world_layout_building_template_write_failed";
    state.statusMessage = "building template could not be written";
    return {false, false, library.statusMessage};
  }

  library.templates.push_back(std::move(captured.value));
  library.selectedIndex = library.templates.size() - 1U;
  const std::uint64_t writtenOrdinal = templateOrdinal(templateId);
  library.nextTemplateOrdinal =
      writtenOrdinal == std::numeric_limits<std::uint64_t>::max()
          ? writtenOrdinal
          : std::max(library.nextTemplateOrdinal, writtenOrdinal + 1U);
  library.statusMessage = "building template saved";
  state.statusMessage = "building template saved";
  return {true, true, "creative_editor_world_layout_building_template_saved"};
}

CreativeEditorWorldLayoutEditReceipt
selectCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutState& state,
    std::size_t templateIndex) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary& library =
      state.buildingTemplates;
  if (templateIndex >= library.templates.size()) {
    return {false, false,
            "creative_editor_world_layout_building_template_selection_invalid"};
  }
  const bool changed = library.selectedIndex != templateIndex ||
                       state.buildingTemplatePlacement.active;
  state.buildingTemplatePlacement = {};
  library.selectedIndex = templateIndex;
  library.statusMessage = library.templates[templateIndex].label + " selected";
  state.statusMessage = "building template selected";
  return {true, changed,
          "creative_editor_world_layout_building_template_selected"};
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutBuildingTemplatePlacement(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutBuildingTemplatePlacementPhase phase,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeWorldLayoutBuildingTransformOperation operation) {
  if (phase >= CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Count) {
    return {false, false,
            "creative_editor_world_layout_building_template_phase_invalid"};
  }
  if (phase ==
      CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Cancel) {
    const bool changed = state.buildingTemplatePlacement.active;
    state.buildingTemplatePlacement = {};
    state.statusMessage = changed ? "building placement cancelled"
                                  : "no building placement to cancel";
    return {true, changed,
            "creative_editor_world_layout_building_template_cancelled"};
  }
  if (phase == CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Begin) {
    const CreativeEditorWorldLayoutBuildingTemplateLibrary& library =
        state.buildingTemplates;
    cr::CreativeTerrainCoord2 anchor;
    if (state.tool != CreativeEditorWorldLayoutTool::Select ||
        library.selectedIndex >= library.templates.size() ||
        !snapTemplateAnchor(point, anchor)) {
      state.statusMessage = "select a template and the Select tool";
      return {false, false,
              "creative_editor_world_layout_building_template_begin_invalid"};
    }
    const std::size_t templateIndex = library.selectedIndex;
    const cr::CreativeWorldLayoutBuildingTemplate selectedTemplate =
        library.templates[templateIndex];
    detail::clearWorldLayoutInteraction(state);
    detail::invalidateWorldLayoutPreview(state);
    state.anchorActive = false;
    state.buildingTemplatePlacement.active = true;
    state.buildingTemplatePlacement.sourceRevision = state.revision;
    state.buildingTemplatePlacement.templateIndex = templateIndex;
    state.buildingTemplatePlacement.orientedTemplate = selectedTemplate;
    return rebuildTemplatePlacement(state, anchor);
  }
  if (!state.buildingTemplatePlacement.active) {
    return {false, false,
            "creative_editor_world_layout_building_template_not_active"};
  }
  if (state.buildingTemplatePlacement.sourceRevision != state.revision) {
    state.buildingTemplatePlacement = {};
    state.statusMessage = "layout changed while placing a building";
    return {false, false,
            "creative_editor_world_layout_building_template_stale"};
  }
  if (phase ==
      CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Transform) {
    cr::CreativeWorldLayoutBuildingTemplateResult transformed =
        cr::transformCreativeWorldLayoutBuildingTemplate(
            state.buildingTemplatePlacement.orientedTemplate, operation);
    if (!transformed.accepted) {
      state.statusMessage = transformed.reasonCode;
      return {false, false, transformed.reasonCode};
    }
    state.buildingTemplatePlacement.orientedTemplate =
        std::move(transformed.value);
    return rebuildTemplatePlacement(state,
                                    state.buildingTemplatePlacement.anchor);
  }
  if (phase ==
      CreativeEditorWorldLayoutBuildingTemplatePlacementPhase::Update) {
    cr::CreativeTerrainCoord2 anchor;
    if (!snapTemplateAnchor(point, anchor)) {
      return {false, false,
              "creative_editor_world_layout_building_template_point_invalid"};
    }
    if (anchor == state.buildingTemplatePlacement.anchor) {
      return {true, false, state.buildingTemplatePlacement.reasonCode};
    }
    return rebuildTemplatePlacement(state, anchor);
  }
  if (!state.buildingTemplatePlacement.previewValid ||
      state.buildingTemplatePlacement.resultBuildingIndex >=
          state.buildingTemplatePlacement.candidate.buildings.size()) {
    state.statusMessage = "building template cannot be placed here";
    return {false, false,
            "creative_editor_world_layout_building_template_commit_invalid"};
  }
  const std::size_t resultBuildingIndex =
      state.buildingTemplatePlacement.resultBuildingIndex;
  const std::uint64_t nextStableOrdinal =
      state.buildingTemplatePlacement.nextStableOrdinal;
  cr::CreativeWorldLayout candidate =
      std::move(state.buildingTemplatePlacement.candidate);
  state.buildingTemplatePlacement = {};
  state.source = std::move(candidate);
  state.nextStableOrdinal = nextStableOrdinal;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     resultBuildingIndex};
  detail::noteWorldLayoutSourceChange(state, "building template placed");
  return {true, true,
          "creative_editor_world_layout_building_template_committed"};
}

}  // namespace iggy3d_creative_app
