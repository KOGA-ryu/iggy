#pragma once
#include "runtime/matrix_board/MatrixBoard.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <string_view>

namespace paths {
struct BookTerm { const char* name; const char* definition; };
struct BookSection {
  const char* id;
  const char* title;
  const char* purpose;
  std::array<const char*,3> explanation;
  std::array<BookTerm,3> terms;
  const char* figurePrompt;
  const char* exampleTitle;
  std::array<const char*,3> exampleSteps;
  const char* exercisePrompt;
  const char* reference;
  unsigned card;
};
const std::array<BookSection,7>& matrixChapter();
const std::array<const char*,7>& textbookParts();
enum class BookPage { Contents, Section, Index };
enum class BookMode { Reading, Exercise };
enum class BookActionKind { OpenSection, Next, Previous, Contents, Index, Resume, Read, Exercise, RememberScroll, SetTextScale };
struct BookAction { BookActionKind kind; unsigned section=0; double value=0; };
struct BookView {
  BookPage page=BookPage::Contents;
  BookMode mode=BookMode::Reading;
  unsigned section=0;
  double scroll=0,textScale=1;
};
// Navigation and reading position belong here. Each distinct exercise keeps
// one MatrixBoard owner; reading never writes mathematical result state.
class Textbook {
public:
  Textbook();
  BoardResult dispatch(BookAction);
  BookView view()const;
  unsigned exerciseIndex()const;
  MatrixBoard& board();
  const MatrixBoard& board()const;
  std::string bookmark()const;
  BoardResult restoreBookmark(std::string_view);
private:
  BookPage page_=BookPage::Contents;
  BookMode mode_=BookMode::Reading;
  unsigned section_=0;
  double textScale_=1;
  std::array<double,7> scrolls_{};
  std::array<MatrixBoard,6> boards_;
};
// Native startup opts into these. --validate never reads or writes bookmarks.
BoardResult readTextbookBookmark(const std::filesystem::path&,Textbook&);
BoardResult writeTextbookBookmark(const std::filesystem::path&,const Textbook&);
} // namespace paths
