#pragma once
#include "runtime/matrix_board/MatrixBoard.hpp"
#include "SystemLesson.hpp"
#include <array>
#include <filesystem>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace paths {
struct BookTerm { const char* name; const char* definition; const char* blockId=""; };
enum class BookBlockKind { Introduction, Definition, Proposition, Example, Figure, Exercise, Summary };
enum class BookHelp : unsigned { Proof, Hint, Answer, Solution, Count };
struct BookPassage {
  enum class Kind { Prose, DisplayMath } kind=Kind::Prose;
  std::string text;
  const char* number="";
};
struct BookReference { const char* label; const char* target; };
struct BookBlock {
  const char* id;
  BookBlockKind kind;
  const char* number;
  const char* title;
  std::vector<BookPassage> body;
  std::array<std::vector<BookPassage>,static_cast<unsigned>(BookHelp::Count)> help{};
  std::vector<BookReference> references{};
};
struct BookHelpView {
  bool available=false,open=false;
  std::span<const BookPassage> passages;
};
struct BookBlockView {
  const char* id;
  BookBlockKind kind;
  const char* number;
  const char* title;
  std::span<const BookPassage> body;
  std::array<BookHelpView,static_cast<unsigned>(BookHelp::Count)> help;
  std::span<const BookReference> references;
};
const std::vector<BookBlock>& rrefLesson();
const std::vector<BookBlock>& systemsLesson();
enum class BookFigureKind { None, RowPlanes, AffinePlanes };
struct BookFigureSpec {
  BookFigureKind kind=BookFigureKind::None;
  const char* id="";
  const char* title="";
  const char* caption="";
};
struct BookSection {
  const char* id;
  const char* title;
  const char* purpose;
  std::vector<const char*> explanation;
  std::vector<BookTerm> terms;
  const char* figurePrompt;
  const char* exampleTitle;
  std::vector<const char*> exampleSteps;
  const char* exercisePrompt;
  const char* reference;
  unsigned card;
  std::span<const BookBlock> lesson{};
  BookFigureSpec figure{};
};
const std::array<BookSection,8>& matrixChapter();
const std::array<const char*,7>& textbookParts();
enum class BookPage { Contents, Section, Index };
enum class BookMode { Reading, Exercise };
enum class BookActionKind { OpenSection, Next, Previous, Contents, Index, Resume, Read, Exercise, RememberScroll, SetTextScale, OpenBlock, ToggleHelp };
struct BookAction {
  BookActionKind kind;
  unsigned section=0;
  double value=0;
  std::string_view target{};
  BookHelp help=BookHelp::Proof;
};
struct BookView {
  BookPage page=BookPage::Contents;
  BookMode mode=BookMode::Reading;
  unsigned section=0;
  double scroll=0,textScale=1;
  std::string_view anchor;
  std::uint64_t anchorRevision=0;
};
// Navigation and reading position belong here. Each distinct exercise keeps
// one MatrixBoard owner; reading never writes mathematical result state.
class Textbook {
public:
  Textbook();
  BoardResult dispatch(BookAction);
  BookView view()const;
  // Only open disclosures in the current reading section publish their text.
  std::vector<BookBlockView> lessonView()const;
  unsigned exerciseIndex()const;
  SystemLesson& systems(){return systems_;}
  const SystemLesson& systems()const{return systems_;}
  MatrixBoard& board();
  const MatrixBoard& board()const;
  std::string bookmark()const;
  BoardResult restoreBookmark(std::string_view);
private:
  BookPage page_=BookPage::Contents;
  BookMode mode_=BookMode::Reading;
  unsigned section_=0;
  double textScale_=1;
  std::array<double,8> scrolls_{};
  std::array<MatrixBoard,6> boards_;
  SystemLesson systems_;
  std::array<std::vector<std::uint8_t>,8> helpMasks_;
  std::string_view anchor_;
  std::uint64_t anchorRevision_=0;
};
// Native startup opts into these. --validate never reads or writes bookmarks.
BoardResult readTextbookBookmark(const std::filesystem::path&,Textbook&);
BoardResult writeTextbookBookmark(const std::filesystem::path&,const Textbook&);
} // namespace paths
