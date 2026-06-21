#include "app/platform/ExecutablePath.hpp"

#include <system_error>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace iggy3d {
namespace {

RenderReason executablePathReason(std::string_view code) {
  if (code == "executable_path_ok") {
    return {code, "executable path ok"};
  }
  if (code == "executable_path_buffer_too_small") {
    return {code, "executable path buffer too small"};
  }
  if (code == "executable_path_canonicalize_failed") {
    return {code, "executable path canonicalize failed"};
  }
  return {"executable_path_unavailable", "executable path unavailable"};
}

ExecutablePathResult makeFailure(std::string_view code) {
  ExecutablePathResult result;
  result.reason = executablePathReason(code);
  return result;
}

ExecutablePathResult makeSuccess(std::filesystem::path path) {
  std::error_code error;
  std::filesystem::path absolute = std::filesystem::absolute(path, error);
  if (error) {
    return makeFailure("executable_path_canonicalize_failed");
  }
  absolute = std::filesystem::weakly_canonical(absolute, error);
  if (error || absolute.empty()) {
    return makeFailure("executable_path_canonicalize_failed");
  }

  ExecutablePathResult result;
  result.resolved = true;
  result.executablePath = absolute;
  result.executableDir = absolute.parent_path();
  result.reason = executablePathReason("executable_path_ok");
  return result;
}

}  // namespace

ExecutablePathResult resolveExecutablePath() {
#if defined(__APPLE__)
  std::uint32_t size = 0U;
  const int sizeProbe = _NSGetExecutablePath(nullptr, &size);
  if (sizeProbe != -1 || size == 0U) {
    return makeFailure("executable_path_unavailable");
  }
  std::vector<char> buffer(size + 1U, '\0');
  if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
    return makeFailure("executable_path_buffer_too_small");
  }
  return makeSuccess(std::filesystem::path{buffer.data()});
#elif defined(__linux__)
  std::vector<char> buffer(4096U, '\0');
  const ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1U);
  if (size <= 0 || static_cast<std::size_t>(size) >= buffer.size() - 1U) {
    return makeFailure("executable_path_unavailable");
  }
  buffer[static_cast<std::size_t>(size)] = '\0';
  return makeSuccess(std::filesystem::path{buffer.data()});
#elif defined(_WIN32)
  std::vector<wchar_t> buffer(32768U, L'\0');
  const DWORD size =
      GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if (size == 0U) {
    return makeFailure("executable_path_unavailable");
  }
  if (size >= buffer.size()) {
    return makeFailure("executable_path_buffer_too_small");
  }
  buffer[static_cast<std::size_t>(size)] = L'\0';
  return makeSuccess(std::filesystem::path{buffer.data()});
#else
  return makeFailure("executable_path_unavailable");
#endif
}

}  // namespace iggy3d
