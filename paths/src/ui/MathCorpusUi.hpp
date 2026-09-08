#pragma once

#include "content/MathCorpus.hpp"
#include "ui/MathNotationUi.hpp"
#include <array>

namespace paths {
enum class CorpusControl : std::size_t { Practice, Subject, Topic, Search, Clear, Previous, Next, Up, Down, Source, Back, Count };
struct CorpusBookmark { std::size_t entry; float scroll; bool original; };
struct MathCorpusUiState {
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
  NotationBounds list, reader;
  std::optional<std::size_t> shown;
  bool original=false, reviewedVisible=false, focusReader=false;
};
void drawMathCorpus(MathCorpusUiState& ui,const MathCorpus& corpus,bool blocked);
} // namespace paths
