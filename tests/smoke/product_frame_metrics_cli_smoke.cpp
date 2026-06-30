#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#endif

namespace {

std::string shellQuote(const std::filesystem::path& path) {
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

int exitCodeFromSystem(int status) {
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

bool readTextFile(const std::filesystem::path& path, std::string& text) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  text.assign(std::istreambuf_iterator<char>(in),
              std::istreambuf_iterator<char>());
  return true;
}

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

bool runCommand(const std::filesystem::path& binary,
                const std::string& args,
                const std::filesystem::path& stdoutPath,
                const std::filesystem::path& stderrPath,
                int& exitCode) {
  const std::string command = shellQuote(binary) + " " + args + " > " +
                              shellQuote(stdoutPath) + " 2> " +
                              shellQuote(stderrPath);
  exitCode = exitCodeFromSystem(std::system(command.c_str()));
  return true;
}

bool runShellCommand(const std::string& command,
                     const std::filesystem::path& stdoutPath,
                     const std::filesystem::path& stderrPath,
                     int& exitCode) {
  const std::string redirected = command + " > " + shellQuote(stdoutPath) +
                                 " 2> " + shellQuote(stderrPath);
  exitCode = exitCodeFromSystem(std::system(redirected.c_str()));
  return true;
}

bool suiteSmoke(const std::filesystem::path& binary) {
  const std::filesystem::path stdoutPath =
      "/tmp/iggy3d_product_frame_metrics_suite.json";
  const std::filesystem::path stderrPath =
      "/tmp/iggy3d_product_frame_metrics_suite.err";
  int exitCode = 1;
  std::string text;
  runCommand(binary,
             "--scenario all --frames 4 --debug-overlay both --no-timing",
             stdoutPath,
             stderrPath,
             exitCode);
  return exitCode == 0 && readTextFile(stdoutPath, text) &&
         contains(text, "\"schema\": \"iggy3d.product_frame_metrics.v1\"") &&
         contains(text, "\"scenario_count\": 4") &&
         contains(text, "\"scenario\": \"default_gameplay\"") &&
         contains(text, "\"scenario\": \"movement_wall_run_corridor\"") &&
         contains(text, "\"debug_overlay\": true") &&
         contains(text, "\"debug_overlay\": false") &&
         contains(text, "\"draw_item_count_max\":") &&
         contains(text, "\"collision_surface_count\":") &&
         contains(text, "\"wall_run_active_frame_count\":") &&
         contains(text, "\"total_ns\": 0");
}

bool wallRunSmoke(const std::filesystem::path& binary) {
  const std::filesystem::path stdoutPath =
      "/tmp/iggy3d_product_frame_metrics_wallrun.json";
  const std::filesystem::path stderrPath =
      "/tmp/iggy3d_product_frame_metrics_wallrun.err";
  int exitCode = 1;
  std::string text;
  runCommand(binary,
             "--scenario movement_wall_run_corridor --frames 12 --no-timing",
             stdoutPath,
             stderrPath,
             exitCode);
  return exitCode == 0 && readTextFile(stdoutPath, text) &&
         contains(text, "\"scenario_count\": 1") &&
         contains(text, "\"room_id\": \"movement_wall_run_corridor\"") &&
         contains(text, "\"active_surface\": \"gameplay\"") &&
         contains(text, "\"input_owner\": \"gameplay\"") &&
         contains(text, "\"gameplay_input_suppressed\": false") &&
         contains(text, "\"wall_run_candidate_frame_count\":") &&
         contains(text, "\"wall_run_active_frame_count\":") &&
         contains(text, "\"wall_run_status_last\":");
}

bool outputFileSmoke(const std::filesystem::path& binary) {
  const std::filesystem::path stdoutPath =
      "/tmp/iggy3d_product_frame_metrics_output_stdout.txt";
  const std::filesystem::path stderrPath =
      "/tmp/iggy3d_product_frame_metrics_output.err";
  const std::filesystem::path outputPath =
      "/tmp/iggy3d_product_frame_metrics_output.json";
  std::filesystem::remove(outputPath);

  int exitCode = 1;
  std::string text;
  runCommand(binary,
             "--scenario default_gameplay --frames 2 --no-timing --output " +
                 shellQuote(outputPath),
             stdoutPath,
             stderrPath,
             exitCode);
  return exitCode == 0 && std::filesystem::exists(outputPath) &&
         readTextFile(outputPath, text) &&
         contains(text, "\"scenario\": \"default_gameplay\"") &&
         contains(text, "\"setup_ns\": 0") &&
         contains(text, "\"projection_or_frame_build_ns\": 0");
}

bool reportToolSmoke(const std::filesystem::path& binary) {
  const std::filesystem::path suitePath =
      "/tmp/iggy3d_product_frame_metrics_report_suite.json";
  const std::filesystem::path singlePath =
      "/tmp/iggy3d_product_frame_metrics_report_single.json";
  const std::filesystem::path stdoutPath =
      "/tmp/iggy3d_product_frame_metrics_report_stdout.txt";
  const std::filesystem::path stderrPath =
      "/tmp/iggy3d_product_frame_metrics_report_stderr.txt";
  int exitCode = 1;
  std::string text;

  runCommand(binary,
             "--scenario all --frames 4 --debug-overlay both --no-timing "
             "--output " +
                 shellQuote(suitePath),
             stdoutPath,
             stderrPath,
             exitCode);
  if (exitCode != 0 || !std::filesystem::exists(suitePath)) {
    return false;
  }

  runCommand(binary,
             "--scenario movement_wall_run_corridor --frames 4 --no-timing "
             "--output " +
                 shellQuote(singlePath),
             stdoutPath,
             stderrPath,
             exitCode);
  if (exitCode != 0 || !std::filesystem::exists(singlePath)) {
    return false;
  }

  runShellCommand("python3 tools/product_frame_metrics_report.py " +
                      shellQuote(suitePath),
                  stdoutPath,
                  stderrPath,
                  exitCode);
  if (exitCode != 0 || !readTextFile(stdoutPath, text) ||
      !contains(text, "# Product Frame Metrics Report") ||
      !contains(text, "| scenario | debug_overlay | frames | total_ns |") ||
      !contains(text, "movement_wall_run_corridor") ||
      !contains(text, "draw_item_count_max")) {
    return false;
  }

  runShellCommand("python3 tools/product_frame_metrics_report.py --format csv " +
                      shellQuote(singlePath),
                  stdoutPath,
                  stderrPath,
                  exitCode);
  if (exitCode != 0 || !readTextFile(stdoutPath, text) ||
      !contains(text,
                "scenario,debug_overlay,frames,total_ns,"
                "projection_or_frame_build_ns") ||
      !contains(text, "movement_wall_run_corridor,False,4,0,0")) {
    return false;
  }

  runShellCommand("python3 tools/product_frame_metrics_report.py " +
                      shellQuote(suitePath) + " --baseline " +
                      shellQuote(suitePath),
                  stdoutPath,
                  stderrPath,
                  exitCode);
  if (exitCode != 0 || !readTextFile(stdoutPath, text) ||
      !contains(text, "## Comparison") ||
      !contains(text, "total_ns_delta") ||
      !contains(text, "| default_gameplay | False | 0 | 0 | 0 |") ||
      !contains(text, "| movement_wall_run_corridor | True | 0 | 0 | 0 |")) {
    return false;
  }

  runShellCommand("python3 tools/product_frame_metrics_report.py --format csv " +
                      shellQuote(suitePath) + " --baseline " +
                      shellQuote(suitePath),
                  stdoutPath,
                  stderrPath,
                  exitCode);
  if (exitCode != 0 || !readTextFile(stdoutPath, text) ||
      !contains(text,
                "scenario,debug_overlay,total_ns_delta,"
                "projection_or_frame_build_ns_delta") ||
      !contains(text, "default_gameplay,False,0,0,0,0,0,0,0,0,0,0,0,0,0,0,same") ||
      !contains(text,
                "movement_wall_run_corridor,True,0,0,0,0,0,0,0,0,0,0,0,0,0,0,same")) {
    return false;
  }

  const std::filesystem::path unknownPath =
      "/tmp/iggy3d_product_frame_metrics_unknown_schema.json";
  {
    std::ofstream out(unknownPath);
    out << "{\"schema\":\"unknown.schema\"}\n";
  }
  runShellCommand("python3 tools/product_frame_metrics_report.py " +
                      shellQuote(unknownPath),
                  stdoutPath,
                  stderrPath,
                  exitCode);
  if (exitCode == 0 || !readTextFile(stderrPath, text) ||
      !contains(text, "unknown schema")) {
    return false;
  }

  const std::filesystem::path malformedPath =
      "/tmp/iggy3d_product_frame_metrics_malformed.json";
  {
    std::ofstream out(malformedPath);
    out << "{";
  }
  runShellCommand("python3 tools/product_frame_metrics_report.py " +
                      shellQuote(malformedPath),
                  stdoutPath,
                  stderrPath,
                  exitCode);
  return exitCode != 0 && readTextFile(stderrPath, text) &&
         contains(text, "malformed json");
}

}  // namespace

int main() {
#if defined(IGGY3D_PRODUCT_FRAME_METRICS_PATH)
  constexpr bool toolBuilt = true;
  const std::filesystem::path binary{IGGY3D_PRODUCT_FRAME_METRICS_PATH};
#else
  constexpr bool toolBuilt = false;
  const std::filesystem::path binary;
#endif

  const bool ok = toolBuilt && std::filesystem::exists(binary) &&
                  suiteSmoke(binary) && wallRunSmoke(binary) &&
                  outputFileSmoke(binary) && reportToolSmoke(binary);
  if (!ok) {
    std::cerr << "product_frame_metrics_cli_smoke failed\n";
    std::cerr << "tool_built=" << (toolBuilt ? "true" : "false") << "\n";
    std::cerr << "tool_path=" << binary << "\n";
    return 1;
  }
  return 0;
}
