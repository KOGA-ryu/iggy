#pragma once
#include "content/MathCorpus.hpp"
#include "runtime/first_move/LayeredQuestionSession.hpp"
#include <chrono>
#include <memory>

namespace paths {
struct CorpusStarter {
  std::string id,title,level,stamp;
  std::size_t subject=0;
  std::optional<std::size_t> topic;
  std::vector<std::string> readingRefs;
  iggy3d::first_move::LayeredQuestionContent question;
};
std::vector<CorpusStarter> loadCorpusStarters(const std::filesystem::path&,const MathCorpus&);
std::vector<CorpusStarter> parseCorpusStarters(std::string_view,const MathCorpus&,const std::filesystem::path& source);

// An unbounded-by-the-old-sorter reading collection of independently frozen
// questions. All answer judgments, working and attempt evidence belong to the
// existing LayeredQuestionSession; this adapter only selects and persists it.
class CorpusPractice {
public:
  explicit CorpusPractice(std::vector<CorpusStarter> questions);
  const std::vector<CorpusStarter>& questions() const { return questions_; }
  std::vector<std::size_t> find(std::optional<std::size_t> subject,
      std::optional<std::size_t> topic,std::string_view query) const;
  void open(std::size_t index);
  std::optional<std::size_t> selected() const { return selected_; }
  iggy3d::first_move::LayeredQuestionSession* active();
  const iggy3d::first_move::LayeredQuestionSession* attempt(std::size_t index) const;
  bool dispatch(const iggy3d::first_move::LayeredQuestionCommand&);
  void loadProgress(const std::filesystem::path&);
  void saveProgress(bool closing=false);
  const std::string& message() const { return message_; }
private:
  std::vector<CorpusStarter> questions_;
  std::vector<std::optional<iggy3d::first_move::LayeredQuestionSession>> attempts_;
  std::optional<std::size_t> selected_;
  std::filesystem::path progressPath_;
  std::optional<std::string> disk_;
  bool dirty_=false,loadBlocked_=false;
  std::chrono::steady_clock::time_point retryAfter_{};
  std::string message_;
};
}
