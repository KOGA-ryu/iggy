#pragma once
#include "content/LearningDocuments.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathNotationUi.hpp"
#include "ui/NativeMath.hpp"
#include "ui/TextbookUi.hpp"
#include <memory>

namespace paths {
struct DocumentReadingUiState {
  std::string entry;
  std::size_t fallbacks=0;
  NotationBounds reading;
  BookReadingUiState book;
  std::vector<std::uint8_t> helpMasks;
  std::string anchor;
  float textScale=1;
};
struct DocumentLessonUiState : DocumentReadingUiState {
  std::unique_ptr<MathObjects> object;
  MathObjectScene scene;
  bool presented=false;
  NotationBounds viewport;
  std::vector<std::pair<std::string,NotationBounds>> questionLinks,parameterControls;
};
// Reading actions change presentation only; the caller records question guidance.
bool applyDocumentReadingAction(DocumentReadingUiState&,const CorpusEntry&,const BookAction&);
std::optional<BookAction> drawDocumentReading(const CorpusEntry&,NativeMath&,DocumentReadingUiState&,bool blocked);
// Formats an imported lesson and returns only explicit question-navigation intent.
std::optional<std::string> drawDocumentLesson(const CorpusEntry&,NativeMath&,DocumentLessonUiState&,bool blocked);
}
