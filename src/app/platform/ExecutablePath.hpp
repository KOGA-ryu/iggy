#pragma once

#include <filesystem>

#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

struct ExecutablePathResult {
  bool resolved = false;
  std::filesystem::path executablePath;
  std::filesystem::path executableDir;
  RenderReason reason{"executable_path_unavailable", "executable path unavailable"};
};

ExecutablePathResult resolveExecutablePath();

}  // namespace iggy3d
