#include "content/EquationSorterContentIO.hpp"

#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <nlohmann/json.hpp>

using namespace paths;
using Json = nlohmann::json;
void expect(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void rejected(const Json& document, const std::string& field) {
  try { (void)parseSorterContent(document.dump(), "edited.json"); }
  catch (const EquationSorterContentError& e) {
    expect(e.source == "edited.json" && e.field == field, "loader retains source and exact error field");
    return;
  }
  throw std::runtime_error("invalid content was accepted");
}
int main() {
  try {
    std::ifstream input(SORTER_FIXTURE), witnessInput(SORTER_SOLUTIONS);
    Json document, witnesses;
    input >> document; witnessInput >> witnesses;
    const auto content = loadSorterContent(SORTER_FIXTURE);
    expect(content.size() == 100 && witnesses.size() == 100, "complete fixture and witnesses");
    // Interpret the rendered strings independently of the authoring script.
    const std::array patterns{
      std::regex(R"(^x ([+-]) ([0-9]+) = (-?[0-9]+)$)"),
      std::regex(R"(^(-?[0-9]+)x = (-?[0-9]+)$)"),
      std::regex(R"(^(-?[0-9]+)x ([+-]) ([0-9]+) = (-?[0-9]+)$)"),
      std::regex(R"(^(-?[0-9]+)\(x ([+-]) ([0-9]+)\) = (-?[0-9]+)$)")};
    std::array<int, 4> forms{};
    for (const auto& e : content) {
      expect(e.subject == SorterSubject::Algebra && !e.hint.empty(), "every prepared algebra card has a subject and next-operation hint");
      std::smatch match;
      std::size_t form = 0;
      for (; form < patterns.size(); ++form) if (std::regex_match(e.text, match, patterns[form])) break;
      expect(form < 4, "equation has a supported independently parsed form");
      ++forms[form];
      const auto solution = witnesses.at(std::to_string(e.id)).get<long long>();
      expect(solution >= -128 && solution <= 128, "bounded integer witness");
      const long long a = form == 0 ? 1 : std::stoll(match[1]);
      const long long b = form == 1 ? 0 : std::stoll(match[form == 0 ? 2 : 3]) *
          (match[form == 0 ? 1 : 2] == "-" ? -1 : 1);
      const long long c = std::stoll(match[form == 0 ? 3 : form == 1 ? 2 : 4]);
      expect(a != 0 && std::abs(a) <= 64 && std::abs(b) <= 64 && std::abs(c) <= 16384, "bounded unique linear solution");
      expect((form == 3 ? a * (solution + b) : a * solution + b) == c, "rendered equation agrees with independent witness");
    }
    expect(forms == std::array{25, 25, 25, 25}, "all four families represented equally");
    auto broken = document; broken["equations"].erase(0); rejected(broken, "/equations");
    broken = document; broken["schema_version"] = 2; rejected(broken, "/schema_version");
    broken = document; broken["equations"][0]["id"] = 0; rejected(broken, "/equations/0/id");
    broken = document; broken["equations"][1]["id"] = broken["equations"][0]["id"]; rejected(broken, "/equations/1/id");
    broken = document; broken["equations"][1]["home_index"] = 0; rejected(broken, "/equations/1/home_index");
    broken = document; broken["equations"][0]["home_index"] = 100; rejected(broken, "/equations/0/home_index");
    broken = document; broken["equations"][0]["id"] = 4294967296ULL; rejected(broken, "/equations/0/id");
    broken = document; broken["equations"][0]["id"] = -2; rejected(broken, "/equations/0/id");
    broken = document; broken["equations"][0].erase("text"); rejected(broken, "/equations/0/text");
    broken = document; broken["equations"][0]["text"] = "\n"; rejected(broken, "/equations/0/text");
    broken = document; broken["equations"][0]["text"] = std::string(97, 'x'); rejected(broken, "/equations/0/text");
    broken = document; broken["equations"][1]["text"] = broken["equations"][0]["text"]; rejected(broken, "/equations/1/text");
    broken = document; broken["equations"][0]["text"] = 7; rejected(broken, "/equations/0/text");
    broken = document; broken["equations"][0]["subject"] = "astronomy"; rejected(broken, "/equations/0/subject");
    broken = document; broken["equations"][0]["subject"] = 7; rejected(broken, "/equations/0/subject");
    broken = document; broken["equations"][0]["hint"] = 7; rejected(broken, "/equations/0/hint");
    broken = document; broken["equations"][0]["hint"] = "\n"; rejected(broken, "/equations/0/hint");
    broken = document; broken["equations"][0]["hint"] = std::string(241, 'x'); rejected(broken, "/equations/0/hint");
    const Json study={{"chapter","Linear equations"},{"type","Bracket equations"},{"form","a(x + b) = c"}};
    broken=document;broken["equations"][0]["study"]=study;
    expect(parseSorterContent(broken.dump(),"study.json")[0].study->chapter=="Linear equations","authored chapter and problem type are loaded verbatim");
    broken["equations"][0]["study"]=7;rejected(broken,"/equations/0/study");
    for(const auto key:{"chapter","type","form"}) {
      broken=document;broken["equations"][0]["study"]=study;broken["equations"][0]["study"].erase(key);
      rejected(broken,std::string("/equations/0/study/")+key);
      broken["equations"][0]["study"]=study;broken["equations"][0]["study"][key]=7;
      rejected(broken,std::string("/equations/0/study/")+key);
      broken["equations"][0]["study"][key]="";rejected(broken,"/equations/0/study");
    }
    broken=document;broken["equations"][0]["study"]=study;broken["equations"][0].erase("subject");
    rejected(broken,"/equations/0/study");
    broken=document;broken["equations"][0]["study"]=study;broken["equations"][1]["study"]=study;
    broken["equations"][1]["study"]["form"]="ax = b";rejected(broken,"/equations/1/study/form");
    for (const auto* text : {"sin(theta) = 1/2", "d/dx (x^3) = 3x^2", "A v = lambda v", "P(A union B) = P(A) + P(B) - P(A intersect B)"}) {
      auto otherSubject = document;
      otherSubject["equations"][0]["text"] = text;
      otherSubject["equations"][0].erase("subject");
      otherSubject["equations"][0].erase("hint");
      const auto unknown = parseSorterContent(otherSubject.dump(), "subject.json")[0];
      expect(unknown.text == text && !unknown.subject && unknown.hint.empty(), "untagged v1 content remains manual; no subject guessing");
    }
    for (const auto& text : {std::string("{"), std::string(1024 * 1024 + 1, ' ')}) {
      bool failed = false;
      try { (void)parseSorterContent(text, "invalid.json"); } catch (const EquationSorterContentError&) { failed = true; }
      expect(failed, "malformed/oversized input refused");
    }
    bool missing = false;
    try { (void)loadSorterContent(std::filesystem::path(SORTER_FIXTURE) / "missing.json"); }
    catch (const EquationSorterContentError&) { missing = true; }
    expect(missing, "missing file refused without fallback");
    auto invalid = content; invalid[1].id = invalid[0].id;
    bool invalidConstructor = false;
    try { EquationSorterSession session(invalid); } catch (const std::invalid_argument&) { invalidConstructor = true; }
    expect(invalidConstructor, "constructor shares structural validation");
    invalid = content; invalid[0].subject = static_cast<SorterSubject>(99);
    expect(validateSorterContent(invalid)->field == "/equations/0/subject", "pure constructor boundary validates subject enum");
    std::cout << "100 equations independently checked; loader errors and four subject notations passed\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
