#pragma once
#include "content/LearningDocuments.hpp"
#include "scene/MathObjectScene.hpp"
#include "ui/MathNotationUi.hpp"
#include "ui/TextbookUi.hpp"
#include <memory>

namespace paths {
struct DocumentLessonUiState {
  std::string entry;
  std::unique_ptr<MathObjects> object;
  MathObjectScene scene;
  bool presented=false;
  NotationBounds viewport;
  std::vector<std::pair<std::string,NotationBounds>> parameterControls;
};
// Supplies a registered figure to the textbook's existing figure regions.
void drawDocumentFigure(const CorpusEntry&,DocumentLessonUiState&,SceneViewport,float textScale,bool blocked);
}
