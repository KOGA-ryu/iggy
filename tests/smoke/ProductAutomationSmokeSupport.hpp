#pragma once

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <string_view>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#endif

namespace iggy3d::smoke {

using ReceiptFields = std::map<std::string, std::string>;

inline std::filesystem::path productAppBinary() {
#if defined(IGGY3D_PRODUCT_APP_PATH)
  return std::filesystem::path{IGGY3D_PRODUCT_APP_PATH};
#else
  return {};
#endif
}

inline bool productAppAvailable(const std::filesystem::path& binary) {
  return !binary.empty() && std::filesystem::exists(binary);
}

inline std::string shellQuote(const std::filesystem::path& path) {
  std::string value = path.string();
  std::string quoted = "'";
  for (const char character : value) {
    if (character == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(character);
    }
  }
  quoted += "'";
  return quoted;
}

inline int exitCodeFromSystem(int status) {
  if (status == -1) {
    return 1;
  }
#if defined(__unix__) || defined(__APPLE__)
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return 1;
#else
  return status;
#endif
}

inline std::string uniqueCaseToken(std::string_view name) {
  static unsigned long long counter = 0ULL;
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return std::string(name) + "_" + std::to_string(now) + "_" +
         std::to_string(++counter);
}

inline bool writeTextFile(const std::filesystem::path& path,
                          std::string_view content) {
  std::ofstream output(path);
  if (!output) {
    return false;
  }
  output << content;
  return static_cast<bool>(output);
}

inline bool parseReceiptFile(const std::filesystem::path& path,
                             ReceiptFields& fields) {
  fields.clear();
  std::ifstream input(path);
  if (!input) {
    return false;
  }
  std::string line;
  while (std::getline(input, line)) {
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos || equals == 0U) {
      return false;
    }
    if (!fields.emplace(line.substr(0, equals), line.substr(equals + 1U)).second) {
      return false;
    }
  }
  return true;
}

inline bool hasField(const ReceiptFields& fields,
                     std::string_view key,
                     std::string_view value) {
  const auto found = fields.find(std::string(key));
  return found != fields.end() && found->second == value;
}

inline bool positiveIntegerField(const ReceiptFields& fields,
                                 std::string_view key) {
  const auto found = fields.find(std::string(key));
  if (found == fields.end() || found->second.empty()) {
    return false;
  }
  unsigned long long value = 0ULL;
  for (const char character : found->second) {
    if (character < '0' || character > '9') {
      return false;
    }
    value = value * 10ULL + static_cast<unsigned long long>(character - '0');
  }
  return value > 0ULL;
}

inline bool runProductCase(const std::filesystem::path& binary,
                           std::string_view name,
                           std::string_view controlText,
                           std::string_view extraArgs,
                           ReceiptFields& fields,
                           int& exitCode) {
  const std::string token = uniqueCaseToken(name);
  const std::filesystem::path control =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_automation_" + token + ".in");
  const std::filesystem::path output =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_automation_" + token + ".out");
  if (!writeTextFile(control, controlText)) {
    return false;
  }

  std::string command = shellQuote(binary) + " --no-window";
  if (!extraArgs.empty()) {
    command += " ";
    command += extraArgs;
  }
  command += " --automation-control " + shellQuote(control) +
             " --print-render-receipt > " + shellQuote(output);
  exitCode = exitCodeFromSystem(std::system(command.c_str()));
  return parseReceiptFile(output, fields);
}

inline bool runProductReceiptCase(const std::filesystem::path& binary,
                                  std::string_view name,
                                  std::string_view extraArgs,
                                  ReceiptFields& fields,
                                  int& exitCode) {
  const std::string token = uniqueCaseToken(name);
  const std::filesystem::path output =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_receipt_" + token + ".out");

  std::string command = shellQuote(binary) + " --no-window";
  if (!extraArgs.empty()) {
    command += " ";
    command += extraArgs;
  }
  command += " --print-render-receipt > " + shellQuote(output);
  exitCode = exitCodeFromSystem(std::system(command.c_str()));
  return parseReceiptFile(output, fields);
}

inline std::filesystem::path cleanSaveRoot(std::string_view name) {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() /
      ("iggy3d_product_automation_" + uniqueCaseToken(name) + "_saves");
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  return root;
}

inline std::string saveRootArg(const std::filesystem::path& root) {
  return std::string{"--save-root "} + shellQuote(root);
}

inline bool productReceipt(const ReceiptFields& fields) {
  return hasField(fields, "app", "iggy3d") &&
         !hasField(fields, "app", "iggy3d_visual_demo") &&
         hasField(fields, "result", "pass");
}

inline bool automationApplied(const ReceiptFields& fields) {
  return hasField(fields, "automation_control_requested", "true") &&
         hasField(fields, "automation_control_loaded", "true") &&
         hasField(fields, "automation_control_status", "applied") &&
         hasField(fields, "automation_control_scope", "frontend_menu");
}

inline bool automationCommandFailed(const ReceiptFields& fields,
                                    std::string_view key) {
  return hasField(fields, "automation_control_loaded", "true") &&
         hasField(fields, "automation_control_status", "command_failed") &&
         hasField(fields, "automation_control_last_key", key) &&
         hasField(fields, "automation_control_last_result", "failed");
}

inline bool seedWorldSave(const std::filesystem::path& binary,
                          std::string_view caseName,
                          const std::filesystem::path& saveRoot,
                          std::string_view title = "New World") {
  ReceiptFields fields;
  int exitCode = 77;
  std::string control = "frontend.select=new_world\nfrontend.execute=true\n";
  if (!title.empty()) {
    control += "world.title=";
    control += title;
    control += "\n";
  }
  control += "world.create=true\n";
  return runProductCase(binary, caseName, control, saveRootArg(saveRoot), fields,
                        exitCode) &&
         exitCode == 0 && productReceipt(fields) && automationApplied(fields) &&
         hasField(fields, "frontend_screen", "gameplay") &&
         hasField(fields, "gameplay_active", "true") &&
         hasField(fields, "world_creation_status",
                  "world_creation_initial_save_written") &&
         hasField(fields, "world_creation_initial_save_id", "save_001") &&
         std::filesystem::exists(saveRoot / "save_001.iggy3d.save");
}

}  // namespace iggy3d::smoke
