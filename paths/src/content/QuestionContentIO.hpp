#pragma once

#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/first_move/LayeredQuestionSession.hpp"

namespace paths {
// Source plus JSON pointer; an empty field identifies the document itself.
class QuestionContentError : public std::runtime_error {
public:
  QuestionContentError(std::filesystem::path source, std::string field, std::string reason);
  const std::filesystem::path source;
  const std::string field;
};

struct QuestionPack {
  std::filesystem::path source;
  std::vector<iggy3d::first_move::LayeredQuestionContent> catalog;
  std::map<std::string, std::vector<std::size_t>, std::less<>> decks;
  [[nodiscard]] const std::vector<std::size_t>& deck(std::string_view mode) const;
};

// Decode prepared instructions, then use the shared ArcadeCollect structural
// checks. No judgments, attempts, progression or file watching live here.
[[nodiscard]] iggy3d::first_move::LayeredQuestionContent parseQuestionContent(
    std::string_view json, const std::filesystem::path& sourcePath,
    std::span<const iggy3d::first_move::MathReference> references={},
    std::span<const iggy3d::first_move::NotationLesson> notation={});
[[nodiscard]] QuestionPack loadQuestionPack(const std::filesystem::path& path);
}  // namespace paths
