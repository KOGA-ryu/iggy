#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool replaceFirstOpeningFacing(std::string& encoded,
                               std::string_view replacement) {
  const std::size_t line = encoded.find("\nO ");
  if (line == std::string::npos) {
    return false;
  }
  std::size_t tokenStart = line + 3U;
  for (std::size_t field = 0U; field < 5U; ++field) {
    tokenStart = encoded.find(' ', tokenStart);
    if (tokenStart == std::string::npos) {
      return false;
    }
    ++tokenStart;
  }
  const std::size_t tokenEnd = encoded.find(' ', tokenStart);
  if (tokenEnd == std::string::npos) {
    return false;
  }
  encoded.replace(tokenStart, tokenEnd - tokenStart, replacement);
  return true;
}

bool replaceFirstLevelToken(std::string& encoded,
                            std::size_t tokenIndex,
                            std::string_view replacement) {
  const std::size_t marker = encoded.find("\nV ");
  if (marker == std::string::npos) {
    return false;
  }
  const std::size_t lineEnd = encoded.find('\n', marker + 1U);
  if (lineEnd == std::string::npos) {
    return false;
  }
  std::size_t tokenStart = marker + 1U;
  for (std::size_t index = 0U; index < tokenIndex; ++index) {
    tokenStart = encoded.find(' ', tokenStart);
    if (tokenStart == std::string::npos || tokenStart >= lineEnd) {
      return false;
    }
    ++tokenStart;
  }
  std::size_t tokenEnd = encoded.find(' ', tokenStart);
  if (tokenEnd == std::string::npos || tokenEnd > lineEnd) {
    tokenEnd = lineEnd;
  }
  encoded.replace(tokenStart, tokenEnd - tokenStart, replacement);
  return true;
}

bool replaceFirstRoofApertureToken(std::string& encoded,
                                   std::size_t tokenIndex,
                                   std::string_view replacement) {
  const std::size_t marker = encoded.find("\nA ");
  if (marker == std::string::npos) {
    return false;
  }
  const std::size_t lineEnd = encoded.find('\n', marker + 1U);
  if (lineEnd == std::string::npos) {
    return false;
  }
  std::size_t tokenStart = marker + 1U;
  for (std::size_t index = 0U; index < tokenIndex; ++index) {
    tokenStart = encoded.find(' ', tokenStart);
    if (tokenStart == std::string::npos || tokenStart >= lineEnd) {
      return false;
    }
    ++tokenStart;
  }
  std::size_t tokenEnd = encoded.find(' ', tokenStart);
  if (tokenEnd == std::string::npos || tokenEnd > lineEnd) {
    tokenEnd = lineEnd;
  }
  encoded.replace(tokenStart, tokenEnd - tokenStart, replacement);
  return true;
}

bool replaceFirstTerrainProfileToken(std::string& encoded,
                                     std::size_t tokenIndex,
                                     std::string_view replacement) {
  const std::size_t marker = encoded.find("\nP ");
  if (marker == std::string::npos) {
    return false;
  }
  const std::size_t lineEnd = encoded.find('\n', marker + 1U);
  if (lineEnd == std::string::npos) {
    return false;
  }
  std::size_t tokenStart = marker + 1U;
  for (std::size_t index = 0U; index < tokenIndex; ++index) {
    tokenStart = encoded.find(' ', tokenStart);
    if (tokenStart == std::string::npos || tokenStart >= lineEnd) {
      return false;
    }
    ++tokenStart;
  }
  std::size_t tokenEnd = encoded.find(' ', tokenStart);
  if (tokenEnd == std::string::npos || tokenEnd > lineEnd) {
    tokenEnd = lineEnd;
  }
  encoded.replace(tokenStart, tokenEnd - tokenStart, replacement);
  return true;
}

bool writeLegacyTerrainProfileLine(std::ostringstream& output,
                                   std::string_view line) {
  std::istringstream fields{std::string(line)};
  std::vector<std::string> tokens;
  for (std::string token; fields >> token;) {
    tokens.push_back(std::move(token));
  }
  constexpr std::size_t kLegacyProfileTokenCount = 13U;
  if (tokens.size() < kLegacyProfileTokenCount) {
    return false;
  }
  tokens.resize(kLegacyProfileTokenCount);
  for (std::size_t index = 0U; index < tokens.size(); ++index) {
    output << (index == 0U ? "" : " ") << tokens[index];
  }
  output << '\n';
  return true;
}

bool writeProfileWithoutRetainingFields(std::ostringstream& output,
                                        std::string_view line) {
  if (!line.starts_with("P ")) {
    output << line << '\n';
    return true;
  }
  std::istringstream fields{std::string(line)};
  std::vector<std::string> tokens;
  for (std::string token; fields >> token;) {
    tokens.push_back(std::move(token));
  }
  // v26 appends retaining-edge source state after the v22 landform seed.
  constexpr std::size_t kVersionTwentyFiveProfileTokenCount = 32U;
  if (tokens.size() < kVersionTwentyFiveProfileTokenCount) {
    return false;
  }
  tokens.resize(kVersionTwentyFiveProfileTokenCount);
  for (std::size_t index = 0U; index < tokens.size(); ++index) {
    output << (index == 0U ? "" : " ") << tokens[index];
  }
  output << '\n';
  return true;
}

bool writeLegacyObjectLine(std::ostringstream& output,
                           std::string_view line) {
  if (!line.starts_with("Y ")) {
    output << line << '\n';
    return true;
  }
  std::istringstream fields{std::string(line)};
  std::vector<std::string> tokens;
  for (std::string token; fields >> token;) {
    tokens.push_back(std::move(token));
  }
  // v25 inserts bridge source state and v27 appends player-spawn state after
  // scale. Neither field family exists in the legacy object record.
  constexpr std::size_t kBridgeFieldsBegin = 27U;
  constexpr std::size_t kCurrentExtensionFieldsEnd = 53U;
  if (tokens.size() <= kCurrentExtensionFieldsEnd) {
    return false;
  }
  tokens.erase(tokens.begin() + kBridgeFieldsBegin,
               tokens.begin() + kCurrentExtensionFieldsEnd);
  for (std::size_t index = 0U; index < tokens.size(); ++index) {
    output << (index == 0U ? "" : " ") << tokens[index];
  }
  output << '\n';
  return true;
}

std::string versionTwentyFourTextWithoutBridgeFields(std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  const std::size_t header = encoded.find(currentHeader);
  if (header == std::string::npos) {
    return {};
  }
  encoded.replace(header, currentHeader.size(), "IGGY3D_WORLD_LAYOUT 24");

  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  const std::size_t layout = encoded.find(currentLayout);
  if (layout == std::string::npos) {
    return {};
  }
  encoded.replace(layout, currentLayout.size(), "L 24 ");

  std::istringstream input(encoded);
  std::ostringstream output;
  for (std::string line; std::getline(input, line);) {
    if (line.starts_with("P ")) {
      if (!writeProfileWithoutRetainingFields(output, line)) {
        return {};
      }
      continue;
    }
    if (!writeLegacyObjectLine(output, line)) {
      return {};
    }
  }
  return output.str();
}

std::string versionTwentySixTextWithoutPlayerSpawnFields(std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  const std::size_t header = encoded.find(currentHeader);
  if (header == std::string::npos) {
    return {};
  }
  encoded.replace(header, currentHeader.size(), "IGGY3D_WORLD_LAYOUT 26");

  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  const std::size_t layout = encoded.find(currentLayout);
  if (layout == std::string::npos) {
    return {};
  }
  encoded.replace(layout, currentLayout.size(), "L 26 ");

  std::istringstream lines(encoded);
  std::ostringstream output;
  for (std::string line; std::getline(lines, line);) {
    if (!line.starts_with("Y ")) {
      output << line << '\n';
      continue;
    }
    std::istringstream fields(line);
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    constexpr std::size_t kPlayerSpawnFieldsBegin = 49U;
    constexpr std::size_t kPlayerSpawnFieldsEnd = 53U;
    if (tokens.size() <= kPlayerSpawnFieldsEnd) {
      return {};
    }
    tokens.erase(tokens.begin() + kPlayerSpawnFieldsBegin,
                 tokens.begin() + kPlayerSpawnFieldsEnd);
    for (std::size_t index = 0U; index < tokens.size(); ++index) {
      output << (index == 0U ? "" : " ") << tokens[index];
    }
    output << '\n';
  }
  return output.str();
}

bool writeLineWithoutConnectorMaterial(std::ostringstream& output,
                                       std::string_view line) {
  if (line.starts_with("P ")) {
    return writeLegacyTerrainProfileLine(output, line);
  }
  if (line.starts_with("L ")) {
    std::istringstream fields{std::string(line)};
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    if (tokens.size() < 3U) {
      return false;
    }
    // Versions before 20 have neither the v24 crossing count nor the v20
    // roof-aperture count at the tail of the layout record.
    tokens.pop_back();
    tokens.pop_back();
    for (std::size_t index = 0U; index < tokens.size(); ++index) {
      output << (index == 0U ? "" : " ") << tokens[index];
    }
    output << '\n';
    return true;
  }
  if (line.starts_with("V ")) {
    std::istringstream fields{std::string(line)};
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    if (tokens.size() != 15U) {
      return false;
    }
    for (std::size_t index = 0U; index < tokens.size(); ++index) {
      if (index == 11U || index == 14U) {
        continue;
      }
      output << (index == 0U ? "" : " ") << tokens[index];
    }
    output << '\n';
    return true;
  }
  if (line.starts_with("Y ")) {
    return writeLegacyObjectLine(output, line);
  }
  if (!line.starts_with("C ")) {
    output << line << '\n';
    return true;
  }
  const std::size_t lastSeparator = line.rfind(' ');
  if (lastSeparator == std::string_view::npos) {
    return false;
  }
  output << line.substr(0U, lastSeparator) << '\n';
  return true;
}

std::string versionEighteenTextWithoutRoofDirectionAndMaterial(
    std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  const std::size_t header = encoded.find(currentHeader);
  if (header == std::string::npos) {
    return {};
  }
  encoded.replace(header, currentHeader.size(), "IGGY3D_WORLD_LAYOUT 18");
  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  const std::size_t layout = encoded.find(currentLayout);
  if (layout == std::string::npos) {
    return {};
  }
  encoded.replace(layout, currentLayout.size(), "L 18 ");

  std::istringstream input(encoded);
  std::ostringstream output;
  for (std::string line; std::getline(input, line);) {
    if (line.starts_with("L ")) {
      std::istringstream fields(line);
      std::vector<std::string> tokens;
      for (std::string token; fields >> token;) {
        tokens.push_back(std::move(token));
      }
      if (tokens.size() < 3U) {
        return {};
      }
      // Version 18 predates both trailing counts.
      tokens.pop_back();
      tokens.pop_back();
      for (std::size_t index = 0U; index < tokens.size(); ++index) {
        output << (index == 0U ? "" : " ") << tokens[index];
      }
      output << '\n';
      continue;
    }
    if (line.starts_with("P ")) {
      if (!writeLegacyTerrainProfileLine(output, line)) {
        return {};
      }
      continue;
    }
    if (!line.starts_with("V ")) {
      if (!writeLegacyObjectLine(output, line)) {
        return {};
      }
      continue;
    }
    std::istringstream fields(line);
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    if (tokens.size() != 15U) {
      return {};
    }
    bool first = true;
    for (std::size_t index = 0U; index < tokens.size(); ++index) {
      if (index == 11U || index == 14U) {
        continue;
      }
      output << (first ? "" : " ") << tokens[index];
      first = false;
    }
    output << '\n';
  }
  return output.str();
}

std::string versionNineteenTextWithoutRoofApertures(std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  const std::size_t header = encoded.find(currentHeader);
  if (header == std::string::npos) {
    return {};
  }
  encoded.replace(header, currentHeader.size(), "IGGY3D_WORLD_LAYOUT 19");

  std::istringstream input(encoded);
  std::ostringstream output;
  for (std::string line; std::getline(input, line);) {
    if (line.starts_with("A ") || line.starts_with("K ")) {
      continue;
    }
    if (line.starts_with("P ")) {
      if (!writeLegacyTerrainProfileLine(output, line)) {
        return {};
      }
      continue;
    }
    if (!line.starts_with("L ")) {
      if (!writeLegacyObjectLine(output, line)) {
        return {};
      }
      continue;
    }
    std::istringstream fields(line);
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    if (tokens.size() < 3U) {
      return {};
    }
    tokens[1] = "19";
    // Version 19 predates both trailing counts.
    tokens.pop_back();
    tokens.pop_back();
    for (std::size_t index = 0U; index < tokens.size(); ++index) {
      output << (index == 0U ? "" : " ") << tokens[index];
    }
    output << '\n';
  }
  return output.str();
}

std::string versionTwentyOneTextWithoutLandformFields(std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  const std::size_t header = encoded.find(currentHeader);
  if (header == std::string::npos) {
    return {};
  }
  encoded.replace(header, currentHeader.size(), "IGGY3D_WORLD_LAYOUT 21");
  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  const std::size_t layout = encoded.find(currentLayout);
  if (layout == std::string::npos) {
    return {};
  }
  encoded.replace(layout, currentLayout.size(), "L 21 ");

  std::istringstream input(encoded);
  std::ostringstream output;
  for (std::string line; std::getline(input, line);) {
    if (line.starts_with("K ")) {
      continue;
    }
    if (line.starts_with("L ")) {
      std::istringstream fields(line);
      std::vector<std::string> tokens;
      for (std::string token; fields >> token;) {
        tokens.push_back(std::move(token));
      }
      constexpr std::size_t kCurrentLayoutTokenCount = 20U;
      constexpr std::size_t kCrossingCountToken = 18U;
      if (tokens.size() != kCurrentLayoutTokenCount) {
        return {};
      }
      for (std::size_t index = 0U; index < tokens.size(); ++index) {
        if (index == kCrossingCountToken) {
          continue;
        }
        output << (index == 0U ? "" : " ") << tokens[index];
      }
      output << '\n';
      continue;
    }
    if (line.starts_with("T ")) {
      std::istringstream fields(line);
      std::vector<std::string> tokens;
      for (std::string token; fields >> token;) {
        tokens.push_back(std::move(token));
      }
      constexpr std::size_t kCurrentPathTokenCount = 26U;
      if (tokens.size() != kCurrentPathTokenCount) {
        return {};
      }
      tokens[2] = "1";
      bool first = true;
      for (std::size_t index = 0U; index < tokens.size(); ++index) {
        if (index >= 12U && index <= 23U) {
          continue;
        }
        output << (first ? "" : " ") << tokens[index];
        first = false;
      }
      output << '\n';
      continue;
    }
    if (line.starts_with("P ")) {
      if (!writeLegacyTerrainProfileLine(output, line)) {
        return {};
      }
      continue;
    }
    if (!writeLegacyObjectLine(output, line)) {
      return {};
    }
  }
  return output.str();
}

std::string versionTwentyTwoTextWithoutRoadFields(std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  const std::size_t header = encoded.find(currentHeader);
  if (header == std::string::npos) {
    return {};
  }
  encoded.replace(header, currentHeader.size(), "IGGY3D_WORLD_LAYOUT 22");
  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  const std::size_t layout = encoded.find(currentLayout);
  if (layout == std::string::npos) {
    return {};
  }
  encoded.replace(layout, currentLayout.size(), "L 22 ");

  std::istringstream input(encoded);
  std::ostringstream output;
  for (std::string line; std::getline(input, line);) {
    if (line.starts_with("K ")) {
      continue;
    }
    if (line.starts_with("L ")) {
      std::istringstream fields(line);
      std::vector<std::string> tokens;
      for (std::string token; fields >> token;) {
        tokens.push_back(std::move(token));
      }
      constexpr std::size_t kCurrentLayoutTokenCount = 20U;
      constexpr std::size_t kCrossingCountToken = 18U;
      if (tokens.size() != kCurrentLayoutTokenCount) {
        return {};
      }
      for (std::size_t index = 0U; index < tokens.size(); ++index) {
        if (index == kCrossingCountToken) {
          continue;
        }
        output << (index == 0U ? "" : " ") << tokens[index];
      }
      output << '\n';
      continue;
    }
    if (line.starts_with("P ")) {
      if (!writeProfileWithoutRetainingFields(output, line)) {
        return {};
      }
      continue;
    }
    if (!line.starts_with("T ")) {
      if (!writeLegacyObjectLine(output, line)) {
        return {};
      }
      continue;
    }
    std::istringstream fields(line);
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    constexpr std::size_t kCurrentPathTokenCount = 26U;
    if (tokens.size() != kCurrentPathTokenCount) {
      return {};
    }
    tokens[2] = "1";
    bool first = true;
    for (std::size_t index = 0U; index < tokens.size(); ++index) {
      if (index >= 12U && index <= 23U) {
        continue;
      }
      output << (first ? "" : " ") << tokens[index];
      first = false;
    }
    output << '\n';
  }
  return output.str();
}

std::string versionTwentyThreeTextWithoutWatercourseFields(
    std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  const std::size_t header = encoded.find(currentHeader);
  if (header == std::string::npos) {
    return {};
  }
  encoded.replace(header, currentHeader.size(), "IGGY3D_WORLD_LAYOUT 23");
  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  const std::size_t layout = encoded.find(currentLayout);
  if (layout == std::string::npos) {
    return {};
  }
  encoded.replace(layout, currentLayout.size(), "L 23 ");

  std::istringstream input(encoded);
  std::ostringstream output;
  for (std::string line; std::getline(input, line);) {
    if (line.starts_with("K ")) {
      continue;
    }
    std::istringstream fields(line);
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    if (line.starts_with("L ")) {
      constexpr std::size_t kCurrentLayoutTokenCount = 20U;
      constexpr std::size_t kCrossingCountToken = 18U;
      if (tokens.size() != kCurrentLayoutTokenCount) {
        return {};
      }
      for (std::size_t index = 0U; index < tokens.size(); ++index) {
        if (index == kCrossingCountToken) {
          continue;
        }
        output << (index == 0U ? "" : " ") << tokens[index];
      }
      output << '\n';
      continue;
    }
    if (line.starts_with("P ")) {
      if (!writeProfileWithoutRetainingFields(output, line)) {
        return {};
      }
      continue;
    }
    if (line.starts_with("T ")) {
      constexpr std::size_t kCurrentPathTokenCount = 26U;
      if (tokens.size() != kCurrentPathTokenCount) {
        return {};
      }
      tokens[2] = "2";
      bool first = true;
      for (std::size_t index = 0U; index < tokens.size(); ++index) {
        if (index >= 18U && index <= 23U) {
          continue;
        }
        output << (first ? "" : " ") << tokens[index];
        first = false;
      }
      output << '\n';
      continue;
    }
    if (!writeLegacyObjectLine(output, line)) {
      return {};
    }
  }
  return output.str();
}

std::string versionTwentyFiveTextWithoutRetainingFields(std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  const std::size_t header = encoded.find(currentHeader);
  if (header == std::string::npos) {
    return {};
  }
  encoded.replace(header, currentHeader.size(), "IGGY3D_WORLD_LAYOUT 25");
  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  const std::size_t layout = encoded.find(currentLayout);
  if (layout == std::string::npos) {
    return {};
  }
  encoded.replace(layout, currentLayout.size(), "L 25 ");

  std::istringstream input(encoded);
  std::ostringstream output;
  for (std::string line; std::getline(input, line);) {
    if (!writeProfileWithoutRetainingFields(output, line)) {
      return {};
    }
  }
  return output.str();
}

std::string legacyOpeningText(std::string encoded, std::uint32_t version,
                              unsigned doorPose, bool includesFacing) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  encoded.replace(encoded.find(currentHeader), currentHeader.size(),
                  "IGGY3D_WORLD_LAYOUT " + std::to_string(version));
  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  encoded.replace(encoded.find(currentLayout), currentLayout.size(),
                  "L " + std::to_string(version) + " ");

  std::istringstream input(encoded);
  std::ostringstream output;
  std::string line;
  while (std::getline(input, line)) {
    if (!line.starts_with("O ")) {
      if (!writeLineWithoutConnectorMaterial(output, line)) {
        return {};
      }
      continue;
    }
    std::istringstream fields(line);
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    if (tokens.size() < 14U) {
      return {};
    }
    for (std::size_t index = 0U; index <= 5U; ++index) {
      output << (index == 0U ? "" : " ") << tokens[index];
    }
    const bool door = tokens[5] ==
                      std::to_string(static_cast<unsigned>(
                          cr::CreativeBuildingOpeningKind::Door));
    output << ' ' << (door ? doorPose : 0U);
    if (includesFacing) {
      output << ' ' << tokens[6];
    }
    // Current opening rows add the window treatment after the door settings;
    // legacy v14/v15 rows carry neither group.
    for (std::size_t index = 14U; index < tokens.size(); ++index) {
      output << ' ' << tokens[index];
    }
    output << '\n';
  }
  return output.str();
}

std::string versionFourteenTextWithoutOpeningFacing(std::string encoded) {
  return legacyOpeningText(
      std::move(encoded), 14U,
      static_cast<unsigned>(
          cr::CreativeBuildingOpeningPose::OpenFromStartNegativeNormal),
      false);
}

std::string versionSixteenTextWithoutWindowTreatment(std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  encoded.replace(encoded.find(currentHeader), currentHeader.size(),
                  "IGGY3D_WORLD_LAYOUT 16");
  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  encoded.replace(encoded.find(currentLayout), currentLayout.size(), "L 16 ");

  std::istringstream input(encoded);
  std::ostringstream output;
  std::string line;
  while (std::getline(input, line)) {
    if (!line.starts_with("O ")) {
      if (!writeLineWithoutConnectorMaterial(output, line)) {
        return {};
      }
      continue;
    }
    std::istringstream fields(line);
    std::vector<std::string> tokens;
    for (std::string token; fields >> token;) {
      tokens.push_back(std::move(token));
    }
    if (tokens.size() <= 14U) {
      return {};
    }
    for (std::size_t index = 0U; index < tokens.size(); ++index) {
      if (index == 13U) {
        continue;
      }
      output << (index == 0U ? "" : " ") << tokens[index];
    }
    output << '\n';
  }
  return output.str();
}

std::string versionSeventeenTextWithoutConnectorMaterial(
    std::string encoded) {
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  encoded.replace(encoded.find(currentHeader), currentHeader.size(),
                  "IGGY3D_WORLD_LAYOUT 17");
  const std::string currentLayout =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  encoded.replace(encoded.find(currentLayout), currentLayout.size(), "L 17 ");

  std::istringstream input(encoded);
  std::ostringstream output;
  std::string line;
  while (std::getline(input, line)) {
    if (!writeLineWithoutConnectorMaterial(output, line)) {
      return {};
    }
  }
  return output.str();
}

cr::CreativeWorldLayout richLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "estate layout\nlevel=0%";
  layout.terrainOwnership = cr::CreativeWorldLayoutTerrainOwnership::ReplaceAll;

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building 1";
  building.name = "Main House\nNorth Wing";
  building.rootMode = cr::CreativeBuildingRootMode::CreateRoom;
  building.rootFootprint = {{-4, -3}, {8, 7}};
  building.rootBaseLayer = -1;
  building.rootHeightCells = 4U;
  building.groundingMode =
      cr::CreativeWorldLayoutGroundingMode::Foundation;
  building.maximumGroundReliefCells = 3U;
  building.tags = {"interior", "author=map maker"};
  layout.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "level.ground";
  level.name = "Ground Level";
  level.floorTopLayer = 1.25;
  level.wallHeightCells = 4U;
  level.floorThicknessLayers = 2U;
  level.ceilingThicknessLayers = 2U;
  level.roofThicknessLayers = 3U;
  level.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  level.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  level.roofSlopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::NegativeX;
  level.roofPitchDegrees = 37.5;
  level.roofOverhangCells = 0.75;
  level.roofMaterial = cr::CreativeStructuralMaterial::Brick;
  layout.levels.push_back(level);

  cr::CreativeWorldLayoutLevel upperLevel = level;
  upperLevel.stableKey = "level.upper";
  upperLevel.name = "Upper Level";
  upperLevel.floorTopLayer = 5.25;
  layout.levels.push_back(upperLevel);

  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room.study";
  room.name = "Study";
  room.footprint = {{0, 0}, {8, 6}};
  room.wallThicknessCells = 0.375;
  room.type = cr::CreativeWorldLayoutRoomType::Living;
  layout.rooms.push_back(room);

  cr::CreativeWorldLayoutRoom upperRoom = room;
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "room.upper";
  upperRoom.name = "Upper Hall";
  upperRoom.type = cr::CreativeWorldLayoutRoomType::Corridor;
  layout.rooms.push_back(upperRoom);

  cr::CreativeWorldLayoutVerticalConnector stair;
  stair.buildingIndex = 0U;
  stair.lowerRoomIndex = 0U;
  stair.upperRoomIndex = 1U;
  stair.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Stair;
  stair.direction = cr::CreativeWorldLayoutVerticalDirection::PositiveX;
  stair.stableKey = "stair.main";
  stair.name = "Main Stair";
  stair.footprint = {{1, 2}, {5, 4}};
  stair.material = cr::CreativeStructuralMaterial::Stone;
  layout.verticalConnectors.push_back(stair);

  cr::CreativeWorldLayoutBox floor;
  floor.buildingIndex = 0U;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.stableKey = "floor.main";
  floor.name = "Ground Floor";
  floor.footprint = {{-4, -3}, {8, 7}};
  floor.anchorLayer = 1.75;
  floor.layerCount = 2U;
  layout.boxes.push_back(floor);

  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.stableKey = "wall.north";
  wall.name = "North Wall";
  wall.start = {-4, -3};
  wall.end = {8, -3};
  wall.heightCells = 4U;
  wall.thicknessCells = 0.375;
  wall.profile = cr::CreativeWorldLayoutWallProfile::Interior;
  wall.material = cr::CreativeStructuralMaterial::Timber;
  wall.joinStyle = cr::CreativeWorldLayoutWallJoinStyle::Square;
  layout.walls.push_back(wall);

  cr::CreativeWorldLayoutOpening opening;
  opening.wallIndex = 0U;
  opening.kind = cr::CreativeBuildingOpeningKind::Window;
  opening.facing = cr::CreativeBuildingOpeningFacing::NegativeNormal;
  opening.stableKey = "window.north.1";
  opening.name = "Window = 1";
  opening.centerOffsetCells = 5.25;
  opening.widthCells = 1.5;
  opening.cutoutBottomCells = 1.0;
  opening.cutoutHeightCells = 1.25;
  opening.insertBottomCells = 1.0;
  opening.insertHeightCells = 1.25;
  opening.insertWidthCells = 1.5;
  opening.insertThicknessCells = 0.1;
  opening.insertAssetId = "homestead/modular/window_frame_1p5x1p2";
  opening.insertAssetSourceBoundsMeters =
      {{-0.75, 0.0, -0.05}, {0.75, 1.2, 0.05}};
  opening.hasInsertAssetSourceBounds = true;
  opening.window.insertKind = cr::CreativeWindowInsertKind::PairedShutters;
  layout.openings.push_back(opening);

  cr::CreativeWorldLayoutOpening roomDoor;
  roomDoor.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  roomDoor.roomIndex = 0U;
  roomDoor.roomEdge = cr::CreativeWorldLayoutRoomEdge::South;
  roomDoor.kind = cr::CreativeBuildingOpeningKind::Door;
  roomDoor.door.leafArrangement = cr::CreativeDoorLeafArrangement::Double;
  roomDoor.door.hingeSide = cr::CreativeDoorHingeSide::MaximumEdge;
  roomDoor.door.swingSide = cr::CreativeDoorSwingSide::NegativeNormal;
  roomDoor.door.initialState = cr::CreativeDoorInitialState::Open;
  roomDoor.door.gameplayLocked = true;
  roomDoor.door.transitionSeconds = 0.8;
  roomDoor.stableKey = "door.study";
  roomDoor.name = "Study Door";
  roomDoor.centerOffsetCells = 2.0;
  layout.openings.push_back(roomDoor);

  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Rock;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  object.stableKey = "rock.imported";
  object.name = "Imported Boulder";
  object.assetId = "boulder_01";
  object.boundsCells = {{2.25, 1.0, -4.5}, {4.75, 3.0, -2.0}};
  object.tags = {"prop", "source=blender"};
  layout.objects.push_back(object);

  cr::CreativeWorldLayoutObject posedAsset;
  posedAsset.kind = cr::CreativeObjectKind::Prop;
  posedAsset.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  posedAsset.stableKey = "prop.posed";
  posedAsset.name = "Posed Catalog Asset";
  posedAsset.assetId = "homestead/interior/dresser_1p3";
  posedAsset.pointCells = {6.0, 0.5, -3.0};
  posedAsset.assetSourceBoundsMeters =
      {{-0.65, 0.0, -0.3}, {0.65, 1.1, 0.3}};
  posedAsset.hasAssetSourceBounds = true;
  posedAsset.yawRadians = 0.7853981633974483;
  posedAsset.scale = {1.25, 0.75, 1.5};
  posedAsset.tags = {"world_layout:catalog_asset"};
  layout.objects.push_back(posedAsset);

  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "hill.west";
  profile.kind = cr::CreativeTerrainRecipeKind::Hill;
  profile.center = {-10, 2};
  profile.baseHeightCells = 2U;
  profile.radiusCells = 8U;
  profile.amplitudeCells = 5U;
  profile.spacingCells = 2U;
  profile.frequency = 2U;
  layout.terrainProfiles.push_back(profile);

  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = "road.entry";
  path.recipe.kind = cr::CreativeTerrainPathKind::Road;
  path.recipe.elevation = cr::CreativeTerrainPathElevation::Level;
  path.recipe.curve = cr::CreativeTerrainPathCurvePolicy::CatmullRom;
  path.recipe.crossSection =
      cr::CreativeTerrainPathCrossSection::Crowned;
  path.recipe.startJoin = cr::CreativeTerrainPathEndpointJoin::Blend;
  path.recipe.endJoin = cr::CreativeTerrainPathEndpointJoin::BuildingPad;
  path.recipe.falloffCells = 3U;
  path.recipe.paintSurface = true;
  path.recipe.material = cr::CreativeTerrainMaterial::Dirt;
  path.recipe.road.shoulderWidthCells = 3U;
  path.recipe.road.maximumGradePermille = 350U;
  path.recipe.road.edgeTreatment =
      cr::CreativeTerrainRoadEdgeTreatment::Curb;
  path.recipe.road.edgeWidthMeters = 0.22;
  path.recipe.road.edgeHeightMeters = 0.18;
  path.recipe.road.edgeMaterial = cr::CreativeStructuralMaterial::Brick;
  path.recipe.nextPointId = 8U;
  path.recipe.points = {{2U, {-6, 8}, 2U, 2U, 1U, 50},
                        {4U, {0, 8}, 3U, 3U, 2U, -25},
                        {7U, {0, 4}, 4U, 1U, 3U, 0}};
  layout.terrainPaths.push_back(path);
  return layout;
}

bool deterministicRoundTripPreservesEveryTable() {
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(richLayout());
  cr::CreativeWorldLayout source = materialized.edited;
  source.topologyEdges[0].wallHeightCells = 7U;
  source.topologyEdges[0].profile =
      cr::CreativeWorldLayoutWallProfile::Exterior;
  source.topologyEdges[0].material = cr::CreativeStructuralMaterial::Brick;
  const cr::CreativeWorldLayoutEncodeResult first =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutEncodeResult second =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(first.encodedText);
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      cr::encodeCreativeWorldLayout(decoded.layout);

  return expect(materialized.accepted && materialized.changed,
                "rich layout materializes explicit room topology") &&
         expect(first.accepted && second.accepted && decoded.accepted &&
                    reencoded.accepted,
                "rich layout codec operations accepted") &&
         expect(first.encodedText == second.encodedText &&
                    first.encodedText == reencoded.encodedText,
                "layout codec is byte deterministic") &&
         expect(decoded.layout.stableKey == source.stableKey,
                "special layout key round trips") &&
         expect(
             decoded.layout.buildings.size() == 1U &&
                 decoded.layout.buildings[0].name == source.buildings[0].name &&
                 decoded.layout.buildings[0].tags == source.buildings[0].tags &&
                 decoded.layout.buildings[0].groundingMode ==
                     cr::CreativeWorldLayoutGroundingMode::Foundation &&
                 decoded.layout.buildings[0].maximumGroundReliefCells == 3U,
             "building strings and tags round trip") &&
         expect(decoded.layout.levels.size() == 2U &&
                    decoded.layout.levels[0].floorTopLayer == 1.25 &&
                    decoded.layout.levels[0].ceilingThicknessLayers == 2U &&
                    decoded.layout.levels[0].roofThicknessLayers == 3U &&
                    decoded.layout.levels[0].roofStyle ==
                        cr::CreativeStructuralRoofStyle::Gable &&
                    decoded.layout.levels[0].roofRidgeAxis ==
                        cr::CreativeStructuralRoofRidgeAxis::Z &&
                    decoded.layout.levels[0].roofSlopeDirection ==
                        cr::CreativeStructuralRoofSlopeDirection::NegativeX &&
                    decoded.layout.levels[0].roofPitchDegrees == 37.5 &&
                    decoded.layout.levels[0].roofOverhangCells == 0.75 &&
                    decoded.layout.levels[0].roofMaterial ==
                        cr::CreativeStructuralMaterial::Brick &&
                    decoded.layout.rooms.size() == 2U &&
                    decoded.layout.rooms[0].footprint.maximum ==
                        source.rooms[0].footprint.maximum &&
                    decoded.layout.rooms[0].levelIndex == 0U &&
                    decoded.layout.rooms[0].type ==
                        cr::CreativeWorldLayoutRoomType::Living &&
                    decoded.layout.rooms[1].type ==
                        cr::CreativeWorldLayoutRoomType::Corridor &&
                    decoded.layout.verticalConnectors.size() == 1U &&
                    decoded.layout.verticalConnectors[0].lowerRoomIndex == 0U &&
                    decoded.layout.verticalConnectors[0].upperRoomIndex == 1U &&
                    decoded.layout.verticalConnectors[0].direction ==
                        cr::CreativeWorldLayoutVerticalDirection::PositiveX &&
                    decoded.layout.verticalConnectors[0].material ==
                        cr::CreativeStructuralMaterial::Stone &&
                    decoded.layout.verticalConnectors[0].footprint.minimum ==
                        source.verticalConnectors[0].footprint.minimum &&
                    decoded.layout.boxes.size() == 1U &&
                    decoded.layout.boxes[0].anchorLayer == 1.75 &&
                    decoded.layout.boxes[0].layerCount == 2U &&
                    decoded.layout.walls.size() == 1U &&
                    decoded.layout.walls[0].profile ==
                        cr::CreativeWorldLayoutWallProfile::Interior &&
                    decoded.layout.walls[0].material ==
                        cr::CreativeStructuralMaterial::Timber &&
                    decoded.layout.openings.size() == 2U &&
                    decoded.layout.openings[0].insertAssetId ==
                        source.openings[0].insertAssetId &&
                    decoded.layout.openings[0]
                        .hasInsertAssetSourceBounds &&
                    decoded.layout.openings[0]
                            .insertAssetSourceBoundsMeters.max.y == 1.2 &&
                    decoded.layout.openings[0].facing ==
                        cr::CreativeBuildingOpeningFacing::NegativeNormal &&
                    decoded.layout.openings[0].window ==
                        source.openings[0].window &&
                    decoded.layout.openings[1].hostKind ==
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                    decoded.layout.openings[1].door ==
                        source.openings[1].door &&
                    decoded.layout.openings[1].roomTopologyEdgeIndex !=
                        cr::kInvalidCreativeWorldLayoutIndex &&
                    decoded.layout.topologyVertices.size() ==
                        source.topologyVertices.size() &&
                    decoded.layout.topologyEdges.size() ==
                        source.topologyEdges.size() &&
                    decoded.layout.topologyEdges[0].wallHeightCells == 7U &&
                    decoded.layout.topologyEdges[0].profile ==
                        cr::CreativeWorldLayoutWallProfile::Exterior &&
                    decoded.layout.topologyEdges[0].material ==
                        cr::CreativeStructuralMaterial::Brick &&
                    decoded.layout.topologyEdges[0].joinStyle ==
                        cr::CreativeWorldLayoutWallJoinStyle::Square &&
                    decoded.layout.roomBoundaries.size() ==
                        source.roomBoundaries.size(),
                "building, room, connector, and opening tables round trip") &&
         expect(decoded.layout.objects.size() == 2U &&
                    decoded.layout.objects[0].assetId == "boulder_01" &&
                    decoded.layout.objects[0].tags == source.objects[0].tags &&
                    decoded.layout.objects[0].boundsCells.min.x == 2.25 &&
                    decoded.layout.objects[1].hasAssetSourceBounds &&
                    decoded.layout.objects[1].assetSourceBoundsMeters.min.x ==
                        -0.65 &&
                    decoded.layout.objects[1].yawRadians ==
                        source.objects[1].yawRadians &&
                    decoded.layout.objects[1].scale.z == 1.5,
                "object-library symbols and pose round trip") &&
         expect(decoded.layout.terrainProfiles.size() == 1U &&
                    decoded.layout.terrainPaths.size() == 1U &&
                    decoded.layout.terrainPaths[0].recipe ==
                        source.terrainPaths[0].recipe,
                "terrain symbol tables round trip");
}

bool playerSpawnSettingsRoundTripAndVersionTwentySixDefaults() {
  cr::CreativeWorldLayout source;
  source.stableKey = "player_spawn_codec";
  cr::CreativeWorldLayoutObject spawn;
  spawn.kind = cr::CreativeObjectKind::SpawnPoint;
  spawn.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  spawn.stableKey = "spawn.north";
  spawn.name = "North Spawn";
  spawn.pointCells = {2.0, 0.25, -3.0};
  spawn.yawRadians = 1.25;
  spawn.playerSpawn.playerProfileId = "default";
  spawn.playerSpawn.spawnGroup = "north_entry";
  spawn.playerSpawn.validationRadiusMeters = 0.75;
  spawn.playerSpawn.fallbackPriority = 4U;
  source.objects.push_back(spawn);

  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};
  const std::string legacyText =
      encoded.accepted
          ? versionTwentySixTextWithoutPlayerSpawnFields(encoded.encodedText)
          : std::string{};
  const cr::CreativeWorldLayoutDecodeResult migrated =
      cr::decodeCreativeWorldLayout(legacyText);

  cr::CreativeWorldLayout invalidSpawn = source;
  invalidSpawn.objects[0].playerSpawn.spawnGroup.clear();
  const cr::CreativeWorldLayoutEncodeResult invalidSpawnResult =
      cr::encodeCreativeWorldLayout(invalidSpawn);
  cr::CreativeWorldLayout invalidNonSpawn = source;
  invalidNonSpawn.objects[0].kind = cr::CreativeObjectKind::Prop;
  const cr::CreativeWorldLayoutEncodeResult invalidNonSpawnResult =
      cr::encodeCreativeWorldLayout(invalidNonSpawn);

  return expect(encoded.accepted && decoded.accepted && reencoded.accepted,
                "player spawn codec operations accepted") &&
         expect(encoded.encodedText == reencoded.encodedText &&
                    decoded.layout.objects.size() == 1U,
                "player spawn codec is byte deterministic") &&
         expect(decoded.layout.objects[0].pointCells.x == spawn.pointCells.x &&
                    decoded.layout.objects[0].pointCells.y == spawn.pointCells.y &&
                    decoded.layout.objects[0].pointCells.z == spawn.pointCells.z &&
                    decoded.layout.objects[0].yawRadians == spawn.yawRadians &&
                    decoded.layout.objects[0].playerSpawn == spawn.playerSpawn,
                "player spawn pose and settings round trip") &&
         expect(!legacyText.empty() && migrated.accepted &&
                    migrated.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    migrated.layout.objects.size() == 1U &&
                    migrated.layout.objects[0].playerSpawn ==
                        cr::CreativePlayerSpawnSettings{},
                "version-twenty-six spawn receives safe defaults") &&
         expect(!invalidSpawnResult.accepted,
                "invalid player spawn settings fail encode") &&
         expect(!invalidNonSpawnResult.accepted,
                "non-spawn object cannot carry spawn settings");
}

bool sourceFingerprintUsesExactVersionedEncoding() {
  cr::CreativeWorldLayout source;
  source.stableKey = "fingerprint_source";
  const std::uint64_t first = cr::fingerprintCreativeWorldLayout(source);
  const std::uint64_t repeated = cr::fingerprintCreativeWorldLayout(source);

  cr::CreativeWorldLayout changed = source;
  changed.stableKey = "fingerprint_source_changed";
  const std::uint64_t changedFingerprint =
      cr::fingerprintCreativeWorldLayout(changed);

  cr::CreativeWorldLayout invalid = source;
  invalid.schemaVersion = cr::kCreativeWorldLayoutSchemaVersion + 1U;
  return expect(first != 0U && repeated == first,
                "source fingerprint is deterministic") &&
         expect(changedFingerprint != 0U && changedFingerprint != first,
                "source fingerprint tracks semantic codec changes") &&
         expect(cr::fingerprintCreativeWorldLayout(invalid) == 0U,
                "source fingerprint fails closed when encoding rejects");
}

bool roofAperturesRoundTripAndVersionNineteenMigratesEmpty() {
  cr::CreativeWorldLayout source = richLayout();
  cr::CreativeWorldLayoutRoofAperture aperture;
  aperture.levelIndex = 1U;
  aperture.kind = cr::CreativeStructuralRoofApertureKind::Skylight;
  aperture.stableKey = "skylight.upper.east";
  aperture.name = "Upper East Skylight";
  aperture.minimumXCells = 1.25;
  aperture.maximumXCells = 2.75;
  aperture.minimumZCells = 2.0;
  aperture.maximumZCells = 3.5;
  source.roofApertures.push_back(aperture);

  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  cr::CreativeWorldLayout invalidKind = source;
  invalidKind.roofApertures[0].kind =
      cr::CreativeStructuralRoofApertureKind::Count;
  const cr::CreativeWorldLayoutEncodeResult invalidKindEncode =
      cr::encodeCreativeWorldLayout(invalidKind);
  cr::CreativeWorldLayout nonFinite = source;
  nonFinite.roofApertures[0].maximumXCells =
      std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWorldLayoutEncodeResult nonFiniteEncode =
      cr::encodeCreativeWorldLayout(nonFinite);
  cr::CreativeWorldLayout overCapacity = source;
  for (std::size_t index = 1U;
       index <= cr::kCreativeStructuralRoofApertureCapacity; ++index) {
    cr::CreativeWorldLayoutRoofAperture extra = aperture;
    extra.stableKey = "skylight.extra." + std::to_string(index);
    extra.name = "Extra Skylight " + std::to_string(index);
    extra.minimumXCells += 0.1 * static_cast<double>(index);
    extra.maximumXCells += 0.1 * static_cast<double>(index);
    overCapacity.roofApertures.push_back(std::move(extra));
  }
  const cr::CreativeWorldLayoutEncodeResult overCapacityEncode =
      cr::encodeCreativeWorldLayout(overCapacity);

  std::string invalidKindText = encoded.encodedText;
  const bool locatedKind = replaceFirstRoofApertureToken(
      invalidKindText, 2U, "255");
  const cr::CreativeWorldLayoutDecodeResult invalidKindDecode =
      cr::decodeCreativeWorldLayout(invalidKindText);

  cr::CreativeWorldLayout legacySource = richLayout();
  legacySource.terrainPaths.clear();
  const cr::CreativeWorldLayoutEncodeResult oldSource =
      cr::encodeCreativeWorldLayout(legacySource);
  const std::string versionNineteen =
      oldSource.accepted
          ? versionNineteenTextWithoutRoofApertures(oldSource.encodedText)
          : std::string{};
  const cr::CreativeWorldLayoutDecodeResult migrated =
      cr::decodeCreativeWorldLayout(versionNineteen);
  const cr::CreativeWorldLayoutEncodeResult migratedReencoded =
      migrated.accepted ? cr::encodeCreativeWorldLayout(migrated.layout)
                        : cr::CreativeWorldLayoutEncodeResult{};
  return expect(encoded.accepted && decoded.accepted && reencoded.accepted,
                "roof aperture codec operations accepted") &&
         expect(encoded.encodedText == reencoded.encodedText &&
                    decoded.layout.roofApertures.size() == 1U,
                "roof aperture codec is deterministic") &&
         expect(decoded.layout.roofApertures[0].levelIndex == 1U &&
                    decoded.layout.roofApertures[0].kind ==
                        cr::CreativeStructuralRoofApertureKind::Skylight &&
                    decoded.layout.roofApertures[0].stableKey ==
                        aperture.stableKey &&
                    decoded.layout.roofApertures[0].name == aperture.name &&
                    decoded.layout.roofApertures[0].minimumXCells == 1.25 &&
                    decoded.layout.roofApertures[0].maximumZCells == 3.5,
                "roof aperture source intent round trips") &&
         expect(!invalidKindEncode.accepted,
                "invalid roof aperture kind is not encoded") &&
         expect(!nonFiniteEncode.accepted &&
                    nonFiniteEncode.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite roof aperture is not encoded") &&
         expect(!overCapacityEncode.accepted &&
                    overCapacityEncode.status ==
                        cr::CreativeWorldLayoutCodecStatus::CapacityExceeded,
                "roof aperture capacity fails closed") &&
         expect(locatedKind && !invalidKindDecode.accepted,
                "invalid serialized roof aperture kind fails closed") &&
         expect(migrated.accepted && migrated.layout.roofApertures.empty() &&
                    migrated.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion,
                "version-nineteen source migrates without fabricated apertures") &&
         expect(migratedReencoded.accepted &&
                    migratedReencoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(
                            cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-nineteen migration writes the current schema");
}

bool directTopologyHostDoesNotInventACardinalRoomSide() {
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(richLayout());
  cr::CreativeWorldLayout source = materialized.edited;
  source.openings[1].roomEdge = cr::CreativeWorldLayoutRoomEdge::Count;
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};

  cr::CreativeWorldLayout missingDirect = source;
  missingDirect.openings[1].roomTopologyEdgeIndex =
      cr::kInvalidCreativeWorldLayoutIndex;
  const cr::CreativeWorldLayoutEncodeResult rejected =
      cr::encodeCreativeWorldLayout(missingDirect);

  return expect(materialized.accepted && encoded.accepted && decoded.accepted,
                "schema-12 direct topology opening round trips") &&
         expect(decoded.layout.openings[1].roomEdge ==
                        cr::CreativeWorldLayoutRoomEdge::Count &&
                    decoded.layout.openings[1].roomTopologyEdgeIndex ==
                        source.openings[1].roomTopologyEdgeIndex,
                "direct host does not fabricate a cardinal side") &&
         expect(!rejected.accepted,
                "non-cardinal room side still requires a direct topology host");
}

bool malformedAndNonFiniteInputsFailClosed() {
  cr::CreativeWorldLayout wrongEncodeSchema = richLayout();
  wrongEncodeSchema.schemaVersion = 99U;
  const cr::CreativeWorldLayoutEncodeResult invalidSchema =
      cr::encodeCreativeWorldLayout(wrongEncodeSchema);

  cr::CreativeWorldLayout layout = richLayout();
  layout.walls[0].thicknessCells = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWorldLayoutEncodeResult nonFinite =
      cr::encodeCreativeWorldLayout(layout);
  cr::CreativeWorldLayout badObject = richLayout();
  badObject.objects[0].pointCells.y =
      std::numeric_limits<double>::infinity();
  const cr::CreativeWorldLayoutEncodeResult nonFiniteObject =
      cr::encodeCreativeWorldLayout(badObject);
  cr::CreativeWorldLayout badBox = richLayout();
  badBox.boxes[0].anchorLayer =
      std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWorldLayoutEncodeResult nonFiniteBox =
      cr::encodeCreativeWorldLayout(badBox);
  cr::CreativeWorldLayout badRoofStyle = richLayout();
  badRoofStyle.levels[0].roofStyle =
      cr::CreativeStructuralRoofStyle::Count;
  const cr::CreativeWorldLayoutEncodeResult invalidRoofStyle =
      cr::encodeCreativeWorldLayout(badRoofStyle);
  cr::CreativeWorldLayout badRoofPitch = richLayout();
  badRoofPitch.levels[0].roofPitchDegrees =
      std::numeric_limits<double>::infinity();
  const cr::CreativeWorldLayoutEncodeResult nonFiniteRoofPitch =
      cr::encodeCreativeWorldLayout(badRoofPitch);
  cr::CreativeWorldLayout badRoofDirection = richLayout();
  badRoofDirection.levels[0].roofSlopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::Count;
  const cr::CreativeWorldLayoutEncodeResult invalidRoofDirection =
      cr::encodeCreativeWorldLayout(badRoofDirection);
  cr::CreativeWorldLayout badRoofMaterial = richLayout();
  badRoofMaterial.levels[0].roofMaterial =
      cr::CreativeStructuralMaterial::Count;
  const cr::CreativeWorldLayoutEncodeResult invalidRoofMaterial =
      cr::encodeCreativeWorldLayout(badRoofMaterial);
  cr::CreativeWorldLayout badRoomType = richLayout();
  badRoomType.rooms[0].type = cr::CreativeWorldLayoutRoomType::Count;
  const cr::CreativeWorldLayoutEncodeResult invalidRoomType =
      cr::encodeCreativeWorldLayout(badRoomType);
  cr::CreativeWorldLayout badObjectScale = richLayout();
  badObjectScale.objects[1].scale.x = 0.0;
  const cr::CreativeWorldLayoutEncodeResult invalidObjectScale =
      cr::encodeCreativeWorldLayout(badObjectScale);
  cr::CreativeWorldLayout badObjectSourceBounds = richLayout();
  badObjectSourceBounds.objects[1].assetSourceBoundsMeters.max.x =
      badObjectSourceBounds.objects[1].assetSourceBoundsMeters.min.x;
  const cr::CreativeWorldLayoutEncodeResult invalidObjectSourceBounds =
      cr::encodeCreativeWorldLayout(badObjectSourceBounds);
  cr::CreativeWorldLayout badOpeningSourceBounds = richLayout();
  badOpeningSourceBounds.openings[0].insertAssetSourceBoundsMeters.max.x =
      badOpeningSourceBounds.openings[0].insertAssetSourceBoundsMeters.min.x;
  const cr::CreativeWorldLayoutEncodeResult invalidOpeningSourceBounds =
      cr::encodeCreativeWorldLayout(badOpeningSourceBounds);
  cr::CreativeWorldLayout mismatchedOpeningAsset = richLayout();
  mismatchedOpeningAsset.openings[0].hasInsertAssetSourceBounds = false;
  const cr::CreativeWorldLayoutEncodeResult invalidOpeningAssetContract =
      cr::encodeCreativeWorldLayout(mismatchedOpeningAsset);
  cr::CreativeWorldLayout badOpeningFacing = richLayout();
  badOpeningFacing.openings[0].facing =
      cr::CreativeBuildingOpeningFacing::Count;
  const cr::CreativeWorldLayoutEncodeResult invalidOpeningFacing =
      cr::encodeCreativeWorldLayout(badOpeningFacing);
  cr::CreativeWorldLayout cutoutOnlyAsset = richLayout();
  cutoutOnlyAsset.openings[0].includeInsert = false;
  const cr::CreativeWorldLayoutEncodeResult cutoutOnlyEncoded =
      cr::encodeCreativeWorldLayout(cutoutOnlyAsset);
  const cr::CreativeWorldLayoutDecodeResult cutoutOnlyDecoded =
      cr::decodeCreativeWorldLayout(cutoutOnlyEncoded.encodedText);

  const cr::CreativeWorldLayoutEncodeResult valid =
      cr::encodeCreativeWorldLayout(richLayout());
  const cr::CreativeWorldLayoutDecodeResult truncated =
      cr::decodeCreativeWorldLayout(
          valid.encodedText.substr(0U, valid.encodedText.find("END")));
  std::string unsupported = valid.encodedText;
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  unsupported.replace(unsupported.find(currentHeader), currentHeader.size(),
                      "IGGY3D_WORLD_LAYOUT 99");
  const cr::CreativeWorldLayoutDecodeResult wrongVersion =
      cr::decodeCreativeWorldLayout(unsupported);
  std::string wrongSchema = valid.encodedText;
  const std::string currentLayoutPrefix =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  wrongSchema.replace(wrongSchema.find(currentLayoutPrefix),
                      currentLayoutPrefix.size(), "L 99 ");
  const cr::CreativeWorldLayoutDecodeResult schemaMismatch =
      cr::decodeCreativeWorldLayout(wrongSchema);
  const std::string overflowingCount =
      "IGGY3D_WORLD_LAYOUT 1\n"
      "L 1 776f726c645f6c61796f7574 0 18446744073709551615 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult overflow =
      cr::decodeCreativeWorldLayout(overflowingCount);
  std::string invalidRoofEnumText = valid.encodedText;
  const bool locatedRoofFields =
      replaceFirstLevelToken(invalidRoofEnumText, 9U, "9");
  const cr::CreativeWorldLayoutDecodeResult invalidRoofEnum =
      cr::decodeCreativeWorldLayout(invalidRoofEnumText);
  std::string invalidRoofDirectionText = valid.encodedText;
  const bool locatedRoofDirection =
      replaceFirstLevelToken(invalidRoofDirectionText, 11U, "9");
  const cr::CreativeWorldLayoutDecodeResult invalidRoofDirectionDecode =
      cr::decodeCreativeWorldLayout(invalidRoofDirectionText);
  std::string invalidRoofMaterialText = valid.encodedText;
  const bool locatedRoofMaterial =
      replaceFirstLevelToken(invalidRoofMaterialText, 14U, "9");
  const cr::CreativeWorldLayoutDecodeResult invalidRoofMaterialDecode =
      cr::decodeCreativeWorldLayout(invalidRoofMaterialText);
  std::string invalidOpeningFacingText = valid.encodedText;
  const bool locatedOpeningFacing =
      replaceFirstOpeningFacing(invalidOpeningFacingText, "9");
  const cr::CreativeWorldLayoutDecodeResult invalidOpeningFacingDecode =
      cr::decodeCreativeWorldLayout(invalidOpeningFacingText);

  return expect(!invalidSchema.accepted &&
                    invalidSchema.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "obsolete in-memory schema is not encoded") &&
         expect(!nonFinite.accepted &&
                    nonFinite.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite source is not encoded") &&
         expect(!nonFiniteObject.accepted &&
                    nonFiniteObject.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite object symbol is not encoded") &&
         expect(!nonFiniteBox.accepted &&
                    nonFiniteBox.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite box anchor is not encoded") &&
         expect(!invalidRoofStyle.accepted &&
                    invalidRoofStyle.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "invalid roof style is not encoded") &&
         expect(!nonFiniteRoofPitch.accepted &&
                    nonFiniteRoofPitch.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite roof pitch is not encoded") &&
         expect(!invalidRoofDirection.accepted &&
                    invalidRoofDirection.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "invalid roof slope direction is not encoded") &&
         expect(!invalidRoofMaterial.accepted &&
                    invalidRoofMaterial.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "invalid roof material is not encoded") &&
         expect(!invalidRoomType.accepted &&
                    invalidRoomType.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "invalid room type is not encoded") &&
         expect(!invalidObjectScale.accepted &&
                    invalidObjectScale.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "non-positive catalog scale is not encoded") &&
         expect(!invalidObjectSourceBounds.accepted &&
                    invalidObjectSourceBounds.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "degenerate catalog source bounds are not encoded") &&
         expect(!invalidOpeningSourceBounds.accepted &&
                    invalidOpeningSourceBounds.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "degenerate opening asset bounds are not encoded") &&
         expect(!invalidOpeningAssetContract.accepted &&
                    invalidOpeningAssetContract.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "partial opening asset metadata is not encoded") &&
         expect(!invalidOpeningFacing.accepted &&
                    invalidOpeningFacing.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "invalid opening facing is not encoded") &&
         expect(cutoutOnlyEncoded.accepted && cutoutOnlyDecoded.accepted &&
                    !cutoutOnlyDecoded.layout.openings[0].includeInsert &&
                    cutoutOnlyDecoded.layout.openings[0].insertAssetId ==
                        cutoutOnlyAsset.openings[0].insertAssetId,
                "cutout-only opening retains its dormant asset identity") &&
         expect(!truncated.accepted, "truncated source is rejected") &&
         expect(!wrongVersion.accepted &&
                    wrongVersion.status ==
                        cr::CreativeWorldLayoutCodecStatus::UnsupportedVersion,
                "unsupported codec version is rejected") &&
         expect(!schemaMismatch.accepted &&
                    schemaMismatch.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "codec and source schema mismatch is rejected") &&
         expect(!overflow.accepted &&
                    overflow.status ==
                        cr::CreativeWorldLayoutCodecStatus::CapacityExceeded,
                "overflowing declared record counts fail before allocation") &&
         expect(locatedRoofFields && !invalidRoofEnum.accepted,
                "invalid serialized roof enum fails closed") &&
         expect(locatedRoofDirection && !invalidRoofDirectionDecode.accepted,
                "invalid serialized roof direction fails closed") &&
         expect(locatedRoofMaterial && !invalidRoofMaterialDecode.accepted,
                "invalid serialized roof material fails closed") &&
         expect(locatedOpeningFacing && !invalidOpeningFacingDecode.accepted,
                "invalid serialized opening facing fails closed");
}

bool versionOneSourceMigratesToCurrentSchema() {
  const std::string versionOne =
      "IGGY3D_WORLD_LAYOUT 1\n"
      "L 1 6c65676163795f6c61796f7574 0 0 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionOne);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(decoded.layout);
  return expect(decoded.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.rooms.empty(),
                "version-one source migrates without fabricated rooms") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "migrated source writes the current codec version");
}

bool versionTwoSourceMigratesWithoutFabricatedObjects() {
  const std::string versionTwo =
      "IGGY3D_WORLD_LAYOUT 2\n"
      "L 2 6c65676163795f6c61796f7574 0 0 0 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionTwo);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(decoded.layout);
  return expect(decoded.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.objects.empty(),
                "version-two source migrates without fabricated objects") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-two migration writes the current codec version");
}

bool versionThreeRoomPreservesWallPlaneDuringMigration() {
  const std::string versionThree =
      "IGGY3D_WORLD_LAYOUT 3\n"
      "L 3 6c65676163795f726f6f6d 0 1 1 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 4 4 0 3 1 0\n"
      "R 0 726f6f6d 526f6f6d 0 0 4 4 2 3 0.25 2\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionThree);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      decoded.accepted ? cr::expandCreativeWorldLayoutRooms(decoded.layout)
                       : cr::CreativeWorldLayoutRoomCompileResult{};
  return expect(decoded.accepted && decoded.layout.rooms.size() == 1U &&
                    decoded.layout.levels.size() == 1U &&
                    decoded.layout.rooms[0].levelIndex == 0U &&
                    decoded.layout.levels[0].floorTopLayer == 3.0 &&
                    decoded.layout.levels[0].floorThicknessLayers == 2U,
                "version-three room migrates base plus half thickness") &&
         expect(expanded.accepted && !expanded.expanded.walls.empty() &&
                    expanded.expanded.walls[0].baseLayer == 3.0,
                "version-three migration preserves the generated wall plane");
}

bool versionFourBoxesMigrateToExplicitAnchorPlanes() {
  const std::string versionFour =
      "IGGY3D_WORLD_LAYOUT 4\n"
      "L 4 6c65676163795f7375726661636573 0 1 0 2 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 4 4 0 3 1 0\n"
      "X 0 " +
      std::to_string(static_cast<unsigned>(cr::CreativeObjectKind::Floor)) +
      " 666c6f6f72 466c6f6f72 0 0 4 4 2 2\n"
      "X 0 " +
      std::to_string(static_cast<unsigned>(cr::CreativeObjectKind::Roof)) +
      " 726f6f66 526f6f66 0 0 4 4 7 1\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionFour);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.boxes.size() == 2U &&
                    decoded.layout.boxes[0].anchorLayer == 2.0 &&
                    decoded.layout.boxes[0].layerCount == 2U &&
                    decoded.layout.boxes[1].anchorLayer == 7.0 &&
                    decoded.layout.boxes[1].layerCount == 1U,
                "version-four box layers migrate to explicit anchor planes") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "migrated box source writes current schema");
}

bool versionFiveRoomsMigrateToSharedLevels() {
  const std::string versionFive =
      "IGGY3D_WORLD_LAYOUT 5\n"
      "L 5 6c65676163795f6c61796f7574 0 1 1 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 4 4 0 3 1 0\n"
      "R 0 726f6f6d 526f6f6d 0 0 4 4 2.5 4 0.25 2\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionFive);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.rooms.size() == 1U &&
                    decoded.layout.levels.size() == 1U &&
                    decoded.layout.rooms[0].levelIndex == 0U &&
                    decoded.layout.levels[0].buildingIndex == 0U &&
                    decoded.layout.levels[0].floorTopLayer == 2.5 &&
                    decoded.layout.levels[0].wallHeightCells == 4U &&
                    decoded.layout.levels[0].floorThicknessLayers == 2U &&
                    decoded.layout.levels[0].ceilingThicknessLayers == 1U &&
                    decoded.layout.levels[0].roofThicknessLayers == 1U,
                "version-five room geometry migrates into one shared level") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-five migration writes the current codec version");
}

bool versionSixSourceMigratesWithoutFabricatedVerticalConnectors() {
  const std::string versionSix =
      "IGGY3D_WORLD_LAYOUT 6\n"
      "L 6 6c65676163795f6c61796f7574 0 0 0 0 0 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionSix);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.verticalConnectors.empty(),
                "version-six source migrates with an empty connector table") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-six migration writes the current codec version");
}

bool versionSevenLevelsMigrateToFlatRoofDefaults() {
  const std::string versionSeven =
      "IGGY3D_WORLD_LAYOUT 7\n"
      "L 7 6c65676163795f726f6f66 0 1 1 1 0 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 4 4 0 3 1 0\n"
      "V 0 6c6576656c 4c6576656c 0 3 1 1 1\n"
      "R 0 0 726f6f6d 526f6f6d 0 0 4 4 0.25\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionSeven);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};
  return expect(decoded.accepted && decoded.layout.levels.size() == 1U &&
                    decoded.layout.levels[0].roofStyle ==
                        cr::CreativeStructuralRoofStyle::Flat &&
                    decoded.layout.levels[0].roofRidgeAxis ==
                        cr::CreativeStructuralRoofRidgeAxis::X &&
                    decoded.layout.levels[0].roofPitchDegrees ==
                        cr::kDefaultCreativeStructuralRoofPitchDegrees &&
                    decoded.layout.levels[0].roofOverhangCells == 0.0,
                "version-seven level migrates to stable flat roof defaults") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-seven roof migration writes current schema");
}

bool versionEightObjectsMigrateToIdentityCatalogPose() {
  const std::string versionEight =
      "IGGY3D_WORLD_LAYOUT 8\n"
      "L 8 6c6567616379 0 0 0 0 0 0 0 0 1 0 0 0\n"
      "Y " +
      std::to_string(static_cast<unsigned>(cr::CreativeObjectKind::Rock)) +
      " 1 726f636b 526f636b 626f756c6465725f3031 1 "
      "0 0 0 1 1 1 4 2 6 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionEight);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.objects.size() == 1U &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    !decoded.layout.objects[0].hasAssetSourceBounds &&
                    decoded.layout.objects[0].yawRadians == 0.0 &&
                    decoded.layout.objects[0].scale.x == 1.0 &&
                    decoded.layout.objects[0].scale.y == 1.0 &&
                    decoded.layout.objects[0].scale.z == 1.0,
                "version-eight objects migrate to identity catalog pose") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-eight migration writes current object fields");
}

bool versionNineOpeningsMigrateToProceduralInserts() {
  const std::string versionNine =
      "IGGY3D_WORLD_LAYOUT 9\n"
      "L 9 6c6567616379 0 1 0 0 0 0 1 1 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 8 4 0 3 1 0\n"
      "W 0 77616c6c 57616c6c 0 0 8 0 0 3 0.25\n"
      "O 0 0 0 0 0 0 646f6f72 446f6f72 4 1 0 2.1 1 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionNine);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.openings.size() == 1U &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.openings[0].insertAssetId.empty() &&
                    !decoded.layout.openings[0]
                         .hasInsertAssetSourceBounds,
                "version-nine openings retain procedural insert semantics") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-nine migration writes current opening fields");
}

bool versionTenBuildingsMigrateToAbsoluteGrounding() {
  const std::string versionTen =
      "IGGY3D_WORLD_LAYOUT 10\n"
      "L 10 6c6567616379 0 1 0 0 0 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 8 4 0 3 1 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionTen);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.buildings.size() == 1U &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.buildings[0].groundingMode ==
                        cr::CreativeWorldLayoutGroundingMode::Absolute &&
                    decoded.layout.buildings[0]
                            .maximumGroundReliefCells == 4U,
                "version-ten buildings retain absolute elevation") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-ten migration writes grounding fields");
}

bool versionTwelveRoomsMigrateToGenericType() {
  const std::string versionTwelve =
      "IGGY3D_WORLD_LAYOUT 12\n"
      "L 12 6c6567616379 0 1 1 1 0 0 0 0 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 8 4 0 3 0 4 1 0\n"
      "V 0 6c6576656c 4c6576656c 0 3 1 1 1 0 0 30 0\n"
      "R 0 0 726f6f6d 526f6f6d 0 0 8 4 0.25\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionTwelve);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.rooms.size() == 1U &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.rooms[0].type ==
                        cr::CreativeWorldLayoutRoomType::Generic,
                "version-twelve rooms migrate to generic semantic type") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-twelve migration writes typed room records");
}

bool versionThirteenWallsMigrateToStableDefaults() {
  const std::string versionThirteen =
      "IGGY3D_WORLD_LAYOUT 13\n"
      "L 13 6c6567616379 0 1 1 1 4 4 4 0 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 8 4 0 3 0 4 1 0\n"
      "V 0 6c6576656c 4c6576656c 0 3 1 1 1 0 0 30 0\n"
      "R 0 0 726f6f6d 526f6f6d 0 0 8 4 0.25 0\n"
      "N 0 7630 0 0\n"
      "N 0 7631 8 0\n"
      "N 0 7632 8 4\n"
      "N 0 7633 0 4\n"
      "E 0 6530 0 1 0.25\n"
      "E 0 6531 1 2 0.25\n"
      "E 0 6532 3 2 0.25\n"
      "E 0 6533 0 3 0.25\n"
      "U 0 0 0 0\n"
      "U 0 1 1 0\n"
      "U 0 2 2 1\n"
      "U 0 3 3 1\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionThirteen);
  const cr::CreativeWorldLayoutRoomGraph graph =
      decoded.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(decoded.layout)
          : cr::CreativeWorldLayoutRoomGraph{};
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && graph.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.topologyEdges.size() == 4U &&
                    std::all_of(
                        decoded.layout.topologyEdges.begin(),
                        decoded.layout.topologyEdges.end(),
                        [](const cr::CreativeWorldLayoutTopologyEdge& edge) {
                          return edge.wallHeightCells == 0U &&
                                 edge.profile ==
                                     cr::CreativeWorldLayoutWallProfile::Automatic &&
                                 edge.material ==
                                     cr::CreativeStructuralMaterial::Blockout &&
                                 edge.joinStyle ==
                                     cr::CreativeWorldLayoutWallJoinStyle::Square;
                        }),
                "version-thirteen topology walls migrate to explicit stable defaults") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-thirteen migration writes current wall attributes");
}

bool versionFourteenOpeningsMigrateToPositiveFacing() {
  cr::CreativeWorldLayout source = richLayout();
  source.terrainPaths.clear();
  source.openings[0].facing =
      cr::CreativeBuildingOpeningFacing::NegativeNormal;
  const cr::CreativeWorldLayoutEncodeResult current =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      current.accepted
          ? cr::decodeCreativeWorldLayout(
                versionFourteenTextWithoutOpeningFacing(current.encodedText))
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    std::all_of(
                        decoded.layout.openings.begin(),
                        decoded.layout.openings.end(),
                        [](const cr::CreativeWorldLayoutOpening& opening) {
                          return opening.facing ==
                                 cr::CreativeBuildingOpeningFacing::
                                     PositiveNormal;
                        }),
                "version-fourteen openings earn the positive-facing default") &&
         expect(reencoded.accepted &&
                    reencoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-fourteen migration writes explicit facing");
}

bool versionFifteenDoorPoseMigratesAgainstCanonicalWorldEdges() {
  cr::CreativeWorldLayout source = richLayout();
  source.terrainPaths.clear();
  source.openings.resize(1U);
  cr::CreativeWorldLayoutOpening& door = source.openings[0];
  door.hostKind = cr::CreativeWorldLayoutOpeningHostKind::Wall;
  door.wallIndex = 0U;
  door.kind = cr::CreativeBuildingOpeningKind::Door;
  door.cutoutBottomCells = 0.0;
  door.cutoutHeightCells = 2.1;
  door.insertAssetId.clear();
  door.hasInsertAssetSourceBounds = false;
  door.insertAssetSourceBoundsMeters = {};
  source.walls[0].start = {8, -3};
  source.walls[0].end = {-4, -3};

  const cr::CreativeWorldLayoutEncodeResult current =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      current.accepted
          ? cr::decodeCreativeWorldLayout(legacyOpeningText(
                current.encodedText, 15U,
                static_cast<unsigned>(cr::CreativeBuildingOpeningPose::
                                          OpenFromStartPositiveNormal),
                true))
          : cr::CreativeWorldLayoutDecodeResult{};

  return expect(decoded.accepted && decoded.layout.openings.size() == 1U,
                "version-fifteen door pose decodes") &&
         expect(decoded.layout.openings[0].door.initialState ==
                        cr::CreativeDoorInitialState::Open &&
                    decoded.layout.openings[0].door.hingeSide ==
                        cr::CreativeDoorHingeSide::MaximumEdge &&
                    decoded.layout.openings[0].door.swingSide ==
                        cr::CreativeDoorSwingSide::PositiveNormal,
                "legacy start hinge migrates against reversed host direction");
}

bool versionSixteenWindowsMigrateToGlazing() {
  cr::CreativeWorldLayout source = richLayout();
  source.terrainPaths.clear();
  source.openings.resize(1U);
  source.openings[0].window.insertKind =
      cr::CreativeWindowInsertKind::PairedShutters;
  const cr::CreativeWorldLayoutEncodeResult current =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      current.accepted
          ? cr::decodeCreativeWorldLayout(
                versionSixteenTextWithoutWindowTreatment(current.encodedText))
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.openings.size() == 1U,
                "version-sixteen window row decodes") &&
         expect(decoded.layout.openings[0].window ==
                    cr::CreativeWindowSettings{},
                "version-sixteen window earns glazing default") &&
         expect(reencoded.accepted &&
                    reencoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-sixteen migration writes explicit treatment");
}

bool versionSeventeenConnectorsMigrateToBlockoutMaterial() {
  cr::CreativeWorldLayout source = richLayout();
  source.terrainPaths.clear();
  source.verticalConnectors[0].material =
      cr::CreativeStructuralMaterial::Brick;
  const cr::CreativeWorldLayoutEncodeResult current =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      current.accepted
          ? cr::decodeCreativeWorldLayout(
                versionSeventeenTextWithoutConnectorMaterial(
                    current.encodedText))
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted &&
                    decoded.layout.verticalConnectors.size() == 1U,
                "version-seventeen connector row decodes") &&
         expect(decoded.layout.verticalConnectors[0].material ==
                    cr::CreativeStructuralMaterial::Blockout,
                "version-seventeen connector earns blockout material") &&
         expect(reencoded.accepted &&
                    reencoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-seventeen migration writes explicit material");
}

bool versionEighteenRoofsMigrateToDirectionAndMaterialDefaults() {
  cr::CreativeWorldLayout source = richLayout();
  source.terrainPaths.clear();
  source.levels[0].roofSlopeDirection =
      cr::CreativeStructuralRoofSlopeDirection::NegativeZ;
  source.levels[0].roofMaterial = cr::CreativeStructuralMaterial::Stone;
  const cr::CreativeWorldLayoutEncodeResult current =
      cr::encodeCreativeWorldLayout(source);
  const std::string legacy =
      current.accepted
          ? versionEighteenTextWithoutRoofDirectionAndMaterial(
                current.encodedText)
          : std::string{};
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(legacy);
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.levels.size() == 2U,
                "version-eighteen roof rows decode") &&
         expect(decoded.layout.levels[0].roofStyle ==
                        cr::CreativeStructuralRoofStyle::Gable &&
                    decoded.layout.levels[0].roofRidgeAxis ==
                        cr::CreativeStructuralRoofRidgeAxis::Z &&
                    decoded.layout.levels[0].roofPitchDegrees == 37.5 &&
                    decoded.layout.levels[0].roofOverhangCells == 0.75 &&
                    decoded.layout.levels[0].roofSlopeDirection ==
                        cr::CreativeStructuralRoofSlopeDirection::PositiveZ &&
                    decoded.layout.levels[0].roofMaterial ==
                        cr::CreativeStructuralMaterial::Blockout,
                "version-eighteen roofs earn explicit direction and material defaults") &&
         expect(reencoded.accepted &&
                    reencoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-eighteen migration writes current roof fields");
}

bool versionTwentyTerrainPathsMigrateToDurableRecipes() {
  const std::string versionTwenty =
      "IGGY3D_WORLD_LAYOUT 20\n"
      "L 20 6c6567616379 0 0 0 0 0 0 0 0 0 0 0 0 0 1 3 0\n"
      "T 726f6164 4 0 3 1 2 1 1 1\n"
      "Q 0 0 5\n"
      "Q 4 0 6\n"
      "Q 8 2 7\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionTwenty);
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};
  const cr::CreativeWorldLayoutDecodeResult roundTripped =
      reencoded.accepted
          ? cr::decodeCreativeWorldLayout(reencoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult encodedAgain =
      roundTripped.accepted
          ? cr::encodeCreativeWorldLayout(roundTripped.layout)
          : cr::CreativeWorldLayoutEncodeResult{};
  const cr::CreativeTerrainPathSourceRecipe* recipe =
      decoded.accepted && decoded.layout.terrainPaths.size() == 1U
          ? &decoded.layout.terrainPaths[0].recipe
          : nullptr;

  return expect(recipe != nullptr &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    recipe->kind == cr::CreativeTerrainPathKind::Road &&
                    recipe->elevation ==
                        cr::CreativeTerrainPathElevation::Level &&
                    recipe->curve ==
                        cr::CreativeTerrainPathCurvePolicy::Linear &&
                    recipe->crossSection ==
                        cr::CreativeTerrainPathCrossSection::Flat &&
                    recipe->startJoin ==
                        cr::CreativeTerrainPathEndpointJoin::Open &&
                    recipe->endJoin ==
                        cr::CreativeTerrainPathEndpointJoin::Open &&
                    recipe->falloffCells == 2U && recipe->paintSurface &&
                    recipe->material == cr::CreativeTerrainMaterial::Dirt &&
                    recipe->version ==
                        cr::kCreativeTerrainPathSourceVersion &&
                    recipe->road == cr::CreativeTerrainRoadSettings{} &&
                    recipe->nextPointId == 4U &&
                    recipe->points.size() == 3U &&
                    recipe->points[0] == cr::CreativeTerrainPathSourcePoint{
                                             1U, {0, 0}, 5U, 2U, 1U, 0} &&
                    recipe->points[1] == cr::CreativeTerrainPathSourcePoint{
                                             2U, {4, 0}, 6U, 2U, 1U, 0} &&
                    recipe->points[2] == cr::CreativeTerrainPathSourcePoint{
                                             3U, {8, 2}, 7U, 2U, 1U, 0},
                "version-twenty paths migrate to stable point identities and exact policies") &&
         expect(reencoded.accepted && roundTripped.accepted &&
                    encodedAgain.accepted &&
                    encodedAgain.encodedText == reencoded.encodedText,
                "version-twenty path migration is deterministic after current re-encode");
}

bool roadFieldsRoundTripAndVersionTwentyTwoPreservesLegacyBehavior() {
  cr::CreativeWorldLayout source = richLayout();
  const cr::CreativeTerrainRoadSettings authored =
      source.terrainPaths[0].recipe.road;
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult current =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  const std::string legacy =
      encoded.accepted
          ? versionTwentyTwoTextWithoutRoadFields(encoded.encodedText)
          : std::string{};
  const cr::CreativeWorldLayoutDecodeResult migrated =
      cr::decodeCreativeWorldLayout(legacy);
  const cr::CreativeTerrainPathSourceRecipe* migratedRoad =
      migrated.accepted && migrated.layout.terrainPaths.size() == 1U
          ? &migrated.layout.terrainPaths[0].recipe
          : nullptr;
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      migrated.accepted ? cr::encodeCreativeWorldLayout(migrated.layout)
                        : cr::CreativeWorldLayoutEncodeResult{};

  return expect(current.accepted &&
                    current.layout.terrainPaths.size() == 1U &&
                    current.layout.terrainPaths[0].recipe.road == authored,
                "current road construction fields round trip exactly") &&
         expect(migratedRoad != nullptr &&
                    migratedRoad->version ==
                        cr::kCreativeTerrainPathSourceVersion &&
                    migratedRoad->road == cr::CreativeTerrainRoadSettings{} &&
                    migratedRoad->road.maximumGradePermille == 0U &&
                    migratedRoad->points == source.terrainPaths[0].recipe.points,
                "version-twenty-two roads migrate to unlimited behavior-preserving defaults") &&
         expect(reencoded.accepted &&
                    reencoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-twenty-two road migration re-encodes current fields");
}

bool watercourseFieldsRoundTripAndVersionTwentyThreeDefaultsEmpty() {
  cr::CreativeWorldLayout source;
  source.stableKey = "watercourse_codec";
  cr::CreativeWorldLayoutTerrainPath river;
  river.stableKey = "river.north";
  river.recipe.kind = cr::CreativeTerrainPathKind::River;
  river.recipe.elevation = cr::CreativeTerrainPathElevation::Grade;
  river.recipe.crossSection = cr::CreativeTerrainPathCrossSection::Channel;
  river.recipe.material = cr::CreativeTerrainMaterial::Sand;
  river.recipe.watercourse.bankSlopeCells = 2U;
  river.recipe.watercourse.drainageDirection =
      cr::CreativeTerrainWatercourseDrainageDirection::StartToEnd;
  river.recipe.watercourse.surfacePolicy =
      cr::CreativeTerrainWaterSurfacePolicy::Reserved;
  river.recipe.watercourse.surfaceInsetCells = 1U;
  river.recipe.watercourse.nextCrossingId = 12U;
  river.recipe.watercourse.crossings = {{11U, 2U, 1U, 2U, 3U}};
  river.recipe.nextPointId = 4U;
  river.recipe.points = {
      {1U, {-4, 2}, 9U, 2U, 3U, 0},
      {2U, {0, 2}, 8U, 2U, 3U, 0},
      {3U, {4, 2}, 7U, 2U, 3U, 0},
  };
  source.terrainPaths.push_back(river);

  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  cr::CreativeWorldLayout legacySource = source;
  legacySource.terrainPaths[0].recipe.watercourse = {};
  const cr::CreativeWorldLayoutEncodeResult legacyCurrent =
      cr::encodeCreativeWorldLayout(legacySource);
  const std::string versionTwentyThree =
      legacyCurrent.accepted
          ? versionTwentyThreeTextWithoutWatercourseFields(
                legacyCurrent.encodedText)
          : std::string{};
  const cr::CreativeWorldLayoutDecodeResult migrated =
      cr::decodeCreativeWorldLayout(versionTwentyThree);
  const cr::CreativeTerrainPathSourceRecipe* migratedPath =
      migrated.accepted && migrated.layout.terrainPaths.size() == 1U
          ? &migrated.layout.terrainPaths[0].recipe
          : nullptr;

  return expect(encoded.accepted && decoded.accepted && reencoded.accepted &&
                    decoded.layout.terrainPaths.size() == 1U,
                "watercourse codec operations accepted") &&
         expect(decoded.layout.terrainPaths[0].recipe == river.recipe &&
                    reencoded.encodedText == encoded.encodedText,
                "watercourse settings crossings and ids round trip exactly") &&
         expect(migratedPath != nullptr &&
                    migratedPath->version ==
                        cr::kCreativeTerrainPathSourceVersion &&
                    migratedPath->watercourse ==
                        cr::CreativeTerrainWatercourseSettings{} &&
                    migratedPath->points ==
                        legacySource.terrainPaths[0].recipe.points,
                "version-twenty-three paths migrate to inert watercourse defaults");
}

bool bridgeFieldsRoundTripAndVersionTwentyFourPreservesLegacyBox() {
  cr::CreativeWorldLayout source;
  source.stableKey = "bridge_codec";
  cr::CreativeWorldLayoutObject bridge;
  bridge.kind = cr::CreativeObjectKind::Bridge;
  bridge.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  bridge.stableKey = "bridge.north";
  bridge.name = "North Bridge";
  bridge.boundsCells = {{-2.0, 0.0, -4.0}, {2.0, 0.5, 4.0}};
  bridge.tags = {"world_layout:object", "bridge:authored"};
  bridge.usesBridgeRecipe = true;
  bridge.bridge.watercoursePathKey = "river.north";
  bridge.bridge.crossingId = 17U;
  bridge.bridge.settings.deckWidthMeters = 3.25;
  bridge.bridge.settings.deckThicknessMeters = 0.45;
  bridge.bridge.settings.deckElevationOffsetMeters = 0.75;
  bridge.bridge.settings.maximumSpanMeters = 42.0;
  bridge.bridge.settings.minimumClearanceMeters = 0.8;
  bridge.bridge.settings.supportStyle =
      cr::CreativeBridgeSupportStyle::None;
  bridge.bridge.settings.supportSpacingMeters = 5.5;
  bridge.bridge.settings.supportWidthMeters = 0.6;
  bridge.bridge.settings.supportDepthMeters = 0.7;
  bridge.bridge.settings.rails = false;
  bridge.bridge.settings.railHeightMeters = 1.2;
  bridge.bridge.settings.railThicknessMeters = 0.15;
  bridge.bridge.settings.maximumApproachGradePermille = 350U;
  bridge.bridge.settings.approachFalloffCells = 3U;
  bridge.bridge.settings.materials.deck =
      cr::CreativeStructuralMaterial::Stone;
  bridge.bridge.settings.materials.supports =
      cr::CreativeStructuralMaterial::Brick;
  bridge.bridge.settings.materials.rails =
      cr::CreativeStructuralMaterial::Timber;
  source.objects.push_back(bridge);

  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  const std::string versionTwentyFour =
      encoded.accepted
          ? versionTwentyFourTextWithoutBridgeFields(encoded.encodedText)
          : std::string{};
  const cr::CreativeWorldLayoutDecodeResult migrated =
      cr::decodeCreativeWorldLayout(versionTwentyFour);
  const cr::CreativeWorldLayoutObject* migratedBridge =
      migrated.accepted && migrated.layout.objects.size() == 1U
          ? &migrated.layout.objects[0]
          : nullptr;
  const cr::CreativeWorldLayoutEncodeResult migratedReencoded =
      migrated.accepted ? cr::encodeCreativeWorldLayout(migrated.layout)
                        : cr::CreativeWorldLayoutEncodeResult{};
  cr::CreativeWorldLayout invalidSource = source;
  invalidSource.objects[0].bridge.watercoursePathKey.clear();
  const cr::CreativeWorldLayoutEncodeResult invalidBridge =
      cr::encodeCreativeWorldLayout(invalidSource);

  return expect(encoded.accepted && decoded.accepted && reencoded.accepted &&
                    decoded.layout.objects.size() == 1U,
                "bridge codec operations accepted") &&
         expect(decoded.layout.objects[0].usesBridgeRecipe &&
                    decoded.layout.objects[0].bridge == bridge.bridge &&
                    reencoded.encodedText == encoded.encodedText,
                "bridge attachment construction and material fields round trip exactly") &&
         expect(migratedBridge != nullptr &&
                    migrated.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    migratedBridge->kind == cr::CreativeObjectKind::Bridge &&
                    !migratedBridge->usesBridgeRecipe &&
                    migratedBridge->bridge == cr::CreativeBridgeSourceRecipe{} &&
                    migratedBridge->boundsCells.min.x == -2.0 &&
                    migratedBridge->boundsCells.max.z == 4.0,
                "version-twenty-four bridge remains the original generic box") &&
         expect(migratedReencoded.accepted &&
                    migratedReencoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-twenty-four bridge migration writes current defaults") &&
         expect(!invalidBridge.accepted,
                "invalid bridge attachment fails closed before serialization");
}

bool landformsRoundTripAndVersionTwentyOnePlateausStayLegacy() {
  cr::CreativeWorldLayout source;
  source.stableKey = "landform_codec";
  cr::CreativeWorldLayoutTerrainProfile terrace;
  terrace.stableKey = "terrace.entry";
  terrace.kind = cr::CreativeTerrainRecipeKind::Terrace;
  terrace.usesLandformRecipe = true;
  terrace.landform.kind = cr::CreativeTerrainLandformKind::Terrace;
  terrace.landform.bounds = {{-3, 5}, 12U, 7U};
  terrace.landform.baseHeightCells = 2U;
  terrace.landform.targetHeightCells = 11U;
  terrace.landform.terraceCount = 3U;
  terrace.landform.direction =
      cr::CreativeTerrainLandformDirection::NegativeZ;
  terrace.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  terrace.landform.edgeWidthCells = 0U;
  terrace.landform.featherCells = 2U;
  terrace.landform.paintSurface = true;
  terrace.landform.material = cr::CreativeTerrainMaterial::Stone;
  terrace.landform.erosion =
      cr::CreativeTerrainLandformErosion::Weathered;
  terrace.landform.erosionReliefCells = 1U;
  terrace.landform.seed = 991U;
  source.terrainProfiles.push_back(terrace);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutTerrainProfile* roundTripped =
      decoded.accepted && decoded.layout.terrainProfiles.size() == 1U
          ? &decoded.layout.terrainProfiles[0]
          : nullptr;

  std::string invalidKindText = encoded.encodedText;
  const bool locatedKind =
      replaceFirstTerrainProfileToken(invalidKindText, 15U, "255");
  const cr::CreativeWorldLayoutDecodeResult invalidKind =
      cr::decodeCreativeWorldLayout(invalidKindText);

  cr::CreativeWorldLayout legacy;
  legacy.stableKey = "legacy_plateau_codec";
  cr::CreativeWorldLayoutTerrainProfile plateau;
  plateau.stableKey = "plateau.legacy";
  plateau.kind = cr::CreativeTerrainRecipeKind::Plateau;
  plateau.center = {4, 9};
  plateau.baseHeightCells = 7U;
  plateau.radiusCells = 8U;
  plateau.spacingCells = 2U;
  legacy.terrainProfiles.push_back(plateau);
  const cr::CreativeWorldLayoutEncodeResult legacyCurrent =
      cr::encodeCreativeWorldLayout(legacy);
  const std::string versionTwentyOne =
      legacyCurrent.accepted
          ? versionTwentyOneTextWithoutLandformFields(
                legacyCurrent.encodedText)
          : std::string{};
  const cr::CreativeWorldLayoutDecodeResult migrated =
      cr::decodeCreativeWorldLayout(versionTwentyOne);
  const cr::CreativeWorldLayoutTerrainProfile* migratedPlateau =
      migrated.accepted && migrated.layout.terrainProfiles.size() == 1U
          ? &migrated.layout.terrainProfiles[0]
          : nullptr;

  return expect(roundTripped != nullptr &&
                    roundTripped->kind ==
                        cr::CreativeTerrainRecipeKind::Terrace &&
                    roundTripped->usesLandformRecipe &&
                    roundTripped->landform == terrace.landform,
                "bounded landform source round trips every recipe field") &&
         expect(locatedKind && !invalidKind.accepted &&
                    invalidKind.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "invalid serialized landform enum fails closed") &&
         expect(migratedPlateau != nullptr &&
                    migrated.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    !migratedPlateau->usesLandformRecipe &&
                    migratedPlateau->kind ==
                        cr::CreativeTerrainRecipeKind::Plateau &&
                    migratedPlateau->center == plateau.center &&
                    migratedPlateau->radiusCells == plateau.radiusCells,
                "version-twenty-one plateau preserves legacy radial semantics");
}

bool retainingEdgesRoundTripAndVersionTwentyFiveDefaultsDisabled() {
  cr::CreativeWorldLayout source;
  source.stableKey = "retaining_edge_codec";
  cr::CreativeWorldLayoutTerrainProfile terrace;
  terrace.stableKey = "terrace.retained";
  terrace.kind = cr::CreativeTerrainRecipeKind::Terrace;
  terrace.usesLandformRecipe = true;
  terrace.landform.kind = cr::CreativeTerrainLandformKind::Terrace;
  terrace.landform.bounds = {{-4, 2}, 12U, 8U};
  terrace.landform.baseHeightCells = 2U;
  terrace.landform.targetHeightCells = 10U;
  terrace.landform.terraceCount = 3U;
  terrace.landform.direction =
      cr::CreativeTerrainLandformDirection::PositiveX;
  terrace.landform.edge = cr::CreativeTerrainLandformEdge::Retaining;
  terrace.landform.edgeWidthCells = 0U;
  terrace.usesRetainingEdgeRecipe = true;
  terrace.retainingEdge.terrainProfileKey = terrace.stableKey;
  terrace.retainingEdge.settings.selection =
      cr::CreativeRetainingEdgeSelection::Perimeter;
  terrace.retainingEdge.settings.kit =
      cr::CreativeRetainingEdgeKit::InfrastructureStone;
  terrace.retainingEdge.settings.thicknessMeters = 0.45;
  terrace.retainingEdge.settings.maximumHeightMeters = 12.0;
  terrace.retainingEdge.settings.closeCorners = false;
  terrace.retainingEdge.settings.capEnds = true;
  terrace.retainingEdge.settings.material =
      cr::CreativeStructuralMaterial::Brick;
  terrace.retainingEdge.settings.transitionCount = 2U;
  terrace.retainingEdge.settings.transitions[0] = {
      cr::canonicalCreativeTerrainHardEdge({0, 0}, {1, 0}),
      cr::CreativeRetainingEdgeTransitionKind::Stair,
      3U,
  };
  terrace.retainingEdge.settings.transitions[1] = {
      cr::canonicalCreativeTerrainHardEdge({2, 1}, {2, 2}),
      cr::CreativeRetainingEdgeTransitionKind::Ramp,
      6U,
  };
  source.terrainProfiles.push_back(terrace);

  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      encoded.accepted
          ? cr::decodeCreativeWorldLayout(encoded.encodedText)
          : cr::CreativeWorldLayoutDecodeResult{};
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};
  const cr::CreativeWorldLayoutTerrainProfile* roundTripped =
      decoded.accepted && decoded.layout.terrainProfiles.size() == 1U
          ? &decoded.layout.terrainProfiles[0]
          : nullptr;

  const std::string versionTwentyFive =
      encoded.accepted
          ? versionTwentyFiveTextWithoutRetainingFields(encoded.encodedText)
          : std::string{};
  const cr::CreativeWorldLayoutDecodeResult migrated =
      cr::decodeCreativeWorldLayout(versionTwentyFive);
  const cr::CreativeWorldLayoutTerrainProfile* migratedTerrace =
      migrated.accepted && migrated.layout.terrainProfiles.size() == 1U
          ? &migrated.layout.terrainProfiles[0]
          : nullptr;

  cr::CreativeWorldLayout mismatched = source;
  mismatched.terrainProfiles[0].retainingEdge.terrainProfileKey =
      "terrace.other";
  const cr::CreativeWorldLayoutEncodeResult mismatchedResult =
      cr::encodeCreativeWorldLayout(mismatched);
  cr::CreativeWorldLayout duplicate = source;
  duplicate.terrainProfiles[0].retainingEdge.settings.transitions[1].edge =
      duplicate.terrainProfiles[0]
          .retainingEdge.settings.transitions[0]
          .edge;
  const cr::CreativeWorldLayoutEncodeResult duplicateResult =
      cr::encodeCreativeWorldLayout(duplicate);

  return expect(encoded.accepted && roundTripped != nullptr &&
                    roundTripped->usesRetainingEdgeRecipe &&
                    roundTripped->retainingEdge == terrace.retainingEdge &&
                    reencoded.accepted &&
                    reencoded.encodedText == encoded.encodedText,
                "retaining source settings and transitions round trip exactly") &&
         expect(migratedTerrace != nullptr &&
                    migrated.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    !migratedTerrace->usesRetainingEdgeRecipe &&
                    migratedTerrace->retainingEdge ==
                        cr::CreativeRetainingEdgeSourceRecipe{},
                "version-twenty-five landforms migrate with retaining source disabled") &&
         expect(!mismatchedResult.accepted,
                "retaining attachment key mismatch fails before serialization") &&
         expect(!duplicateResult.accepted,
                "duplicate retaining transition seam fails before serialization");
}

}  // namespace

int main() {
  const bool ok = deterministicRoundTripPreservesEveryTable() &&
                  playerSpawnSettingsRoundTripAndVersionTwentySixDefaults() &&
                  sourceFingerprintUsesExactVersionedEncoding() &&
                  roofAperturesRoundTripAndVersionNineteenMigratesEmpty() &&
                  directTopologyHostDoesNotInventACardinalRoomSide() &&
                  malformedAndNonFiniteInputsFailClosed() &&
                  versionOneSourceMigratesToCurrentSchema() &&
                  versionTwoSourceMigratesWithoutFabricatedObjects() &&
                  versionThreeRoomPreservesWallPlaneDuringMigration() &&
                  versionFourBoxesMigrateToExplicitAnchorPlanes() &&
                  versionFiveRoomsMigrateToSharedLevels() &&
                  versionSixSourceMigratesWithoutFabricatedVerticalConnectors() &&
                  versionSevenLevelsMigrateToFlatRoofDefaults() &&
                  versionEightObjectsMigrateToIdentityCatalogPose() &&
                  versionNineOpeningsMigrateToProceduralInserts() &&
                  versionTenBuildingsMigrateToAbsoluteGrounding() &&
                  versionTwelveRoomsMigrateToGenericType() &&
                  versionThirteenWallsMigrateToStableDefaults() &&
                  versionFourteenOpeningsMigrateToPositiveFacing() &&
                  versionFifteenDoorPoseMigratesAgainstCanonicalWorldEdges() &&
                  versionSixteenWindowsMigrateToGlazing() &&
                  versionSeventeenConnectorsMigrateToBlockoutMaterial() &&
                  versionEighteenRoofsMigrateToDirectionAndMaterialDefaults() &&
                  versionTwentyTerrainPathsMigrateToDurableRecipes() &&
                  roadFieldsRoundTripAndVersionTwentyTwoPreservesLegacyBehavior() &&
                  watercourseFieldsRoundTripAndVersionTwentyThreeDefaultsEmpty() &&
                  bridgeFieldsRoundTripAndVersionTwentyFourPreservesLegacyBox() &&
                  landformsRoundTripAndVersionTwentyOnePlateausStayLegacy() &&
                  retainingEdgesRoundTripAndVersionTwentyFiveDefaultsDisabled();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
