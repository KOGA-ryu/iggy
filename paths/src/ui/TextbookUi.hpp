#pragma once
#include "runtime/textbook/Textbook.hpp"
#include "MatrixBoardUi.hpp"
#include "NativeMath.hpp"
#include "TextbookFigureUi.hpp"
#include <limits>
#include <functional>
#include <optional>
namespace paths {
struct BookReadingUiState {
  struct Equation { std::string source; NativeMath::Equation layout; };
  float equationPixels=0;
  std::vector<Equation> equations;
  std::size_t fallbacks=0;
};
struct TextbookUiState : BookReadingUiState {
  bool hasPending=false;
  BookAction pending{BookActionKind::Contents};
  std::vector<MatrixBoardUiState> boards=std::vector<MatrixBoardUiState>(matrixCards().size()+1);
  char search[128]{};
  unsigned displayedSection=std::numeric_limits<unsigned>::max();
  bool wasReading=false;
  unsigned restoreFrames=0;
  bool showContents=true;
  std::uint64_t seenAnchorRevision=0;
  std::string message;
  LessonPresentation presentation=LessonPresentation::Together;
  float readingFraction=.46f;
  TextbookFigureUiState figure;
};
// One block renderer for compiled textbook sections and imported lessons.
std::optional<BookAction> drawBookBlock(const BookBlockView&,BookReadingUiState&,NativeMath&,float width,
    const std::function<void()>& figure={});
// Returns a transient request to visit the object collection.
bool drawTextbook(Textbook&,TextbookUiState&,NativeMath&,SceneFrame&);
}
