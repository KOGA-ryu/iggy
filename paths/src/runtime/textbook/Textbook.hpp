#pragma once
#include "runtime/matrix_board/MatrixBoard.hpp"
#include "SystemLesson.hpp"
#include "ObjectLesson.hpp"
#include "BookLesson.hpp"
#include <array>
#include <filesystem>
#include <cstdint>
#include <span>
#include <memory>
#include <string>
#include <string_view>

namespace paths {
struct BookTerm { const char* name; const char* definition; const char* blockId=""; };
const std::vector<BookBlock>& rrefLesson();
const std::vector<BookBlock>& systemsLesson();
const std::vector<BookBlock>& determinantLesson();
enum class BookFigureKind { None, RowPlanes, AffinePlanes, Object };
enum class BookExerciseKind { MatrixBoard, Systems, Object, External };
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
  BookExerciseKind exercise=BookExerciseKind::MatrixBoard;
  const ObjectLessonSpec* object=nullptr;
  // Native bookmarks require complete catalogue generations. Imported books
  // store an explicit saved row count and reconcile additions by stable ID.
  unsigned bookmarkGeneration=1;
  const char* part="IV. Linear Algebra";
  const char* chapter="Chapter 1 / Matrices and Elimination";
};
BookSection determinantSection();
std::span<const BookSection> matrixChapter();
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
// one domain owner; reading never writes mathematical result state.
class Textbook {
public:
  explicit Textbook(std::span<const BookSection> sections={});
  std::span<const BookSection> sections()const{return sections_;}
  bool nativeCatalogue()const{return nativeCatalogue_;}
  void retainReading(const Textbook& previous);
  BoardResult dispatch(BookAction);
  BookView view()const;
  // Only open disclosures in the current reading section publish their text.
  std::vector<BookBlockView> lessonView()const;
  unsigned exerciseIndex()const;
  BookExerciseKind exerciseKind()const{return sections_[section_].exercise;}
  bool hasBoard()const{return exerciseKind()==BookExerciseKind::MatrixBoard || exerciseKind()==BookExerciseKind::Systems;}
  ObjectLesson& objectLesson();
  const ObjectLesson& objectLesson()const;
  SystemLesson& systems(){return systems_;}
  const SystemLesson& systems()const{return systems_;}
  MatrixBoard& board();
  const MatrixBoard& board()const;
  std::string bookmark()const;
  BoardResult restoreBookmark(std::string_view);
private:
  std::span<const BookSection> sections_;
  bool nativeCatalogue_=true;
  BookPage page_=BookPage::Contents;
  BookMode mode_=BookMode::Reading;
  unsigned section_=0;
  double textScale_=1;
  std::vector<double> scrolls_;
  std::array<MatrixBoard,6> boards_;
  SystemLesson systems_;
  std::vector<std::vector<std::uint8_t>> helpMasks_;
  std::vector<std::unique_ptr<ObjectLesson>> objects_;
  std::string_view anchor_;
  std::uint64_t anchorRevision_=0;
};
// Native startup opts into these. --validate never reads or writes bookmarks.
BoardResult readTextbookBookmark(const std::filesystem::path&,Textbook&);
BoardResult writeTextbookBookmark(const std::filesystem::path&,const Textbook&);
} // namespace paths
