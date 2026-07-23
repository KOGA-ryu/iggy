#include "EditorToolDescriptor.hpp"

#include <array>
#include <cstddef>

#include "EditorWorldLayout.hpp"

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

using Descriptor = CreativeEditorToolDescriptor;
using Id = CreativeEditorToolId;
using Category = CreativeEditorToolCategory;
using Maturity = CreativeEditorToolMaturity;
using Exposure = CreativeEditorToolExposure;
using Lifecycle = CreativeEditorToolLifecycle;
using Held = cr::CreativeHeldItemKind;
using WorldTool = CreativeEditorWorldLayoutTool;
using WorldActivation = CreativeEditorWorldLayoutToolActivation;
using WorldInput = CreativeEditorWorldLayoutInputProfile;
using WorldPreview = CreativeEditorWorldLayoutPreviewProfile;
using WorldFill = CreativeEditorWorldLayoutPreviewFill;
using OptionSpec = CreativeEditorToolOptionSpec;
using ActionSpec = CreativeEditorToolActionSpec;
using Choice = CreativeEditorToolOptionChoice;

constexpr Choice kTerrainOperationChoices[] = {
    {CreativeEditorToolGlyph::RegionFlatten, "Flatten",
     static_cast<std::uint8_t>(cr::CreativeTerrainRegionMode::Flatten)},
    {CreativeEditorToolGlyph::RegionRaise, "Raise",
     static_cast<std::uint8_t>(cr::CreativeTerrainRegionMode::Raise)},
    {CreativeEditorToolGlyph::RegionLower, "Lower",
     static_cast<std::uint8_t>(cr::CreativeTerrainRegionMode::Lower)},
    {CreativeEditorToolGlyph::RegionSmooth, "Smooth",
     static_cast<std::uint8_t>(cr::CreativeTerrainRegionMode::Smooth)},
    {CreativeEditorToolGlyph::RegionNoise, "Noise",
     static_cast<std::uint8_t>(cr::CreativeTerrainRegionMode::Noise)},
    {CreativeEditorToolGlyph::BadgeRemove, "Erase",
     static_cast<std::uint8_t>(cr::CreativeTerrainRegionMode::Erase)},
};

constexpr Choice kTerrainMaskChoices[] = {
    {CreativeEditorToolGlyph::MaskRectangle, "Rectangle mask",
     static_cast<std::uint8_t>(cr::CreativeTerrainCompositionMask::Rectangle)},
    {CreativeEditorToolGlyph::MaskEllipse, "Ellipse mask",
     static_cast<std::uint8_t>(cr::CreativeTerrainCompositionMask::Ellipse)},
};

constexpr OptionSpec kTerrainRegionOptions[] = {
    {CreativeEditorToolOptionBinding::TerrainOperation,
     CreativeEditorToolOptionWidget::GlyphChoice, "Operation", "", 0.0, 0.0,
     kTerrainOperationChoices},
    {CreativeEditorToolOptionBinding::TerrainMask,
     CreativeEditorToolOptionWidget::GlyphChoice, "Shape", "", 0.0, 0.0,
     kTerrainMaskChoices},
    {CreativeEditorToolOptionBinding::TerrainAmount,
     CreativeEditorToolOptionWidget::IntDrag, "Amount", "Amount", 1.0,
     static_cast<double>(cr::kCreativeTerrainRegionMaximumAmountCells), {}},
    {CreativeEditorToolOptionBinding::TerrainTargetHeight,
     CreativeEditorToolOptionWidget::IntDrag, "", "",
     static_cast<double>(cr::kCreativeTerrainMinimumHeightCells),
     static_cast<double>(cr::kCreativeTerrainMaximumHeightCells), {}},
    {CreativeEditorToolOptionBinding::TerrainNoiseRelief,
     CreativeEditorToolOptionWidget::IntDrag, "Relief", "Relief", 0.0,
     static_cast<double>(cr::kCreativeTerrainMaximumHeightCells), {}},
    {CreativeEditorToolOptionBinding::TerrainNoiseScale,
     CreativeEditorToolOptionWidget::DoubleDrag, "Scale", "Scale",
     cr::kCreativeTerrainGeneratorMinimumHorizontalScaleCells,
     cr::kCreativeTerrainGeneratorMaximumHorizontalScaleCells, {}},
    {CreativeEditorToolOptionBinding::TerrainSeed,
     CreativeEditorToolOptionWidget::SeedButton, "New seed", "", 0.0, 0.0,
     {}},
    {CreativeEditorToolOptionBinding::TerrainFeather,
     CreativeEditorToolOptionWidget::IntDrag, "Feather", "Feather", 0.0,
     static_cast<double>(cr::kCreativeTerrainCompositionMaximumFeatherCells),
     {}},
};

constexpr ActionSpec kTerrainRegionActions[] = {
    {CreativeDesktopCommandId::WorldLayoutTerrainRegionPreview,
     CreativeEditorToolGlyph::ActionPreview, "Preview",
     CreativeEditorToolActionRule::IdleValidRegion, true},
    {CreativeDesktopCommandId::WorldLayoutTerrainRegionApply,
     CreativeEditorToolGlyph::ActionApply, "Apply region",
     CreativeEditorToolActionRule::OwnedPreview, false},
    {CreativeDesktopCommandId::WorldLayoutTerrainRegionCancel,
     CreativeEditorToolGlyph::ActionCancel, "Cancel",
     CreativeEditorToolActionRule::RegionActivity, false},
};

constexpr std::uint8_t kTerrainRegionLocks =
    kCreativeEditorToolLockTemplatePlacement |
    kCreativeEditorToolLockBuildingTransform |
    kCreativeEditorToolLockForeignTerrainPreview |
    kCreativeEditorToolLockExactLayoutPreview;

constexpr std::size_t descriptorIndex(Id id) noexcept {
  return static_cast<std::size_t>(id);
}

consteval auto makeToolDescriptors() {
  std::array<Descriptor, static_cast<std::size_t>(Id::Count)> rows{};

  const auto define = [&rows](Id id, Category category, Maturity maturity,
                              Exposure exposure, Lifecycle lifecycle,
                              CreativeEditorToolGlyph glyph,
                              std::string_view name,
                              std::string_view aliases,
                              std::string_view purpose) -> Descriptor& {
    Descriptor& row = rows[descriptorIndex(id)];
    row.id = id;
    row.category = category;
    row.maturity = maturity;
    row.exposure = exposure;
    row.lifecycle = lifecycle;
    row.glyph = glyph;
    row.name = name;
    row.aliases = aliases;
    row.purpose = purpose;
    return row;
  };
  const auto bindHeld = [](Descriptor& row, Held held,
                           CreativeEditorActionHintProfile hints) {
    row.heldItemKind = held;
    row.catalogTool = true;
    row.quickEditProfile = CreativeEditorQuickEditProfile::Generic;
    row.actionHintProfile = hints;
  };
  const auto bindWorld = [](Descriptor& row, WorldTool tool) {
    row.worldLayoutActivation = WorldActivation::Tool;
    row.worldLayoutTool = tool;
  };

  Descriptor& select =
      define(Id::Select, Category::Workspace, Maturity::M2StableRecipe,
             Exposure::Workspace, Lifecycle::Observational,
             CreativeEditorToolGlyph::ParentSelect, "Select",
             "inspect pick object hierarchy", "Inspect and select authored output.");
  select.controllerHint = "X selects. Circle clears the selection.";
  bindHeld(select, Held::ObjectSelect,
           CreativeEditorActionHintProfile::ObjectSelect);
  bindWorld(select, WorldTool::Select);
  select.worldLayoutInputProfile = WorldInput::Point;
  select.worldLayoutPreviewProfile = WorldPreview::None;
  select.worldLayoutPreviewFill = WorldFill::None;
  select.defaultWheelEligible = true;

  Descriptor& transform =
      define(Id::Transform, Category::Workspace, Maturity::M2StableRecipe,
             Exposure::Workspace, Lifecycle::Destructive,
             CreativeEditorToolGlyph::ParentObject, "Transform",
             "move translate rotate scale mirror", "Move and reshape selected output.");
  transform.controllerHint = "X confirms. Circle cancels.";
  bindHeld(transform, Held::ObjectMove,
           CreativeEditorActionHintProfile::ObjectMove);
  transform.commandProfile = CreativeEditorToolCommandProfile::ObjectMove;
  transform.quickEditProfile = CreativeEditorQuickEditProfile::None;
  transform.displayProfile = CreativeEditorToolDisplayProfile::Selection;
  transform.defaultWheelEligible = true;

  Descriptor& group =
      define(Id::Group, Category::Workspace, Maturity::M2StableRecipe,
             Exposure::Experimental, Lifecycle::Destructive,
             CreativeEditorToolGlyph::ParentObject, "Group",
             "ungroup combine assembly parent selection",
             "Create and revise a reusable selection group.");
  group.controllerHint = "X applies the group operation. Circle cancels.";
  bindHeld(group, Held::ObjectGroup,
           CreativeEditorActionHintProfile::ObjectGroup);
  group.commandProfile = CreativeEditorToolCommandProfile::ObjectGroup;

  Descriptor& material =
      define(Id::MaterialPlacement, Category::Placement,
             Maturity::M2StableRecipe, Exposure::Experimental,
             Lifecycle::Parametric, CreativeEditorToolGlyph::AssetBox,
             "Material Placement", "place block material object asset",
             "Place one material or catalog object at an exact target.");
  material.controllerHint = "X places. Circle removes.";
  material.heldItemKind = Held::Material;
  material.optionFilterProfile =
      CreativeEditorToolOptionFilterProfile::MaterialPlacement;
  material.quickEditProfile = CreativeEditorQuickEditProfile::Generic;
  material.actionHintProfile = CreativeEditorActionHintProfile::Material;
  material.displayProfile = CreativeEditorToolDisplayProfile::Material;

  Descriptor& assetPlacement =
      define(Id::CatalogAssetPlacement, Category::Placement,
             Maturity::M2StableRecipe, Exposure::Experimental,
             Lifecycle::Parametric, CreativeEditorToolGlyph::AssetBox,
             "Catalog Asset Placement", "asset prefab mesh prop place",
             "Place a catalog asset with explicit host and snapping.");
  assetPlacement.controllerHint = "X places. Circle cancels.";
  bindWorld(assetPlacement, WorldTool::CatalogAsset);
  assetPlacement.worldLayoutInputProfile = WorldInput::Point;
  assetPlacement.worldLayoutPreviewProfile = WorldPreview::CatalogAsset;
  assetPlacement.worldLayoutPreviewFill = WorldFill::None;

  Descriptor& brush =
      define(Id::MaterialBrush, Category::Volume, Maturity::M1Prototype,
             Exposure::Experimental, Lifecycle::Destructive,
             CreativeEditorToolGlyph::ParentObject, "Material Brush",
             "brush paint sculpt material sphere cube cylinder",
             "Paint material over a bounded 3D brush footprint.");
  brush.controllerHint = "Hold X to paint. Circle removes.";
  bindHeld(brush, Held::MaterialBrush,
           CreativeEditorActionHintProfile::MaterialBrush);
  brush.commandProfile = CreativeEditorToolCommandProfile::MaterialBrush;

  Descriptor& volumeSelect =
      define(Id::VolumeSelect, Category::Volume, Maturity::M1Prototype,
             Exposure::Experimental, Lifecycle::Observational,
             CreativeEditorToolGlyph::MaskRectangle, "Volume Select",
             "region volume corner selection", "Select an exact bounded 3D region.");
  volumeSelect.controllerHint = "X advances corners. Circle cancels.";
  bindHeld(volumeSelect, Held::VolumeSelect,
           CreativeEditorActionHintProfile::VolumeSelect);

  const auto defineVolume = [&](Id id, Held held, std::string_view name,
                                std::string_view aliases,
                                CreativeEditorActionHintProfile hints,
                                CreativeEditorVolumeSettingsProfile settings) {
    Descriptor& row = define(id, Category::Volume, Maturity::M1Prototype,
                             Exposure::Experimental, Lifecycle::Destructive,
                             CreativeEditorToolGlyph::ParentObject, name,
                             aliases, "Apply one bounded volume operation.");
    row.controllerHint = "X advances or applies. Circle cancels.";
    bindHeld(row, held, hints);
    row.volumeSettingsProfile = settings;
  };
  defineVolume(Id::VolumeFill, Held::VolumeFill, "Volume Fill",
               "fill solid region box ellipse cylinder",
               CreativeEditorActionHintProfile::DirectShapeVolume,
               CreativeEditorVolumeSettingsProfile::Fill);
  defineVolume(Id::VolumeHollow, Held::VolumeHollow, "Volume Hollow",
               "hollow shell walls region",
               CreativeEditorActionHintProfile::DirectShapeVolume,
               CreativeEditorVolumeSettingsProfile::Hollow);
  defineVolume(Id::VolumeReplace, Held::VolumeReplace, "Volume Replace",
               "replace swap material region",
               CreativeEditorActionHintProfile::VolumeOperation,
               CreativeEditorVolumeSettingsProfile::Replace);
  defineVolume(Id::VolumeErase, Held::VolumeErase, "Volume Erase",
               "erase delete remove region",
               CreativeEditorActionHintProfile::VolumeOperation,
               CreativeEditorVolumeSettingsProfile::Erase);
  defineVolume(Id::VolumeClone, Held::VolumeClone, "Volume Clone",
               "clone copy duplicate region",
               CreativeEditorActionHintProfile::VolumeOperation,
               CreativeEditorVolumeSettingsProfile::Clone);

  Descriptor& connectedFill =
      define(Id::ConnectedFill, Category::Volume, Maturity::M1Prototype,
             Exposure::Experimental, Lifecycle::Destructive,
             CreativeEditorToolGlyph::ParentObject, "Connected Fill",
             "flood paint bucket connected material region",
             "Replace one bounded connected material region.");
  connectedFill.controllerHint = "X fills. Circle erases.";
  bindHeld(connectedFill, Held::ConnectedFill,
           CreativeEditorActionHintProfile::ConnectedFill);

  Descriptor& extrude =
      define(Id::SurfaceExtrude, Category::Volume, Maturity::M1Prototype,
             Exposure::Experimental, Lifecycle::Destructive,
             CreativeEditorToolGlyph::ParentObject, "Surface Extrude",
             "extrude inset pull push face surface",
             "Push or inset one connected surface.");
  extrude.controllerHint = "X extrudes. Circle insets.";
  bindHeld(extrude, Held::SurfaceExtrude,
           CreativeEditorActionHintProfile::SurfaceExtrude);

  Descriptor& array =
      define(Id::LinearArray, Category::Volume, Maturity::M1Prototype,
             Exposure::Experimental, Lifecycle::Parametric,
             CreativeEditorToolGlyph::ParentObject, "Array",
             "linear radial ring pattern repeat duplicate",
             "Repeat selected output with deterministic spacing.");
  array.controllerHint = "X applies. Circle cancels.";
  bindHeld(array, Held::LinearArray,
           CreativeEditorActionHintProfile::LinearArray);

  const auto defineWorld = [&](Id id, Category category, Maturity maturity,
                               WorldTool tool, WorldInput input,
                               WorldPreview preview, WorldFill fill,
                               std::string_view interactionPrompt,
                               CreativeEditorToolGlyph glyph,
                               std::string_view name,
                               std::string_view aliases,
                               std::string_view purpose) -> Descriptor& {
    Descriptor& row = define(id, category, maturity, Exposure::Experimental,
                             Lifecycle::Parametric, glyph, name, aliases,
                             purpose);
    row.controllerHint = "X confirms. Circle cancels.";
    bindWorld(row, tool);
    row.worldLayoutInputProfile = input;
    row.worldLayoutPreviewProfile = preview;
    row.worldLayoutPreviewFill = fill;
    row.worldLayoutInteractionPrompt = interactionPrompt;
    return row;
  };

  defineWorld(Id::BuildingShell, Category::Building,
              Maturity::M2StableRecipe, WorldTool::BuildingShell,
              WorldInput::Drag, WorldPreview::Rectangle, WorldFill::Structure,
              "drag building shell to its opposite corner",
              CreativeEditorToolGlyph::BuildingShell, "Building Shell",
              "building footprint exterior shell",
              "Create and revise a building envelope.");
  Descriptor& room =
      defineWorld(Id::Room, Category::Building, Maturity::M1Prototype,
                  WorldTool::Room, WorldInput::Drag, WorldPreview::Rectangle,
                  WorldFill::Structure,
                  "drag room to its opposite corner",
                  CreativeEditorToolGlyph::AddRoom, "Room",
                  "room floor plan interior", "Create and revise one room.");
  bindHeld(room, Held::BuildingRoom,
           CreativeEditorActionHintProfile::BuildingRoom);
  room.keyboardQuickEditHints = true;
  defineWorld(Id::Floor, Category::Building, Maturity::M1Prototype,
              WorldTool::Floor, WorldInput::Drag, WorldPreview::Rectangle,
              WorldFill::None, "drag floor to its opposite corner",
              CreativeEditorToolGlyph::Floor, "Floor",
              "slab floor ceiling level", "Create a floor or slab region.");
  defineWorld(Id::Partition, Category::Building, Maturity::M1Prototype,
              WorldTool::Wall, WorldInput::Drag, WorldPreview::AxisLine,
              WorldFill::None, "drag partition to its end",
              CreativeEditorToolGlyph::Partition,
              "Partition", "wall interior divider",
              "Create and revise an interior wall.");
  defineWorld(Id::Door, Category::Building, Maturity::M2StableRecipe,
              WorldTool::Door, WorldInput::Point, WorldPreview::DoorOpening,
              WorldFill::None, {}, CreativeEditorToolGlyph::Door, "Door",
              "opening portal entry", "Insert a hosted door opening.");
  defineWorld(Id::Window, Category::Building, Maturity::M2StableRecipe,
              WorldTool::Window, WorldInput::Point,
              WorldPreview::WindowOpening, WorldFill::None, {},
              CreativeEditorToolGlyph::Window, "Window",
              "opening glazing casement", "Insert a hosted window opening.");
  defineWorld(Id::Stair, Category::Building, Maturity::M2StableRecipe,
              WorldTool::Stair, WorldInput::Drag, WorldPreview::Rectangle,
              WorldFill::VerticalConnector,
              "Stair drag starts at the low end and finishes at the high end",
              CreativeEditorToolGlyph::Stair, "Stair",
              "stairs vertical connector", "Connect levels with traversable stairs.");
  defineWorld(Id::Ramp, Category::Building, Maturity::M2StableRecipe,
              WorldTool::Ramp, WorldInput::Drag, WorldPreview::Rectangle,
              WorldFill::Ramp,
              "Ramp drag starts at the low end and finishes at the high end",
              CreativeEditorToolGlyph::Ramp, "Ramp",
              "slope vertical connector", "Connect levels with a traversable ramp.");

  defineWorld(Id::Plateau, Category::Terrain, Maturity::M1Prototype,
              WorldTool::Plateau, WorldInput::Point, WorldPreview::None,
              WorldFill::None, {}, CreativeEditorToolGlyph::Plateau,
              "Plateau", "terrace cliff flat terrain",
              "Create a bounded elevated terrain platform.");
  defineWorld(Id::Road, Category::Terrain, Maturity::M1Prototype,
              WorldTool::Road, WorldInput::Polyline, WorldPreview::TerrainPath,
              WorldFill::Road, {}, CreativeEditorToolGlyph::Road, "Road",
              "path route grade", "Create a terrain-conforming route.");
  defineWorld(Id::Ditch, Category::Terrain, Maturity::M1Prototype,
              WorldTool::Ditch, WorldInput::Polyline,
              WorldPreview::TerrainPath, WorldFill::Ditch, {},
              CreativeEditorToolGlyph::Ditch, "Ditch",
              "trench riverbed watercourse", "Cut a bounded terrain channel.");
  defineWorld(Id::Bridge, Category::Object, Maturity::M1Prototype,
              WorldTool::Bridge, WorldInput::Drag, WorldPreview::Rectangle,
              WorldFill::Structure,
              "drag bridge to its opposite corner",
              CreativeEditorToolGlyph::Bridge, "Bridge",
              "crossing span road", "Create a traversable crossing span.");

  const auto defineTerrainHeld = [&](Id id, Held held, Maturity maturity,
                                     std::string_view name,
                                     std::string_view aliases,
                                     CreativeEditorQuickEditProfile quickEdit,
                                     CreativeEditorActionHintProfile hints) {
    Descriptor& row =
        define(id, Category::Terrain, maturity, Exposure::Experimental,
               Lifecycle::Destructive, CreativeEditorToolGlyph::ParentTerrain,
               name, aliases, "Edit bounded terrain source data.");
    row.controllerHint = "X applies or advances. Circle cancels or removes.";
    bindHeld(row, held, hints);
    row.quickEditProfile = quickEdit;
    row.keyboardQuickEditHints = true;
    return &row;
  };
  defineTerrainHeld(Id::TerrainControl, Held::TerrainControl,
                    Maturity::M1Prototype, "Terrain Rod",
                    "terrain control height elevation radius",
                    CreativeEditorQuickEditProfile::TerrainControl,
                    CreativeEditorActionHintProfile::TerrainControl);
  defineTerrainHeld(Id::TerrainPaint, Held::TerrainPaint,
                    Maturity::M1Prototype, "Terrain Paint",
                    "terrain surface material grass dirt stone",
                    CreativeEditorQuickEditProfile::Generic,
                    CreativeEditorActionHintProfile::TerrainPaint);
  Descriptor* terrainGrade =
      defineTerrainHeld(Id::TerrainGrade, Held::TerrainGrade,
                        Maturity::M2StableRecipe, "Terrain Grade",
                        "grade ramp slope road pad bridge approach",
                        CreativeEditorQuickEditProfile::TerrainGrade,
                        CreativeEditorActionHintProfile::TerrainGrade);
  terrainGrade->lifecycle = Lifecycle::Parametric;
  terrainGrade->purpose =
      "Author or reopen an exact graded terrain corridor operation.";
  defineTerrainHeld(Id::TerrainSculpt, Held::TerrainSculpt,
                    Maturity::M2StableRecipe, "Terrain Sculpt",
                    "raise lower flatten smooth brush",
                    CreativeEditorQuickEditProfile::TerrainSculpt,
                    CreativeEditorActionHintProfile::TerrainSculpt);
  Descriptor* terrainProfile =
      defineTerrainHeld(Id::TerrainProfile, Held::TerrainProfile,
                        Maturity::M2StableRecipe, "Terrain Profile",
                        "hill basin ring crater ridge wave ripple",
                        CreativeEditorQuickEditProfile::TerrainProfile,
                        CreativeEditorActionHintProfile::TerrainProfile);
  terrainProfile->lifecycle = Lifecycle::Parametric;
  terrainProfile->purpose =
      "Author an editable mathematical elevation profile operation.";
  defineTerrainHeld(Id::TerrainPath, Held::TerrainPath,
                    Maturity::M1Prototype, "Terrain Path",
                    "road river ridge trench route spline",
                    CreativeEditorQuickEditProfile::TerrainPath,
                    CreativeEditorActionHintProfile::TerrainPath);
  Descriptor& terrainRegion =
      define(Id::TerrainRegion, Category::Terrain, Maturity::M1Prototype,
             Exposure::Experimental, Lifecycle::Destructive,
             CreativeEditorToolGlyph::MaskRectangle, "Terrain Region",
             "area raise lower flatten smooth noise mask",
             "Apply one previewed operation to a bounded terrain region.");
  terrainRegion.controllerHint = "X applies or advances. Circle cancels.";
  bindHeld(terrainRegion, Held::TerrainRegion,
           CreativeEditorActionHintProfile::TerrainRegion);
  terrainRegion.quickEditProfile = CreativeEditorQuickEditProfile::TerrainRegion;
  terrainRegion.keyboardQuickEditHints = true;
  terrainRegion.worldLayoutActivation = WorldActivation::TerrainRegionSession;
  terrainRegion.locks = kTerrainRegionLocks;
  terrainRegion.options = kTerrainRegionOptions;
  terrainRegion.actions = kTerrainRegionActions;

  Descriptor& playerSpawn =
      defineWorld(Id::PlayerSpawn, Category::Gameplay, Maturity::M0NameOnly,
                  WorldTool::PlayerSpawn, WorldInput::Point,
                  WorldPreview::None, WorldFill::None, {},
                  CreativeEditorToolGlyph::PlayerSpawn, "Player Spawn",
                  "player start spawn point", "Place a validated player start.");
  playerSpawn.lifecycle = Lifecycle::Parametric;
  Descriptor& npcSpawn =
      defineWorld(Id::NpcSpawn, Category::Gameplay, Maturity::M0NameOnly,
                  WorldTool::NpcSpawn, WorldInput::Point, WorldPreview::None,
                  WorldFill::None, {}, CreativeEditorToolGlyph::NpcSpawn,
                  "NPC Spawn", "npc guard patrol spawn",
                  "Place and configure an NPC spawn.");
  npcSpawn.lifecycle = Lifecycle::Parametric;

  Descriptor& logic =
      define(Id::LogicLink, Category::Logic, Maturity::M1Prototype,
             Exposure::Experimental, Lifecycle::Parametric,
             CreativeEditorToolGlyph::ParentGameplay, "Logic Link",
             "connect wire switch door trigger",
             "Connect compatible authored logic endpoints.");
  logic.controllerHint = "X advances endpoints. Circle cancels.";
  bindHeld(logic, Held::LogicLink,
           CreativeEditorActionHintProfile::LogicLink);
  logic.quickEditProfile = CreativeEditorQuickEditProfile::LogicLink;
  logic.keyboardQuickEditHints = true;

  Descriptor& measure =
      define(Id::Measure, Category::Workspace, Maturity::M2StableRecipe,
             Exposure::Experimental, Lifecycle::Observational,
             CreativeEditorToolGlyph::Dimensions, "Measure",
             "distance height slope perimeter area dimensions",
             "Inspect exact world dimensions without changing the document.");
  measure.controllerHint = "X adds a point. Circle cancels.";
  bindHeld(measure, Held::Measure,
           CreativeEditorActionHintProfile::Measurement);
  measure.keyboardQuickEditHints = true;

  return rows;
}

constexpr auto kToolDescriptors = makeToolDescriptors();
constexpr Descriptor kInvalidDescriptor{};

consteval auto makeWorldToolDescriptorIds() {
  std::array<Id, static_cast<std::size_t>(WorldTool::Count)> ids{};
  ids.fill(Id::Count);
  for (const Descriptor& descriptor : kToolDescriptors) {
    if (descriptor.worldLayoutActivation != WorldActivation::Tool ||
        descriptor.worldLayoutTool >= WorldTool::Count) {
      continue;
    }
    const std::size_t index =
        static_cast<std::size_t>(descriptor.worldLayoutTool);
    if (ids[index] != Id::Count) {
      throw "duplicate world-layout tool descriptor";
    }
    ids[index] = descriptor.id;
  }
  for (Id id : ids) {
    if (id == Id::Count) {
      throw "missing world-layout tool descriptor";
    }
  }
  return ids;
}

constexpr auto kWorldToolDescriptorIds = makeWorldToolDescriptorIds();

constexpr std::string_view maturityLabel(Maturity maturity) noexcept {
  switch (maturity) {
    case Maturity::M0NameOnly: return "M0";
    case Maturity::M1Prototype: return "M1";
    case Maturity::M2StableRecipe: return "M2";
    case Maturity::M3Product: return "M3";
    case Maturity::M4Integrated: return "M4";
    case Maturity::Count: break;
  }
  return "INVALID";
}

constexpr std::string_view lifecycleLabel(Lifecycle lifecycle) noexcept {
  switch (lifecycle) {
    case Lifecycle::Observational: return "Observational";
    case Lifecycle::Parametric: return "Parametric";
    case Lifecycle::Destructive: return "Destructive";
    case Lifecycle::Count: break;
  }
  return "Invalid";
}

consteval auto makeCatalogToolSpecs() {
  constexpr std::size_t kCatalogToolCount =
      cr::kCreativeHeldItemKindCount - 1U;
  std::array<cr::CreativeCatalogToolSpec, kCatalogToolCount> specs{};
  std::size_t count = 0U;
  for (const Descriptor& descriptor : kToolDescriptors) {
    if (!descriptor.catalogTool) {
      continue;
    }
    specs[count++] = {
        descriptor.heldItemKind,
        descriptor.name,
        descriptor.aliases,
        descriptor.exposure == Exposure::Experimental,
        descriptor.defaultWheelEligible,
        descriptor.purpose,
        maturityLabel(descriptor.maturity),
        lifecycleLabel(descriptor.lifecycle),
        descriptor.controllerHint,
    };
  }
  if (count != specs.size()) {
    throw "catalog tool descriptor count mismatch";
  }
  return specs;
}

constexpr auto kCatalogToolSpecs = makeCatalogToolSpecs();

}  // namespace

std::span<const CreativeEditorToolDescriptor>
creativeEditorToolDescriptors() noexcept {
  return kToolDescriptors;
}

const CreativeEditorToolDescriptor& describeCreativeEditorTool(
    CreativeEditorToolId id) noexcept {
  const std::size_t index = descriptorIndex(id);
  return index < kToolDescriptors.size() ? kToolDescriptors[index]
                                         : kInvalidDescriptor;
}

const CreativeEditorToolDescriptor& describeCreativeEditorHeldItemTool(
    cr::CreativeHeldItemKind kind) noexcept {
  if (kind >= Held::Count) {
    return kInvalidDescriptor;
  }
  const auto found = std::find_if(
      kToolDescriptors.begin(), kToolDescriptors.end(),
      [kind](const Descriptor& descriptor) {
        return descriptor.heldItemKind == kind;
      });
  return found == kToolDescriptors.end() ? kInvalidDescriptor : *found;
}

const CreativeEditorToolDescriptor& describeCreativeEditorWorldLayoutTool(
    CreativeEditorWorldLayoutTool tool) noexcept {
  const std::size_t index = static_cast<std::size_t>(tool);
  return index < kWorldToolDescriptorIds.size()
             ? describeCreativeEditorTool(kWorldToolDescriptorIds[index])
             : kInvalidDescriptor;
}

const CreativeEditorToolDescriptor& activeCreativeEditorWorldLayoutToolDescriptor(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography) noexcept {
  if (topography.region.editingEnabled) {
    return describeCreativeEditorTool(Id::TerrainRegion);
  }
  if (state.buildingTemplatePlacement.active) {
    return describeCreativeEditorTool(Id::Select);
  }
  const Descriptor& descriptor =
      describeCreativeEditorWorldLayoutTool(state.tool);
  return descriptor.id == Id::Count ? describeCreativeEditorTool(Id::Select)
                                    : descriptor;
}

CreativeEditorToolStatus evaluateCreativeEditorTool(
    const CreativeEditorToolDescriptor& descriptor,
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutTopographyState& topography,
    const CreativeEditorTerrainGenerationState& terrainGeneration) noexcept {
  CreativeEditorToolStatus status;
  if ((descriptor.locks & kCreativeEditorToolLockTemplatePlacement) != 0U &&
      state.buildingTemplatePlacement.active) {
    status.unavailable = true;
  }
  if ((descriptor.locks & kCreativeEditorToolLockBuildingTransform) != 0U &&
      state.buildingTransform.active) {
    status.unavailable = true;
  }
  if ((descriptor.locks & kCreativeEditorToolLockForeignTerrainPreview) != 0U &&
      terrainGeneration.previewActive && !topography.region.ownsPreview) {
    status.unavailable = true;
  }
  if ((descriptor.locks & kCreativeEditorToolLockExactLayoutPreview) != 0U &&
      creativeEditorWorldLayoutPreviewActive(state)) {
    status.unavailable = true;
  }
  switch (descriptor.worldLayoutActivation) {
    case WorldActivation::None:
      break;
    case WorldActivation::Tool:
      status.active = state.tool == descriptor.worldLayoutTool &&
                      !state.buildingTemplatePlacement.active;
      break;
    case WorldActivation::TerrainRegionSession:
      status.active = topography.region.editingEnabled;
      break;
    case WorldActivation::Count:
      status.unavailable = true;
      break;
  }
  return status;
}

CreativeEditorToolDescriptorValidation validateCreativeEditorToolDescriptors(
    std::span<const CreativeEditorToolDescriptor> descriptors) noexcept {
  const auto fail = [](CreativeEditorToolDescriptorValidationStatus status,
                       CreativeEditorToolId tool) {
    return CreativeEditorToolDescriptorValidation{status, tool};
  };
  if (descriptors.size() != static_cast<std::size_t>(Id::Count)) {
    return fail(CreativeEditorToolDescriptorValidationStatus::WrongCount,
                Id::Count);
  }
  std::array<bool, cr::kCreativeHeldItemKindCount> heldItems{};
  std::array<bool, static_cast<std::size_t>(WorldTool::Count)> worldTools{};
  for (std::size_t index = 0U; index < descriptors.size(); ++index) {
    const Descriptor& descriptor = descriptors[index];
    if (descriptor.id != static_cast<Id>(index)) {
      return fail(CreativeEditorToolDescriptorValidationStatus::MisorderedId,
                  descriptor.id);
    }
    if (descriptor.category >= Category::Count ||
        descriptor.maturity >= Maturity::Count ||
        descriptor.exposure >= Exposure::Count ||
        descriptor.lifecycle >= Lifecycle::Count ||
        descriptor.volumeSettingsProfile >=
            CreativeEditorVolumeSettingsProfile::Count ||
        descriptor.worldLayoutActivation >= WorldActivation::Count) {
      return fail(CreativeEditorToolDescriptorValidationStatus::InvalidEnum,
                  descriptor.id);
    }
    if (descriptor.name.empty() || descriptor.purpose.empty()) {
      return fail(CreativeEditorToolDescriptorValidationStatus::MissingIdentity,
                  descriptor.id);
    }
    if (descriptor.heldItemKind < Held::Count) {
      const std::size_t heldIndex = static_cast<std::size_t>(
          descriptor.heldItemKind);
      if (heldItems[heldIndex]) {
        return fail(
            CreativeEditorToolDescriptorValidationStatus::DuplicateHeldItem,
            descriptor.id);
      }
      heldItems[heldIndex] = true;
    } else if (descriptor.catalogTool) {
      return fail(
          CreativeEditorToolDescriptorValidationStatus::InvalidCatalogBinding,
          descriptor.id);
    }
    if (descriptor.worldLayoutActivation == WorldActivation::Tool) {
      if (descriptor.worldLayoutTool >= WorldTool::Count) {
        return fail(CreativeEditorToolDescriptorValidationStatus::
                        InvalidWorldLayoutBinding,
                    descriptor.id);
      }
      const std::size_t toolIndex =
          static_cast<std::size_t>(descriptor.worldLayoutTool);
      if (worldTools[toolIndex]) {
        return fail(CreativeEditorToolDescriptorValidationStatus::
                        DuplicateWorldLayoutBinding,
                    descriptor.id);
      }
      worldTools[toolIndex] = true;

      const WorldInput input = descriptor.worldLayoutInputProfile;
      const WorldPreview preview = descriptor.worldLayoutPreviewProfile;
      const WorldFill fill = descriptor.worldLayoutPreviewFill;
      const bool profileEnumsValid =
          input < WorldInput::Count && preview < WorldPreview::Count &&
          fill < WorldFill::Count;
      const bool profileCombinationValid =
          (input == WorldInput::Point &&
           (preview == WorldPreview::None ||
            preview == WorldPreview::DoorOpening ||
            preview == WorldPreview::WindowOpening ||
            preview == WorldPreview::CatalogAsset)) ||
          (input == WorldInput::Drag &&
           (preview == WorldPreview::Rectangle ||
            preview == WorldPreview::AxisLine) &&
           !descriptor.worldLayoutInteractionPrompt.empty()) ||
          (input == WorldInput::Polyline &&
           preview == WorldPreview::TerrainPath);
      const bool fillValid =
          fill == WorldFill::None || preview == WorldPreview::Rectangle ||
          preview == WorldPreview::TerrainPath;
      if (!profileEnumsValid || !profileCombinationValid || !fillValid) {
        return fail(CreativeEditorToolDescriptorValidationStatus::
                        InvalidWorldLayoutPolicy,
                    descriptor.id);
      }
    } else if (descriptor.worldLayoutInputProfile != WorldInput::Count ||
               descriptor.worldLayoutPreviewProfile != WorldPreview::Count ||
               descriptor.worldLayoutPreviewFill != WorldFill::Count ||
               !descriptor.worldLayoutInteractionPrompt.empty()) {
      return fail(CreativeEditorToolDescriptorValidationStatus::
                      InvalidWorldLayoutPolicy,
                  descriptor.id);
    }
    if (descriptor.exposure == Exposure::Default &&
        descriptor.maturity < Maturity::M3Product) {
      return fail(CreativeEditorToolDescriptorValidationStatus::
                      DefaultProductBelowM3,
                  descriptor.id);
    }
    if (descriptor.defaultWheelEligible &&
        descriptor.exposure != Exposure::Workspace &&
        descriptor.exposure != Exposure::Default) {
      return fail(CreativeEditorToolDescriptorValidationStatus::
                      InvalidDefaultWheelExposure,
                  descriptor.id);
    }
  }
  if (std::find(worldTools.begin(), worldTools.end(), false) !=
      worldTools.end()) {
    return fail(
        CreativeEditorToolDescriptorValidationStatus::MissingWorldLayoutBinding,
        Id::Count);
  }
  return {};
}

bool creativeEditorToolDefaultVisible(
    const CreativeEditorToolDescriptor& descriptor) noexcept {
  return descriptor.exposure == Exposure::Workspace ||
         descriptor.exposure == Exposure::Default;
}

bool creativeEditorToolExperimental(
    const CreativeEditorToolDescriptor& descriptor) noexcept {
  return descriptor.exposure == Exposure::Experimental;
}

bool creativeEditorToolHasWorldLayoutSurface(
    const CreativeEditorToolDescriptor& descriptor) noexcept {
  return descriptor.worldLayoutActivation == WorldActivation::Tool ||
         descriptor.worldLayoutActivation ==
             WorldActivation::TerrainRegionSession;
}

std::span<const cr::CreativeCatalogToolSpec>
creativeEditorCatalogToolSpecs() noexcept {
  return kCatalogToolSpecs;
}

cr::CreativeHotbarState makeCreativeEditorDefaultHotbar(
    std::span<const cr::CreativeObjectKind> materialPalette) noexcept {
  cr::CreativeHotbarState hotbar;
  for (cr::CreativeHotbarEntry& entry : hotbar.entries) {
    entry.kind = Held::Count;
  }
  const cr::CreativeObjectKind material =
      materialPalette.empty() ? cr::CreativeObjectKind::Unknown
                              : materialPalette.front();
  std::size_t slot = 0U;
  for (const Descriptor& descriptor : kToolDescriptors) {
    if (!descriptor.defaultWheelEligible ||
        descriptor.heldItemKind >= Held::Count ||
        slot == hotbar.entries.size()) {
      continue;
    }
    const cr::CreativeObjectKind objectKind =
        cr::creativeHeldItemUsesMaterial(descriptor.heldItemKind)
            ? material
            : cr::CreativeObjectKind::Unknown;
    hotbar.entries[slot++] = {descriptor.heldItemKind, objectKind};
  }
  return hotbar;
}

CreativeEditorWorldLayoutPaletteCategory
creativeEditorWorldLayoutPaletteCategory(
    const CreativeEditorToolDescriptor& descriptor) noexcept {
  switch (descriptor.category) {
    case Category::Workspace:
    case Category::Building:
      return CreativeEditorWorldLayoutPaletteCategory::Structure;
    case Category::Terrain:
      return CreativeEditorWorldLayoutPaletteCategory::Terrain;
    case Category::Placement:
    case Category::Volume:
    case Category::Object:
    case Category::Logic:
      return CreativeEditorWorldLayoutPaletteCategory::Object;
    case Category::Gameplay:
      return CreativeEditorWorldLayoutPaletteCategory::Gameplay;
    case Category::Count:
      break;
  }
  return CreativeEditorWorldLayoutPaletteCategory::Count;
}

}  // namespace iggy3d_creative_app
