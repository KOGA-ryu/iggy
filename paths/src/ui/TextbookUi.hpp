#pragma once
#include "runtime/textbook/Textbook.hpp"
#include "MatrixBoardUi.hpp"
#include "NativeMath.hpp"
#include "TextbookFigureUi.hpp"
#include <limits>
#include <functional>
#include <optional>
#include <initializer_list>
namespace paths {
struct TextCopyOption { const char* label; std::string_view text; };
// Attach to the last submitted item, using an ID unique within its UI scope.
// Copy retains source bytes; equation text is LaTeX, never pixels.
void drawTextCopyMenu(const char* id,std::initializer_list<TextCopyOption>);
// Source selection from existing document placements, without typesetting.
TextCopyOption documentCopyText(const NativeMath::Document&,std::size_t placement);
void drawDocumentCopyMenu(const NativeMath::Document&,float x,float y);
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
// Content adapters use the complete native textbook screen. They supply content
// and exercise/figure owners, never a second page layout.
struct TextbookContentUi {
  const char* exitLabel="Practice";
  std::function<void(unsigned,float)> reading,exercise;
  std::function<void(unsigned,SceneViewport,float)> figure;
  std::function<void(const BookAction&)> action;
};
// One block renderer for compiled textbook sections and imported lessons.
std::optional<BookAction> drawBookBlock(const BookBlockView&,BookReadingUiState&,NativeMath&,float width,
    const std::function<void()>& figure={});
// Returns a transient request to visit the object collection.
bool drawTextbook(Textbook&,TextbookUiState&,NativeMath&,SceneFrame&,const TextbookContentUi* content=nullptr);
}
