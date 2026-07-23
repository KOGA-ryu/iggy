#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iterator>
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

std::size_t templateIndexForId(
    const CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    std::string_view templateId) noexcept {
  const auto found = std::find_if(
      library.templates.begin(), library.templates.end(),
      [templateId](const auto& value) {
        return value.templateId == templateId;
      });
  return found == library.templates.end()
             ? cr::kInvalidCreativeWorldLayoutIndex
             : static_cast<std::size_t>(
                   std::distance(library.templates.begin(), found));
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

void prepareTemplatePlacementTerrain(
    CreativeEditorWorldLayoutState& state,
    const cr::CreativeDocument* document) {
  auto& placement = state.buildingTemplatePlacement;
  if (document == nullptr) {
    placement.terrainDocumentId = cr::kInvalidDocumentId;
    placement.terrainDocumentRevision = 0U;
    placement.terrainSurfacePrepared = true;
    placement.terrainGrid = {};
    placement.terrainSurface = {};
    return;
  }
  if (placement.terrainSurfacePrepared &&
      placement.terrainDocumentId == document->id() &&
      placement.terrainDocumentRevision == document->revision()) {
    return;
  }

  placement.terrainDocumentId = document->id();
  placement.terrainDocumentRevision = document->revision();
  placement.terrainSurfacePrepared = true;
  placement.terrainGrid = document->gridSettings();
  placement.terrainSurface = {};

  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(*document, state.source);
  if (!compiled.receipt.accepted) {
    return;
  }
  cr::CreativeDocument staged = *document;
  if ((!compiled.plan.terrainEdits.empty() &&
       !staged.applyTerrainControlEdits(compiled.plan.terrainEdits).accepted) ||
      (!compiled.plan.materialEdits.empty() &&
       !staged.applyTerrainMaterialEdits(compiled.plan.materialEdits).accepted)) {
    return;
  }
  placement.terrainSurface = cr::buildCreativeComposedTerrainSurfacePlan(
      staged.terrainField(), staged.terrainHeightField(),
      staged.terrainHardEdges());
}

CreativeEditorWorldLayoutEditReceipt rebuildTemplatePlacement(
    CreativeEditorWorldLayoutState& state,
    cr::CreativeTerrainCoord2 anchor,
    const cr::CreativeDocument* document) {
  prepareTemplatePlacementTerrain(state, document);
  const auto& placement = state.buildingTemplatePlacement;
  const cr::CreativeTerrainSurfacePlan* terrainPointer =
      placement.terrainSurface.accepted ? &placement.terrainSurface : nullptr;
  state.buildingTemplatePlacement.analysis =
      cr::analyzeCreativeWorldLayoutBuildingTemplatePlacement(
          {&state.source,
           &state.buildingTemplatePlacement.orientedTemplate,
           anchor,
           placement.terrainGrid,
           terrainPointer});
  cr::CreativeWorldLayoutBuildingEditResult stamped =
      cr::stampCreativeWorldLayoutBuildingTemplate(
          state.source, state.buildingTemplatePlacement.orientedTemplate,
          {anchor, state.nextStableOrdinal, false});
  state.buildingTemplatePlacement.anchor = anchor;
  state.buildingTemplatePlacement.previewPositioned = stamped.accepted;
  state.buildingTemplatePlacement.previewValid =
      stamped.accepted && state.buildingTemplatePlacement.analysis.accepted;
  state.buildingTemplatePlacement.reasonCode =
      stamped.accepted ? state.buildingTemplatePlacement.analysis.reasonCode
                       : stamped.reasonCode;
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
  if (state.buildingTemplatePlacement.previewValid) {
    state.statusMessage = "building template placement is clear";
    return {true, true,
            "creative_editor_world_layout_building_template_preview_ready"};
  }
  const auto& analysis = state.buildingTemplatePlacement.analysis;
  state.statusMessage =
      analysis.status ==
              cr::CreativeWorldLayoutBuildingTemplatePlacementStatus::
                  BuildingOverlap
          ? "building template overlaps an existing building"
      : analysis.status ==
                cr::CreativeWorldLayoutBuildingTemplatePlacementStatus::
                    TerrainRejected
          ? "building template terrain grounding is not valid here"
          : std::string{analysis.reasonCode};
  return {true, true, std::string{analysis.reasonCode}};
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

namespace {

CreativeEditorWorldLayoutBuildingTemplateInstallReceipt
installWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const cr::CreativeWorldLayoutBuildingTemplate& sourceTemplate,
    bool replaceExisting) {
  CreativeEditorWorldLayoutBuildingTemplateInstallReceipt receipt;
  receipt.requested = true;
  if (library.root.empty() ||
      !cr::validCreativeWorldLayoutBuildingTemplate(sourceTemplate) ||
      sourceTemplate.orientation !=
          cr::CreativeWorldLayoutBuildingTemplateOrientation::Identity) {
    receipt.reasonCode =
        "creative_editor_world_layout_building_template_install_invalid";
    return receipt;
  }

  cr::CreativeWorldLayoutBuildingTemplateResult canonical =
      cr::loadCreativeWorldLayoutBuildingTemplate(
          sourceTemplate.normalizedLayout);
  if (!canonical.accepted ||
      canonical.value.templateId != sourceTemplate.templateId ||
      canonical.value.sourceFingerprint != sourceTemplate.sourceFingerprint) {
    receipt.reasonCode =
        "creative_editor_world_layout_building_template_install_invalid";
    return receipt;
  }

  const std::size_t existingIndex =
      templateIndexForId(library, sourceTemplate.templateId);
  if (existingIndex < library.templates.size()) {
    receipt.templateIndex = existingIndex;
    const bool sourceChanged =
        library.templates[existingIndex].sourceFingerprint !=
        sourceTemplate.sourceFingerprint;
    if (sourceChanged && !replaceExisting) {
      receipt.reasonCode =
          "creative_editor_world_layout_building_template_install_conflict";
      return receipt;
    }
    if (sourceChanged) {
      const cr::CreativeWorldLayoutEncodeResult encoded =
          cr::encodeCreativeWorldLayout(canonical.value.normalizedLayout);
      const std::filesystem::path path =
          library.root /
          (canonical.value.templateId +
           std::string(kBuildingTemplateExtension));
      if (!encoded.accepted ||
          !writeTemplateFileAtomically(path, encoded.encodedText)) {
        receipt.reasonCode =
            "creative_editor_world_layout_building_template_install_write_failed";
        return receipt;
      }
      library.templates[existingIndex] = std::move(canonical.value);
      library.statusMessage = "built-in building template updated";
      receipt.accepted = true;
      receipt.changed = true;
      receipt.reasonCode =
          "creative_editor_world_layout_building_template_builtin_updated";
      return receipt;
    }
    receipt.accepted = true;
    receipt.reasonCode =
        "creative_editor_world_layout_building_template_install_no_change";
    return receipt;
  }
  if (library.templates.size() >=
      kCreativeEditorWorldLayoutBuildingTemplateCapacity) {
    receipt.reasonCode =
        "creative_editor_world_layout_building_template_install_capacity_reached";
    return receipt;
  }

  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(canonical.value.normalizedLayout);
  const std::filesystem::path path =
      library.root /
      (canonical.value.templateId + std::string(kBuildingTemplateExtension));
  if (!encoded.accepted ||
      !writeTemplateFileAtomically(path, encoded.encodedText)) {
    receipt.reasonCode =
        "creative_editor_world_layout_building_template_install_write_failed";
    return receipt;
  }

  receipt.templateIndex = library.templates.size();
  library.templates.push_back(std::move(canonical.value));
  if (library.selectedIndex >= library.templates.size()) {
    library.selectedIndex = receipt.templateIndex;
  }
  library.statusMessage = "building template installed";
  receipt.accepted = true;
  receipt.changed = true;
  receipt.reasonCode =
      "creative_editor_world_layout_building_template_installed";
  return receipt;
}

}  // namespace

CreativeEditorWorldLayoutBuildingTemplateInstallReceipt
installCreativeEditorWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const cr::CreativeWorldLayoutBuildingTemplate& sourceTemplate) {
  return installWorldLayoutBuildingTemplate(library, sourceTemplate, false);
}

CreativeEditorWorldLayoutBuildingTemplateInstallReceipt
installCreativeEditorBuiltInWorldLayoutBuildingTemplate(
    CreativeEditorWorldLayoutBuildingTemplateLibrary& library,
    const cr::CreativeWorldLayoutBuildingTemplate& sourceTemplate) {
  return installWorldLayoutBuildingTemplate(library, sourceTemplate, true);
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

cr::CreativeWorldLayoutBuildingTemplateSyncReceipt
inspectCreativeEditorWorldLayoutBuildingTemplateSync(
    const CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex) {
  const cr::CreativeWorldLayoutBuildingTemplateInstanceProvenance provenance =
      cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(state.source,
                                                                buildingIndex);
  const std::size_t templateIndex =
      provenance.valid
          ? templateIndexForId(state.buildingTemplates, provenance.templateId)
          : cr::kInvalidCreativeWorldLayoutIndex;
  const cr::CreativeWorldLayoutBuildingTemplate* sourceTemplate =
      templateIndex < state.buildingTemplates.templates.size()
          ? &state.buildingTemplates.templates[templateIndex]
          : nullptr;
  return cr::inspectCreativeWorldLayoutBuildingTemplateSync(
      state.source, buildingIndex, sourceTemplate);
}

CreativeEditorWorldLayoutEditReceipt
updateCreativeEditorWorldLayoutBuildingTemplateFromInstance(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex) {
  CreativeEditorWorldLayoutBuildingTemplateLibrary& library =
      state.buildingTemplates;
  const cr::CreativeWorldLayoutBuildingTemplateInstanceProvenance provenance =
      cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(state.source,
                                                                buildingIndex);
  const std::size_t templateIndex =
      provenance.valid ? templateIndexForId(library, provenance.templateId)
                       : cr::kInvalidCreativeWorldLayoutIndex;
  if (!provenance.valid || templateIndex >= library.templates.size() ||
      library.root.empty()) {
    state.statusMessage = "selected building has no template source";
    return {false, false,
            "creative_editor_world_layout_building_template_update_source_missing"};
  }

  const cr::CreativeWorldLayoutBuildingTemplate& existing =
      library.templates[templateIndex];
  cr::CreativeWorldLayoutBuildingTemplateResult captured =
      cr::captureCreativeWorldLayoutBuildingTemplate(
          state.source,
          {buildingIndex, existing.templateId, existing.label});
  if (!captured.accepted) {
    state.statusMessage = "building could not update its template";
    return {false, false, captured.reasonCode};
  }
  cr::CreativeWorldLayoutBuildingTemplateResult canonicalized =
      cr::orientCreativeWorldLayoutBuildingTemplate(
          captured.value,
          cr::inverseCreativeWorldLayoutBuildingTemplateOrientation(
              provenance.orientation));
  if (!canonicalized.accepted) {
    state.statusMessage = "building orientation could not be normalized";
    return {false, false, canonicalized.reasonCode};
  }
  cr::CreativeWorldLayoutBuildingTemplateResult replacement =
      cr::loadCreativeWorldLayoutBuildingTemplate(
          std::move(canonicalized.value.normalizedLayout));
  if (!replacement.accepted) {
    state.statusMessage = "updated building template is invalid";
    return {false, false, replacement.reasonCode};
  }

  const cr::CreativeWorldLayoutBuildingTemplateFingerprint instanceFingerprint =
      cr::fingerprintCreativeWorldLayoutBuilding(state.source, buildingIndex);
  if (!instanceFingerprint.valid) {
    state.statusMessage = "selected building cannot be fingerprinted";
    return {false, false,
            "creative_editor_world_layout_building_template_update_fingerprint_invalid"};
  }
  const bool libraryChanged =
      replacement.value.sourceFingerprint != existing.sourceFingerprint;
  const bool provenanceChanged =
      provenance.sourceFingerprint != replacement.value.sourceFingerprint.value ||
      provenance.instanceBaselineFingerprint != instanceFingerprint.value;
  if (!libraryChanged && !provenanceChanged) {
    library.statusMessage = "building template is already current";
    state.statusMessage = library.statusMessage;
    return {true, false,
            "creative_editor_world_layout_building_template_update_no_change"};
  }

  cr::CreativeWorldLayout candidate = state.source;
  if (provenanceChanged) {
    cr::CreativeWorldLayoutBuildingTemplateInstanceProvenance updated =
        provenance;
    updated.sourceFingerprint = replacement.value.sourceFingerprint.value;
    updated.instanceBaselineFingerprint = instanceFingerprint.value;
    if (!cr::setCreativeWorldLayoutBuildingTemplateInstanceProvenance(
            candidate, buildingIndex, updated)) {
      state.statusMessage = "building template provenance could not be updated";
      return {false, false,
              "creative_editor_world_layout_building_template_update_provenance_invalid"};
    }
  }

  if (libraryChanged) {
    const cr::CreativeWorldLayoutEncodeResult encoded =
        cr::encodeCreativeWorldLayout(replacement.value.normalizedLayout);
    const std::filesystem::path path =
        library.root /
        (replacement.value.templateId + std::string(kBuildingTemplateExtension));
    if (!encoded.accepted ||
        !writeTemplateFileAtomically(path, encoded.encodedText)) {
      library.statusMessage = "building template update could not be written";
      state.statusMessage = library.statusMessage;
      return {false, false,
              "creative_editor_world_layout_building_template_update_write_failed"};
    }
    library.templates[templateIndex] = std::move(replacement.value);
  }
  library.selectedIndex = templateIndex;
  library.statusMessage = "building template updated from selected instance";
  if (provenanceChanged) {
    state.source = std::move(candidate);
    detail::noteWorldLayoutSourceChange(
        state, "building template updated from selected instance");
  } else {
    state.statusMessage = library.statusMessage;
  }
  return {true, provenanceChanged,
          "creative_editor_world_layout_building_template_updated"};
}

CreativeEditorWorldLayoutEditReceipt
detachCreativeEditorWorldLayoutBuildingTemplateInstance(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex) {
  const cr::CreativeWorldLayoutBuildingEditResult detached =
      cr::detachCreativeWorldLayoutBuildingTemplateInstance(state.source,
                                                            buildingIndex);
  if (!detached.accepted) {
    state.statusMessage = detached.reasonCode;
    return {false, false, detached.reasonCode};
  }
  if (!detached.changed) {
    state.statusMessage = "selected building is already independent";
    return {true, false, detached.reasonCode};
  }
  state.source = detached.edited;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  detail::noteWorldLayoutSourceChange(state,
                                      "building template instance detached");
  return {true, true, detached.reasonCode};
}

CreativeEditorWorldLayoutEditReceipt
refreshCreativeEditorWorldLayoutBuildingTemplateInstances(
    CreativeEditorWorldLayoutState& state,
    std::size_t buildingIndex,
    cr::CreativeWorldLayoutBuildingTemplateRefreshMode mode) {
  const cr::CreativeWorldLayoutBuildingTemplateInstanceProvenance provenance =
      cr::creativeWorldLayoutBuildingTemplateInstanceProvenance(state.source,
                                                                buildingIndex);
  const std::size_t templateIndex =
      provenance.valid
          ? templateIndexForId(state.buildingTemplates, provenance.templateId)
          : cr::kInvalidCreativeWorldLayoutIndex;
  if (!provenance.valid ||
      templateIndex >= state.buildingTemplates.templates.size()) {
    state.statusMessage = "selected building has no template source";
    return {false, false,
            "creative_editor_world_layout_building_template_refresh_source_missing"};
  }

  const cr::CreativeWorldLayoutBuildingTemplateRefreshResult refreshed =
      cr::refreshCreativeWorldLayoutBuildingTemplateInstances(
          state.source,
          {&state.buildingTemplates.templates[templateIndex], mode,
           buildingIndex, state.nextStableOrdinal});
  if (!refreshed.accepted) {
    state.buildingTemplates.statusMessage = refreshed.reasonCode;
    state.statusMessage =
        refreshed.status ==
                cr::CreativeWorldLayoutBuildingTemplateRefreshStatus::
                    NoEligibleInstances
            ? "no building template instances need refresh"
            : "building template instances could not be refreshed";
    return {false, false, refreshed.reasonCode};
  }

  state.source = refreshed.edited;
  state.nextStableOrdinal = refreshed.nextStableOrdinal;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                     buildingIndex};
  repairCreativeEditorWorldLayoutActiveLevel(state, buildingIndex);
  state.buildingTemplates.selectedIndex = templateIndex;
  state.buildingTemplates.statusMessage =
      std::to_string(refreshed.refreshedInstanceCount) +
      " building template instance(s) refreshed";
  detail::noteWorldLayoutSourceChange(state,
                                      state.buildingTemplates.statusMessage);
  return {true, true,
          "creative_editor_world_layout_building_template_instances_refreshed"};
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
    cr::CreativeWorldLayoutBuildingTransformOperation operation,
    const cr::CreativeDocument* document) {
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
    return rebuildTemplatePlacement(state, anchor, document);
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
                                    state.buildingTemplatePlacement.anchor,
                                    document);
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
    return rebuildTemplatePlacement(state, anchor, document);
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
  state.activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  repairCreativeEditorWorldLayoutActiveLevel(state, resultBuildingIndex);
  detail::noteWorldLayoutSourceChange(state, "building template placed");
  return {true, true,
          "creative_editor_world_layout_building_template_committed"};
}

}  // namespace iggy3d_creative_app
