#include "EditorDesktopHistoryModel.hpp"

#include "EditorWorldLayoutHistory.hpp"
#include "EditorWorldLayoutState.hpp"
#include "app/iggy3d/creative/history/History.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace iggy3d_creative_app {
namespace {

[[nodiscard]] std::string historyLabel(std::string_view source) {
  constexpr std::array<std::string_view, 5U> prefixes{
      "desktop_", "keyboard_", "object_actions_", "controller_", "mouse_"};
  for (std::string_view prefix : prefixes) {
    if (source.starts_with(prefix)) {
      source.remove_prefix(prefix.size());
      break;
    }
  }
  std::string label;
  label.reserve(source.size());
  bool previousSpace = false;
  for (char character : source) {
    const bool separator = character == '_' || character == '-';
    if (separator) {
      if (!label.empty() && !previousSpace) {
        label.push_back(' ');
      }
      previousSpace = true;
      continue;
    }
    label.push_back(character);
    previousSpace = false;
  }
  while (!label.empty() && label.back() == ' ') {
    label.pop_back();
  }
  if (label.empty()) {
    return "Unnamed edit";
  }
  if (label.front() >= 'a' && label.front() <= 'z') {
    label.front() = static_cast<char>(label.front() - 'a' + 'A');
  }
  return label;
}

}  // namespace

CreativeDesktopHistoryModel buildCreativeDesktopHistoryModel(
    const cr::CreativeDocumentHistory& documentHistory,
    const CreativeEditorWorldLayoutState* worldLayout) {
  CreativeDesktopHistoryModel model;
  model.documentMaxDepth = documentHistory.maxDepth;
  model.sourceSynchronized =
      worldLayout == nullptr ||
      worldLayout->revision == worldLayout->generatedRevision;
  if (worldLayout != nullptr) {
    model.sourceMaxDepth = worldLayout->sourceHistory.maxDepth;
  }

  const auto appendSource = [](auto& rows, const auto& entries,
                               bool firstIsNext) {
    rows.reserve(rows.size() + entries.size());
    bool first = true;
    for (auto iterator = entries.rbegin(); iterator != entries.rend();
         ++iterator) {
      CreativeDesktopHistoryEntry row;
      row.domain = CreativeDesktopHistoryDomain::WorldLayout;
      row.source = iterator->source;
      row.label = historyLabel(row.source);
      row.nextAction = first && firstIsNext;
      rows.push_back(std::move(row));
      first = false;
    }
  };
  const auto appendDocument = [&](auto& rows, const auto& entries,
                                  bool firstIsNext) {
    rows.reserve(rows.size() + entries.size());
    bool first = true;
    for (auto iterator = entries.rbegin(); iterator != entries.rend();
         ++iterator) {
      CreativeDesktopHistoryEntry row;
      row.domain = CreativeDesktopHistoryDomain::Document;
      row.source = iterator->source;
      row.label = historyLabel(row.source);
      row.nextAction = first && firstIsNext;
      row.blockedByUnsynchronizedSource = !model.sourceSynchronized;
      rows.push_back(std::move(row));
      first = false;
    }
  };

  const bool sourceUndo =
      worldLayout != nullptr &&
      creativeEditorWorldLayoutSourceUndoAvailable(*worldLayout);
  const bool sourceRedo =
      worldLayout != nullptr &&
      creativeEditorWorldLayoutSourceRedoAvailable(*worldLayout);
  if (worldLayout != nullptr) {
    appendSource(model.undoEntries, worldLayout->sourceHistory.undoEntries,
                 sourceUndo);
    appendSource(model.redoEntries, worldLayout->sourceHistory.redoEntries,
                 sourceRedo);
  }
  const bool documentUndo =
      model.sourceSynchronized && cr::creativeUndoAvailable(documentHistory);
  const bool documentRedo =
      model.sourceSynchronized && cr::creativeRedoAvailable(documentHistory);
  appendDocument(model.undoEntries, documentHistory.undoSnapshots,
                 !sourceUndo && documentUndo);
  appendDocument(model.redoEntries, documentHistory.redoSnapshots,
                 !sourceRedo && documentRedo);
  model.canUndo = sourceUndo || documentUndo;
  model.canRedo = sourceRedo || documentRedo;
  return model;
}

}  // namespace iggy3d_creative_app
