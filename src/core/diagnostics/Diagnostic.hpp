#pragma once

#include <cstdint>
#include <string>

namespace iggy3d {

enum class DiagnosticSeverity : std::uint8_t {
  Info,
  Warning,
  Error,
};

enum class DiagnosticDomain : std::uint8_t {
  Core,
  Content,
  Runtime,
  Command,
  Save,
  Replay,
  App,
};

struct DiagnosticLocation {
  std::string file;
  std::uint32_t line = 0;
  std::uint32_t column = 0;
};

struct Diagnostic {
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
  DiagnosticDomain domain = DiagnosticDomain::Core;
  std::string code;
  std::string message;
  DiagnosticLocation location;
};

inline Diagnostic makeDiagnostic(
    DiagnosticDomain domain,
    DiagnosticSeverity severity,
    std::string code,
    std::string message,
    DiagnosticLocation location = {}) {
  return Diagnostic{severity, domain, std::move(code), std::move(message), std::move(location)};
}

inline bool hasLocation(const Diagnostic& diagnostic) {
  return !diagnostic.location.file.empty() || diagnostic.location.line != 0 ||
         diagnostic.location.column != 0;
}

}  // namespace iggy3d
