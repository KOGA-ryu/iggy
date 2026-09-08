#pragma once
#include "runtime/textbook/Textbook.hpp"
#include "MatrixBoardUi.hpp"
#include <limits>
namespace paths {
struct TextbookUiState {
  bool hasPending=false;
  BookAction pending{BookActionKind::Contents};
  std::array<MatrixBoardUiState,6> boards;
  char search[128]{};
  unsigned displayedSection=std::numeric_limits<unsigned>::max();
  bool wasReading=false;
  unsigned restoreFrames=0;
  std::string message;
};
// Returns a transient request to visit the object collection.
bool drawTextbook(Textbook&,TextbookUiState&);
}
