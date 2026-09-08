#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::first_move {
inline constexpr std::size_t kNotationTermCapacity=128, kNotationLibraryCapacity=64;
inline constexpr std::size_t kNotationLessonCapacity=8, kNotationTokenCapacity=16;
struct NotationDefinition {
  std::string id, title, meaning, definition, example;
  std::uint32_t version=0;
};
struct NotationToken {
  std::string text, role;
  NotationDefinition definition; // Frozen copy resolved from one shared authored term.
};
struct NotationCheck {
  std::string prompt, correct, retry;
  std::size_t answer=0;
};
struct NotationLesson {
  std::string id, title, context, reading;
  std::uint32_t version=0;
  std::vector<NotationToken> tokens;
  std::optional<NotationCheck> check;
};
// Independent of an equation grammar or a renderer. Content supplies context;
// identical glyphs in different occurrences need not have the same meaning.
[[nodiscard]] bool validNotationDefinition(const NotationDefinition&) noexcept;
[[nodiscard]] bool validNotationLesson(const NotationLesson&) noexcept;
[[nodiscard]] bool validNotationLessons(std::span<const NotationLesson>) noexcept;
enum class NotationVerdict : std::uint8_t { Unavailable, Retry, Correct };
// Practice reads an authored example only; it cannot mutate a solving session.
[[nodiscard]] NotationVerdict checkNotation(const NotationLesson&,std::size_t token) noexcept;
} // namespace iggy3d::first_move
