#pragma once
#include "runtime/motion/MotionLesson.hpp"
#include <filesystem>

namespace paths {
class MotionProgressFile {
public:
  explicit MotionProgressFile(std::filesystem::path path):path_(std::move(path)){}
  void load(MotionLesson&);
  void save(const MotionLesson&,bool closing=false);
  [[nodiscard]] std::string_view message() const { return message_; }
  [[nodiscard]] bool failed() const { return blocked_; }
private:
  std::filesystem::path path_;
  std::optional<std::string> disk_;
  std::optional<std::size_t> savedRevision_;
  std::string message_;
  bool blocked_=false;
};
} // namespace paths
