#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr const char* kWindowStatePath =
    "src/app/iggy3d/ProductAppWindowState.hpp";
constexpr const char* kOwnershipMapPath =
    "docs/god_struct_member_ownership.tsv";
constexpr const char* kFixHint =
    "add it to docs/god_struct_member_ownership.tsv with its target store from "
    "docs/god_struct_decomposition_target_map.md";

const std::set<std::string>& validOwners() {
  static const std::set<std::string> owners = {
      "RoomStore",
      "CreativeIdentityStore",
      "CreativeFlyAnchorStore",
      "CreativeAuthoringStore",
      "SaveSessionStore",
      "ViewportStore",
      "InputDeviceStore",
      "GameplayStore",
      "DebugHudStore",
      "FrontendWindowShell",
      "PresentPathStore",
      "app-global-remainder",
      "delete",
  };
  return owners;
}

std::string trim(std::string_view value) {
  const std::size_t begin = value.find_first_not_of(" \t\r\n");
  if (begin == std::string_view::npos) {
    return {};
  }
  const std::size_t end = value.find_last_not_of(" \t\r\n");
  return std::string(value.substr(begin, end - begin + 1U));
}

bool startsWith(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() &&
         value.substr(0U, prefix.size()) == prefix;
}

std::string joinDeclaration(const std::vector<std::string>& lines) {
  std::ostringstream out;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i != 0U) {
      out << ' ';
    }
    out << lines[i];
  }
  return out.str();
}

bool identifierChar(char value) {
  return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
         (value >= '0' && value <= '9') || value == '_';
}

bool parseMemberName(std::string_view declaration, std::string& out) {
  const std::size_t semicolon = declaration.find(';');
  if (semicolon == std::string_view::npos) {
    return false;
  }
  std::string head = std::string(declaration.substr(0U, semicolon));
  const std::size_t initializer = head.find('=');
  if (initializer != std::string::npos) {
    head = head.substr(0U, initializer);
  }
  head = trim(head);
  if (head.empty() || head.find(',') != std::string::npos) {
    return false;
  }

  std::size_t end = head.size();
  while (end > 0U && !identifierChar(head[end - 1U])) {
    --end;
  }
  if (end == 0U) {
    return false;
  }
  std::size_t begin = end;
  while (begin > 0U && identifierChar(head[begin - 1U])) {
    --begin;
  }
  if (begin == end) {
    return false;
  }
  out = head.substr(begin, end - begin);
  return !out.empty();
}

struct ExtractedMember {
  std::string name;
  std::uint64_t line = 0;
  std::string declaration;
};

std::vector<ExtractedMember> extractMembers(bool& ok) {
  std::ifstream in(kWindowStatePath);
  if (!in) {
    std::cerr << "ownership coverage: cannot read " << kWindowStatePath
              << '\n';
    ok = false;
    return {};
  }

  std::vector<ExtractedMember> members;
  std::vector<std::string> declarationLines;
  bool inStruct = false;
  bool sawStruct = false;
  std::uint64_t lineNo = 0;
  std::uint64_t declarationStartLine = 0;
  std::string line;
  while (std::getline(in, line)) {
    ++lineNo;
    const std::string stripped = trim(line);
    if (!inStruct) {
      if (stripped == "struct ProductAppWindowState {") {
        inStruct = true;
        sawStruct = true;
      }
      continue;
    }
    if (stripped == "};") {
      if (!declarationLines.empty()) {
        std::cerr << "ownership coverage: unterminated declaration before "
                  << kWindowStatePath << ':' << declarationStartLine << '\n';
        ok = false;
      }
      inStruct = false;
      break;
    }
    if (stripped.empty() || startsWith(stripped, "//")) {
      continue;
    }
    if (declarationLines.empty()) {
      declarationStartLine = lineNo;
    }
    declarationLines.push_back(stripped);
    if (stripped.find(';') == std::string::npos) {
      continue;
    }

    const std::string declaration = joinDeclaration(declarationLines);
    std::string memberName;
    if (!parseMemberName(declaration, memberName)) {
      std::cerr << "ownership coverage: ambiguous ProductAppWindowState member "
                   "declaration at "
                << kWindowStatePath << ':' << declarationStartLine << ": "
                << declaration << '\n';
      ok = false;
    } else {
      members.push_back({memberName, declarationStartLine, declaration});
    }
    declarationLines.clear();
  }

  if (!sawStruct) {
    std::cerr << "ownership coverage: ProductAppWindowState struct not found in "
              << kWindowStatePath << '\n';
    ok = false;
  } else if (inStruct) {
    std::cerr << "ownership coverage: ProductAppWindowState closing brace not "
                 "found in "
              << kWindowStatePath << '\n';
    ok = false;
  }
  return members;
}

std::map<std::string, std::string> loadOwnershipMap(bool& ok) {
  std::ifstream in(kOwnershipMapPath);
  if (!in) {
    std::cerr << "ownership coverage: cannot read " << kOwnershipMapPath
              << '\n';
    ok = false;
    return {};
  }

  std::map<std::string, std::string> ownership;
  std::uint64_t lineNo = 0;
  std::string line;
  while (std::getline(in, line)) {
    ++lineNo;
    const std::string stripped = trim(line);
    if (stripped.empty() || startsWith(stripped, "#")) {
      continue;
    }
    const std::size_t tab = stripped.find('\t');
    if (tab == std::string::npos || stripped.find('\t', tab + 1U) != std::string::npos) {
      std::cerr << "ownership coverage: expected exactly one tab at "
                << kOwnershipMapPath << ':' << lineNo << '\n';
      ok = false;
      continue;
    }
    const std::string member = stripped.substr(0U, tab);
    const std::string owner = stripped.substr(tab + 1U);
    if (member.empty() || owner.empty()) {
      std::cerr << "ownership coverage: empty member or owner at "
                << kOwnershipMapPath << ':' << lineNo << '\n';
      ok = false;
      continue;
    }
    if (!validOwners().contains(owner)) {
      std::cerr << "ownership coverage: invalid owner '" << owner << "' for "
                << member << " at " << kOwnershipMapPath << ':' << lineNo
                << '\n';
      ok = false;
    }
    const auto inserted = ownership.emplace(member, owner);
    if (!inserted.second) {
      std::cerr << "ownership coverage: duplicate owner row for " << member
                << " at " << kOwnershipMapPath << ':' << lineNo << '\n';
      ok = false;
    }
  }
  return ownership;
}

bool coverageMatches(const std::vector<ExtractedMember>& members,
                     const std::map<std::string, std::string>& ownership) {
  bool ok = true;
  std::set<std::string> memberNames;
  for (const ExtractedMember& member : members) {
    if (!memberNames.insert(member.name).second) {
      std::cerr << "ownership coverage: duplicate extracted member "
                << member.name << " at " << kWindowStatePath << ':'
                << member.line << '\n';
      ok = false;
    }
    if (!ownership.contains(member.name)) {
      std::cerr << "ownership coverage: unassigned ProductAppWindowState "
                   "member '"
                << member.name << "' at " << kWindowStatePath << ':'
                << member.line << "; " << kFixHint << '\n';
      ok = false;
    }
  }
  for (const auto& [member, owner] : ownership) {
    (void)owner;
    if (!memberNames.contains(member)) {
      std::cerr << "ownership coverage: stale TSV row for missing "
                   "ProductAppWindowState member '"
                << member << "'; remove or rename it in " << kOwnershipMapPath
                << '\n';
      ok = false;
    }
  }
  return ok;
}

void printSummary(const std::map<std::string, std::string>& ownership) {
  std::map<std::string, std::uint64_t> counts;
  for (const auto& [member, owner] : ownership) {
    (void)member;
    ++counts[owner];
  }
  std::cout << "god-struct ownership coverage: assigned=" << ownership.size();
  for (const std::string& owner : validOwners()) {
    const std::uint64_t count = counts[owner];
    if (count > 0U) {
      std::cout << ' ' << owner << '=' << count;
    }
  }
  std::cout << '\n';
}

}  // namespace

int main() {
  bool ok = true;
  const std::vector<ExtractedMember> members = extractMembers(ok);
  const std::map<std::string, std::string> ownership = loadOwnershipMap(ok);
  ok = ok && coverageMatches(members, ownership);
  if (ok) {
    printSummary(ownership);
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
