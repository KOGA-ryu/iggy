#pragma once
#include "runtime/textbook/Textbook.hpp"
#include "MatrixBoardUi.hpp"
#include "NativeMath.hpp"
#include "TextbookFigureUi.hpp"
#include <limits>
namespace paths {
struct TextbookUiState {
  bool hasPending=false;
  BookAction pending{BookActionKind::Contents};
  std::array<MatrixBoardUiState,7> boards;
  char search[128]{};
  unsigned displayedSection=std::numeric_limits<unsigned>::max();
  bool wasReading=false;
  unsigned restoreFrames=0;
  bool showContents=true;
  std::uint64_t seenAnchorRevision=0;
  struct Equation { std::string source; NativeMath::Equation layout; };
  float equationPixels=0;
  std::vector<Equation> equations;
  std::string message;
  LessonPresentation presentation=LessonPresentation::Together;
  float readingFraction=.46f;
  TextbookFigureUiState figure;
};
// Returns a transient request to visit the object collection.
bool drawTextbook(Textbook&,TextbookUiState&,NativeMath&,SceneFrame&);
}
