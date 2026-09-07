#pragma once

#include "runtime/sorter/EquationSorterSession.hpp"
#include <filesystem>

namespace paths {
// Filesystem adapter only. Question and selection owners validate restoration.
// Failed loads preserve the original file and disable replacement for this run.
class StudyProgressFile {
public:
  explicit StudyProgressFile(std::filesystem::path path);
  void load(EquationSorterSession&);
  void save(const EquationSorterSession&);
  [[nodiscard]] const std::string& message() const { return message_; }
  [[nodiscard]] bool failed() const { return failed_; }
private:
  std::filesystem::path path_;
  std::optional<std::string> disk_;
  std::optional<std::uint64_t> checkedRevision_;
  std::string message_;
  bool blocked_=false, failed_=false;
};
} // namespace paths
