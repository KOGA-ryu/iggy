#include "EditorToolDescriptor.hpp"
#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutPanel.hpp"
#include "EditorWorldLayoutPanelInternal.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace cr = iggy3d::creative;

using iggy3d_creative_app::CreativeEditorTerrainGenerationState;
using iggy3d_creative_app::CreativeEditorToolActionRule;
using iggy3d_creative_app::CreativeEditorToolActionSpec;
using iggy3d_creative_app::CreativeEditorToolDescriptor;
using iggy3d_creative_app::CreativeEditorToolDescriptorValidationStatus;
using iggy3d_creative_app::CreativeEditorToolExposure;
using iggy3d_creative_app::CreativeEditorToolGlyph;
using iggy3d_creative_app::CreativeEditorToolId;
using iggy3d_creative_app::CreativeEditorToolLifecycle;
using iggy3d_creative_app::CreativeEditorToolMaturity;
using iggy3d_creative_app::CreativeEditorToolOptionBinding;
using iggy3d_creative_app::CreativeEditorToolOptionSpec;
using iggy3d_creative_app::CreativeEditorToolOptionWidget;
using iggy3d_creative_app::CreativeEditorVolumeSettingsProfile;
using iggy3d_creative_app::CreativeEditorWorldLayoutPaletteCategory;
using iggy3d_creative_app::CreativeEditorWorldLayoutInputProfile;
using iggy3d_creative_app::CreativeEditorWorldLayoutPreviewFill;
using iggy3d_creative_app::CreativeEditorWorldLayoutPreviewProfile;
using iggy3d_creative_app::CreativeEditorWorldLayoutState;
using iggy3d_creative_app::CreativeEditorWorldLayoutTool;
using iggy3d_creative_app::CreativeEditorWorldLayoutToolActivation;
using iggy3d_creative_app::CreativeEditorWorldLayoutTopographyState;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

const CreativeEditorToolDescriptor* findDescriptor(
    std::string_view name) {
  for (const CreativeEditorToolDescriptor& descriptor :
       iggy3d_creative_app::creativeEditorToolDescriptors()) {
    if (descriptor.name == name) {
      return &descriptor;
    }
  }
  return nullptr;
}

const CreativeEditorToolOptionSpec* findOption(
    const CreativeEditorToolDescriptor& descriptor,
    CreativeEditorToolOptionBinding binding) {
  for (const CreativeEditorToolOptionSpec& option : descriptor.options) {
    if (option.binding == binding) {
      return &option;
    }
  }
  return nullptr;
}

bool descriptorsOwnProductTruthAndReleaseVisibility() {
  const auto descriptors = iggy3d_creative_app::creativeEditorToolDescriptors();
  const auto catalogSpecs =
      iggy3d_creative_app::creativeEditorCatalogToolSpecs();
  const auto validation =
      iggy3d_creative_app::validateCreativeEditorToolDescriptors(descriptors);
  std::size_t worldSurfaceCount = 0U;
  std::size_t defaultWorldSurfaceCount = 0U;
  std::size_t experimentalWorldSurfaceCount = 0U;
  bool worldGlyphsInked = true;
  bool estateTemplateMiscounted = false;
  bool catalogProjectionComplete = true;
  for (const CreativeEditorToolDescriptor& descriptor : descriptors) {
    estateTemplateMiscounted =
        estateTemplateMiscounted || descriptor.name == "Estate House";
    if (!iggy3d_creative_app::creativeEditorToolHasWorldLayoutSurface(
            descriptor)) {
      continue;
    }
    ++worldSurfaceCount;
    defaultWorldSurfaceCount +=
        iggy3d_creative_app::creativeEditorToolDefaultVisible(descriptor)
            ? 1U
            : 0U;
    experimentalWorldSurfaceCount +=
        iggy3d_creative_app::creativeEditorToolExperimental(descriptor) ? 1U
                                                                        : 0U;
    worldGlyphsInked =
        worldGlyphsInked &&
        !iggy3d_creative_app::creativeEditorToolGlyphOps(descriptor.glyph)
             .empty();
  }
  for (const cr::CreativeCatalogToolSpec& spec : catalogSpecs) {
    const CreativeEditorToolDescriptor& descriptor =
        iggy3d_creative_app::describeCreativeEditorHeldItemTool(spec.kind);
    catalogProjectionComplete =
        catalogProjectionComplete && descriptor.catalogTool &&
        descriptor.heldItemKind == spec.kind && descriptor.name == spec.label &&
        spec.experimental ==
            iggy3d_creative_app::creativeEditorToolExperimental(descriptor) &&
        !spec.purpose.empty() && !spec.maturity.empty() &&
        !spec.lifecycle.empty() && !spec.inputHint.empty();
  }
  constexpr std::array materialPalette{cr::CreativeObjectKind::Wall};
  const cr::CreativeHotbarState defaultHotbar =
      iggy3d_creative_app::makeCreativeEditorDefaultHotbar(materialPalette);
  std::size_t defaultHotbarCount = 0U;
  bool defaultHotbarRowsReleased = true;
  for (const cr::CreativeHotbarEntry& entry : defaultHotbar.entries) {
    if (entry.kind >= cr::CreativeHeldItemKind::Count) {
      continue;
    }
    ++defaultHotbarCount;
    const CreativeEditorToolDescriptor& descriptor =
        iggy3d_creative_app::describeCreativeEditorHeldItemTool(entry.kind);
    defaultHotbarRowsReleased =
        defaultHotbarRowsReleased && descriptor.defaultWheelEligible &&
        !iggy3d_creative_app::creativeEditorToolExperimental(descriptor) &&
        descriptor.heldItemKind == entry.kind &&
        descriptor.actionHintProfile !=
            iggy3d_creative_app::CreativeEditorActionHintProfile::None &&
        !descriptor.controllerHint.empty();
  }
  const CreativeEditorToolDescriptor* terrainRegion =
      findDescriptor("Terrain Region");
  const CreativeEditorToolDescriptor* terrainRod =
      findDescriptor("Terrain Rod");
  const CreativeEditorToolDescriptor* terrainSculpt =
      findDescriptor("Terrain Sculpt");
  const CreativeEditorToolDescriptor* terrainGrade =
      findDescriptor("Terrain Grade");
  const CreativeEditorToolDescriptor* terrainProfile =
      findDescriptor("Terrain Profile");
  const bool rodShelvedFromDefaultProduct =
      terrainRod != nullptr &&
      terrainRod->heldItemKind == cr::CreativeHeldItemKind::TerrainControl &&
      terrainRod->maturity == CreativeEditorToolMaturity::M1Prototype &&
      iggy3d_creative_app::creativeEditorToolExperimental(*terrainRod) &&
      !iggy3d_creative_app::creativeEditorToolDefaultVisible(*terrainRod) &&
      !terrainRod->defaultWheelEligible;
  const bool regionShapeRight =
      terrainRegion != nullptr &&
      terrainRegion->worldLayoutActivation ==
          CreativeEditorWorldLayoutToolActivation::TerrainRegionSession &&
      iggy3d_creative_app::creativeEditorWorldLayoutPaletteCategory(
          *terrainRegion) == CreativeEditorWorldLayoutPaletteCategory::Terrain &&
      terrainRegion->glyph == CreativeEditorToolGlyph::MaskRectangle &&
      terrainRegion->options.size() == 8U &&
      terrainRegion->actions.size() == 3U &&
      terrainRegion->locks != iggy3d_creative_app::kCreativeEditorToolLockNone;
  const bool sculptContractHonest =
      terrainSculpt != nullptr &&
      terrainSculpt->maturity == CreativeEditorToolMaturity::M2StableRecipe &&
      terrainSculpt->lifecycle == CreativeEditorToolLifecycle::Destructive &&
      iggy3d_creative_app::creativeEditorToolExperimental(*terrainSculpt) &&
      !iggy3d_creative_app::creativeEditorToolDefaultVisible(*terrainSculpt) &&
      !terrainSculpt->defaultWheelEligible;
  const bool gradeContractHonest =
      terrainGrade != nullptr &&
      terrainGrade->maturity == CreativeEditorToolMaturity::M2StableRecipe &&
      terrainGrade->lifecycle == CreativeEditorToolLifecycle::Parametric &&
      iggy3d_creative_app::creativeEditorToolExperimental(*terrainGrade) &&
      !iggy3d_creative_app::creativeEditorToolDefaultVisible(*terrainGrade) &&
      !terrainGrade->defaultWheelEligible;
  const bool profileContractHonest =
      terrainProfile != nullptr &&
      terrainProfile->maturity == CreativeEditorToolMaturity::M2StableRecipe &&
      terrainProfile->lifecycle == CreativeEditorToolLifecycle::Parametric &&
      iggy3d_creative_app::creativeEditorToolExperimental(*terrainProfile) &&
      !iggy3d_creative_app::creativeEditorToolDefaultVisible(*terrainProfile) &&
      !terrainProfile->defaultWheelEligible;

  std::vector<CreativeEditorToolDescriptor> invalid(descriptors.begin(),
                                                     descriptors.end());
  invalid[static_cast<std::size_t>(CreativeEditorToolId::Group)].exposure =
      CreativeEditorToolExposure::Default;
  const auto rejected =
      iggy3d_creative_app::validateCreativeEditorToolDescriptors(invalid);

  return expect(validation.status ==
                    CreativeEditorToolDescriptorValidationStatus::Valid,
                "the canonical descriptor table passes its release gate") &&
         expect(descriptors.size() ==
                    static_cast<std::size_t>(CreativeEditorToolId::Count),
                "every product tool id has exactly one descriptor") &&
         expect(worldSurfaceCount == 17U && defaultWorldSurfaceCount == 1U &&
                    experimentalWorldSurfaceCount == 16U,
                "the drafting surface exposes Select by default and gates all "
                "unfinished authoring tools") &&
         expect(worldGlyphsInked,
                "every drafting-bound descriptor carries a real glyph") &&
         expect(catalogSpecs.size() == cr::kCreativeHeldItemKindCount - 1U &&
                    catalogProjectionComplete,
                "catalog rows are complete projections of held-tool "
                "descriptors") &&
         expect(defaultHotbarCount == 2U && defaultHotbarRowsReleased &&
                    defaultHotbar.entries[0].kind ==
                        cr::CreativeHeldItemKind::ObjectSelect &&
                    defaultHotbar.entries[1].kind ==
                        cr::CreativeHeldItemKind::ObjectMove &&
                    defaultHotbar.entries[2].kind ==
                        cr::CreativeHeldItemKind::Count,
                "default hotbar contains only released workspace tools") &&
         expect(!estateTemplateMiscounted,
                "building templates are not counted as tools") &&
         expect(rodShelvedFromDefaultProduct,
                "the shelved terrain rod stays off default product surfaces") &&
         expect(regionShapeRight,
                "terrain region owns its settings, actions, and locks") &&
         expect(sculptContractHonest,
                "terrain sculpt is a stable destructive recipe kept off "
                "default product surfaces") &&
         expect(gradeContractHonest,
                "terrain grade is a stable parametric recipe kept off "
                "default product surfaces") &&
         expect(profileContractHonest,
                "terrain profile is a stable parametric recipe kept off "
                "default product surfaces") &&
         expect(rejected.status == CreativeEditorToolDescriptorValidationStatus::
                                       DefaultProductBelowM3 &&
                    rejected.tool == CreativeEditorToolId::Group,
                "the release gate rejects a sub-M3 default product tool");
}

bool volumeSettingsProfilesAreDescriptorOwned() {
  struct ExpectedProfile {
    cr::CreativeHeldItemKind held;
    CreativeEditorVolumeSettingsProfile profile;
  };
  constexpr std::array expected{
      ExpectedProfile{cr::CreativeHeldItemKind::VolumeFill,
                      CreativeEditorVolumeSettingsProfile::Fill},
      ExpectedProfile{cr::CreativeHeldItemKind::VolumeHollow,
                      CreativeEditorVolumeSettingsProfile::Hollow},
      ExpectedProfile{cr::CreativeHeldItemKind::VolumeReplace,
                      CreativeEditorVolumeSettingsProfile::Replace},
      ExpectedProfile{cr::CreativeHeldItemKind::VolumeErase,
                      CreativeEditorVolumeSettingsProfile::Erase},
      ExpectedProfile{cr::CreativeHeldItemKind::VolumeClone,
                      CreativeEditorVolumeSettingsProfile::Clone},
  };

  bool exact = true;
  for (const ExpectedProfile& row : expected) {
    exact = exact &&
            iggy3d_creative_app::describeCreativeEditorHeldItemTool(row.held)
                    .volumeSettingsProfile == row.profile;
  }
  std::size_t configuredCount = 0U;
  for (const CreativeEditorToolDescriptor& descriptor :
       iggy3d_creative_app::creativeEditorToolDescriptors()) {
    configuredCount +=
        descriptor.volumeSettingsProfile !=
                CreativeEditorVolumeSettingsProfile::None
            ? 1U
            : 0U;
  }

  std::vector<CreativeEditorToolDescriptor> invalid(
      iggy3d_creative_app::creativeEditorToolDescriptors().begin(),
      iggy3d_creative_app::creativeEditorToolDescriptors().end());
  invalid[static_cast<std::size_t>(CreativeEditorToolId::VolumeFill)]
      .volumeSettingsProfile = CreativeEditorVolumeSettingsProfile::Count;
  const auto rejected =
      iggy3d_creative_app::validateCreativeEditorToolDescriptors(invalid);

  return expect(exact && configuredCount == expected.size(),
                "volume submode settings resolve from exactly five product "
                "tool descriptors") &&
         expect(rejected.status ==
                    CreativeEditorToolDescriptorValidationStatus::InvalidEnum,
                "the release gate rejects an invalid volume settings profile");
}

bool evaluationTracksActiveWorldLayoutTool() {
  CreativeEditorWorldLayoutState state;
  const CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorTerrainGenerationState terrainGeneration;
  const CreativeEditorToolDescriptor* select = findDescriptor("Select");
  const CreativeEditorToolDescriptor* partition = findDescriptor("Partition");
  const CreativeEditorToolDescriptor* asset =
      findDescriptor("Catalog Asset Placement");
  if (!expect(select != nullptr && partition != nullptr && asset != nullptr,
              "select, partition, and asset placement descriptors exist")) {
    return false;
  }
  const auto evaluate = [&](const CreativeEditorToolDescriptor& descriptor) {
    return iggy3d_creative_app::evaluateCreativeEditorTool(
        descriptor, state, topography, terrainGeneration);
  };
  const auto defaultSelect = evaluate(*select);
  const auto defaultPartition = evaluate(*partition);
  state.tool = CreativeEditorWorldLayoutTool::Wall;
  const auto wallSelect = evaluate(*select);
  const auto wallPartition = evaluate(*partition);
  state.buildingTemplatePlacement.active = true;
  state.buildingTemplatePlacement.templateIndex = 0U;
  const auto partitionDuringPlacement = evaluate(*partition);
  state.buildingTemplatePlacement.active = false;
  state.tool = CreativeEditorWorldLayoutTool::CatalogAsset;
  const auto assetActive = evaluate(*asset);
  return expect(defaultSelect.active && !defaultSelect.unavailable,
                "select is the default active tool") &&
         expect(!defaultPartition.active, "partition starts inactive") &&
         expect(!wallSelect.active && wallPartition.active,
                "activating wall moves the highlight to partition") &&
         expect(!partitionDuringPlacement.active,
                "tool highlights drop during template placement") &&
         expect(assetActive.active,
                "catalog asset placement has one active drafting binding");
}

bool terrainSessionLocksFollowTheWorkflow() {
  CreativeEditorWorldLayoutState state;
  CreativeEditorWorldLayoutTopographyState topography;
  CreativeEditorTerrainGenerationState terrainGeneration;
  const CreativeEditorToolDescriptor* session =
      findDescriptor("Terrain Region");
  if (!expect(session != nullptr, "the terrain region session exists")) {
    return false;
  }
  const auto evaluate = [&]() {
    return iggy3d_creative_app::evaluateCreativeEditorTool(
        *session, state, topography, terrainGeneration);
  };
  const auto idle = evaluate();
  topography.region.editingEnabled = true;
  const auto editing = evaluate();
  terrainGeneration.previewActive = true;
  const auto foreignPreview = evaluate();
  topography.region.ownsPreview = true;
  const auto ownedPreview = evaluate();
  terrainGeneration.previewActive = false;
  topography.region.ownsPreview = false;
  state.buildingTransform.active = true;
  const auto duringTransform = evaluate();
  state.buildingTransform.active = false;
  state.buildingTemplatePlacement.active = true;
  const auto duringPlacement = evaluate();
  return expect(!idle.active && !idle.unavailable,
                "the session starts idle and available") &&
         expect(editing.active, "enabling region editing lights the session") &&
         expect(foreignPreview.unavailable,
                "a foreign terrain preview locks the session") &&
         expect(!ownedPreview.unavailable,
                "owning the preview keeps the session available") &&
         expect(duringTransform.unavailable,
                "building transforms lock the session") &&
         expect(duringPlacement.unavailable,
                "template placement locks the session");
}

bool activeDescriptorFollowsTheSession() {
  CreativeEditorWorldLayoutState state;
  CreativeEditorWorldLayoutTopographyState topography;
  const auto active = [&]() {
    return iggy3d_creative_app::activeCreativeEditorWorldLayoutToolDescriptor(
               state, topography)
        .id;
  };
  const auto defaultActive = active();
  state.tool = CreativeEditorWorldLayoutTool::Wall;
  const auto wallActive = active();
  state.tool = CreativeEditorWorldLayoutTool::CatalogAsset;
  const auto catalogActive = active();
  topography.region.editingEnabled = true;
  const auto regionActive = active();
  topography.region.editingEnabled = false;
  state.buildingTemplatePlacement.active = true;
  state.buildingTemplatePlacement.templateIndex = 0U;
  const auto placingActive = active();
  return expect(defaultActive == CreativeEditorToolId::Select,
                "select is the default descriptor") &&
         expect(wallActive == CreativeEditorToolId::Partition,
                "the wall tool activates the partition descriptor") &&
         expect(catalogActive == CreativeEditorToolId::CatalogAssetPlacement,
                "the catalog tool activates the asset placement descriptor") &&
         expect(regionActive == CreativeEditorToolId::TerrainRegion,
                "region editing takes over the active descriptor") &&
         expect(placingActive == CreativeEditorToolId::Select,
                "template placement does not become a fake product tool");
}

bool worldLayoutCanvasPolicyComesFromDescriptors() {
  struct ExpectedPolicy {
    CreativeEditorWorldLayoutTool tool;
    CreativeEditorWorldLayoutInputProfile input;
    CreativeEditorWorldLayoutPreviewProfile preview;
    CreativeEditorWorldLayoutPreviewFill fill;
  };
  constexpr std::array expected{
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Select,
                     CreativeEditorWorldLayoutInputProfile::Point,
                     CreativeEditorWorldLayoutPreviewProfile::None,
                     CreativeEditorWorldLayoutPreviewFill::None},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::BuildingShell,
                     CreativeEditorWorldLayoutInputProfile::Drag,
                     CreativeEditorWorldLayoutPreviewProfile::Rectangle,
                     CreativeEditorWorldLayoutPreviewFill::Structure},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Room,
                     CreativeEditorWorldLayoutInputProfile::Drag,
                     CreativeEditorWorldLayoutPreviewProfile::Rectangle,
                     CreativeEditorWorldLayoutPreviewFill::Structure},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Floor,
                     CreativeEditorWorldLayoutInputProfile::Drag,
                     CreativeEditorWorldLayoutPreviewProfile::Rectangle,
                     CreativeEditorWorldLayoutPreviewFill::None},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Wall,
                     CreativeEditorWorldLayoutInputProfile::Drag,
                     CreativeEditorWorldLayoutPreviewProfile::AxisLine,
                     CreativeEditorWorldLayoutPreviewFill::None},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Door,
                     CreativeEditorWorldLayoutInputProfile::Point,
                     CreativeEditorWorldLayoutPreviewProfile::DoorOpening,
                     CreativeEditorWorldLayoutPreviewFill::None},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Window,
                     CreativeEditorWorldLayoutInputProfile::Point,
                     CreativeEditorWorldLayoutPreviewProfile::WindowOpening,
                     CreativeEditorWorldLayoutPreviewFill::None},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Stair,
                     CreativeEditorWorldLayoutInputProfile::Drag,
                     CreativeEditorWorldLayoutPreviewProfile::Rectangle,
                     CreativeEditorWorldLayoutPreviewFill::VerticalConnector},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Ramp,
                     CreativeEditorWorldLayoutInputProfile::Drag,
                     CreativeEditorWorldLayoutPreviewProfile::Rectangle,
                     CreativeEditorWorldLayoutPreviewFill::Ramp},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Plateau,
                     CreativeEditorWorldLayoutInputProfile::Point,
                     CreativeEditorWorldLayoutPreviewProfile::None,
                     CreativeEditorWorldLayoutPreviewFill::None},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Road,
                     CreativeEditorWorldLayoutInputProfile::Polyline,
                     CreativeEditorWorldLayoutPreviewProfile::TerrainPath,
                     CreativeEditorWorldLayoutPreviewFill::Road},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Ditch,
                     CreativeEditorWorldLayoutInputProfile::Polyline,
                     CreativeEditorWorldLayoutPreviewProfile::TerrainPath,
                     CreativeEditorWorldLayoutPreviewFill::Ditch},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::Bridge,
                     CreativeEditorWorldLayoutInputProfile::Drag,
                     CreativeEditorWorldLayoutPreviewProfile::Rectangle,
                     CreativeEditorWorldLayoutPreviewFill::Structure},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::CatalogAsset,
                     CreativeEditorWorldLayoutInputProfile::Point,
                     CreativeEditorWorldLayoutPreviewProfile::CatalogAsset,
                     CreativeEditorWorldLayoutPreviewFill::None},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::PlayerSpawn,
                     CreativeEditorWorldLayoutInputProfile::Point,
                     CreativeEditorWorldLayoutPreviewProfile::None,
                     CreativeEditorWorldLayoutPreviewFill::None},
      ExpectedPolicy{CreativeEditorWorldLayoutTool::NpcSpawn,
                     CreativeEditorWorldLayoutInputProfile::Point,
                     CreativeEditorWorldLayoutPreviewProfile::None,
                     CreativeEditorWorldLayoutPreviewFill::None},
  };

  bool exactPolicies =
      expected.size() ==
      static_cast<std::size_t>(CreativeEditorWorldLayoutTool::Count);
  for (const ExpectedPolicy& policy : expected) {
    const CreativeEditorToolDescriptor& descriptor =
        iggy3d_creative_app::describeCreativeEditorWorldLayoutTool(policy.tool);
    exactPolicies =
        exactPolicies && descriptor.worldLayoutTool == policy.tool &&
        descriptor.worldLayoutInputProfile == policy.input &&
        descriptor.worldLayoutPreviewProfile == policy.preview &&
        descriptor.worldLayoutPreviewFill == policy.fill &&
        (policy.input != CreativeEditorWorldLayoutInputProfile::Drag ||
         !descriptor.worldLayoutInteractionPrompt.empty()) &&
        std::string_view(
            iggy3d_creative_app::creativeEditorWorldLayoutToolLabel(
                policy.tool)) == descriptor.name;
  }

  const auto descriptors = iggy3d_creative_app::creativeEditorToolDescriptors();
  std::vector<CreativeEditorToolDescriptor> invalidPolicy(descriptors.begin(),
                                                          descriptors.end());
  invalidPolicy[static_cast<std::size_t>(CreativeEditorToolId::Partition)]
      .worldLayoutInputProfile = CreativeEditorWorldLayoutInputProfile::Count;
  const auto policyRejected =
      iggy3d_creative_app::validateCreativeEditorToolDescriptors(invalidPolicy);

  std::vector<CreativeEditorToolDescriptor> duplicateBinding(
      descriptors.begin(), descriptors.end());
  duplicateBinding[static_cast<std::size_t>(CreativeEditorToolId::NpcSpawn)]
      .worldLayoutTool = CreativeEditorWorldLayoutTool::PlayerSpawn;
  const auto duplicateRejected =
      iggy3d_creative_app::validateCreativeEditorToolDescriptors(
          duplicateBinding);

  std::vector<CreativeEditorToolDescriptor> missingBinding(descriptors.begin(),
                                                           descriptors.end());
  CreativeEditorToolDescriptor& missing =
      missingBinding[static_cast<std::size_t>(CreativeEditorToolId::NpcSpawn)];
  missing.worldLayoutActivation = CreativeEditorWorldLayoutToolActivation::None;
  missing.worldLayoutTool = CreativeEditorWorldLayoutTool::Count;
  missing.worldLayoutInputProfile = CreativeEditorWorldLayoutInputProfile::Count;
  missing.worldLayoutPreviewProfile =
      CreativeEditorWorldLayoutPreviewProfile::Count;
  missing.worldLayoutPreviewFill = CreativeEditorWorldLayoutPreviewFill::Count;
  missing.worldLayoutInteractionPrompt = {};
  const auto missingRejected =
      iggy3d_creative_app::validateCreativeEditorToolDescriptors(missingBinding);

  return expect(exactPolicies,
                "every drafting tool resolves one exact descriptor-owned "
                "input and preview policy") &&
         expect(policyRejected.status ==
                    CreativeEditorToolDescriptorValidationStatus::
                        InvalidWorldLayoutPolicy,
                "the release gate rejects an incomplete canvas policy") &&
         expect(duplicateRejected.status ==
                    CreativeEditorToolDescriptorValidationStatus::
                        DuplicateWorldLayoutBinding,
                "the release gate rejects duplicate drafting bindings") &&
         expect(missingRejected.status ==
                    CreativeEditorToolDescriptorValidationStatus::
                        MissingWorldLayoutBinding,
                "the release gate rejects missing drafting bindings");
}

bool optionsExposeTheRightFieldsPerOperation() {
  CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorToolDescriptor* session =
      findDescriptor("Terrain Region");
  if (!expect(session != nullptr, "the terrain region session exists")) {
    return false;
  }
  const CreativeEditorToolOptionSpec* operationOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainOperation);
  const CreativeEditorToolOptionSpec* maskOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainMask);
  const CreativeEditorToolOptionSpec* amountOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainAmount);
  const CreativeEditorToolOptionSpec* heightOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainTargetHeight);
  const CreativeEditorToolOptionSpec* reliefOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainNoiseRelief);
  const CreativeEditorToolOptionSpec* scaleOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainNoiseScale);
  const CreativeEditorToolOptionSpec* seedOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainSeed);
  const CreativeEditorToolOptionSpec* featherOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainFeather);
  if (!expect(operationOption != nullptr && maskOption != nullptr &&
                  amountOption != nullptr &&
                  heightOption != nullptr && reliefOption != nullptr &&
                  scaleOption != nullptr && seedOption != nullptr &&
                  featherOption != nullptr,
              "all eight terrain options are declared")) {
    return false;
  }
  using Operation = cr::CreativeTerrainRegionMode;
  bool visibilityRight = true;
  for (const Operation operation :
       {Operation::Flatten, Operation::Raise, Operation::Lower,
        Operation::Smooth, Operation::Noise, Operation::Erase}) {
    topography.region.recipe.mode = operation;
    const bool noise = operation == Operation::Noise;
    const auto visible = [&](const CreativeEditorToolOptionSpec* option) {
      return iggy3d_creative_app::creativeEditorToolOptionVisible(*option,
                                                                  topography);
    };
    if (!visible(operationOption) || !visible(maskOption) ||
        !visible(featherOption) ||
        visible(amountOption) !=
            (operation == Operation::Raise || operation == Operation::Lower ||
             operation == Operation::Smooth) ||
        visible(heightOption) !=
            (operation == Operation::Flatten || operation == Operation::Noise) ||
        visible(reliefOption) != noise || visible(scaleOption) != noise ||
        visible(seedOption) != noise) {
      visibilityRight = false;
    }
  }
  topography.region.recipe.mode = Operation::Flatten;
  const std::string_view flattenFull =
      iggy3d_creative_app::creativeEditorToolOptionFullLabel(*heightOption,
                                                             topography);
  const std::string_view flattenCompact =
      iggy3d_creative_app::creativeEditorToolOptionCompactLabel(*heightOption,
                                                                topography);
  topography.region.recipe.mode = Operation::Noise;
  const std::string_view noiseFull =
      iggy3d_creative_app::creativeEditorToolOptionFullLabel(*heightOption,
                                                             topography);
  const bool clampsRight =
      heightOption->minimum ==
          static_cast<double>(cr::kCreativeTerrainMinimumHeightCells) &&
      heightOption->maximum ==
          static_cast<double>(cr::kCreativeTerrainMaximumHeightCells) &&
      amountOption->minimum == 1.0 &&
      amountOption->maximum == static_cast<double>(
                                   cr::kCreativeTerrainRegionMaximumAmountCells) &&
      featherOption->maximum ==
          static_cast<double>(
              cr::kCreativeTerrainCompositionMaximumFeatherCells) &&
      scaleOption->minimum ==
          cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells &&
      scaleOption->maximum ==
          cr::kCreativeTerrainGeneratorMaximumHorizontalScaleCells;
  const bool choicesRight =
      operationOption->widget == CreativeEditorToolOptionWidget::GlyphChoice &&
      operationOption->choices.size() == 6U &&
      operationOption->choices[4].glyph ==
          CreativeEditorToolGlyph::RegionNoise &&
      operationOption->choices[4].value ==
          static_cast<std::uint8_t>(Operation::Noise) &&
      operationOption->choices[5].glyph ==
          CreativeEditorToolGlyph::BadgeRemove &&
      operationOption->choices[5].value ==
          static_cast<std::uint8_t>(Operation::Erase) &&
      maskOption->choices.size() == 2U &&
      maskOption->choices[1].glyph == CreativeEditorToolGlyph::MaskEllipse;
  return expect(visibilityRight,
                "option visibility follows the operation exactly") &&
         expect(flattenFull == "Height" && flattenCompact == "Height" &&
                    noiseFull == "Base height",
                "the target height labels follow the operation") &&
         expect(clampsRight, "option clamps pin the engine constants") &&
         expect(choicesRight,
                "operation and mask choices carry the region glyphs");
}

bool optionBindingsReadWriteAndClamp() {
  CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorToolDescriptor* session =
      findDescriptor("Terrain Region");
  const CreativeEditorToolOptionSpec* heightOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainTargetHeight);
  const CreativeEditorToolOptionSpec* amountOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainAmount);
  const CreativeEditorToolOptionSpec* reliefOption = findOption(
      *session, CreativeEditorToolOptionBinding::TerrainNoiseRelief);
  const CreativeEditorToolOptionSpec* scaleOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainNoiseScale);
  const CreativeEditorToolOptionSpec* operationOption =
      findOption(*session, CreativeEditorToolOptionBinding::TerrainOperation);
  const bool heightChanged =
      iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
          *heightOption, topography, 12.0);
  const bool heightUnchanged =
      !iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
          *heightOption, topography, 12.0);
  const bool heightStored =
      topography.region.recipe.targetHeightCells == 12U;
  static_cast<void>(iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
      *amountOption, topography, 99999.0));
  const bool amountClamped =
      topography.region.recipe.amountCells ==
      cr::kCreativeTerrainRegionMaximumAmountCells;
  static_cast<void>(iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
      *reliefOption, topography, 99999.0));
  const bool reliefClamped =
      topography.region.recipe.noiseReliefCells ==
      cr::kCreativeTerrainMaximumHeightCells;
  static_cast<void>(iggy3d_creative_app::setCreativeEditorToolOptionScalarValue(
      *scaleOption, topography, 0.0));
  const bool scaleClamped =
      topography.region.recipe.noiseScaleCells ==
      cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells;
  const bool operationChanged =
      iggy3d_creative_app::setCreativeEditorToolOptionChoiceValue(
          *operationOption, topography,
          static_cast<std::uint8_t>(
              cr::CreativeTerrainRegionMode::Noise));
  const bool operationStored =
      topography.region.recipe.mode == cr::CreativeTerrainRegionMode::Noise;
  const bool invalidRejected =
      !iggy3d_creative_app::setCreativeEditorToolOptionChoiceValue(
          *operationOption, topography, 99U);
  topography.region.recipe.seed = 5U;
  iggy3d_creative_app::advanceCreativeEditorToolOptionSeed(topography);
  const bool seedAdvanced = topography.region.recipe.seed == 6U;
  topography.region.recipe.seed = std::numeric_limits<std::uint64_t>::max();
  iggy3d_creative_app::advanceCreativeEditorToolOptionSeed(topography);
  const bool seedWrapped = topography.region.recipe.seed == 0U;
  return expect(heightChanged && heightStored,
                "setting the target height writes the region state") &&
         expect(heightUnchanged,
                "rewriting the same value reports no change") &&
         expect(amountClamped, "amount writes clamp to the recipe maximum") &&
         expect(reliefClamped, "relief writes clamp to the engine maximum") &&
         expect(scaleClamped, "scale writes clamp to the engine minimum") &&
         expect(operationChanged && operationStored,
                "choice writes switch the region operation") &&
         expect(invalidRejected, "out-of-range choices are rejected") &&
         expect(seedAdvanced && seedWrapped,
                "the seed advances and wraps at the numeric limit");
}

bool actionRulesMatchTheBuildWindow() {
  CreativeEditorWorldLayoutTopographyState topography;
  const CreativeEditorToolDescriptor* session =
      findDescriptor("Terrain Region");
  if (!expect(session != nullptr && session->actions.size() == 3U,
              "the session declares three actions")) {
    return false;
  }
  const CreativeEditorToolActionSpec& preview = session->actions[0];
  const CreativeEditorToolActionSpec& apply = session->actions[1];
  const CreativeEditorToolActionSpec& cancel = session->actions[2];
  const bool commandsRight =
      preview.command ==
          iggy3d_creative_app::CreativeDesktopCommandId::
              WorldLayoutTerrainRegionPreview &&
      apply.command == iggy3d_creative_app::CreativeDesktopCommandId::
                           WorldLayoutTerrainRegionApply &&
      cancel.command == iggy3d_creative_app::CreativeDesktopCommandId::
                            WorldLayoutTerrainRegionCancel;
  const bool onChangeRight = preview.runOnOptionChange &&
                             !apply.runOnOptionChange &&
                             !cancel.runOnOptionChange;
  const auto enabled = [&](const CreativeEditorToolActionSpec& action) {
    return iggy3d_creative_app::creativeEditorToolActionEnabled(action,
                                                                topography);
  };
  const bool idleAllDisabled =
      !enabled(preview) && !enabled(apply) && !enabled(cancel);
  topography.region.regionValid = true;
  const bool validRegion =
      enabled(preview) && !enabled(apply) && enabled(cancel);
  topography.region.selecting = true;
  const bool whileSelecting = !enabled(preview) && enabled(cancel);
  topography.region.selecting = false;
  topography.region.ownsPreview = true;
  const bool owningPreview = enabled(apply);
  return expect(commandsRight, "the actions pin the region command ids") &&
         expect(onChangeRight,
                "only preview re-runs after an option change") &&
         expect(idleAllDisabled, "an empty region disables every action") &&
         expect(validRegion,
                "a valid idle region enables preview and cancel") &&
         expect(whileSelecting,
                "selecting suspends preview but keeps cancel") &&
         expect(owningPreview, "owning the preview enables apply");
}

bool statusLineComposesFromTheRightSources() {
  CreativeEditorWorldLayoutState state;
  CreativeEditorWorldLayoutTopographyState topography;
  CreativeEditorTerrainGenerationState terrainGeneration;
  iggy3d_creative_app::CreativeEditorWorldLayoutCanvasHoverStatus hover;
  const auto compose = [&]() {
    return iggy3d_creative_app::composeCreativeEditorWorldLayoutStatusLine(
        state, topography, terrainGeneration, hover);
  };
  const auto idle = compose();
  hover.present = true;
  hover.cellX = -0.2;
  hover.cellZ = 5.9;
  hover.semanticRole = "exterior wall";
  state.canvasPixelsPerCell = 33.6F;
  const auto hovered = compose();
  state.tool = CreativeEditorWorldLayoutTool::CatalogAsset;
  state.catalogPlacement.snapMode =
      iggy3d_creative_app::CreativeEditorWorldLayoutCatalogSnapMode::Wall;
  const auto snapping = compose();
  topography.region.editingEnabled = true;
  topography.region.regionValid = true;
  topography.region.recipe.bounds.minimum.x = -1;
  topography.region.recipe.bounds.minimum.z = -1;
  topography.region.recipe.bounds.widthCells = 2U;
  topography.region.recipe.bounds.depthCells = 2U;
  const auto validRegion = compose();
  topography.region.ownsPreview = true;
  terrainGeneration.operationPreview.receipt.replay.modifiedCellCount = 57U;
  terrainGeneration.operationPreview.receipt.replay.outputCellCount = 4096U;
  const auto owningPreview = compose();
  return expect(idle.cursor == "Cell --" && idle.zoom == "Zoom 100%",
                "the idle bar shows no cell and the baseline zoom") &&
         expect(idle.snap.empty() && idle.cells.empty(),
                "the idle bar omits snap and cell segments") &&
         expect(!idle.regionMessage && idle.message == state.statusMessage,
                "outside region editing the layout status is the message") &&
         expect(hovered.cursor == "Cell -1, 5 | exterior wall",
                "hover cursor names the semantic plan target") &&
         expect(hovered.zoom == "Zoom 120%",
                "zoom reads as a percentage of the default scale") &&
         expect(snapping.snap == "Snap wall",
                "the catalog tool reports its snap mode") &&
         expect(validRegion.regionMessage &&
                    validRegion.message == topography.region.statusMessage,
                "region editing promotes the region status message") &&
         expect(validRegion.cells == "4 candidate cells",
                "a valid region reports its candidate cells") &&
         expect(owningPreview.cells == "Changed 57 / 4096 cells",
                "an owned preview reports the replay cell counts") &&
         expect(owningPreview.phase ==
                    iggy3d_creative_app::
                        CreativeEditorWorldLayoutTerrainRegionPhase::Ready,
                "owning the preview classifies as ready");
}

bool inspectionStatusAndViewZoomStaySynchronized() {
  CreativeEditorWorldLayoutState state;
  state.source.stableKey = "inspection_layout";
  state.canvasPixelsPerCell = 56.0F;
  state.elevationPixelsPerCell = 14.0F;
  CreativeEditorWorldLayoutTopographyState topography;
  CreativeEditorTerrainGenerationState terrainGeneration;
  iggy3d_creative_app::CreativeEditorWorldLayoutCanvasHoverStatus hover;
  const auto compose = [&]() {
    return iggy3d_creative_app::composeCreativeEditorWorldLayoutStatusLine(
        state, topography, terrainGeneration, hover);
  };

  state.viewMode =
      iggy3d_creative_app::CreativeEditorWorldLayoutViewMode::Plan;
  const auto authoredPlan = compose();
  state.viewMode =
      iggy3d_creative_app::CreativeEditorWorldLayoutViewMode::Elevation;
  const auto authoredElevation = compose();

  state.preview.document = cr::CreativeDocument::create("Inspection Preview");
  const bool previewDocumentReady = state.preview.document.assignId(7301U);
  state.preview.accepted = true;
  state.previewVisible = true;
  state.previewLayoutRevision = state.revision;
  state.previewContentRevision = 1U;
  state.previewSource = state.source;
  const auto validPreview = compose();

  state.previewVisible = false;
  state.wallManipulation.active = true;
  state.wallManipulation.previewValid = false;
  state.wallManipulation.reasonCode = "wall target is blocked";
  const auto invalidPreview = compose();

  return expect(authoredPlan.zoom == "Zoom 200%" &&
                    authoredElevation.zoom == "Zoom 50%",
                "each drafting view reports its own preserved zoom") &&
         expect(authoredPlan.inspection.text ==
                        "Authored | inspection_layout" &&
                    authoredPlan.inspection.text ==
                        authoredElevation.inspection.text,
                "Plan and Elevation name the same authored source") &&
         expect(previewDocumentReady &&
                    validPreview.inspection.validity ==
                        iggy3d_creative_app::
                            CreativeEditorWorldLayoutPreviewValidity::Valid &&
                    validPreview.inspection.text ==
                        "Preview valid | inspection_layout",
                "an exact preview has one shared valid presentation") &&
         expect(invalidPreview.inspection.validity ==
                        iggy3d_creative_app::
                            CreativeEditorWorldLayoutPreviewValidity::Invalid &&
                    invalidPreview.inspection.text ==
                        "Preview invalid | inspection_layout",
                "an invalid interaction has one shared failure presentation");
}

bool blockoutDraftAndPatternChoicesArePinned() {
  const iggy3d_creative_app::CreativeEditorDesktopUiState desktopUi;
  const auto& draft = desktopUi.worldLayoutBlockoutDraft;
  const iggy3d_creative_app::CreativeEditorWorldLayoutRoomSettings defaults;
  const bool footprintRight = draft.shell.footprint.minimum.x == 0 &&
                              draft.shell.footprint.minimum.z == 0 &&
                              draft.shell.footprint.maximum.x == 8 &&
                              draft.shell.footprint.maximum.z == 8;
  const bool shellKeepsDefaults =
      draft.shell.floorTopLayer == defaults.floorTopLayer &&
      draft.floorToFloorCells == defaults.wallHeightCells &&
      draft.shell.wallThicknessCells == defaults.wallThicknessCells &&
      draft.shell.floorThicknessLayers == defaults.floorThicknessLayers &&
      draft.shell.roofThicknessLayers == defaults.roofThicknessLayers &&
      draft.shell.roofStyle == defaults.roofStyle &&
      draft.shell.roofRidgeAxis == defaults.roofRidgeAxis &&
      draft.shell.roofPitchDegrees == defaults.roofPitchDegrees &&
      draft.shell.roofOverhangCells == defaults.roofOverhangCells;
  const bool topologyKeepsDefaults =
      draft.connectRooms && draft.facade.includeEntrance &&
      draft.facade.entranceEdge == cr::CreativeWorldLayoutRoomEdge::South &&
      draft.facade.entranceOffsetCells == 0.0 &&
      draft.facade.includeExteriorWindows && draft.storeys.count == 1U &&
      draft.storeys.connectStoreys &&
      draft.storeys.connectorKind ==
          cr::CreativeWorldLayoutVerticalConnectorKind::Stair &&
      draft.storeys.preferredDirection ==
          cr::CreativeWorldLayoutVerticalDirection::PositiveZ;
  const bool architectureKeepsDefaults =
      draft.architecturalProfileKind ==
          cr::CreativeWorldLayoutArchitecturalProfileKind::Custom &&
      draft.ceilingThicknessLayers == 1U &&
      draft.exteriorWallMaterial ==
          cr::CreativeStructuralMaterial::Blockout &&
      draft.interiorWallMaterial == cr::CreativeStructuralMaterial::Blockout;
  const auto choices =
      iggy3d_creative_app::creativeEditorWorldLayoutBlockoutPatternChoices();
  using Pattern = cr::CreativeWorldLayoutBuildingBlockoutPattern;
  const bool choicesRight =
      choices.size() == 4U &&
      std::string_view(choices[0].label) == "1 room" &&
      choices[0].pattern == Pattern::SingleRoom &&
      std::string_view(choices[1].label) == "Split X" &&
      choices[1].pattern == Pattern::SplitX &&
      std::string_view(choices[2].label) == "Split Z" &&
      choices[2].pattern == Pattern::SplitZ &&
      std::string_view(choices[3].label) == "2 x 2" &&
      choices[3].pattern == Pattern::Grid2x2;
  bool patternsUnique = true;
  for (std::size_t index = 0U; index < choices.size(); ++index) {
    for (std::size_t other = index + 1U; other < choices.size(); ++other) {
      if (choices[index].pattern == choices[other].pattern) {
        patternsUnique = false;
      }
    }
  }
  const cr::CreativeWorldLayoutBuildingBlockoutStoreySettings storeyDefaults;
  const bool storeysKeepDefaults =
      draft.storeys.count == storeyDefaults.count &&
      draft.storeys.connectStoreys == storeyDefaults.connectStoreys &&
      draft.storeys.connectorKind == storeyDefaults.connectorKind &&
      draft.storeys.preferredDirection == storeyDefaults.preferredDirection;
  return expect(footprintRight,
                "the blockout draft starts at the 0,0 to 8,8 footprint") &&
         expect(draft.pattern == Pattern::SingleRoom,
                "the blockout draft starts as a single room") &&
         expect(shellKeepsDefaults,
                "non-footprint shell values keep the room-settings defaults") &&
         expect(architectureKeepsDefaults,
                "blockout architecture starts custom with blockout walls") &&
         expect(storeysKeepDefaults,
                "storey settings keep the backend defaults: one storey, "
                "stairs on, stair kind, positive-Z preference") &&
         expect(topologyKeepsDefaults,
                "blockout drafts default to one connected storey, stairs, "
                "and a south facade") &&
         expect(choicesRight,
                "the four pattern choices map one-to-one onto the planner "
                "enum") &&
         expect(patternsUnique, "pattern choices never repeat an enum value");
}

bool blockoutArchitecturalProfilesResolveAgainstTheDocumentGrid() {
  iggy3d_creative_app::CreativeEditorWorldLayoutBuildingBlockoutSettings
      settings;
  cr::CreativeGridSettings halfMeterGrid;
  halfMeterGrid.cellSizeMeters = 0.5;
  const bool residential = iggy3d_creative_app::
      applyCreativeEditorWorldLayoutBlockoutArchitecturalProfile(
          settings, halfMeterGrid,
          cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  const bool residentialRight =
      residential && settings.floorToFloorCells == 6U &&
      settings.shell.floorThicknessLayers == 4U &&
      settings.ceilingThicknessLayers == 1U &&
      settings.shell.roofThicknessLayers == 1U &&
      settings.architecturalProfileKind ==
          cr::CreativeWorldLayoutArchitecturalProfileKind::Residential;
  const bool grand = iggy3d_creative_app::
      applyCreativeEditorWorldLayoutBlockoutArchitecturalProfile(
          settings, halfMeterGrid,
          cr::CreativeWorldLayoutArchitecturalProfileKind::Grand);
  const bool grandRight =
      grand && settings.floorToFloorCells == 10U &&
      settings.shell.floorThicknessLayers == 6U &&
      settings.ceilingThicknessLayers == 1U &&
      settings.shell.roofThicknessLayers == 1U &&
      settings.architecturalProfileKind ==
          cr::CreativeWorldLayoutArchitecturalProfileKind::Grand;
  const auto unchanged = settings;
  cr::CreativeGridSettings unrepresentableGrid;
  unrepresentableGrid.cellSizeMeters = 0.7;
  const bool rejected = !iggy3d_creative_app::
      applyCreativeEditorWorldLayoutBlockoutArchitecturalProfile(
          settings, unrepresentableGrid,
          cr::CreativeWorldLayoutArchitecturalProfileKind::Residential);
  return expect(residentialRight,
                "residential profile resolves to exact half-meter cells") &&
         expect(grandRight,
                "grand profile resolves to exact half-meter cells") &&
         expect(rejected && settings.floorToFloorCells ==
                                unchanged.floorToFloorCells &&
                    settings.architecturalProfileKind ==
                        unchanged.architecturalProfileKind,
                "unrepresentable profiles reject without mutating the draft");
}

bool blockoutEditDraftSyncFollowsTheSource() {
  iggy3d_creative_app::CreativeEditorDesktopBlockoutEditDraft draft;
  CreativeEditorWorldLayoutState state;
  const auto inSync = [&]() {
    return iggy3d_creative_app::creativeEditorWorldLayoutBlockoutEditInSync(
        draft, state);
  };
  const bool defaultsInert =
      !draft.active &&
      draft.buildingIndex == cr::kInvalidCreativeWorldLayoutIndex &&
      draft.sourceRevision == 0U;
  const bool inactiveOut = !inSync();
  state.source.buildings.emplace_back();
  state.revision = 41U;
  draft.active = true;
  draft.buildingIndex = 0U;
  draft.sourceRevision = 41U;
  const bool freshIn = inSync();
  state.revision = 42U;
  const bool staleOut = !inSync();
  draft.sourceRevision = 42U;
  draft.buildingIndex = 5U;
  const bool missingBuildingOut = !inSync();
  return expect(defaultsInert, "the edit draft defaults are inert") &&
         expect(inactiveOut, "an inactive draft is never in sync") &&
         expect(freshIn,
                "a draft read at the current revision is in sync") &&
         expect(staleOut, "advancing the source revision marks it stale") &&
         expect(missingBuildingOut,
                "a vanished building index is never in sync");
}

}  // namespace

int main() {
  bool ok = true;
  ok = descriptorsOwnProductTruthAndReleaseVisibility() && ok;
  ok = volumeSettingsProfilesAreDescriptorOwned() && ok;
  ok = evaluationTracksActiveWorldLayoutTool() && ok;
  ok = terrainSessionLocksFollowTheWorkflow() && ok;
  ok = activeDescriptorFollowsTheSession() && ok;
  ok = worldLayoutCanvasPolicyComesFromDescriptors() && ok;
  ok = optionsExposeTheRightFieldsPerOperation() && ok;
  ok = optionBindingsReadWriteAndClamp() && ok;
  ok = actionRulesMatchTheBuildWindow() && ok;
  ok = statusLineComposesFromTheRightSources() && ok;
  ok = inspectionStatusAndViewZoomStaySynchronized() && ok;
  ok = blockoutDraftAndPatternChoicesArePinned() && ok;
  ok = blockoutArchitecturalProfilesResolveAgainstTheDocumentGrid() && ok;
  ok = blockoutEditDraftSyncFollowsTheSource() && ok;
  return ok ? 0 : 1;
}
