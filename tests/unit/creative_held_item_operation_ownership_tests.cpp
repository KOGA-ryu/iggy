#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#include "EditorHeldItemWorldOperationsInternal.hpp"

namespace {

namespace app = iggy3d_creative_app;
namespace cr = iggy3d::creative;

struct OperationPin {
  std::string_view name;
  std::string_view ownerFile;
};

constexpr std::array kOperationPins{
    OperationPin{"SelectObject", "EditorHeldItemSelectionOperations.cpp"},
    OperationPin{"ClearSelection", "EditorHeldItemSelectionOperations.cpp"},
    OperationPin{"SampleTargetMaterial",
                 "EditorHeldItemSelectionOperations.cpp"},
    OperationPin{"RejectActiveInteraction",
                 "EditorHeldItemSelectionOperations.cpp"},
    OperationPin{"SetVolumeFirstCorner",
                 "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"SetVolumeSecondCorner",
                 "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"ExpandVolumeSelection",
                 "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"AdvanceVolumeSelection",
                 "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"BeginShapeVolume", "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"CommitShapeVolume", "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"AdvanceShapeVolume", "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"ApplyVolumeOperation",
                 "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"ApplyArray", "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"AcceptArray", "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"PaintConnectedFill",
                 "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"EraseConnectedFill",
                 "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"ExtrudeSurface", "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"InsetSurface", "EditorHeldItemVolumeOperations.cpp"},
    OperationPin{"UpsertTerrainControl",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"RemoveTerrainControl",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"SampleTerrainControl",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"BeginTerrainGrade",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"ApplyTerrainGrade",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"CancelTerrainGrade",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"ApplyTerrainProfile",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"LockTerrainProfileBase",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"UnlockTerrainProfileBase",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"AddTerrainPathPoint",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"ApplyTerrainPath",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"RemoveTerrainPathPoint",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"ApplyTerrainRegion",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"AdvanceTerrainRegion",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"SampleTerrainRegionHeight",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"CancelTerrainRegion",
                 "EditorHeldItemTerrainOperations.cpp"},
    OperationPin{"ApplyObjectGroup",
                 "EditorHeldItemRelationshipOperations.cpp"},
    OperationPin{"AdvanceLogicLink",
                 "EditorHeldItemRelationshipOperations.cpp"},
    OperationPin{"ClearLogicLinkSource",
                 "EditorHeldItemRelationshipOperations.cpp"},
    OperationPin{"AdvanceBuildingRoom",
                 "EditorHeldItemRelationshipOperations.cpp"},
    OperationPin{"CancelBuildingRoom",
                 "EditorHeldItemRelationshipOperations.cpp"},
    OperationPin{"AppendMeasurementPoint",
                 "EditorHeldItemRelationshipOperations.cpp"},
    OperationPin{"CompleteMeasurement",
                 "EditorHeldItemRelationshipOperations.cpp"},
    OperationPin{"CancelMeasurement",
                 "EditorHeldItemRelationshipOperations.cpp"},
};

consteval bool everyOperationHasOwner() {
  for (std::uint8_t value = 0U;
       value < static_cast<std::uint8_t>(
                   cr::CreativeHeldItemWorldOperation::Count);
       ++value) {
    const auto operation =
        static_cast<cr::CreativeHeldItemWorldOperation>(value);
    if (app::creativeHeldItemWorldOperationOwner(operation) ==
        app::CreativeHeldItemWorldOperationOwner::Invalid) {
      return false;
    }
  }
  return app::creativeHeldItemWorldOperationOwner(
             cr::CreativeHeldItemWorldOperation::Count) ==
         app::CreativeHeldItemWorldOperationOwner::Invalid;
}

static_assert(everyOperationHasOwner());
static_assert(kOperationPins.size() + 1U ==
              static_cast<std::size_t>(
                  cr::CreativeHeldItemWorldOperation::Count));

std::string readFile(std::string_view filename) {
  const std::filesystem::path path =
      std::filesystem::path{"apps/iggy3d_creative"} / filename;
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

bool eachOperationHasOneExecutorOwner() {
  constexpr std::array files{
      std::string_view{"EditorHeldItemSelectionOperations.cpp"},
      std::string_view{"EditorHeldItemVolumeOperations.cpp"},
      std::string_view{"EditorHeldItemTerrainOperations.cpp"},
      std::string_view{"EditorHeldItemRelationshipOperations.cpp"},
  };
  std::array<std::string, files.size()> sources;
  for (std::size_t index = 0U; index < files.size(); ++index) {
    sources[index] = readFile(files[index]);
  }
  for (const OperationPin& pin : kOperationPins) {
    const std::string token =
        "CreativeHeldItemWorldOperation::" + std::string(pin.name);
    std::size_t occurrences = 0U;
    std::string_view actualOwner;
    for (std::size_t index = 0U; index < files.size(); ++index) {
      if (occurrenceCount(sources[index], token) > 0U) {
        ++occurrences;
        actualOwner = files[index];
      }
    }
    if (occurrences != 1U || actualOwner != pin.ownerFile) {
      return false;
    }
  }
  return true;
}

bool coordinatorIsSingleExhaustiveBoundary() {
  const std::string coordinator =
      readFile("EditorHeldItemWorldOperations.cpp");
  return occurrenceCount(
             coordinator, "creativeHeldItemWorldOperationOwner(operation)") ==
             1U &&
         occurrenceCount(
             coordinator,
             "executeCreativeHeldItemSelectionTransformOperation(") == 1U &&
         occurrenceCount(
             coordinator,
             "executeCreativeHeldItemVolumeSurfaceOperation(") == 1U &&
         occurrenceCount(
             coordinator, "executeCreativeHeldItemTerrainOperation(") == 1U &&
         occurrenceCount(
             coordinator,
             "executeCreativeHeldItemRelationshipOperation(") == 1U &&
         coordinator.find("if (executeCreativeHeldItem") ==
             std::string::npos;
}

bool leafExecutorsOwnNoSecondQueueOrMutationBoundary() {
  constexpr std::array files{
      std::string_view{"EditorHeldItemSelectionOperations.cpp"},
      std::string_view{"EditorHeldItemVolumeOperations.cpp"},
      std::string_view{"EditorHeldItemTerrainOperations.cpp"},
      std::string_view{"EditorHeldItemRelationshipOperations.cpp"},
  };
  for (std::string_view file : files) {
    const std::string source = readFile(file);
    if (source.find("CreativeHeldItemWorldOperationList") !=
            std::string::npos ||
        source.find("DocumentMutation") != std::string::npos ||
        source.find("CreativeDesktopCommandFrame") != std::string::npos) {
      return false;
    }
  }
  return true;
}

}  // namespace

int main() {
  if (!eachOperationHasOneExecutorOwner()) {
    std::cerr << "held operation executor ownership drifted\n";
    return 1;
  }
  if (!coordinatorIsSingleExhaustiveBoundary()) {
    std::cerr << "held operation coordinator drifted\n";
    return 1;
  }
  if (!leafExecutorsOwnNoSecondQueueOrMutationBoundary()) {
    std::cerr << "held operation leaf boundary widened\n";
    return 1;
  }
  return 0;
}
