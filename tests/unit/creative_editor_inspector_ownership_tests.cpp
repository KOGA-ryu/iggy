#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

std::size_t occurrenceCount(
    std::string_view text,
    std::string_view token) {
  std::size_t count = 0U;
  std::size_t offset = 0U;
  while ((offset = text.find(token, offset)) != std::string_view::npos) {
    ++count;
    offset += token.size();
  }
  return count;
}

struct OwnerPin {
  std::string_view file;
  std::string_view definition;
};

bool originalFunctionsHaveOneFocusedOwner() {
  constexpr std::array owners{
      OwnerPin{
          "EditorDesktopInspector.cpp",
          "void buildCreativeEditorDesktopInspectorPanel("},
      OwnerPin{
          "EditorDesktopInspectorShared.cpp",
          "int inputTextResizeCallback("},
      OwnerPin{
          "EditorDesktopInspectorShared.cpp",
          "bool creativeDesktopInputTextStdString("},
      OwnerPin{
          "EditorDesktopInspectorShared.cpp",
          "void appendCreativeDesktopInspectorHoverTooltip("},
      OwnerPin{
          "EditorDesktopInspectorShared.cpp",
          "void refreshCreativeDesktopInspectorDraft("},
      OwnerPin{
          "EditorDesktopInspectorShared.cpp",
          "void queueCreativeDesktopObjectNavigation("},
      OwnerPin{
          "EditorDesktopInspectorShared.cpp",
          "ImVec4 creativeDesktopLogicDiagnosticColor("},
      OwnerPin{
          "EditorDesktopInspectorRelationships.cpp",
          "void appendCreativeDesktopMeasurementInspector("},
      OwnerPin{
          "EditorDesktopInspectorRelationships.cpp",
          "void queueLogicSource("},
      OwnerPin{
          "EditorDesktopInspectorRelationships.cpp",
          "void queueSetLogicLink("},
      OwnerPin{
          "EditorDesktopInspectorRelationships.cpp",
          "void queueRemoveLogicLink("},
      OwnerPin{
          "EditorDesktopInspectorRelationships.cpp",
          "const cr::CreativeLogicDiagnostic* diagnosticForSource("},
      OwnerPin{
          "EditorDesktopInspectorRelationships.cpp",
          "void appendLogicLinkRow("},
      OwnerPin{
          "EditorDesktopInspectorRelationships.cpp",
          "void appendNewLogicLinkControl("},
      OwnerPin{
          "EditorDesktopInspectorRelationships.cpp",
          "void appendCreativeDesktopLogicInspector("},
      OwnerPin{
          "EditorDesktopInspectorSelection.cpp",
          "const char* triStateLabel("},
      OwnerPin{
          "EditorDesktopInspectorSelection.cpp",
          "TriState foldFlag("},
      OwnerPin{
          "EditorDesktopInspectorSelection.cpp",
          "void appendCreativeDesktopMultiSelectionInspector("},
      OwnerPin{
          "EditorDesktopInspectorActors.cpp",
          "void appendCreativeDesktopPlayerSpawnFields("},
      OwnerPin{
          "EditorDesktopInspectorActors.cpp",
          "void appendCreativeDesktopNpcSpawnFields("},
      OwnerPin{
          "EditorDesktopInspectorActors.cpp",
          "void appendCreativeDesktopLootPointFields("},
      OwnerPin{
          "EditorDesktopInspectorActors.cpp",
          "void appendCreativeDesktopExitPointFields("},
      OwnerPin{
          "EditorDesktopInspectorActors.cpp",
          "void appendCreativeDesktopMovingPlatformFields("},
      OwnerPin{
          "EditorDesktopInspectorObject.cpp",
          "void appendTransformFields("},
      OwnerPin{
          "EditorDesktopInspectorObject.cpp",
          "void appendGroupPivotFields("},
      OwnerPin{
          "EditorDesktopInspectorObject.cpp",
          "void appendCreativeDesktopSingleObjectInspector("},
  };
  bool clean = true;
  for (const OwnerPin& owner : owners) {
    const std::string source = readFile(
        std::filesystem::path{"apps/iggy3d_creative"} / owner.file);
    clean = expect(
                occurrenceCount(source, owner.definition) == 1U,
                "each original inspector function has one focused owner") &&
            clean;
  }
  return clean;
}

struct CommandPin {
  std::string_view id;
  std::size_t count = 0U;
};

bool commandInventoryIsPreservedExactly() {
  constexpr std::array files{
      std::string_view{"EditorDesktopInspector.cpp"},
      std::string_view{"EditorDesktopInspectorActors.cpp"},
      std::string_view{"EditorDesktopInspectorObject.cpp"},
      std::string_view{"EditorDesktopInspectorRelationships.cpp"},
      std::string_view{"EditorDesktopInspectorSelection.cpp"},
      std::string_view{"EditorDesktopInspectorShared.cpp"},
  };
  std::string source;
  for (std::string_view file : files) {
    source += readFile(
        std::filesystem::path{"apps/iggy3d_creative"} / file);
  }
  constexpr std::array commands{
      CommandPin{"ClearLogicSource", 1U},
      CommandPin{"DeleteSelection", 2U},
      CommandPin{"DuplicateSelection", 2U},
      CommandPin{"FocusObject", 1U},
      CommandPin{"RemoveLogicLink", 1U},
      CommandPin{"RemoveMeasurementAnnotation", 1U},
      CommandPin{"RenameObject", 1U},
      CommandPin{"RestartMovingPlatformPreview", 1U},
      CommandPin{"SaveMeasurementAnnotation", 1U},
      CommandPin{"SeekMovingPlatformPreview", 1U},
      CommandPin{"SelectMovingPlatformWaypoint", 1U},
      CommandPin{"SelectObjects", 1U},
      CommandPin{"SetExitPointSettings", 1U},
      CommandPin{"SetGroupPivot", 1U},
      CommandPin{"SetLogicLink", 1U},
      CommandPin{"SetLogicSource", 1U},
      CommandPin{"SetLootPointSettings", 1U},
      CommandPin{"SetMovingPlatformSettings", 1U},
      CommandPin{"SetMovingPlatformWaypointDwell", 1U},
      CommandPin{"SetNpcSpawnSettings", 1U},
      CommandPin{"SetObjectTransform", 1U},
      CommandPin{"SetObjectsLocked", 3U},
      CommandPin{"SetObjectsVisible", 3U},
      CommandPin{"SetPlayerSpawnSettings", 1U},
      CommandPin{"ToggleMovingPlatformPreview", 1U},
      CommandPin{"WorldLayoutAdoptObjectSource", 1U},
      CommandPin{"WorldLayoutCancelGeneratedSettingsPreview", 1U},
  };
  bool clean = true;
  for (const CommandPin& command : commands) {
    const std::string token =
        "CreativeDesktopCommandId::" + std::string(command.id);
    clean = expect(
                occurrenceCount(source, token) == command.count,
                "inspector command inventory remains exact") &&
            clean;
  }
  return clean;
}

bool selectionAndPanelOrderHaveOneCoordinator() {
  constexpr std::array files{
      std::string_view{"EditorDesktopInspector.cpp"},
      std::string_view{"EditorDesktopInspectorActors.cpp"},
      std::string_view{"EditorDesktopInspectorObject.cpp"},
      std::string_view{"EditorDesktopInspectorRelationships.cpp"},
      std::string_view{"EditorDesktopInspectorSelection.cpp"},
      std::string_view{"EditorDesktopInspectorShared.cpp"},
  };
  std::string source;
  for (std::string_view file : files) {
    source += readFile(
        std::filesystem::path{"apps/iggy3d_creative"} / file);
  }
  if (!expect(
          occurrenceCount(source, "resolveCreativeDesktopSelection(") ==
              1U &&
              occurrenceCount(
                  source,
                  "refreshCreativeEditorDesktopObjectActionContext(") ==
                  1U,
          "selection and admission resolve once at the coordinator")) {
    return false;
  }

  const std::string coordinator = readFile(
      "apps/iggy3d_creative/EditorDesktopInspector.cpp");
  constexpr std::array coordinatorOrder{
      std::string_view{"appendCreativeDesktopMeasurementInspector"},
      std::string_view{"appendCreativeDesktopActiveTransformInspector"},
      std::string_view{"appendCreativeDesktopVolumeInspector"},
      std::string_view{"appendCreativeDesktopMultiSelectionInspector"},
      std::string_view{"appendCreativeDesktopSingleObjectInspector"},
      std::string_view{"appendCreativeDesktopLogicInspector"},
  };
  std::size_t previous = 0U;
  for (std::string_view token : coordinatorOrder) {
    const std::size_t position = coordinator.find(token, previous);
    if (!expect(position != std::string::npos,
                "coordinator preserves established panel order")) {
      return false;
    }
    previous = position + token.size();
  }

  const std::string objectOwner = readFile(
      "apps/iggy3d_creative/EditorDesktopInspectorObject.cpp");
  constexpr std::array singleOrder{
      std::string_view{"appendTransformFields"},
      std::string_view{"appendCreativeDesktopPlayerSpawnFields"},
      std::string_view{"appendCreativeDesktopNpcSpawnFields"},
      std::string_view{"appendCreativeDesktopLootPointFields"},
      std::string_view{"appendCreativeDesktopExitPointFields"},
      std::string_view{"appendCreativeDesktopMovingPlatformFields"},
      std::string_view{"appendCreativeDesktopGeneratedSourceSettings"},
  };
  previous = 0U;
  for (std::string_view token : singleOrder) {
    const std::size_t position = objectOwner.find(token, previous);
    if (!expect(position != std::string::npos,
                "single-object controls preserve their established order")) {
      return false;
    }
    previous = position + token.size();
  }
  return true;
}

bool controlsAndDraftLifetimeRemainVisible() {
  constexpr std::array files{
      std::string_view{"EditorDesktopInspectorActors.cpp"},
      std::string_view{"EditorDesktopInspectorObject.cpp"},
      std::string_view{"EditorDesktopInspectorRelationships.cpp"},
      std::string_view{"EditorDesktopInspectorSelection.cpp"},
  };
  std::string source;
  for (std::string_view file : files) {
    source += readFile(
        std::filesystem::path{"apps/iggy3d_creative"} / file);
  }
  constexpr std::array labels{
      std::string_view{"Measurement"},
      std::string_view{"Saved measurements"},
      std::string_view{"Logic links"},
      std::string_view{"Player Spawn"},
      std::string_view{"NPC Spawn"},
      std::string_view{"Loot"},
      std::string_view{"Exit Objective"},
      std::string_view{"Motion"},
      std::string_view{"Route Preview"},
      std::string_view{"Transform"},
      std::string_view{"Group Pivot"},
      std::string_view{"World Layout"},
  };
  bool clean = true;
  for (std::string_view label : labels) {
    clean = expect(source.find(label) != std::string::npos,
                   "every established inspector control family remains") &&
            clean;
  }
  const std::string objectOwner = readFile(
      "apps/iggy3d_creative/EditorDesktopInspectorObject.cpp");
  return clean &&
         expect(
             objectOwner.find(
                 "refreshCreativeDesktopInspectorDraft") !=
                     std::string::npos &&
                 objectOwner.find("draft.editing = false") !=
                     std::string::npos &&
                 objectOwner.find(
                     "draft.editing && input.cancelPressed") !=
                     std::string::npos,
             "single owner retains draft refresh, frame activity, and cancel");
}

}  // namespace

int main() {
  const bool passed =
      originalFunctionsHaveOneFocusedOwner() &&
      commandInventoryIsPreservedExactly() &&
      selectionAndPanelOrderHaveOneCoordinator() &&
      controlsAndDraftLifetimeRemainVisible();
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
