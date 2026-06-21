#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

enum class ResultStatus : std::uint8_t { Ok, Error };

struct ErrorInfo {
  std::string code;
  std::string message;
};

template <typename T>
struct Result {
  ResultStatus status = ResultStatus::Error;
  T value{};
  ErrorInfo error{};
};

struct StatusResult {
  ResultStatus status = ResultStatus::Error;
  ErrorInfo error{};
};

}  // namespace iggy3d
