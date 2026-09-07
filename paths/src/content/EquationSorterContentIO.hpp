#pragma once

#include <filesystem>
#include <stdexcept>
#include <string_view>

#include "runtime/sorter/EquationSorterSession.hpp"

namespace paths {
class EquationSorterContentError : public std::runtime_error {
public:
  EquationSorterContentError(std::filesystem::path source, std::string field, std::string reason);
  const std::filesystem::path source;
  const std::string field;
};
[[nodiscard]] std::vector<SorterEquation> parseSorterContent(
    std::string_view text, const std::filesystem::path& source);
[[nodiscard]] std::vector<SorterEquation> loadSorterContent(const std::filesystem::path& path);
} // namespace paths
