#include "EditorControls.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

constexpr std::string_view kControlFileHeader =
    "iggy3d_creative_controls 2";
constexpr std::string_view kLegacyControlFileHeader =
    "iggy3d_creative_controls 1";
[[nodiscard]] bool parseDevice(std::string_view value,
                               cr::CreativeControlDevice& out) noexcept {
  if (value == "KeyboardMouse") {
    out = cr::CreativeControlDevice::KeyboardMouse;
    return true;
  }
  if (value == "Gamepad") {
    out = cr::CreativeControlDevice::Gamepad;
    return true;
  }
  return false;
}

CreativeEditorControlPersistenceReceipt parseControlProfileStream(
    std::istream& input,
    cr::CreativeControlProfile& profile) {
  CreativeEditorControlPersistenceReceipt receipt;
  std::string header;
  std::getline(input, header);
  const bool legacyV1 = header == kLegacyControlFileHeader;
  if (header != kControlFileHeader && !legacyV1) {
    receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
    return receipt;
  }

  cr::CreativeControlProfile candidate =
      cr::makeDefaultCreativeControlProfile();
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream row(line);
    std::string kind;
    row >> kind;
    if (kind == "mouse_sensitivity") {
      row >> candidate.mouseLookSensitivity;
    } else if (kind == "gamepad_sensitivity") {
      row >> candidate.gamepadLookSensitivity;
    } else if (kind == "movement_stick") {
      int invertX = 0;
      int invertY = 0;
      row >> candidate.movementStick.deadzone >>
          candidate.movementStick.responseExponent >> invertX >> invertY;
      candidate.movementStick.invertX = invertX != 0;
      candidate.movementStick.invertY = invertY != 0;
    } else if (kind == "look_stick") {
      int invertX = 0;
      int invertY = 0;
      row >> candidate.lookStick.deadzone >>
          candidate.lookStick.responseExponent >> invertX >> invertY;
      candidate.lookStick.invertX = invertX != 0;
      candidate.lookStick.invertY = invertY != 0;
    } else if (kind == "menu_repeat") {
      row >> candidate.menuRepeatDelayMilliseconds >>
          candidate.menuRepeatIntervalMilliseconds;
    } else if (kind == "bind") {
      std::string actionName;
      std::string deviceName;
      std::uint16_t ordinal = 0;
      std::string keyName;
      unsigned requiredAll = 0;
      unsigned requiredAny = 0;
      unsigned allowed = 0;
      row >> actionName >> deviceName >> ordinal >> keyName >> requiredAll >>
          requiredAny >> allowed;
      cr::CreativeInputActionId action = cr::CreativeInputActionId::Count;
      cr::CreativeControlDevice device = cr::CreativeControlDevice::Count;
      cr::CreativeInputKey key = cr::CreativeInputKey::Count;
      if (!cr::parseCreativeInputActionId(actionName, action) ||
          !parseDevice(deviceName, device) ||
          !cr::parseCreativeInputKey(keyName, key)) {
        receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
        return receipt;
      }
      if (legacyV1 && action == cr::CreativeInputActionId::PickAction &&
          device == cr::CreativeControlDevice::Gamepad && ordinal == 0U &&
          key == cr::CreativeInputKey::GamepadWest) {
        key = cr::CreativeInputKey::GamepadTouchpad;
      }
      std::size_t groupIndexValue = candidate.groupCount;
      for (std::size_t index = 0; index < candidate.groupCount; ++index) {
        if (candidate.groupActions[index] == action &&
            candidate.groupDevices[index] == device &&
            candidate.groupOrdinals[index] == ordinal) {
          groupIndexValue = index;
          break;
        }
      }
      if (groupIndexValue == candidate.groupCount) {
        receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
        return receipt;
      }
      const std::uint16_t groupIndex = static_cast<std::uint16_t>(
          groupIndexValue);
      if (!cr::applyStoredCreativeControlChord(
              candidate, groupIndex, key,
              static_cast<cr::CreativeInputModifierMask>(requiredAll),
              static_cast<cr::CreativeInputModifierMask>(requiredAny),
              static_cast<cr::CreativeInputModifierMask>(allowed))) {
        receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
        return receipt;
      }
    } else {
      receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
      return receipt;
    }
    if (!row) {
      receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
      return receipt;
    }
    row >> std::ws;
    if (!row.eof()) {
      receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
      return receipt;
    }
  }
  if (!input.eof() || !cr::isValidCreativeControlProfile(candidate)) {
    receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
    return receipt;
  }
  profile = candidate;
  receipt.status = CreativeEditorControlPersistenceStatus::Loaded;
  receipt.bindingCount = profile.bindingCount;
  receipt.accepted = true;
  return receipt;
}

CreativeEditorControlPersistenceReceipt serializeControlProfileStream(
    std::ostream& output,
    const cr::CreativeControlProfile& profile) {
  CreativeEditorControlPersistenceReceipt receipt;
  receipt.status = CreativeEditorControlPersistenceStatus::IoError;
  if (!cr::isValidCreativeControlProfile(profile)) {
    receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
    return receipt;
  }
  output << kControlFileHeader << '\n' << std::setprecision(
      std::numeric_limits<float>::max_digits10);
  output << "mouse_sensitivity " << profile.mouseLookSensitivity << '\n';
  output << "gamepad_sensitivity " << profile.gamepadLookSensitivity << '\n';
  output << "movement_stick " << profile.movementStick.deadzone << ' '
         << profile.movementStick.responseExponent << ' '
         << (profile.movementStick.invertX ? 1 : 0) << ' '
         << (profile.movementStick.invertY ? 1 : 0) << '\n';
  output << "look_stick " << profile.lookStick.deadzone << ' '
         << profile.lookStick.responseExponent << ' '
         << (profile.lookStick.invertX ? 1 : 0) << ' '
         << (profile.lookStick.invertY ? 1 : 0) << '\n';
  output << "menu_repeat " << profile.menuRepeatDelayMilliseconds << ' '
         << profile.menuRepeatIntervalMilliseconds << '\n';
  for (std::size_t group = 0; group < profile.groupCount; ++group) {
    if (cr::creativeControlActionIsReserved(profile.groupActions[group])) {
      continue;
    }
    const cr::CreativeInputBinding* binding = cr::creativeControlGroupBinding(
        profile, static_cast<std::uint16_t>(group));
    if (binding == nullptr) {
      receipt.status = CreativeEditorControlPersistenceStatus::Invalid;
      return receipt;
    }
    output << "bind " << cr::toString(profile.groupActions[group]) << ' '
           << cr::toString(profile.groupDevices[group]) << ' '
           << profile.groupOrdinals[group] << ' '
           << cr::toString(binding->trigger) << ' '
           << static_cast<unsigned>(binding->requiredAllModifiers) << ' '
           << static_cast<unsigned>(binding->requiredAnyModifiers) << ' '
           << static_cast<unsigned>(binding->allowedModifiers) << '\n';
  }
  if (!output) {
    return receipt;
  }
  receipt.status = CreativeEditorControlPersistenceStatus::Saved;
  receipt.bindingCount = profile.bindingCount;
  receipt.accepted = true;
  return receipt;
}

}  // namespace

CreativeEditorControlPersistenceReceipt parseCreativeEditorControlProfile(
    std::string_view text,
    cr::CreativeControlProfile& profile) {
  std::istringstream input{std::string{text}};
  return parseControlProfileStream(input, profile);
}

CreativeEditorControlPersistenceReceipt serializeCreativeEditorControlProfile(
    const cr::CreativeControlProfile& profile,
    std::string& text) {
  std::ostringstream output;
  CreativeEditorControlPersistenceReceipt receipt =
      serializeControlProfileStream(output, profile);
  if (receipt.accepted) {
    text = output.str();
  } else {
    text.clear();
  }
  return receipt;
}

CreativeEditorControlPersistenceReceipt loadCreativeEditorControlProfile(
    cr::CreativeControlProfile& profile,
    const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    return {};
  }
  return parseControlProfileStream(input, profile);
}

CreativeEditorControlPersistenceReceipt saveCreativeEditorControlProfile(
    const cr::CreativeControlProfile& profile,
    const std::filesystem::path& path) {
  std::string text;
  CreativeEditorControlPersistenceReceipt receipt =
      serializeCreativeEditorControlProfile(profile, text);
  if (!receipt.accepted) {
    return receipt;
  }

  receipt.accepted = false;
  receipt.status = CreativeEditorControlPersistenceStatus::IoError;
  std::error_code error;
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) {
      return receipt;
    }
  }
  const std::filesystem::path temporary = path.string() + ".tmp";
  std::ofstream output(temporary, std::ios::trunc);
  if (!output.is_open()) {
    return receipt;
  }
  output << text;
  output.close();
  if (!output) {
    std::filesystem::remove(temporary, error);
    return receipt;
  }
  std::filesystem::rename(temporary, path, error);
  if (error) {
    error.clear();
    std::filesystem::remove(path, error);
    error.clear();
    std::filesystem::rename(temporary, path, error);
  }
  if (error) {
    std::filesystem::remove(temporary, error);
    return receipt;
  }
  receipt.status = CreativeEditorControlPersistenceStatus::Saved;
  receipt.bindingCount = profile.bindingCount;
  receipt.accepted = true;
  return receipt;
}

}  // namespace iggy3d_creative_app
