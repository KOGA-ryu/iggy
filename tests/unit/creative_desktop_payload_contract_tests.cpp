#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

#include "EditorDesktopCommands.hpp"

namespace {

namespace app = iggy3d_creative_app;

using ExpectedPayloadOrder = std::tuple<
    std::monostate,
    app::CreativeDesktopSaveAsPayload,
    app::CreativeDesktopMapTemplatePayload,
    app::CreativeDesktopMeasurementAnnotationPayload,
    app::CreativeDesktopSelectPayload,
    app::CreativeDesktopLogicLinkPayload,
    app::CreativeDesktopRenamePayload,
    app::CreativeDesktopObjectFlagPayload,
    app::CreativeDesktopTransformPayload,
    app::CreativeDesktopGroupPivotPayload,
    app::CreativeDesktopMovingPlatformPayload,
    app::CreativeDesktopPlayerSpawnPayload,
    app::CreativeDesktopNpcSpawnPayload,
    app::CreativeDesktopLootPointPayload,
    app::CreativeDesktopExitPointPayload,
    app::CreativeDesktopMovingPlatformPreviewPayload,
    app::CreativeDesktopMovingPlatformWaypointPayload,
    app::CreativeDesktopTerrainOperationPayload,
    app::CreativeDesktopTerrainStampPayload,
    app::CreativeDesktopWorldLayoutToolPayload,
    app::CreativeDesktopWorldLayoutCatalogAssetPayload,
    app::CreativeDesktopWorldLayoutBuildingBlockoutPayload,
    app::CreativeDesktopWorldLayoutBuildingBlockoutUpdatePayload,
    app::CreativeDesktopWorldLayoutBuildingSelectionPayload,
    app::CreativeDesktopWorldLayoutSourcePayload,
    app::CreativeDesktopWorldLayoutObjectSourcePayload,
    app::CreativeDesktopWorldLayoutSourceRenamePayload,
    app::CreativeDesktopWorldLayoutLevelOperationPayload,
    app::CreativeDesktopWorldLayoutLevelSettingsPayload,
    app::CreativeDesktopWorldLayoutLevelDatumPayload,
    app::CreativeDesktopWorldLayoutRoofApertureCreatePayload,
    app::CreativeDesktopWorldLayoutRoofApertureManipulationPayload,
    app::CreativeDesktopWorldLayoutRoofManipulationPayload,
    app::CreativeDesktopGeneratedLevelSettingsPayload,
    app::CreativeDesktopWorldLayoutObjectSettingsPayload,
    app::CreativeDesktopWorldLayoutBuildingManipulationPayload,
    app::CreativeDesktopWorldLayoutBuildingDuplicatePayload,
    app::CreativeDesktopWorldLayoutBuildingTransformPayload,
    app::CreativeDesktopGeneratedBuildingOperationPayload,
    app::CreativeDesktopWorldLayoutBuildingGroundingPayload,
    app::CreativeDesktopWorldLayoutBuildingArchitecturePayload,
    app::CreativeDesktopWorldLayoutBuildingTemplateCapturePayload,
    app::CreativeDesktopWorldLayoutBuildingTemplateSyncPayload,
    app::CreativeDesktopWorldLayoutBuildingTemplateSelectionPayload,
    app::CreativeDesktopWorldLayoutBuildingTemplatePlacementPayload,
    app::CreativeDesktopWorldLayoutPointPayload,
    app::CreativeDesktopWorldLayoutGesturePayload,
    app::CreativeDesktopGeneratedRoomSettingsPayload,
    app::CreativeDesktopWorldLayoutRoomSplitPayload,
    app::CreativeDesktopWorldLayoutRoomMergePayload,
    app::CreativeDesktopWorldLayoutWallSplitPayload,
    app::CreativeDesktopWorldLayoutWallMergePayload,
    app::CreativeDesktopWorldLayoutRoomManipulationPayload,
    app::CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload,
    app::CreativeDesktopWorldLayoutRoomCornerManipulationPayload,
    app::CreativeDesktopGeneratedVerticalConnectorSettingsPayload,
    app::CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload,
    app::CreativeDesktopWorldLayoutBoxSettingsPayload,
    app::CreativeDesktopWorldLayoutBoxManipulationPayload,
    app::CreativeDesktopWorldLayoutWallSettingsPayload,
    app::CreativeDesktopGeneratedWallSettingsPayload,
    app::CreativeDesktopWorldLayoutWallManipulationPayload,
    app::CreativeDesktopWorldLayoutOpeningSettingsPayload,
    app::CreativeDesktopGeneratedOpeningSettingsPayload,
    app::CreativeDesktopWorldLayoutPropertyEditPayload,
    app::CreativeDesktopWorldLayoutOpeningInsertPayload,
    app::CreativeDesktopWorldLayoutAssetRepairPayload,
    app::CreativeDesktopWorldLayoutBuildingRepairPayload,
    app::CreativeDesktopWorldLayoutOpeningManipulationPayload,
    app::CreativeDesktopWorldLayoutConfirmPayload>;

template <std::size_t... Index>
consteval bool payloadOrderMatches(std::index_sequence<Index...>) {
  return (std::is_same_v<
              std::variant_alternative_t<
                  Index, app::CreativeDesktopCommandPayload>,
              std::tuple_element_t<Index, ExpectedPayloadOrder>> &&
          ...);
}

static_assert(
    std::variant_size_v<app::CreativeDesktopCommandPayload> ==
    std::tuple_size_v<ExpectedPayloadOrder>);
static_assert(payloadOrderMatches(
    std::make_index_sequence<std::tuple_size_v<ExpectedPayloadOrder>>{}));
static_assert(std::is_default_constructible_v<
              app::CreativeDesktopCommandPayload>);
static_assert(std::is_copy_constructible_v<
              app::CreativeDesktopCommandPayload>);
static_assert(std::is_move_constructible_v<
              app::CreativeDesktopCommandPayload>);

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

std::size_t occurrenceCount(std::string_view text,
                            std::string_view needle) {
  std::size_t count = 0U;
  std::size_t position = 0U;
  while ((position = text.find(needle, position)) != std::string_view::npos) {
    ++count;
    position += needle.size();
  }
  return count;
}

bool contractHeadersStayAcyclicAndNarrow() {
  constexpr std::array headers{
      "EditorDesktopCommandPayloadCommon.hpp",
      "EditorDesktopCommandPayloadObject.hpp",
      "EditorDesktopCommandPayloadTerrain.hpp",
      "EditorDesktopCommandPayloadWorldLayoutSource.hpp",
      "EditorDesktopCommandPayloadWorldLayoutPlan.hpp",
      "EditorDesktopCommandPayloadWorldLayoutBuilding.hpp",
      "EditorDesktopCommandPayloadWorldLayoutReview.hpp",
  };
  bool clean = true;
  for (std::string_view header : headers) {
    const std::string source = readFile(
        std::filesystem::path{"apps/iggy3d_creative"} / header);
    clean = clean &&
            source.find("EditorDesktopCommandPayloads.hpp") ==
                std::string::npos;
    for (std::string_view other : headers) {
      if (header != other &&
          source.find(std::string(other)) != std::string::npos) {
        clean = false;
      }
    }
  }
  const std::string aggregate = readFile(
      "apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp");
  for (std::string_view header : headers) {
    clean = clean && occurrenceCount(aggregate, header) == 1U;
  }
  return clean;
}

bool aggregateHasOneEnvelopeAndOnlyOneProductionIncluder() {
  const std::string aggregate = readFile(
      "apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp");
  if (occurrenceCount(aggregate, "using CreativeDesktopCommandPayload =") !=
      1U) {
    return false;
  }
  const std::string commands =
      readFile("apps/iggy3d_creative/EditorDesktopCommands.hpp");
  return occurrenceCount(
             commands, "#include \"EditorDesktopCommandPayloads.hpp\"") ==
         1U;
}

bool everyAlternativeHasDispatcherAdmission() {
  const std::string aggregate = readFile(
      "apps/iggy3d_creative/EditorDesktopCommandPayloads.hpp");
  const std::size_t begin =
      aggregate.find("using CreativeDesktopCommandPayload = std::variant<");
  const std::size_t end =
      aggregate.find(">;", begin);
  if (begin == std::string::npos || end == std::string::npos) {
    return false;
  }

  std::string dispatchers;
  const std::filesystem::path root{"apps/iggy3d_creative"};
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(root)) {
    const std::string filename = entry.path().filename().string();
    if (filename.starts_with("EditorDesktop") &&
        filename.ends_with("Commands.cpp")) {
      dispatchers += readFile(entry.path());
    }
  }
  std::string compactDispatchers;
  compactDispatchers.reserve(dispatchers.size());
  for (char character : dispatchers) {
    if (!std::isspace(static_cast<unsigned char>(character))) {
      compactDispatchers.push_back(character);
    }
  }

  std::string_view alternatives{aggregate.data() + begin, end - begin};
  std::size_t lineStart = 0U;
  while (lineStart < alternatives.size()) {
    const std::size_t lineEnd = alternatives.find('\n', lineStart);
    std::string line{alternatives.substr(
        lineStart, lineEnd == std::string_view::npos
                       ? alternatives.size() - lineStart
                       : lineEnd - lineStart)};
    const std::size_t first = line.find_first_not_of(" \t");
    if (first != std::string::npos) {
      line.erase(0U, first);
    }
    if (!line.empty() && line.back() == ',') {
      line.pop_back();
    }
    if (line == "std::monostate") {
      if (compactDispatchers.find("holds_alternative<std::monostate>") ==
          std::string::npos) {
        return false;
      }
    } else if (line.starts_with("CreativeDesktop") &&
               compactDispatchers.find("get_if<" + line + ">") ==
                   std::string::npos &&
               compactDispatchers.find("payloadAs<" + line + ">") ==
                   std::string::npos) {
      std::cerr << "missing dispatcher admission: " << line << '\n';
      return false;
    }
    if (lineEnd == std::string_view::npos) {
      break;
    }
    lineStart = lineEnd + 1U;
  }
  return true;
}

}  // namespace

int main() {
  if (!contractHeadersStayAcyclicAndNarrow()) {
    std::cerr << "payload contracts are cyclic or broad\n";
    return 1;
  }
  if (!aggregateHasOneEnvelopeAndOnlyOneProductionIncluder()) {
    std::cerr << "payload aggregate ownership drifted\n";
    return 1;
  }
  if (!everyAlternativeHasDispatcherAdmission()) {
    std::cerr << "payload dispatcher admission drifted\n";
    return 1;
  }
  return 0;
}
