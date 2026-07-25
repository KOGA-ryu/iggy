#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace fs = std::filesystem;

struct OwnershipRule {
  std::string_view symbol;
  std::string_view owner;
};

std::string readFile(const fs::path& path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

bool editorEditSymbolsHaveOneImplementationOwner() {
  constexpr std::string_view kHistory =
      "apps/iggy3d_creative/EditorEditHistory.cpp";
  constexpr std::string_view kActions =
      "apps/iggy3d_creative/EditorEditObjectActions.cpp";
  constexpr std::string_view kProperties =
      "apps/iggy3d_creative/EditorEditObjectProperties.cpp";
  constexpr std::string_view kClipboard =
      "apps/iggy3d_creative/EditorEditClipboardAttachment.cpp";
  constexpr std::string_view kActors =
      "apps/iggy3d_creative/EditorEditActorSettings.cpp";
  constexpr std::array rules{
      OwnershipRule{"clearEditHistory(", kHistory},
      OwnershipRule{"beginEditTransaction(", kHistory},
      OwnershipRule{"setEditTransactionOperation(", kHistory},
      OwnershipRule{"cancelEditTransaction(", kHistory},
      OwnershipRule{"completeEditTransaction(", kHistory},
      OwnershipRule{"undoLastEdit(", kHistory},
      OwnershipRule{"redoLastEdit(", kHistory},
      OwnershipRule{"deleteSelectedObjectsWithUndo(", kActions},
      OwnershipRule{"deleteCreativeEditorSelectionWithUndo(", kActions},
      OwnershipRule{"deleteCreativeEditorObjectsWithUndo(", kActions},
      OwnershipRule{"deleteObjectsWithUndo(", kActions},
      OwnershipRule{"transformSelectedObjectsWithUndo(", kActions},
      OwnershipRule{"duplicateSelectedObjectsWithUndo(", kActions},
      OwnershipRule{"duplicateCreativeEditorSelectionWithUndo(", kActions},
      OwnershipRule{"transformCreativeEditorSelectionWithUndo(", kActions},
      OwnershipRule{"renameCreativeEditorObjectWithUndo(", kProperties},
      OwnershipRule{"setCreativeEditorObjectsVisibleWithUndo(", kProperties},
      OwnershipRule{"setCreativeEditorObjectsLockedWithUndo(", kProperties},
      OwnershipRule{"setCreativeEditorObjectTransformWithUndo(", kProperties},
      OwnershipRule{"toggleCreativeEditorSelectionVisibilityWithUndo(",
                    kProperties},
      OwnershipRule{"toggleCreativeEditorSelectionLockedWithUndo(",
                    kProperties},
      OwnershipRule{"detachCreativeEditorObjectWithUndo(", kClipboard},
      OwnershipRule{"reattachCreativeEditorObjectWithUndo(", kClipboard},
      OwnershipRule{"copySelectionToClipboard(", kClipboard},
      OwnershipRule{"cutSelectionToClipboardWithHistory(", kClipboard},
      OwnershipRule{"cutCreativeEditorSelectionToClipboardWithHistory(",
                    kClipboard},
      OwnershipRule{"pasteClipboardWithHistory(", kClipboard},
      OwnershipRule{"setCreativeEditorMovingPlatformSettingsWithUndo(",
                    kActors},
      OwnershipRule{"setCreativeEditorPlayerSpawnSettingsWithUndo(", kActors},
      OwnershipRule{"setCreativeEditorNpcSpawnSettingsWithUndo(", kActors},
      OwnershipRule{"setCreativeEditorLootPointSettingsWithUndo(", kActors},
      OwnershipRule{"setCreativeEditorExitPointSettingsWithUndo(", kActors},
  };

  std::vector<fs::path> sources;
  for (const fs::directory_entry& entry :
       fs::directory_iterator("apps/iggy3d_creative")) {
    if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
      sources.push_back(entry.path());
    }
  }

  bool clean = true;
  for (const OwnershipRule& rule : rules) {
    std::vector<std::string> owners;
    for (const fs::path& path : sources) {
      const std::string source = readFile(path);
      bool definesSymbol = false;
      std::size_t lineStart = 0U;
      while (lineStart < source.size()) {
        const std::size_t lineEnd = source.find('\n', lineStart);
        const std::string_view line{
            source.data() + lineStart,
            (lineEnd == std::string::npos ? source.size() : lineEnd) -
                lineStart};
        if (!line.empty() && line.front() != ' ' && line.front() != '\t' &&
            line.find(rule.symbol) != std::string_view::npos) {
          definesSymbol = true;
          break;
        }
        if (lineEnd == std::string::npos) {
          break;
        }
        lineStart = lineEnd + 1U;
      }
      if (definesSymbol) {
        owners.push_back(path.lexically_normal().generic_string());
      }
    }
    if (owners.size() != 1U || owners.front() != rule.owner) {
      std::cerr << "FAIL: " << rule.symbol << " expected owner " << rule.owner
                << " found";
      for (const std::string& owner : owners) {
        std::cerr << ' ' << owner;
      }
      std::cerr << '\n';
      clean = false;
    }
  }
  return clean;
}

}  // namespace

int main() {
  return editorEditSymbolsHaveOneImplementationOwner() ? EXIT_SUCCESS
                                                        : EXIT_FAILURE;
}
