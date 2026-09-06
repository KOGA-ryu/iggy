#include "FirstMoveUi.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "platform/NativeVulkanHost.hpp"

namespace fm = iggy3d::first_move;

namespace {

constexpr std::uint32_t kDefaultWidth = 1440U;
constexpr std::uint32_t kDefaultHeight = 900U;
constexpr std::uint32_t kMinimumWidth = 800U;
constexpr std::uint32_t kMinimumHeight = 600U;
constexpr std::uint32_t kMaximumWidth = 3840U;
constexpr std::uint32_t kMaximumHeight = 2160U;

struct ScriptStep {
  std::size_t lineNumber = 0U;
  std::string source;
  fm::FirstMoveAction action;
};

struct ScriptActionRecord {
  std::size_t lineNumber = 0U;
  std::string source;
  bool accepted = false;
  bool changed = false;
  std::string reason;
};

struct LaunchOptions {
  bool offscreen = false;
  bool frameLimitExplicit = false;
  std::uint64_t frameLimit = 0U;
  std::uint32_t width = kDefaultWidth;
  std::uint32_t height = kDefaultHeight;
  float textScale = 1.0F;
  std::filesystem::path capturePath;
  std::filesystem::path reportPath;
  std::filesystem::path scriptPath;
  std::vector<ScriptStep> script;
  std::optional<std::string> startMode;
  fm::PathsScreen resolvedStartScreen = fm::PathsScreen::Title;
  fm::FirstMoveMode resolvedStartMode = fm::FirstMoveMode::GuidedQuestion;
};

struct ParseResult {
  bool ok = false;
  bool help = false;
  std::string error;
};

[[nodiscard]] bool validatePathConfiguration(const LaunchOptions& options,
                                             std::string& error);

void printUsage(FILE* stream) {
  std::fprintf(
      stream,
      "usage: paths [--offscreen] [--frames N] [--capture PATH] "
      "[--report PATH] [--script PATH] [--resolution WxH] "
      "[--text-scale NUMBER] [--start-mode title|guided|hunt]\n"
      "  --offscreen          render without creating a window or surface\n"
      "  --frames N           exit after N drawable frames (N > 0)\n"
      "  --capture PATH       write PNG, RGBA, metadata, and SHA-256 artifacts\n"
      "  --report PATH        write the run evidence report as JSON\n"
      "  --script PATH        play one validated semantic command per frame\n"
      "  --resolution WxH     800x600 through 3840x2160\n"
      "  --text-scale NUMBER  initial text scale from 1.0 through 1.5\n"
      "  --start-mode MODE    begin on title, guided, or hunt\n");
}

[[nodiscard]] bool parsePositiveU64(std::string_view text,
                                    std::uint64_t& value) {
  if (text.empty()) {
    return false;
  }
  const char* begin = text.data();
  const char* end = text.data() + text.size();
  const std::from_chars_result parsed = std::from_chars(begin, end, value);
  return parsed.ec == std::errc{} && parsed.ptr == end && value > 0U;
}

[[nodiscard]] bool parsePositiveSize(std::string_view text,
                                     std::size_t& value) {
  std::uint64_t parsed = 0U;
  if (!parsePositiveU64(text, parsed) ||
      parsed > static_cast<std::uint64_t>(
                   std::numeric_limits<std::size_t>::max())) {
    return false;
  }
  value = static_cast<std::size_t>(parsed);
  return true;
}

[[nodiscard]] bool parseResolution(std::string_view text,
                                   std::uint32_t& width,
                                   std::uint32_t& height) {
  const std::size_t separator = text.find('x');
  if (separator == std::string_view::npos || separator == 0U ||
      separator + 1U >= text.size() ||
      text.find('x', separator + 1U) != std::string_view::npos) {
    return false;
  }
  std::uint64_t parsedWidth = 0U;
  std::uint64_t parsedHeight = 0U;
  if (!parsePositiveU64(text.substr(0U, separator), parsedWidth) ||
      !parsePositiveU64(text.substr(separator + 1U), parsedHeight) ||
      parsedWidth < kMinimumWidth || parsedWidth > kMaximumWidth ||
      parsedHeight < kMinimumHeight || parsedHeight > kMaximumHeight) {
    return false;
  }
  width = static_cast<std::uint32_t>(parsedWidth);
  height = static_cast<std::uint32_t>(parsedHeight);
  return true;
}

[[nodiscard]] bool parseTextScale(const char* text, float& scale) {
  if (text == nullptr || *text == '\0') {
    return false;
  }
  errno = 0;
  char* end = nullptr;
  const float parsed = std::strtof(text, &end);
  if (errno == ERANGE || end == text || end == nullptr || *end != '\0' ||
      !std::isfinite(parsed) || parsed < 1.0F || parsed > 1.5F) {
    return false;
  }
  scale = parsed;
  return true;
}

[[nodiscard]] std::string_view trim(std::string_view text) noexcept {
  constexpr std::string_view whitespace = " \t\r\n";
  const std::size_t first = text.find_first_not_of(whitespace);
  if (first == std::string_view::npos) {
    return {};
  }
  const std::size_t last = text.find_last_not_of(whitespace);
  return text.substr(first, last - first + 1U);
}

[[nodiscard]] bool parseScriptCommand(std::string_view line,
                                      fm::FirstMoveAction& action) {
  std::istringstream tokens{std::string(line)};
  std::string verb;
  tokens >> verb;
  if (verb == "select") {
    std::string rowToken;
    std::string columnToken;
    std::string extra;
    tokens >> rowToken >> columnToken;
    if (rowToken.empty() || columnToken.empty() || (tokens >> extra)) {
      return false;
    }
    std::size_t row = 0U;
    std::size_t column = 0U;
    if (!parsePositiveSize(rowToken, row) ||
        !parsePositiveSize(columnToken, column) || row > fm::kHuntRowCount ||
        column > fm::kHuntCellsPerRow) {
      return false;
    }
    action = fm::FirstMoveAction::huntSelectCell(row - 1U, column - 1U);
    return true;
  }
  if (verb == "open_mode") {
    std::string name, extra;
    tokens >> name;
    const auto* descriptor = fm::findFirstMoveMode(name);
    if (!descriptor || (tokens >> extra)) return false;
    action = {descriptor->openAction};
    return true;
  }
  if (verb == "guided_select") {
    std::string optionToken;
    std::string trailing;
    tokens >> optionToken;
    std::size_t option = 0U;
    if (optionToken.empty() || (tokens >> trailing) ||
        !parsePositiveSize(optionToken, option) || option > 4U) {
      return false;
    }
    action = fm::FirstMoveAction::guidedSelectOption(option - 1U);
    return true;
  }

  struct NamedCommand {
    std::string_view name;
    fm::FirstMoveActionKind kind = fm::FirstMoveActionKind::HuntToggleMark;
  };
  constexpr std::array<NamedCommand, 25U> commands{{
      {"back_title", fm::FirstMoveActionKind::BackToTitle},
      {"open_stats", fm::FirstMoveActionKind::OpenStats},
      {"close_stats", fm::FirstMoveActionKind::CloseStats},
      {"left", fm::FirstMoveActionKind::HuntMoveLeft},
      {"right", fm::FirstMoveActionKind::HuntMoveRight},
      {"up", fm::FirstMoveActionKind::HuntMoveUp},
      {"down", fm::FirstMoveActionKind::HuntMoveDown},
      {"toggle", fm::FirstMoveActionKind::HuntToggleMark},
      {"commit", fm::FirstMoveActionKind::HuntCommitRow},
      {"clear", fm::FirstMoveActionKind::HuntClearReady},
      {"explain", fm::FirstMoveActionKind::HuntOpenReview},
      {"close", fm::FirstMoveActionKind::HuntCloseReview},
      {"release", fm::FirstMoveActionKind::HuntReleaseReviewed},
      {"restart", fm::FirstMoveActionKind::HuntRestart},
      {"mode_guided", fm::FirstMoveActionKind::SwitchToGuided},
      {"mode_hunt", fm::FirstMoveActionKind::SwitchToHunt},
      {"guided_open", fm::FirstMoveActionKind::GuidedOpen},
      {"guided_check", fm::FirstMoveActionKind::GuidedCheck},
      {"guided_retry", fm::FirstMoveActionKind::GuidedTryAgain},
      {"guided_show", fm::FirstMoveActionKind::GuidedShowAnswer},
      {"guided_next", fm::FirstMoveActionKind::GuidedContinue},
      {"guided_back", fm::FirstMoveActionKind::GuidedBackToGrid},
      {"guided_restart", fm::FirstMoveActionKind::GuidedRestart},
      {"review_first", fm::FirstMoveActionKind::ReviewFirst},
      {"review_last", fm::FirstMoveActionKind::ReviewLast},
  }};
  std::string extra;
  if (tokens >> extra) {
    return false;
  }
  const auto found = std::find_if(
      commands.begin(), commands.end(),
      [&verb](const NamedCommand& candidate) { return candidate.name == verb; });
  if (found != commands.end()) {
    action = {found->kind};
    return true;
  }
  return false;
}

[[nodiscard]] bool readScript(const std::filesystem::path& path,
                              std::vector<ScriptStep>& script,
                              std::string& error) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    error = "cannot open script '" + path.generic_string() + "'";
    return false;
  }
  std::string line;
  std::size_t lineNumber = 0U;
  while (std::getline(input, line)) {
    ++lineNumber;
    const std::string_view content = trim(line);
    if (content.empty() || content.front() == '#') {
      continue;
    }
    fm::FirstMoveAction action;
    if (!parseScriptCommand(content, action)) {
      error = "invalid script command at line " + std::to_string(lineNumber);
      return false;
    }
    script.push_back({lineNumber, std::string(content), action});
  }
  if (input.bad()) {
    error = "failed while reading script '" + path.generic_string() + "'";
    return false;
  }
  return true;
}

[[nodiscard]] ParseResult parseArguments(int argc,
                                         char** argv,
                                         LaunchOptions& options) {
  ParseResult result;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument = argv[index];
    if (argument == "--help") {
      result.ok = true;
      result.help = true;
      return result;
    }
    if (argument == "--offscreen") {
      options.offscreen = true;
      continue;
    }
    if (index + 1 >= argc) {
      result.error = "missing value for '" + std::string(argument) + "'";
      return result;
    }
    const char* value = argv[++index];
    if (argument == "--frames") {
      if (!parsePositiveU64(value, options.frameLimit)) {
        result.error = "--frames requires a positive integer";
        return result;
      }
      options.frameLimitExplicit = true;
    } else if (argument == "--capture") {
      options.capturePath = value;
      if (options.capturePath.empty()) {
        result.error = "--capture requires a nonempty path";
        return result;
      }
    } else if (argument == "--report") {
      options.reportPath = value;
      if (options.reportPath.empty()) {
        result.error = "--report requires a nonempty path";
        return result;
      }
    } else if (argument == "--script") {
      options.scriptPath = value;
      if (options.scriptPath.empty()) {
        result.error = "--script requires a nonempty path";
        return result;
      }
    } else if (argument == "--resolution") {
      if (!parseResolution(value, options.width, options.height)) {
        result.error = "--resolution must be WxH within 800x600..3840x2160";
        return result;
      }
    } else if (argument == "--text-scale") {
      if (!parseTextScale(value, options.textScale)) {
        result.error = "--text-scale must be finite and within 1.0..1.5";
        return result;
      }
    } else if (argument == "--start-mode") {
      const std::string_view mode = value;
      if (mode != "title" && !fm::findFirstMoveMode(mode)) {
        result.error = "--start-mode must be title, guided, or hunt";
        return result;
      }
      options.startMode = mode;
    } else {
      result.error = "unknown argument '" + std::string(argument) + "'";
      return result;
    }
  }

  if (!validatePathConfiguration(options, result.error)) {
    return result;
  }
  if (!options.scriptPath.empty() &&
      !readScript(options.scriptPath, options.script, result.error)) {
    return result;
  }
  if (options.script.size() >
      static_cast<std::size_t>(std::numeric_limits<std::uint64_t>::max() - 4U)) {
    result.error = "script is too long";
    return result;
  }
  const std::uint64_t scriptFrames =
      static_cast<std::uint64_t>(options.script.size());
  if (options.frameLimitExplicit && options.frameLimit < scriptFrames) {
    result.error = "--frames would truncate the validated script";
    return result;
  }
  if (!options.frameLimitExplicit &&
      (!options.scriptPath.empty() || !options.capturePath.empty())) {
    options.frameLimit = scriptFrames + 4U;
  }
  const std::string mode = options.startMode.value_or(
      options.scriptPath.empty() ? "title" : "hunt");
  options.resolvedStartScreen = mode == "title" ? fm::PathsScreen::Title
                                               : fm::PathsScreen::Playing;
  if (const auto* descriptor = fm::findFirstMoveMode(mode))
    options.resolvedStartMode = descriptor->mode;
  result.ok = true;
  return result;
}

[[nodiscard]] bool outputParentExists(const std::filesystem::path& path) {
  const std::filesystem::path parent =
      path.parent_path().empty() ? std::filesystem::path{"."}
                                 : path.parent_path();
  std::error_code error;
  return std::filesystem::is_directory(parent, error) && !error;
}

[[nodiscard]] std::filesystem::path normalizedPath(
    const std::filesystem::path& path) {
  std::error_code error;
  const std::filesystem::path absolute = std::filesystem::absolute(path, error);
  if (error) {
    return path.lexically_normal();
  }
  const std::filesystem::path canonical =
      std::filesystem::weakly_canonical(absolute, error);
  return error ? absolute.lexically_normal() : canonical;
}

[[nodiscard]] bool rejectDirectoryOutput(const std::filesystem::path& path,
                                         std::string& error) {
  std::error_code statusError;
  const bool exists = std::filesystem::exists(path, statusError);
  if (statusError) {
    error = "cannot inspect output path '" + path.generic_string() + "': " +
            statusError.message();
    return false;
  }
  if (exists && std::filesystem::is_directory(path, statusError)) {
    error = "output path is a directory: '" + path.generic_string() + "'";
    return false;
  }
  if (statusError) {
    error = "cannot inspect output path '" + path.generic_string() + "': " +
            statusError.message();
    return false;
  }
  return true;
}

[[nodiscard]] bool removeIfPresent(const std::filesystem::path& path,
                                   std::string& error) {
  if (!rejectDirectoryOutput(path, error)) {
    return false;
  }
  std::error_code removeError;
  static_cast<void>(std::filesystem::remove(path, removeError));
  if (removeError) {
    error = "cannot replace output '" + path.generic_string() + "': " +
            removeError.message();
    return false;
  }
  return true;
}

[[nodiscard]] bool validatePathConfiguration(const LaunchOptions& options,
                                             std::string& error) {
  std::vector<std::filesystem::path> outputs;
  if (!options.capturePath.empty()) {
    if (!outputParentExists(options.capturePath)) {
      error = "capture parent directory does not exist";
      return false;
    }
    const paths::CapturePaths artifacts =
        paths::capturePaths(options.capturePath);
    outputs = {artifacts.screenshotPath, artifacts.rawPath, artifacts.metaPath,
               artifacts.hashPath};
  }
  if (!options.reportPath.empty()) {
    if (!outputParentExists(options.reportPath)) {
      error = "report parent directory does not exist";
      return false;
    }
    outputs.push_back(options.reportPath);
    std::filesystem::path reportTemporary = options.reportPath;
    reportTemporary += ".tmp";
    outputs.push_back(std::move(reportTemporary));
  }
  for (const std::filesystem::path& output : outputs) {
    if (!rejectDirectoryOutput(output, error)) {
      return false;
    }
  }
  for (std::size_t left = 0U; left < outputs.size(); ++left) {
    const std::filesystem::path normalizedLeft = normalizedPath(outputs[left]);
    for (std::size_t right = left + 1U; right < outputs.size(); ++right) {
      if (normalizedLeft == normalizedPath(outputs[right])) {
        error = "output paths overlap";
        return false;
      }
    }
  }
  if (!options.scriptPath.empty()) {
    const std::filesystem::path normalizedScript =
        normalizedPath(options.scriptPath);
    for (const std::filesystem::path& output : outputs) {
      if (normalizedScript == normalizedPath(output)) {
        error = "script path aliases an output path";
        return false;
      }
    }
    std::error_code scriptStatusError;
    if (std::filesystem::is_directory(options.scriptPath, scriptStatusError)) {
      error = "script path is a directory";
      return false;
    }
    if (scriptStatusError) {
      error = "cannot inspect script path: " + scriptStatusError.message();
      return false;
    }
  }
  return true;
}

void writeJsonString(std::ostream& output, std::string_view text) {
  output.put('"');
  for (const char character : text) {
    switch (character) {
      case '"': output << "\\\""; break;
      case '\\': output << "\\\\"; break;
      case '\b': output << "\\b"; break;
      case '\f': output << "\\f"; break;
      case '\n': output << "\\n"; break;
      case '\r': output << "\\r"; break;
      case '\t': output << "\\t"; break;
      default:
        if (static_cast<unsigned char>(character) < 0x20U) {
          constexpr char digits[] = "0123456789abcdef";
          const auto value = static_cast<unsigned char>(character);
          output << "\\u00" << digits[(value >> 4U) & 0x0FU]
                 << digits[value & 0x0FU];
        } else {
          output.put(character);
        }
        break;
    }
  }
  output.put('"');
}

void writeOptionalMask(std::ostream& output,
                       const std::optional<std::uint8_t>& value) {
  if (value.has_value()) {
    output << static_cast<unsigned int>(*value);
  } else {
    output << "null";
  }
}

void writeOptionalBool(std::ostream& output,
                       const std::optional<bool>& value) {
  if (value.has_value()) {
    output << (*value ? "true" : "false");
  } else {
    output << "null";
  }
}

void writeOptionalSize(std::ostream& output,
                       const std::optional<std::size_t>& value) {
  if (value.has_value()) {
    output << *value;
  } else {
    output << "null";
  }
}

void writeRun(std::ostream& output, const fm::HuntRunRecord& run) {
  const fm::HuntRunSummary summary = fm::summarizeRun(run);
  output << "    {\"run_number\": " << run.runNumber
         << ", \"prior_exposure\": " << (run.priorExposure ? "true" : "false")
         << ", \"score\": " << run.score
         << ",\n     \"finished\": " << (summary.finished ? "true" : "false")
         << ", \"assisted\": " << (summary.assisted ? "true" : "false")
         << ",\n     \"correct_rows\": " << summary.correctRows
         << ", \"incorrect_rows\": " << summary.incorrectRows
         << ", \"explained_rows\": " << summary.explainedRows
         << ",\n     \"untouched_rows\": " << summary.untouchedRows
         << ",\n     \"rows\": [\n";
  for (std::size_t index = 0U; index < run.rows.size(); ++index) {
    const fm::HuntRowRecord& row = run.rows[index];
    output << "       {\"row\": " << row.rowIndex + 1U << ", \"state\": ";
    writeJsonString(output, fm::rowStateName(row.state));
    output << ", \"selected_mask\": "
           << static_cast<unsigned int>(row.selectedMask)
           << ", \"initial_mask\": ";
    writeOptionalMask(output, row.initialSelectionMask);
    output << ", \"initial_correct\": ";
    writeOptionalBool(output, row.initialCorrect);
    output << ", \"explained\": " << (row.explained ? "true" : "false")
           << '}' << (index + 1U == run.rows.size() ? "\n" : ",\n");
  }
  output << "     ]\n    }";
}

void writeGuidedRun(std::ostream& output,
                    const fm::LayeredQuestionRunRecord& run) {
  const fm::LayeredQuestionRunSummary summary =
      fm::summarizeLayeredQuestionRun(run);
  output << "    {\"run_number\": " << run.runNumber
         << ", \"prior_exposure\": "
         << (run.priorExposure ? "true" : "false")
         << ", \"phase\": ";
  writeJsonString(output, fm::layeredQuestionPhaseName(run.phase));
  output << ", \"current_step\": " << run.currentStep + 1U
         << ", \"completed\": " << (run.completed ? "true" : "false")
         << ",\n     \"summary\": {\"correct_on_first_try\": "
         << summary.correctOnFirstTry
         << ", \"corrected_after_retry\": " << summary.correctedAfterRetry
         << ", \"shown_answers\": " << summary.shownAnswers
         << ", \"incorrect_checked_attempts\": "
         << summary.incorrectCheckedAttempts
         << ", \"assisted\": " << (summary.assisted ? "true" : "false")
         << "},\n     \"steps\": [\n";
  for (std::size_t index = 0U; index < run.steps.size(); ++index) {
    const fm::LayeredQuestionStepRecord& step = run.steps[index];
    output << "       {\"step\": " << index + 1U
           << ", \"first_checked_option\": ";
    writeOptionalSize(output, step.firstCheckedOption);
    output << ", \"first_correct\": ";
    writeOptionalBool(output, step.firstCorrect);
    output << ", \"selected_option\": ";
    writeOptionalSize(output, step.selectedOption);
    output << ", \"resolved_by_player\": "
           << (step.resolvedByPlayer ? "true" : "false")
           << ", \"answer_shown\": "
           << (step.answerShown ? "true" : "false")
           << ", \"incorrect_checked_attempts\": "
           << step.incorrectCheckedAttempts << ", \"attempts\": [";
    for (std::size_t attemptIndex = 0U;
         attemptIndex < step.attempts.size(); ++attemptIndex) {
      const fm::LayeredQuestionAttemptRecord& attempt =
          step.attempts[attemptIndex];
      output << "{\"option\": " << attempt.optionIndex
             << ", \"correct\": "
             << (attempt.correct ? "true" : "false") << '}';
      if (attemptIndex + 1U != step.attempts.size()) {
        output << ", ";
      }
    }
    output << "]}" << (index + 1U == run.steps.size() ? "\n" : ",\n");
  }
  output << "     ]\n    }";
}

[[nodiscard]] bool writeReport(
    const std::filesystem::path& path,
    const LaunchOptions& options,
    const fm::FirstMoveUiState& ui,
    const fm::HuntSession& session,
    const fm::LayeredQuestionSession& guided,
    const std::vector<ScriptActionRecord>& actions,
    std::string& error) {
  std::filesystem::path temporary = path;
  temporary += ".tmp";
  if (!removeIfPresent(temporary, error)) {
    return false;
  }
  std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
  if (!output) {
    error = "cannot open report '" + temporary.generic_string() + "'";
    return false;
  }
  output << "{\n  \"product\": \"paths\", \"report_schema_version\": 1,\n  \"start_screen\": ";
  writeJsonString(output, fm::pathsScreenName(options.resolvedStartScreen));
  output << ", \"final_screen\": ";
  writeJsonString(output, fm::pathsScreenName(ui.screen));
  output << ",\n  \"pack_id\": ";
  writeJsonString(output, fm::kHuntPackId);
  output << ", \"pack_version\": " << fm::kHuntPackVersion
         << ",\n  \"start_mode\": ";
  writeJsonString(output, fm::firstMoveModeName(options.resolvedStartMode));
  output << ", \"final_mode\": ";
  writeJsonString(output, fm::firstMoveModeName(ui.mode));
  output << ",\n  \"runs\": [\n";
  bool firstRun = true;
  for (const fm::HuntRunRecord& run : session.archivedRuns()) {
    if (!firstRun) {
      output << ",\n";
    }
    writeRun(output, run);
    firstRun = false;
  }
  if (!firstRun) {
    output << ",\n";
  }
  writeRun(output, session.currentRun());
  output << "\n  ],\n  \"guided_question_id\": ";
  writeJsonString(output, fm::kLayeredQuestionId);
  output << ", \"guided_question_version\": "
         << fm::kLayeredQuestionVersion << ",\n  \"guided_runs\": [\n";
  bool firstGuidedRun = true;
  for (const fm::LayeredQuestionRunRecord& run : guided.archivedRuns()) {
    if (!firstGuidedRun) {
      output << ",\n";
    }
    writeGuidedRun(output, run);
    firstGuidedRun = false;
  }
  if (!firstGuidedRun) {
    output << ",\n";
  }
  writeGuidedRun(output, guided.currentRun());
  output << "\n  ],\n  \"guided_archived_runs\": [\n";
  for (std::size_t index = 0U; index < guided.archivedRuns().size(); ++index) {
    writeGuidedRun(output, guided.archivedRuns()[index]);
    output << (index + 1U == guided.archivedRuns().size() ? "\n" : ",\n");
  }
  output << "  ],\n  \"guided_current_run\":\n";
  writeGuidedRun(output, guided.currentRun());
  output << ",\n  \"script_actions\": [\n";
  for (std::size_t index = 0U; index < actions.size(); ++index) {
    const ScriptActionRecord& action = actions[index];
    output << "    {\"line\": " << action.lineNumber << ", \"command\": ";
    writeJsonString(output, action.source);
    output << ", \"accepted\": " << (action.accepted ? "true" : "false")
           << ", \"changed\": " << (action.changed ? "true" : "false")
           << ", \"reason\": ";
    writeJsonString(output, action.reason);
    output << '}' << (index + 1U == actions.size() ? "\n" : ",\n");
  }
  output << "  ]\n}\n";
  output.flush();
  if (!output) {
    error = "failed writing report '" + temporary.generic_string() + "'";
    output.close();
    std::error_code ignored;
    static_cast<void>(std::filesystem::remove(temporary, ignored));
    return false;
  }
  output.close();
  std::error_code renameError;
  std::filesystem::rename(temporary, path, renameError);
  if (renameError) {
    error = "cannot finalize report '" + path.generic_string() + "': " +
            renameError.message();
    std::error_code ignored;
    static_cast<void>(std::filesystem::remove(temporary, ignored));
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  LaunchOptions options;
  const ParseResult parsed = parseArguments(argc, argv, options);
  if (!parsed.ok) {
    std::fprintf(stderr, "paths: %s\n", parsed.error.c_str());
    printUsage(stderr);
    return 2;
  }
  if (parsed.help) { printUsage(stdout); return 0; }
  try {
    paths::NativeVulkanHost host({options.offscreen, options.width, options.height});
    fm::HuntSession hunt;
    fm::LayeredQuestionSession guided;
    fm::FirstMoveUiState ui;
    ui.screen = options.resolvedStartScreen;
    ui.mode = options.resolvedStartMode;
    ui.textScale = options.textScale;
    std::vector<ScriptActionRecord> actions;
    std::size_t scriptIndex = 0, skippedFrames = 0;
    std::uint64_t renderedFrames = 0;
    while (!ui.quitRequested && (!options.frameLimit || renderedFrames < options.frameLimit)) {
      const auto frame = host.frame(
          [&](const SDL_Event& event) { fm::queueFirstMoveInput(ui, hunt, guided, event); },
          [&] {
            if (scriptIndex < options.script.size()) {
              const auto& step = options.script[scriptIndex++];
              const auto result = fm::dispatchFirstMoveAction(ui, hunt, guided, step.action);
              actions.push_back({step.lineNumber, step.source, result.accepted,
                                  result.changed, std::string(result.reason)});
              if (!result.accepted)
                std::fprintf(stderr, "paths: script line %zu rejected (%.*s)\n",
                    step.lineNumber, static_cast<int>(result.reason.size()), result.reason.data());
            }
            fm::renderFirstMoveUiFrame(ui, hunt, guided);
          });
      if (frame.status == paths::FrameStatus::Failed) throw std::runtime_error(frame.error);
      if (frame.status == paths::FrameStatus::Closed) break;
      if (frame.status == paths::FrameStatus::Rendered) { ++renderedFrames; skippedFrames = 0; }
      else if (options.frameLimit && ++skippedFrames > 300)
        throw std::runtime_error("Window remained nondrawable during bounded run");
    }
    if (scriptIndex != options.script.size())
      throw std::runtime_error("Run ended before all validated script commands were applied");
    std::string error;
    if (!options.capturePath.empty() && !host.capture(paths::capturePaths(options.capturePath), error))
      throw std::runtime_error(error);
    if (!options.reportPath.empty() && !writeReport(options.reportPath, options, ui, hunt, guided, actions, error))
      throw std::runtime_error(error);
    std::fprintf(stderr, "paths: completed frames=%llu script_actions=%zu score=%u\n",
        static_cast<unsigned long long>(renderedFrames), actions.size(), hunt.currentRun().score);
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "paths: %s\n", error.what());
    return 1;
  }
}
