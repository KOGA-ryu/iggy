#include "content/FixtureScenarioLoader.hpp"

#include "content/NpcBehaviorProfileId.hpp"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string_view>

namespace iggy3d {

namespace {

enum class Table {
  None,
  Scenario,
  Defaults,
  Player,
  Entity,
  Objective,
  AiActor,
  AiGuardAnchor,
};

struct Line {
  std::string text;
  std::uint32_t number = 0;
};

struct EntityFlags {
  bool stableName = false;
  bool kind = false;
  bool active = false;
  bool persistent = false;
  bool position = false;
  bool boundsMin = false;
  bool boundsMax = false;
  bool targetable = false;
  bool targetActions = false;
  bool itemId = false;
  bool itemCount = false;
  bool interaction = false;
  bool deactivate = false;
  bool objectiveRef = false;
  bool combatant = false;
  bool factionId = false;
  bool hitPoints = false;
  bool maxHitPoints = false;
  std::uint32_t startLine = 0;
};

struct PlayerFlags {
  bool slot = false;
  bool kind = false;
  bool actor = false;
  std::uint32_t startLine = 0;
};

struct ObjectiveFlags {
  bool id = false;
  bool initialStatus = false;
  bool condition = false;
  bool playerSlot = false;
  bool itemId = false;
  bool itemCount = false;
  bool completeStatus = false;
  std::uint32_t startLine = 0;
};

struct AiActorFlags {
  bool actor = false;
  bool behaviorProfileId = false;
  std::uint32_t startLine = 0;
};

struct AiGuardAnchorFlags {
  bool actor = false;
  bool anchor = false;
  bool leashRadius = false;
  bool returnRadius = false;
  bool homeTolerance = false;
  std::uint32_t startLine = 0;
};

struct Parser {
  ScenarioLoadResult result;
  Table table = Table::None;
  bool scenarioId = false;
  bool fixedTick = false;
  bool interactionRange = false;
  bool movementDistance = false;
  bool slowScale = false;
  bool initialClock = false;
  bool realtimeCamera = false;
  bool tacticalCamera = false;
  std::vector<PlayerFlags> playerFlags;
  std::vector<EntityFlags> entityFlags;
  std::vector<ObjectiveFlags> objectiveFlags;
  std::vector<AiActorFlags> aiActorFlags;
  std::vector<AiGuardAnchorFlags> aiGuardAnchorFlags;
};

std::string_view trim(std::string_view value) {
  while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) {
    value.remove_prefix(1);
  }
  while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
    value.remove_suffix(1);
  }
  return value;
}

std::string stripComment(std::string_view value) {
  bool inString = false;
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '"') {
      inString = !inString;
    } else if (value[index] == '#' && !inString) {
      return std::string(trim(value.substr(0, index)));
    }
  }
  return std::string(trim(value));
}

Diagnostic diag(std::string code, std::string message, std::uint32_t line, std::uint32_t column) {
  return makeDiagnostic(DiagnosticDomain::Content, DiagnosticSeverity::Error, std::move(code),
                        std::move(message), DiagnosticLocation{"", line, column});
}

ScenarioLoadResult fail(Parser& parser, ScenarioLoadStatus status, std::string code,
                        std::string message, std::uint32_t line, std::uint32_t column) {
  parser.result.status = status;
  parser.result.diagnostics.push_back(diag(std::move(code), std::move(message), line, column));
  return parser.result;
}

bool parseString(std::string_view value, std::string& out) {
  value = trim(value);
  if (value.size() < 2U || value.front() != '"' || value.back() != '"') {
    return false;
  }
  out = std::string(value.substr(1, value.size() - 2U));
  return true;
}

bool parseBool(std::string_view value, bool& out) {
  value = trim(value);
  if (value == "true") {
    out = true;
    return true;
  }
  if (value == "false") {
    out = false;
    return true;
  }
  return false;
}

bool parseU32(std::string_view value, std::uint32_t& out) {
  value = trim(value);
  if (value.empty() || value.front() == '-') {
    return false;
  }
  std::uint64_t parsed = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end ||
      parsed > std::numeric_limits<std::uint32_t>::max()) {
    return false;
  }
  out = static_cast<std::uint32_t>(parsed);
  return true;
}

bool parseI32(std::string_view value, std::int32_t& out) {
  value = trim(value);
  if (value.empty()) {
    return false;
  }
  std::int64_t parsed = 0;
  const auto* begin = value.data();
  const auto* end = value.data() + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc{} || result.ptr != end ||
      parsed < std::numeric_limits<std::int32_t>::min() ||
      parsed > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  out = static_cast<std::int32_t>(parsed);
  return true;
}

bool parseFloat(std::string_view value, float& out) {
  const std::string text(trim(value));
  if (text.empty() || text.front() == '"') {
    return false;
  }
  char* end = nullptr;
  errno = 0;
  const float parsed = std::strtof(text.c_str(), &end);
  if (errno != 0 || end == text.c_str() || *end != '\0' || !std::isfinite(parsed)) {
    return false;
  }
  out = parsed;
  return true;
}

std::vector<std::string_view> splitCommaList(std::string_view value) {
  std::vector<std::string_view> parts;
  std::size_t start = 0;
  bool inString = false;
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '"') {
      inString = !inString;
    } else if (value[index] == ',' && !inString) {
      parts.push_back(trim(value.substr(start, index - start)));
      start = index + 1U;
    }
  }
  parts.push_back(trim(value.substr(start)));
  return parts;
}

bool parseVec3(std::string_view value, Vec3& out) {
  value = trim(value);
  if (value.size() < 2U || value.front() != '[' || value.back() != ']') {
    return false;
  }
  const auto parts = splitCommaList(value.substr(1, value.size() - 2U));
  if (parts.size() != 3U) {
    return false;
  }
  return parseFloat(parts[0], out.x) && parseFloat(parts[1], out.y) && parseFloat(parts[2], out.z);
}

bool parseTargetActions(std::string_view value, std::vector<TargetAction>& out) {
  value = trim(value);
  if (value.size() < 2U || value.front() != '[' || value.back() != ']') {
    return false;
  }
  const std::string_view body = trim(value.substr(1, value.size() - 2U));
  out.clear();
  if (body.empty()) {
    return true;
  }
  for (std::string_view part : splitCommaList(body)) {
    std::string token;
    if (!parseString(part, token)) {
      return false;
    }
    if (token == "Interact") {
      out.push_back(TargetAction::Interact);
    } else if (token == "Inspect") {
      out.push_back(TargetAction::Inspect);
    } else if (token == "Move") {
      out.push_back(TargetAction::Move);
    } else if (token == "Attack") {
      out.push_back(TargetAction::Attack);
    } else {
      return false;
    }
  }
  return true;
}

bool parseCamera(std::string_view value, CameraMode& out) {
  std::string token;
  if (!parseString(value, token)) {
    return false;
  }
  if (token == "FirstPerson") {
    out = CameraMode::FirstPerson;
  } else if (token == "ThirdPerson") {
    out = CameraMode::ThirdPerson;
  } else if (token == "TacticalOverhead") {
    out = CameraMode::TacticalOverhead;
  } else {
    return false;
  }
  return true;
}

bool parsePlayerKind(std::string_view value, PlayerSlotKind& out) {
  std::string token;
  if (!parseString(value, token)) {
    return false;
  }
  if (token == "Local") {
    out = PlayerSlotKind::Local;
  } else if (token == "Remote") {
    out = PlayerSlotKind::Remote;
  } else if (token == "Ai") {
    out = PlayerSlotKind::Ai;
  } else if (token == "Observer") {
    out = PlayerSlotKind::Observer;
  } else {
    return false;
  }
  return true;
}

bool parseEntityKind(std::string_view value, EntityKind& out) {
  std::string token;
  if (!parseString(value, token)) {
    return false;
  }
  if (token == "Player") {
    out = EntityKind::Player;
  } else if (token == "Pickup") {
    out = EntityKind::Pickup;
  } else if (token == "Door") {
    out = EntityKind::Door;
  } else if (token == "Marker") {
    out = EntityKind::Marker;
  } else if (token == "Npc") {
    out = EntityKind::Npc;
  } else {
    return false;
  }
  return true;
}

bool parseInteraction(std::string_view value, InteractionKind& out) {
  std::string token;
  if (!parseString(value, token)) {
    return false;
  }
  if (token == "Pickup") {
    out = InteractionKind::Pickup;
  } else if (token == "OpenDoor") {
    out = InteractionKind::OpenDoor;
  } else if (token == "Inspect") {
    out = InteractionKind::Inspect;
  } else if (token == "Activate") {
    out = InteractionKind::Activate;
  } else {
    return false;
  }
  return true;
}

bool parseObjectiveStatus(std::string_view value, ObjectiveStatusSeed& out) {
  std::string token;
  if (!parseString(value, token)) {
    return false;
  }
  if (token == "Active") {
    out = ObjectiveStatusSeed::Active;
  } else if (token == "Complete") {
    out = ObjectiveStatusSeed::Complete;
  } else if (token == "Failed") {
    out = ObjectiveStatusSeed::Failed;
  } else {
    return false;
  }
  return true;
}

ScenarioLoadResult validateRequired(Parser& parser) {
  if (!parser.scenarioId || parser.result.seed.scenarioId.empty()) {
    return fail(parser, ScenarioLoadStatus::MissingScenarioId, "scenario.missing_id",
                "missing scenario id", 0, 0);
  }
  if (!parser.fixedTick || !parser.interactionRange || !parser.movementDistance ||
      !parser.slowScale || !parser.initialClock || !parser.realtimeCamera ||
      !parser.tacticalCamera) {
    return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                "missing defaults key", 0, 0);
  }
  for (std::size_t index = 0; index < parser.playerFlags.size(); ++index) {
    const PlayerFlags& flags = parser.playerFlags[index];
    if (!flags.slot || !flags.kind || !flags.actor) {
      return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                  "missing player key", flags.startLine, 1);
    }
  }
  for (std::size_t index = 0; index < parser.entityFlags.size(); ++index) {
    const EntityFlags& flags = parser.entityFlags[index];
    ScenarioEntitySeed& entity = parser.result.seed.entities[index];
    if (!flags.stableName || !flags.kind || !flags.active || !flags.persistent || !flags.position ||
        !flags.boundsMin || !flags.boundsMax || !flags.targetable || !flags.targetActions) {
      return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                  "missing entity key", flags.startLine, 1);
    }
    if (entity.interaction.kind == InteractionKind::Pickup &&
        (!flags.itemId || !flags.itemCount || !flags.objectiveRef || !flags.deactivate)) {
      return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                  "missing pickup key", flags.startLine, 1);
    }
    if (entity.combatantEnabled) {
      if (!flags.factionId || !flags.hitPoints || !flags.maxHitPoints) {
        return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                    "missing combatant key", flags.startLine, 1);
      }
      if (entity.combatant.maxHitPoints <= 0 || entity.combatant.hitPoints <= 0 ||
          entity.combatant.hitPoints > entity.combatant.maxHitPoints) {
        return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                    "invalid combatant hit points", flags.startLine, 1);
      }
      entity.combatant.defeated = false;
    } else if (flags.factionId || flags.hitPoints || flags.maxHitPoints) {
      return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                  "combat fields require combatant true", flags.startLine, 1);
    }
  }
  for (const ObjectiveFlags& flags : parser.objectiveFlags) {
    if (!flags.id || !flags.initialStatus || !flags.condition || !flags.playerSlot || !flags.itemId ||
        !flags.itemCount || !flags.completeStatus) {
      return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                  "missing objective key", flags.startLine, 1);
    }
  }
  for (const AiActorFlags& flags : parser.aiActorFlags) {
    if (!flags.actor || !flags.behaviorProfileId) {
      return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                  "missing ai actor key", flags.startLine, 1);
    }
  }
  for (const AiGuardAnchorFlags& flags : parser.aiGuardAnchorFlags) {
    if (!flags.actor || !flags.anchor || !flags.leashRadius || !flags.returnRadius ||
        !flags.homeTolerance) {
      return fail(parser, ScenarioLoadStatus::MissingRequiredKey, "scenario.missing_required_key",
                  "missing ai guard anchor key", flags.startLine, 1);
    }
  }
  return parser.result;
}

}  // namespace

ScenarioLoadResult parseScenarioText(const std::string& scenarioText) {
  Parser parser;
  std::istringstream input(scenarioText);
  std::string rawLine;
  std::uint32_t lineNumber = 0;
  while (std::getline(input, rawLine)) {
    ++lineNumber;
    const std::string line = stripComment(rawLine);
    if (line.empty()) {
      continue;
    }
    if (line == "[scenario]") {
      parser.table = Table::Scenario;
      continue;
    }
    if (line == "[defaults]") {
      parser.table = Table::Defaults;
      continue;
    }
    if (line == "[[players]]") {
      parser.table = Table::Player;
      parser.result.seed.players.push_back({});
      parser.playerFlags.push_back(PlayerFlags{.startLine = lineNumber});
      continue;
    }
    if (line == "[[entities]]") {
      parser.table = Table::Entity;
      parser.result.seed.entities.push_back({});
      parser.entityFlags.push_back(EntityFlags{.startLine = lineNumber});
      continue;
    }
    if (line == "[[objectives]]") {
      parser.table = Table::Objective;
      parser.result.seed.objectives.push_back({});
      parser.objectiveFlags.push_back(ObjectiveFlags{.startLine = lineNumber});
      continue;
    }
    if (line == "[[ai_actors]]") {
      parser.table = Table::AiActor;
      parser.result.seed.aiActors.push_back({});
      parser.aiActorFlags.push_back(AiActorFlags{.startLine = lineNumber});
      continue;
    }
    if (line == "[[ai_guard_anchors]]") {
      parser.table = Table::AiGuardAnchor;
      parser.result.seed.aiGuardAnchors.push_back({});
      parser.aiGuardAnchorFlags.push_back(AiGuardAnchorFlags{.startLine = lineNumber});
      continue;
    }
    if (line.starts_with("[")) {
      return fail(parser, ScenarioLoadStatus::UnsupportedKey, "scenario.unsupported_key",
                  "unsupported table", lineNumber, 1);
    }

    const std::size_t equals = line.find('=');
    if (equals == std::string::npos) {
      return fail(parser, ScenarioLoadStatus::ParseError, "scenario.parse_error", "expected key",
                  lineNumber, 1);
    }
    const std::string key(trim(std::string_view(line).substr(0, equals)));
    const std::string_view value = trim(std::string_view(line).substr(equals + 1U));
    const std::uint32_t column = static_cast<std::uint32_t>(rawLine.find(key) + 1U);

    if (parser.table == Table::Scenario) {
      if (key != "id") {
        return fail(parser, ScenarioLoadStatus::UnsupportedKey, "scenario.unsupported_key",
                    "unsupported scenario key", lineNumber, column);
      }
      parser.scenarioId = true;
      if (!parseString(value, parser.result.seed.scenarioId)) {
        return fail(parser, ScenarioLoadStatus::ParseError, "scenario.parse_error",
                    "invalid scenario id", lineNumber, column);
      }
    } else if (parser.table == Table::Defaults) {
      if (key == "fixed_tick_rate_hz") {
        parser.fixedTick = parseU32(value, parser.result.seed.config.fixedTickRateHz);
        if (!parser.fixedTick) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid fixed tick rate", lineNumber, column);
        }
      } else if (key == "interaction_range_meters") {
        parser.interactionRange = parseFloat(value, parser.result.seed.config.interactionRangeMeters);
        if (!parser.interactionRange) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid interaction range", lineNumber, column);
        }
      } else if (key == "movement_distance_meters") {
        parser.movementDistance = parseFloat(value, parser.result.seed.config.movementDistanceMeters);
        if (!parser.movementDistance) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid movement distance", lineNumber, column);
        }
      } else if (key == "slow_time_scale") {
        parser.slowScale = parseFloat(value, parser.result.seed.config.slowTimeScale);
        if (!parser.slowScale) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid slow time scale", lineNumber, column);
        }
      } else if (key == "initial_clock") {
        std::string token;
        if (!parseString(value, token) || token != "Normal") {
          return fail(parser, ScenarioLoadStatus::InvalidEnum, "scenario.invalid_enum",
                      "invalid initial_clock " + token + ", expected Normal", lineNumber, column);
        }
        parser.initialClock = true;
        parser.result.seed.initialClockMode = ClockMode::Normal;
      } else if (key == "default_realtime_camera") {
        parser.realtimeCamera = parseCamera(value, parser.result.seed.defaultRealtimeCamera);
        if (!parser.realtimeCamera) {
          return fail(parser, ScenarioLoadStatus::InvalidEnum, "scenario.invalid_enum",
                      "invalid realtime camera", lineNumber, column);
        }
      } else if (key == "default_tactical_camera") {
        parser.tacticalCamera = parseCamera(value, parser.result.seed.defaultTacticalCamera);
        if (!parser.tacticalCamera) {
          return fail(parser, ScenarioLoadStatus::InvalidEnum, "scenario.invalid_enum",
                      "invalid tactical camera", lineNumber, column);
        }
      } else {
        return fail(parser, ScenarioLoadStatus::UnsupportedKey, "scenario.unsupported_key",
                    "unsupported defaults key", lineNumber, column);
      }
    } else if (parser.table == Table::Player && !parser.result.seed.players.empty()) {
      ScenarioPlayerSeed& player = parser.result.seed.players.back();
      PlayerFlags& flags = parser.playerFlags.back();
      if (key == "slot") {
        flags.slot = parseU32(value, player.slot);
      } else if (key == "kind") {
        flags.kind = parsePlayerKind(value, player.kind);
      } else if (key == "actor") {
        flags.actor = parseString(value, player.actorStableName);
      } else {
        return fail(parser, ScenarioLoadStatus::UnsupportedKey, "scenario.unsupported_key",
                    "unsupported player key", lineNumber, column);
      }
      if ((!flags.slot && key == "slot") || (!flags.kind && key == "kind") ||
          (!flags.actor && key == "actor")) {
        return fail(parser, key == "slot" ? ScenarioLoadStatus::InvalidNumber
                                           : ScenarioLoadStatus::InvalidEnum,
                    key == "slot" ? "scenario.invalid_number" : "scenario.invalid_enum",
                    "invalid player value", lineNumber, column);
      }
    } else if (parser.table == Table::Entity && !parser.result.seed.entities.empty()) {
      ScenarioEntitySeed& entity = parser.result.seed.entities.back();
      EntityFlags& flags = parser.entityFlags.back();
      if (key == "stable_name") {
        flags.stableName = parseString(value, entity.stableName);
      } else if (key == "kind") {
        flags.kind = parseEntityKind(value, entity.kind);
      } else if (key == "active") {
        flags.active = parseBool(value, entity.active);
      } else if (key == "persistent") {
        flags.persistent = parseBool(value, entity.persistent);
      } else if (key == "position") {
        flags.position = parseVec3(value, entity.transform.position);
      } else if (key == "bounds_min") {
        flags.boundsMin = parseVec3(value, entity.localBounds.min);
      } else if (key == "bounds_max") {
        flags.boundsMax = parseVec3(value, entity.localBounds.max);
      } else if (key == "targetable") {
        flags.targetable = parseBool(value, entity.targeting.targetable);
      } else if (key == "target_actions") {
        flags.targetActions = parseTargetActions(value, entity.targeting.actions);
      } else if (key == "combatant") {
        bool enabled = false;
        flags.combatant = parseBool(value, enabled);
        entity.combatantEnabled = enabled;
      } else if (key == "faction_id") {
        flags.factionId = parseU32(value, entity.combatant.factionId);
      } else if (key == "hit_points") {
        flags.hitPoints = parseI32(value, entity.combatant.hitPoints);
      } else if (key == "max_hit_points") {
        flags.maxHitPoints = parseI32(value, entity.combatant.maxHitPoints);
      } else if (key == "item_id") {
        flags.itemId = parseString(value, entity.interaction.itemId);
      } else if (key == "item_count") {
        flags.itemCount = parseU32(value, entity.interaction.itemCount);
      } else if (key == "interaction") {
        flags.interaction = parseInteraction(value, entity.interaction.kind);
        if (flags.interaction && entity.interaction.kind == InteractionKind::Pickup) {
          entity.interaction.primaryEffect = InteractionEffectKind::AddItemToInventory;
        }
      } else if (key == "deactivate_on_success") {
        flags.deactivate = parseBool(value, entity.interaction.deactivateTargetOnSuccess);
      } else if (key == "objective_ref") {
        flags.objectiveRef = parseString(value, entity.interaction.objectiveId);
      } else {
        return fail(parser, ScenarioLoadStatus::UnsupportedKey, "scenario.unsupported_key",
                    "unsupported entity key", lineNumber, column);
      }
      const bool ok = (key == "stable_name" && flags.stableName) || (key == "kind" && flags.kind) ||
                      (key == "active" && flags.active) || (key == "persistent" && flags.persistent) ||
                      (key == "position" && flags.position) ||
                      (key == "bounds_min" && flags.boundsMin) ||
                      (key == "bounds_max" && flags.boundsMax) ||
                      (key == "targetable" && flags.targetable) ||
                      (key == "target_actions" && flags.targetActions) ||
                      (key == "combatant" && flags.combatant) ||
                      (key == "faction_id" && flags.factionId) ||
                      (key == "hit_points" && flags.hitPoints) ||
                      (key == "max_hit_points" && flags.maxHitPoints) ||
                      (key == "item_id" && flags.itemId) || (key == "item_count" && flags.itemCount) ||
                      (key == "interaction" && flags.interaction) ||
                      (key == "deactivate_on_success" && flags.deactivate) ||
                      (key == "objective_ref" && flags.objectiveRef);
      if (!ok) {
        const bool numberKey = key == "position" || key == "bounds_min" || key == "bounds_max" ||
                               key == "item_count" || key == "faction_id" ||
                               key == "hit_points" || key == "max_hit_points";
        return fail(parser, numberKey ? ScenarioLoadStatus::InvalidNumber
                                      : ScenarioLoadStatus::InvalidEnum,
                    numberKey ? "scenario.invalid_number" : "scenario.invalid_enum",
                    "invalid entity value", lineNumber, column);
      }
    } else if (parser.table == Table::Objective && !parser.result.seed.objectives.empty()) {
      ScenarioObjectiveSeed& objective = parser.result.seed.objectives.back();
      ObjectiveFlags& flags = parser.objectiveFlags.back();
      if (key == "id") {
        flags.id = parseString(value, objective.id);
      } else if (key == "initial_status") {
        flags.initialStatus = parseObjectiveStatus(value, objective.initialStatus);
      } else if (key == "condition") {
        flags.condition = parseString(value, objective.condition);
      } else if (key == "player_slot") {
        flags.playerSlot = parseU32(value, objective.playerSlot);
      } else if (key == "item_id") {
        flags.itemId = parseString(value, objective.itemId);
      } else if (key == "item_count") {
        flags.itemCount = parseU32(value, objective.itemCount);
      } else if (key == "complete_status") {
        flags.completeStatus = parseObjectiveStatus(value, objective.completeStatus);
      } else {
        return fail(parser, ScenarioLoadStatus::UnsupportedKey, "scenario.unsupported_key",
                    "unsupported objective key", lineNumber, column);
      }
      const bool ok = (key == "id" && flags.id) || (key == "initial_status" && flags.initialStatus) ||
                      (key == "condition" && flags.condition) ||
                      (key == "player_slot" && flags.playerSlot) ||
                      (key == "item_id" && flags.itemId) || (key == "item_count" && flags.itemCount) ||
                      (key == "complete_status" && flags.completeStatus);
      if (!ok) {
        const bool numberKey = key == "player_slot" || key == "item_count";
        return fail(parser, numberKey ? ScenarioLoadStatus::InvalidNumber
                                      : ScenarioLoadStatus::InvalidEnum,
                    numberKey ? "scenario.invalid_number" : "scenario.invalid_enum",
                    "invalid objective value", lineNumber, column);
      }
    } else if (parser.table == Table::AiActor && !parser.result.seed.aiActors.empty()) {
      ScenarioAiActorSeed& aiActor = parser.result.seed.aiActors.back();
      AiActorFlags& flags = parser.aiActorFlags.back();
      if (key == "actor") {
        flags.actor = parseString(value, aiActor.actorStableName);
        if (!flags.actor) {
          return fail(parser, ScenarioLoadStatus::ParseError, "scenario.parse_error",
                      "invalid ai actor stable name", lineNumber, column);
        }
      } else if (key == "behavior_profile_id") {
        flags.behaviorProfileId = parseString(value, aiActor.behaviorProfileId);
        if (!flags.behaviorProfileId) {
          return fail(parser, ScenarioLoadStatus::ParseError, "scenario.parse_error",
                      "invalid ai actor profile id", lineNumber, column);
        }
        if (!isValidNpcBehaviorProfileId(aiActor.behaviorProfileId)) {
          return fail(parser, ScenarioLoadStatus::InvalidEnum, "scenario.invalid_enum",
                      "invalid ai actor profile id", lineNumber, column);
        }
      } else if (key == "facing_degrees") {
        if (!parseFloat(value, aiActor.facingDegrees)) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid ai actor facing degrees", lineNumber, column);
        }
        aiActor.hasFacing = true;
      } else if (key == "waypoint") {
        // Repeatable: each `waypoint = [x,y,z]` appends to the ordered patrol route.
        Vec3 waypoint;
        if (!parseVec3(value, waypoint)) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid ai actor waypoint", lineNumber, column);
        }
        aiActor.patrolWaypoints.push_back(waypoint);
      } else if (key == "patrol_mode") {
        std::string mode;
        if (!parseString(value, mode)) {
          return fail(parser, ScenarioLoadStatus::ParseError, "scenario.parse_error",
                      "invalid ai actor patrol mode", lineNumber, column);
        }
        if (mode == "loop") {
          aiActor.patrolMode = PatrolMode::Loop;
        } else if (mode == "ping_pong") {
          aiActor.patrolMode = PatrolMode::PingPong;
        } else {
          return fail(parser, ScenarioLoadStatus::InvalidEnum, "scenario.invalid_enum",
                      "invalid ai actor patrol mode", lineNumber, column);
        }
      } else {
        return fail(parser, ScenarioLoadStatus::UnsupportedKey, "scenario.unsupported_key",
                    "unsupported ai actor key", lineNumber, column);
      }
    } else if (parser.table == Table::AiGuardAnchor &&
               !parser.result.seed.aiGuardAnchors.empty()) {
      ScenarioAiGuardAnchorSeed& guard = parser.result.seed.aiGuardAnchors.back();
      AiGuardAnchorFlags& flags = parser.aiGuardAnchorFlags.back();
      if (key == "actor") {
        flags.actor = parseString(value, guard.actorStableName);
        if (!flags.actor) {
          return fail(parser, ScenarioLoadStatus::ParseError, "scenario.parse_error",
                      "invalid ai guard actor stable name", lineNumber, column);
        }
      } else if (key == "anchor") {
        flags.anchor = parseString(value, guard.anchorStableName);
        if (!flags.anchor) {
          return fail(parser, ScenarioLoadStatus::ParseError, "scenario.parse_error",
                      "invalid ai guard anchor stable name", lineNumber, column);
        }
      } else if (key == "leash_radius_meters") {
        flags.leashRadius = parseFloat(value, guard.leashRadiusMeters);
        if (!flags.leashRadius) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid ai guard leash radius", lineNumber, column);
        }
      } else if (key == "return_radius_meters") {
        flags.returnRadius = parseFloat(value, guard.returnRadiusMeters);
        if (!flags.returnRadius) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid ai guard return radius", lineNumber, column);
        }
      } else if (key == "home_tolerance_meters") {
        flags.homeTolerance = parseFloat(value, guard.homeToleranceMeters);
        if (!flags.homeTolerance) {
          return fail(parser, ScenarioLoadStatus::InvalidNumber, "scenario.invalid_number",
                      "invalid ai guard home tolerance", lineNumber, column);
        }
      } else {
        return fail(parser, ScenarioLoadStatus::UnsupportedKey, "scenario.unsupported_key",
                    "unsupported ai guard anchor key", lineNumber, column);
      }
    } else {
      return fail(parser, ScenarioLoadStatus::ParseError, "scenario.parse_error",
                  "key outside supported table", lineNumber, column);
    }
  }
  return validateRequired(parser);
}

}  // namespace iggy3d
