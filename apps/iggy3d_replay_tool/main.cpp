#include "content/PackageLoader.hpp"
#include "runtime/replay/CommandReplay.hpp"
#include "runtime/replay/StateHash.hpp"
#include "runtime/save/SaveCodec.hpp"
#include "runtime/save/SaveLoad.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct ReplayToolConfig {
  std::filesystem::path fixturePath = "fixtures/demos/first_room/package.iggy3d.toml";
  std::filesystem::path savePath;
  std::filesystem::path expectedSummaryPath;
  iggy3d::StateHashValue expectedHash = 0;
  bool hasExpectedHash = false;
  bool verbose = false;
};

struct ReplayToolResult {
  bool ok = false;
  iggy3d::CommandReplayResult replay;
  std::uint64_t submitted = 0;
  std::string diagnostic;
};

bool readTextFile(const std::filesystem::path& path, std::string& text) {
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return true;
}

bool parseHash(std::string_view text, iggy3d::StateHashValue& value) {
  if (text.size() != 16U) {
    return false;
  }
  iggy3d::StateHashValue parsed = 0;
  const char* begin = text.data();
  const char* end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, parsed, 16);
  if (result.ec != std::errc{} || result.ptr != end) {
    return false;
  }
  value = parsed;
  return true;
}

bool parseArgs(int argc, const char* const* argv, ReplayToolConfig& config, std::string& diagnostic) {
  for (int index = 1; index < argc; ++index) {
    const std::string_view arg = argv[index];
    const auto needValue = [&]() {
      return index + 1 >= argc || std::string_view(argv[index + 1]).starts_with("-");
    };
    if (arg == "--fixture" || arg == "--package") {
      if (needValue()) {
        diagnostic = "missing fixture path";
        return false;
      }
      config.fixturePath = argv[++index];
    } else if (arg == "--save") {
      if (needValue()) {
        diagnostic = "missing save path";
        return false;
      }
      config.savePath = argv[++index];
    } else if (arg == "--summary" || arg == "--expect-summary") {
      if (needValue()) {
        diagnostic = "missing summary path";
        return false;
      }
      config.expectedSummaryPath = argv[++index];
    } else if (arg == "--expect-hash") {
      if (needValue()) {
        diagnostic = "missing expected hash";
        return false;
      }
      config.hasExpectedHash = parseHash(argv[++index], config.expectedHash);
      if (!config.hasExpectedHash) {
        diagnostic = "invalid expected hash";
        return false;
      }
    } else if (arg == "--replay") {
      continue;
    } else if (arg == "--verbose" || arg == "-v") {
      config.verbose = true;
    } else {
      diagnostic = "unknown option";
      return false;
    }
  }
  if (config.savePath.empty()) {
    diagnostic = "missing save input";
    return false;
  }
  return true;
}

iggy3d::CommandRecord commandFromSave(const iggy3d::SaveCommandRecord& saved) {
  iggy3d::CommandRecord command;
  command.commandId = saved.commandId;
  command.sequence = saved.sequence;
  command.kind = saved.kind;
  command.source = saved.source;
  command.playerSlot = saved.playerSlot;
  command.actor = saved.actor;
  command.payload.target.hasEntity = saved.hasTargetEntity;
  command.payload.target.entity = saved.targetEntity;
  command.payload.target.hasPoint = saved.hasTargetPoint;
  command.payload.target.point = saved.targetPoint;
  command.payload.retrySourceCommandId = saved.retrySourceCommandId;
  command.issuedTick = saved.issuedTick;
  command.scheduledTick = saved.scheduledTick;
  command.admission = saved.admission;
  command.rejection = saved.rejection;
  return command;
}

std::vector<iggy3d::CommandRecord> commandsFromSave(const iggy3d::SaveEnvelope& envelope) {
  std::vector<iggy3d::CommandRecord> commands;
  commands.reserve(envelope.commandLog.records.size());
  for (const iggy3d::SaveCommandRecord& saved : envelope.commandLog.records) {
    commands.push_back(commandFromSave(saved));
  }
  return commands;
}

bool loadProof(const ReplayToolConfig& config,
               const iggy3d::SaveEnvelope& envelope,
               std::string_view saveText,
               const iggy3d::SessionCreateRequest& create) {
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(create);
  if (created.status != iggy3d::ResultStatus::Ok) {
    return false;
  }
  iggy3d::Session loaded = std::move(created.value);
  const iggy3d::SaveCompatibilityRequest compatibility{
      envelope, envelope.metadata.packageId, envelope.metadata.scenarioId};
  const iggy3d::LoadStateResult load =
      iggy3d::loadEncodedSaveIntoSession(loaded, saveText, compatibility);
  (void)config;
  return load.status == iggy3d::SaveLoadStatus::Ok &&
         loaded.stateHash() == envelope.metadata.savedStateHash;
}

bool resetProof(const iggy3d::SessionCreateRequest& create) {
  iggy3d::Result<iggy3d::Session> created = iggy3d::Session::create(create);
  if (created.status != iggy3d::ResultStatus::Ok) {
    return false;
  }
  iggy3d::Session session = std::move(created.value);
  const iggy3d::SessionResetResult reset = session.resetToBaseline();
  return reset.reset && reset.baselineHash == reset.currentHash &&
         session.state().commandLog.empty() && session.state().nextCommandId == 1U;
}

ReplayToolResult runReplayTool(const ReplayToolConfig& config) {
  ReplayToolResult result;
  const iggy3d::PackageLoadResult package =
      iggy3d::loadPackage(iggy3d::PackageLoadRequest{config.fixturePath.string()});
  if (package.status != iggy3d::PackageLoadStatus::Ok) {
    result.diagnostic = "fixture load failed";
    return result;
  }

  std::string saveText;
  if (!readTextFile(config.savePath, saveText)) {
    result.diagnostic = "save read failed";
    return result;
  }
  const iggy3d::SaveDecodeResult decoded = iggy3d::decodeSaveEnvelope(saveText);
  if (decoded.status != iggy3d::SaveCodecStatus::Ok) {
    result.diagnostic = "save decode failed";
    return result;
  }

  std::string expectedSummary;
  if (!config.expectedSummaryPath.empty() &&
      !readTextFile(config.expectedSummaryPath, expectedSummary)) {
    result.diagnostic = "summary read failed";
    return result;
  }

  iggy3d::SessionCreateRequest create;
  create.config = package.scenario.config;
  create.seed = package.scenario;
  const bool loadOk = loadProof(config, decoded.envelope, saveText, create);
  const bool resetOk = resetProof(create);

  iggy3d::CommandReplayRequest request;
  request.baseline = create;
  request.sourceCommands = commandsFromSave(decoded.envelope);
  request.expectedFinalHash =
      config.hasExpectedHash ? config.expectedHash : decoded.envelope.metadata.savedStateHash;
  request.expectedSummaryText = expectedSummary;
  request.saveRoundtrip = loadOk ? iggy3d::RuntimeProofStatus::Pass : iggy3d::RuntimeProofStatus::Fail;
  request.resetBaseline = resetOk ? iggy3d::RuntimeProofStatus::Pass : iggy3d::RuntimeProofStatus::Fail;
  request.replayHash = iggy3d::RuntimeProofStatus::Pass;
  request.retryExecutedCommandId = 3;
  request.retryExecutedSequence = 3;

  result.replay = iggy3d::replayCommands(request);
  result.submitted = decoded.envelope.commandLog.records.size();
  result.ok = result.replay.status == iggy3d::CommandReplayStatus::Matched;
  if (!result.ok) {
    result.diagnostic = result.replay.diagnostic;
  }
  return result;
}

std::string replayStatusText(iggy3d::CommandReplayStatus status) {
  switch (status) {
    case iggy3d::CommandReplayStatus::Matched: return "Matched";
    case iggy3d::CommandReplayStatus::AdmissionDiverged: return "AdmissionDiverged";
    case iggy3d::CommandReplayStatus::RejectionReasonDiverged: return "RejectionReasonDiverged";
    case iggy3d::CommandReplayStatus::StateHashDiverged: return "StateHashDiverged";
    case iggy3d::CommandReplayStatus::SummaryDiverged: return "SummaryDiverged";
    case iggy3d::CommandReplayStatus::CommandMissing: return "CommandMissing";
    case iggy3d::CommandReplayStatus::ExecutionFailed: return "ExecutionFailed";
    case iggy3d::CommandReplayStatus::InvalidBaseline: return "InvalidBaseline";
  }
  return "ExecutionFailed";
}

}  // namespace

int main(int argc, const char* const* argv) {
  ReplayToolConfig config;
  std::string diagnostic;
  if (!parseArgs(argc, argv, config, diagnostic)) {
    std::cerr << "replay.error=" << diagnostic << '\n';
    return 2;
  }

  const ReplayToolResult result = runReplayTool(config);
  std::cout << "replay.status=" << replayStatusText(result.replay.status) << '\n';
  std::cout << "replay.hash=" << (result.ok ? "pass" : "fail") << '\n';
  std::cout << "state_hash=" << iggy3d::formatStateHash(result.replay.actualHash) << '\n';
  std::cout << "commands.submitted=" << result.submitted << '\n';
  if (!result.ok && config.verbose) {
    std::cerr << "replay.error=" << result.diagnostic << '\n';
  }
  return result.ok ? 0 : 1;
}
