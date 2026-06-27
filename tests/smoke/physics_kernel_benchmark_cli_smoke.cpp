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

bool suiteStdoutSmoke(const std::filesystem::path& binary) {
  const std::filesystem::path stdoutPath =
      "/tmp/iggy3d_physics_kernel_cli_suite.json";
  const std::filesystem::path stderrPath =
      "/tmp/iggy3d_physics_kernel_cli_suite.err";
  int exitCode = 1;
  std::string text;
  runCommand(binary, "--suite --no-timing", stdoutPath, stderrPath, exitCode);
  return exitCode == 0 && readTextFile(stdoutPath, text) &&
         contains(text, "\"schema\": "
                        "\"iggy3d.physics_kernel_benchmark.suite.v1\"") &&
         contains(text, "\"case_count\": 5") &&
         contains(text, "\"elapsed_nanoseconds\": 0");
}

bool caseStdoutSmoke(const std::filesystem::path& binary) {
  const std::filesystem::path stdoutPath =
      "/tmp/iggy3d_physics_kernel_cli_case.json";
  const std::filesystem::path stderrPath =
      "/tmp/iggy3d_physics_kernel_cli_case.err";
  int exitCode = 1;
  std::string text;
  runCommand(binary,
             "--case --kernel broadphase_grid --scenario dense_overlap "
             "--iterations 2 --no-timing",
             stdoutPath, stderrPath, exitCode);
  return exitCode == 0 && readTextFile(stdoutPath, text) &&
         contains(text, "\"schema\": "
                        "\"iggy3d.physics_kernel_benchmark.case.v1\"") &&
         contains(text, "\"kernel\": \"broadphase_grid\"") &&
         contains(text, "\"scenario\": \"dense_overlap\"") &&
         contains(text, "\"iterations\": 2") &&
         contains(text, "\"elapsed_nanoseconds\": 0");
}

bool caseOutputFileSmoke(const std::filesystem::path& binary) {
  const std::filesystem::path stdoutPath =
      "/tmp/iggy3d_physics_kernel_cli_output_stdout.txt";
  const std::filesystem::path stderrPath =
      "/tmp/iggy3d_physics_kernel_cli_output.err";
  const std::filesystem::path outputPath =
      "/tmp/iggy3d_physics_kernel_cli_output.json";
  std::filesystem::remove(outputPath);

  int exitCode = 1;
  std::string text;
  runCommand(binary,
             "--case --kernel kinematic_motor --scenario wall_slide "
             "--no-timing --output " +
                 shellQuote(outputPath),
             stdoutPath, stderrPath, exitCode);
  return exitCode == 0 && std::filesystem::exists(outputPath) &&
         readTextFile(outputPath, text) &&
         contains(text, "\"schema\": "
                        "\"iggy3d.physics_kernel_benchmark.case.v1\"") &&
         contains(text, "\"kernel\": \"kinematic_motor\"") &&
         contains(text, "\"scenario\": \"wall_slide\"") &&
         contains(text, "\"elapsed_nanoseconds\": 0");
}

bool invalidKernelSmoke(const std::filesystem::path& binary) {
  const std::filesystem::path stdoutPath =
      "/tmp/iggy3d_physics_kernel_cli_invalid.out";
  const std::filesystem::path stderrPath =
      "/tmp/iggy3d_physics_kernel_cli_invalid.err";
  int exitCode = 0;
  std::string err;
  runCommand(binary,
             "--case --kernel missing_kernel --scenario dense_overlap",
             stdoutPath, stderrPath, exitCode);
  return exitCode != 0 && readTextFile(stderrPath, err) &&
         contains(err, "unknown kernel");
}

}  // namespace

int main() {
#if defined(IGGY3D_PHYSICS_KERNEL_BENCH_PATH)
  const std::filesystem::path binary{IGGY3D_PHYSICS_KERNEL_BENCH_PATH};
  const bool binaryExists = std::filesystem::exists(binary);
  const bool suitePassed = binaryExists && suiteStdoutSmoke(binary);
  const bool casePassed = binaryExists && caseStdoutSmoke(binary);
  const bool outputPassed = binaryExists && caseOutputFileSmoke(binary);
  const bool invalidPassed = binaryExists && invalidKernelSmoke(binary);
#else
  const bool binaryExists = false;
  const bool suitePassed = false;
  const bool casePassed = false;
  const bool outputPassed = false;
  const bool invalidPassed = false;
#endif

  const bool passed =
      binaryExists && suitePassed && casePassed && outputPassed &&
      invalidPassed;
  std::cout << "smoke=physics_kernel_benchmark_cli\n";
  std::cout << "binary_exists=" << (binaryExists ? "true" : "false")
            << "\n";
  std::cout << "suite_passed=" << (suitePassed ? "true" : "false")
            << "\n";
  std::cout << "case_passed=" << (casePassed ? "true" : "false") << "\n";
  std::cout << "output_passed=" << (outputPassed ? "true" : "false")
            << "\n";
  std::cout << "invalid_passed=" << (invalidPassed ? "true" : "false")
            << "\n";
  std::cout << "result=" << (passed ? "pass" : "fail") << "\n";
  std::cout << "reason_code="
            << (passed ? "physics_kernel_benchmark_cli_smoke_pass"
                       : "physics_kernel_benchmark_cli_smoke_failed")
            << "\n";
  return passed ? 0 : 1;
}
