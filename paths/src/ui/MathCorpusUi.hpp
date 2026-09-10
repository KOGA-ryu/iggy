#pragma once

#include "content/MathCorpus.hpp"
#include "content/CorpusPractice.hpp"
#include "ui/MathNotationUi.hpp"
#include "ui/NativeMath.hpp"
#include "ui/DocumentLessonUi.hpp"
#include <array>
#include <deque>

namespace paths {
struct CorpusTextbook {
  struct Binding {std::optional<std::size_t> entry;std::vector<std::size_t> questions;};
  std::deque<std::string> strings;
  std::deque<std::vector<BookBlock>> lessons;
  std::vector<BookSection> sections;
  std::vector<Binding> bindings;
  std::unique_ptr<Textbook> book;
};
std::unique_ptr<CorpusTextbook> makeCorpusTextbook(const MathCorpus&,std::span<const CorpusStarter>);
struct MathCorpusUiState {
  NativeMath* math=nullptr;
  CorpusPractice* practice=nullptr;
  bool open=false,importFailed=false,livePreview=false;
  std::string importMessage;
  std::uint64_t previewRevision=0;
  DocumentLessonUiState document;
  std::unique_ptr<CorpusTextbook> textbook;
  TextbookUiState textbookUi;
  std::filesystem::path bookmarkPath;
  std::string lastBookmark;
  std::array<char,iggy3d::first_move::kSupportDraftCapacity+1> supportDraft{};
  std::vector<std::size_t> questionMatches;
};
void drawMathCorpus(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked);
// Data-only reconciliation; does not initialize ImGui, fonts, or a native host.
void refreshMathCorpusPreview(MathCorpusUiState&,const MathCorpus&);
bool recordQuestionReadingHelp(MathCorpusUiState&,BookHelp);
std::optional<std::size_t> drawCorpusQuestions(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked);
} // namespace paths
