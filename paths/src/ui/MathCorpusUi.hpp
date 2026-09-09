#pragma once

#include "content/MathCorpus.hpp"
#include "content/CorpusPractice.hpp"
#include "ui/MathNotationUi.hpp"
#include "ui/NativeMath.hpp"
#include "ui/DocumentLessonUi.hpp"
#include <array>

namespace paths {
enum class CorpusControl : std::size_t { Practice, Subject, Topic, Search, Clear, Previous, Next, Up, Down, Source, Back, Equations, Format, Questions, NextStarter, ReplayStarter, FocusQuestion, Method, CheckWork, UndoWork, OpenDocument, TypeAnswer, Count };
struct NativeMathSample {const char* label;const char* title;const char* latex;};
inline constexpr std::array<NativeMathSample,4> nativeMathSamples{{
  {"Fraction","A fraction",R"(\frac{a+b}{c+d})"},
  {"Root","A nested root",R"(\sqrt{1+\sqrt{1+x^{2}}})"},
  {"Matrix","A matrix with a fractional entry",R"(A=\begin{bmatrix}1&\frac{1}{2}&0\\-2&3&1\\0&-1&4\end{bmatrix})"},
  {"095","095 · Eigenmodes of a lopsided drum",R"(\Delta u=-\lambda^{2}\left(1+\frac{x}{2}\right)u)"}
}};
enum class MathPanelControl : std::size_t { Smaller,Larger,Source,Close,Count };
struct NativeMathPanelState {
  bool open=false,source=false;
  std::size_t sample=0;
  int pixels=24;
  std::array<NotationBounds,4> samples{};
  std::array<NotationBounds,static_cast<std::size_t>(MathPanelControl::Count)> controls{};
  NotationBounds viewport,ink;
  float horizontalScrollMax=0;
  std::string error;
};
struct CorpusBookmark { std::size_t entry; float scroll; bool original; bool raw=false; float horizontal=0; };
struct MathCorpusUiState {
  NativeMath* math=nullptr;
  CorpusPractice* practice=nullptr;
  bool questions=false;
  bool focusQuestion=false,readingOpen=false;
  bool focusDocument=false;
  std::string importMessage;
  bool importFailed=false;
  DocumentLessonUiState document;
  std::size_t readingIndex=0;
  std::string readingQuestion,readingSource;
  std::size_t readingFallbacks=0;
  NotationBounds currentWorking,readingBody;
  std::vector<NotationBounds> readingChoices;
  std::array<NotationBounds,4> supportLevels{},supportHelp{};
  std::array<char,iggy3d::first_move::kSupportDraftCapacity+1> supportDraft{};
  NotationBounds supportEditor;
  std::vector<std::size_t> questionMatches;
  std::vector<NotationBounds> answerTiles;
  NotationBounds questionInk,choiceArea;
  NativeMathPanelState equations;
  bool open=false, refresh=true, top=true, follow=true;
  std::optional<std::size_t> subject, topic, entry;
  std::array<char,128> query{};
  std::vector<std::size_t> matches;
  float scroll=0, scrollMax=0, pageHeight=0;
  std::array<NotationBounds,static_cast<std::size_t>(CorpusControl::Count)> controls{};
  std::vector<std::pair<std::size_t,NotationBounds>> rows;
  std::vector<std::pair<std::size_t,NotationBounds>> subjectRows, topicRows;
  std::vector<std::pair<std::size_t,NotationBounds>> relatedRows;
  std::vector<CorpusBookmark> trail;
  std::optional<float> restoreScroll;
  std::optional<float> restoreHorizontal;
  NotationBounds list, reader, body;
  float horizontal=0,horizontalMax=0;
  std::size_t bodyEquations=0,bodyFallbacks=0;
  std::optional<std::size_t> shown;
  bool original=false, raw=false, reviewedVisible=false, focusReader=false;
};
void drawMathCorpus(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked);
void drawCorpusQuestions(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked);
} // namespace paths
