#include "content/EquationSorterContentIO.hpp"
#include "content/QuestionContentIO.hpp"

#include <fstream>
#include <algorithm>
#include <limits>
#include <utility>
#include <nlohmann/json.hpp>

namespace paths {
namespace { constexpr std::size_t maxFileBytes = 1024 * 1024; }
EquationSorterContentError::EquationSorterContentError(
    std::filesystem::path sourcePath, std::string fieldPath, std::string reason)
    : std::runtime_error(sourcePath.string() + fieldPath + ": " + reason),
      source(std::move(sourcePath)), field(std::move(fieldPath)) {}

std::vector<SorterEquation> parseSorterContent(std::string_view text, const std::filesystem::path& source) {
  const auto fail = [&](std::string field, std::string reason) -> void {
    throw EquationSorterContentError(source, std::move(field), std::move(reason));
  };
  if (text.size() > maxFileBytes) fail("", "file exceeds 1 MiB");
  using Json = nlohmann::json;
  Json document;
  try { document = Json::parse(text); }
  catch (const Json::exception& e) { fail("", e.what()); }
  if (!document.is_object()) fail("", "expected an object");
  const auto member = [&](const Json& object, const char* name, const std::string& base) -> const Json& {
    const auto found = object.find(name);
    if (found == object.end()) fail(base + "/" + name, "missing required field");
    return *found;
  };
  const auto number = [&](const Json& value, const std::string& field) -> std::uint32_t {
    if (!value.is_number_unsigned() || value.get<std::uint64_t>() > std::numeric_limits<std::uint32_t>::max())
      fail(field, "expected an unsigned 32-bit integer");
    return value.get<std::uint32_t>();
  };
  if (number(member(document, "schema_version", ""), "/schema_version") != 1)
    fail("/schema_version", "unsupported schema version; expected 1");
  const auto& records = member(document, "equations", "");
  if (!records.is_array() || records.size() != sorterEquationCount)
    fail("/equations", "expected exactly 100 equations");
  std::vector<SorterEquation> content;
  content.reserve(sorterEquationCount);
  for (std::size_t i = 0; i < records.size(); ++i) {
    const auto field = "/equations/" + std::to_string(i);
    const auto& record = records[i];
    if (!record.is_object()) fail(field, "expected an object");
    const auto id = number(member(record, "id", field), field + "/id");
    const auto home = number(member(record, "home_index", field), field + "/home_index");
    const auto& label = member(record, "text", field);
    if (!label.is_string()) fail(field + "/text", "expected a string");
    content.push_back({id, home, label.get<std::string>()});
    if (const auto subject = record.find("subject"); subject != record.end()) {
      if (!subject->is_string()) fail(field + "/subject", "expected a maths subject string");
      const auto found = std::find_if(sorterSubjects.begin(), sorterSubjects.end(), [&](const auto& s) { return s.key == subject->get<std::string>(); });
      if (found == sorterSubjects.end()) fail(field + "/subject", "unknown maths subject");
      content.back().subject = static_cast<SorterSubject>(found - sorterSubjects.begin());
    }
    if (const auto hint = record.find("hint"); hint != record.end()) {
      if (!hint->is_string()) fail(field + "/hint", "expected a hint string");
      content.back().hint = hint->get<std::string>();
    }
    if(const auto study=record.find("study");study!=record.end()) {
      if(!study->is_object())fail(field+"/study","expected a study classification object");
      SorterEquation::StudyTopic topic;
      for(const auto& [key,destination]:std::array{
          std::pair{"chapter",&topic.chapter},std::pair{"type",&topic.type},std::pair{"form",&topic.form}}) {
        const auto& value=member(*study,key,field+"/study");
        if(!value.is_string())fail(field+"/study/"+key,"expected a title string");
        *destination=value.get<std::string>();
      }
      content.back().study=std::move(topic);
    }
    if (const auto solve=record.find("solve_pack"); solve!=record.end()) {
      if (!solve->is_string()) fail(field+"/solve_pack","expected a relative pack path");
      const std::filesystem::path relative=solve->get<std::string>();
      if (relative.empty() || relative.is_absolute()) fail(field+"/solve_pack","expected a nonempty relative pack path");
      content.back().solvePack=relative.string();
    }
  }
  if (const auto error = validateSorterContent(content)) fail(error->field, error->reason);
  return content;
}
std::vector<SorterEquation> loadSorterContent(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw EquationSorterContentError(path, "", "cannot open file");
  std::string text(maxFileBytes + 1, '\0');
  input.read(text.data(), static_cast<std::streamsize>(text.size()));
  if (input.bad()) throw EquationSorterContentError(path, "", "cannot read file");
  text.resize(static_cast<std::size_t>(input.gcount()));
  auto content=parseSorterContent(text,path);
  for (std::size_t i=0;i<content.size();++i) if (!content[i].solvePack.empty()) {
    try {
      const auto pack=loadQuestionPack(path.parent_path()/content[i].solvePack);
      if (pack.catalog.size()!=1 || pack.deck("solve")!=std::vector<std::size_t>{0})
        throw std::invalid_argument("solve pack must contain one question and one solve deck entry");
      content[i].solution=std::make_shared<const iggy3d::first_move::LayeredQuestionContent>(pack.catalog[0]);
    } catch (const std::exception& error) {
      throw EquationSorterContentError(path,"/equations/"+std::to_string(i)+"/solve_pack",error.what());
    }
  }
  if (const auto error=validateSorterContent(content)) throw EquationSorterContentError(path,error->field,error->reason);
  return content;
}
} // namespace paths
