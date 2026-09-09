#pragma once
#include "content/LearningDocuments.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathNotationUi.hpp"
#include "ui/NativeMath.hpp"
#include "ui/TextbookUi.hpp"
#include <memory>

namespace paths {
struct DocumentLessonUiState {
  std::string entry;
  std::unique_ptr<MathObjects> object;
  MathObjectScene scene;
  bool presented=false;
  std::size_t fallbacks=0;
  NotationBounds reading,viewport;
  std::vector<std::pair<std::string,NotationBounds>> questionLinks,parameterControls;
  BookReadingUiState book;
  std::vector<std::uint8_t> helpMasks;
  std::string anchor;
  float textScale=1;
};
// Formats an imported lesson and returns only explicit question-navigation intent.
std::optional<std::string> drawDocumentLesson(const CorpusEntry&,NativeMath&,DocumentLessonUiState&,bool blocked);
}
