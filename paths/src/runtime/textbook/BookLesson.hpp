#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace paths {
enum class BookBlockKind { Introduction, Definition, Proposition, Example, Figure, Exercise, Summary };
enum class BookHelp : unsigned { Proof, Hint, Answer, Solution, Count };
struct BookPassage {
  enum class Kind { Prose, DisplayMath } kind=Kind::Prose;
  std::string text;
  std::string number;
};
struct BookReference { std::string label,target; };
struct BookBlock {
  std::string id;
  BookBlockKind kind;
  std::string number,title;
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
// Both authored and imported lessons expose only the disclosures the reader opened.
std::vector<BookBlockView> bookLessonView(std::span<const BookBlock>,std::span<const std::uint8_t> helpMasks);
}
