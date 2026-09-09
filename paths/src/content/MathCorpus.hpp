#pragma once

#include "runtime/first_move/MathNotation.hpp"
#include "runtime/textbook/BookLesson.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace paths {
struct CorpusSubject { std::string id, title; };
struct CorpusTopic { std::string id, title; std::size_t subject=0; };
struct CorpusCitation { std::string title, url, locator; };
struct CorpusReview {
  iggy3d::first_move::NotationDefinition term;
  std::string reviewedOn, conditions, change;
  std::vector<CorpusCitation> sources;
};
struct CorpusFigure {
  std::string provider,caption;
  unsigned level=0;
  std::vector<std::pair<std::string,double>> parameters;
};
struct CorpusEntry {
  std::string id, title, kind, body, source;
  std::size_t topic=0, firstLine=0, lastLine=0;
  std::optional<CorpusReview> review;
  std::vector<std::size_t> related;
  bool document=false;
  std::optional<CorpusFigure> figure;
  std::vector<std::string> questions;
  std::vector<BookBlock> lesson;
};
// Source reading material is independent of playable questions and save evidence.
struct MathCorpus {
  std::vector<CorpusSubject> subjects;
  std::vector<CorpusTopic> topics;
  std::vector<CorpusEntry> entries;
  std::size_t reviewedCount=0;
  std::vector<std::size_t> find(std::optional<std::size_t> subject,
      std::optional<std::size_t> topic,std::string_view query) const;
};
MathCorpus parseMathCorpus(std::string_view text);
MathCorpus loadMathCorpus(const std::filesystem::path& path);
} // namespace paths
