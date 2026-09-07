#include "content/QuestionContentIO.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <utility>

#include <nlohmann/json.hpp>

namespace paths {
namespace fm = iggy3d::first_move;
namespace {
using Json = nlohmann::json;
constexpr std::size_t maxFileBytes = 1024 * 1024;
constexpr std::uint32_t schemaVersion = 1;

std::string pointerToken(std::string_view token) {
  std::string result;
  for(const char c : token) {
    switch(c) {
      case '~': result += "~0"; break;
      case '/': result += "~1"; break;
      default: result += c; break;
    }
  }
  return result;
}

// A decoding cursor keeps every type/reference error attached to its source.
struct Field {
  const Json& value;
  const std::filesystem::path& source;
  std::string path;
  [[noreturn]] void fail(std::string reason) const {
    throw QuestionContentError(source, path, std::move(reason));
  }
  void object() const { if(!value.is_object()) fail("expected an object"); }
  Field member(std::string_view name) const {
    object();
    const auto child = path + "/" + pointerToken(name);
    const auto found = value.find(name);
    if(found == value.end()) throw QuestionContentError(source, child, "missing required field");
    return {*found, source, child};
  }
  std::size_t array(std::size_t capacity) const {
    if(!value.is_array()) fail("expected an array");
    if(value.size() > capacity) fail("exceeds capacity of " + std::to_string(capacity));
    return value.size();
  }
  Field element(std::size_t index) const {
    return {value.at(index), source, path + "/" + std::to_string(index)};
  }
  std::string string() const {
    if(!value.is_string()) fail("expected a string");
    return value.get<std::string>();
  }
  std::uint32_t integer() const {
    if(!value.is_number_unsigned() || value.get<std::uint64_t>() > std::numeric_limits<std::uint32_t>::max())
      fail("expected an unsigned 32-bit integer");
    return value.get<std::uint32_t>();
  }
  int graphInteger() const {
    if(!value.is_number_integer() || value.get<double>() < -20 || value.get<double>() > 20)
      fail("expected an integer from -20 through 20");
    return value.get<int>();
  }
};

std::string readText(const std::filesystem::path& source) {
  std::ifstream input(source, std::ios::binary);
  if(!input) throw QuestionContentError(source, "", "cannot open file");
  std::string text(maxFileBytes + 1, '\0');
  input.read(text.data(), static_cast<std::streamsize>(text.size()));
  if(input.bad()) throw QuestionContentError(source, "", "cannot read file");
  text.resize(static_cast<std::size_t>(input.gcount()));
  if(text.size() > maxFileBytes) throw QuestionContentError(source, "", "file exceeds 1 MiB");
  return text;
}
Json parseJson(std::string_view text, const std::filesystem::path& source) {
  if(text.size() > maxFileBytes) throw QuestionContentError(source, "", "file exceeds 1 MiB");
  try { return Json::parse(text); }
  catch(const Json::exception& e) { throw QuestionContentError(source, "", e.what()); }
}
void checkSchema(const Field& root) {
  const auto version = root.member("schema_version");
  if(version.integer() != schemaVersion) version.fail("unsupported schema version; expected 1");
}
template<class T, std::size_t N>
T enumValue(const Field& field, const std::array<std::pair<std::string_view, T>, N>& choices) {
  const auto text = field.string();
  const auto found = std::find_if(choices.begin(), choices.end(), [&](const auto& choice) {return choice.first == text;});
  if(found == choices.end()) field.fail("unknown value: " + text);
  return found->second;
}
constexpr std::array purposes{
  std::pair{std::string_view("answer_choice"), fm::StepPurpose::AnswerChoice},
  std::pair{std::string_view("operation_choice"), fm::StepPurpose::OperationChoice},
  std::pair{std::string_view("calculation"), fm::StepPurpose::Calculation},
  std::pair{std::string_view("verification"), fm::StepPurpose::Verification},
  std::pair{std::string_view("graph_choice"), fm::StepPurpose::GraphChoice},
};
constexpr std::array completions{
  std::pair{std::string_view("any_accepted"), fm::CompletionRule::AnyAccepted},
  std::pair{std::string_view("all_accepted"), fm::CompletionRule::AllAccepted},
};

std::string validationField(const fm::QuestionValidationResult& result) {
  std::string path;
  if(result.workingStateIndex) path = "/working_states/" + std::to_string(*result.workingStateIndex);
  if(result.stepIndex) path = "/steps/" + std::to_string(*result.stepIndex);
  if(result.optionIndex) path += "/options/" + std::to_string(*result.optionIndex);
  constexpr std::array names{
    std::pair{std::string_view("version"), "content_version"},
    std::pair{std::string_view("workingStates"), "working_states"},
    std::pair{std::string_view("acceptedOptions"), "accepted_option_ids"},
    std::pair{std::string_view("semantics.purpose"), "semantics/purpose"},
    std::pair{std::string_view("semantics.completion"), "semantics/completion"},
    std::pair{std::string_view("semantics.before"), "semantics/before"},
    std::pair{std::string_view("semantics.after"), "semantics/after"},
  };
  const auto found = std::find_if(names.begin(), names.end(), [&](const auto& name) {return name.first == result.field;});
  return path + "/" + std::string(found == names.end() ? result.field : found->second);
}

std::uint8_t resolveAcceptedOptions(std::span<const fm::LayeredQuestionOptionContent> options,
                                    const Field& acceptedOptionIds) {
  const auto count = acceptedOptionIds.array(fm::kQuestionChoiceCapacity);
  std::uint8_t mask = 0;
  for(std::size_t i = 0; i < count; ++i) {
    const auto field = acceptedOptionIds.element(i);
    const fm::OptionId id{field.integer()};
    const auto found = std::find_if(options.begin(), options.end(), [&](const auto& option) {return option.id == id;});
    if(found == options.end()) field.fail("unknown accepted option ID: " + std::to_string(id.value));
    const auto bit = static_cast<std::uint8_t>(1U << (found - options.begin()));
    if(mask & bit) field.fail("duplicate accepted option ID: " + std::to_string(id.value));
    mask |= bit;
  }
  return mask;
}

std::vector<std::size_t> resolveDeck(std::span<const fm::LayeredQuestionContent> catalog,
                                    const Field& questionReferences) {
  const auto count = questionReferences.array(fm::kQuestionCatalogCapacity);
  if(!count) questionReferences.fail("deck must contain at least one question reference");
  std::vector<std::size_t> deck;
  for(std::size_t i = 0; i < count; ++i) {
    const auto ref = questionReferences.element(i);
    const auto id = ref.member("question_id").string();
    const auto version = ref.member("content_version").integer();
    const auto found = std::find_if(catalog.begin(), catalog.end(), [&](const auto& q) {return q.id == id && q.version == version;});
    if(found == catalog.end()) ref.fail("unknown question reference: " + id + " version " + std::to_string(version));
    deck.push_back(static_cast<std::size_t>(found - catalog.begin()));
  }
  return deck;
}
}  // namespace

QuestionContentError::QuestionContentError(std::filesystem::path source, std::string field, std::string reason)
    : std::runtime_error(source.string() + ":" + (field.empty() ? "<root>" : field) + ": " + reason),
      source(std::move(source)), field(std::move(field)) {}

const std::vector<std::size_t>& QuestionPack::deck(std::string_view mode) const {
  const auto found = decks.find(mode);
  if(found == decks.end()) throw QuestionContentError(source, "/decks/" + pointerToken(mode), "missing deck for startup mode");
  return found->second;
}

fm::LayeredQuestionContent parseQuestionContent(std::string_view json, const std::filesystem::path& sourcePath) {
  const auto document = parseJson(json, sourcePath);
  const Field root{document, sourcePath, ""};
  checkSchema(root);
  fm::LayeredQuestionContent question;
  question.id = root.member("id").string();
  question.version = root.member("content_version").integer();
  question.equation = root.member("equation").string();
  question.skill = root.member("skill").string();
  question.description = root.member("description").string();
  if(document.contains("working_model")) {
    const auto model=root.member("working_model");
    question.mathModel=enumValue(model,std::array{
        std::pair{std::string_view("linear_moves"),fm::MathWorkingModel::LinearEquation},
        std::pair{std::string_view("matrix_rows"),fm::MathWorkingModel::RowReduction}});
    question.supportsMathMoves=true;
  }
  if(document.contains("line_graph")) {
    const auto graph=root.member("line_graph");
    question.lineGraph=fm::LineGraph{graph.member("rise").graphInteger(),graph.member("run").graphInteger(),
        graph.member("intercept").graphInteger(),graph.member("x_min").graphInteger(),graph.member("x_max").graphInteger(),
        graph.member("y_min").graphInteger(),graph.member("y_max").graphInteger()};
    if(graph.value.contains("second")) {
      const auto second=graph.member("second");
      question.lineGraph->second=fm::GraphLine{second.member("rise").graphInteger(),second.member("run").graphInteger(),second.member("intercept").graphInteger()};
    }
  }
  const auto states = root.member("working_states");
  const auto stateCount = states.array(fm::kQuestionWorkingStateCapacity);
  for(std::size_t i = 0; i < stateCount; ++i) {
    const auto state = states.element(i);
    question.workingStates.push_back({{state.member("id").integer()}, state.member("display").string()});
    if(state.value.contains("graph_stage")) {
      constexpr std::array stages{std::pair{std::string_view{"grid"},fm::GraphStage::Grid},
          std::pair{std::string_view{"intercept"},fm::GraphStage::Intercept},
          std::pair{std::string_view{"run"},fm::GraphStage::Run},
          std::pair{std::string_view{"rise"},fm::GraphStage::Rise},
          std::pair{std::string_view{"line"},fm::GraphStage::Line},
          std::pair{std::string_view{"first_line"},fm::GraphStage::FirstLine},
          std::pair{std::string_view{"both_lines"},fm::GraphStage::BothLines},
          std::pair{std::string_view{"classified"},fm::GraphStage::Classified},
          std::pair{std::string_view{"system_solution"},fm::GraphStage::SystemSolution}};
      question.workingStates.back().graphStage=enumValue(state.member("graph_stage"),stages);
    }
    if(state.value.contains("highlights")) {
      const auto spans=state.member("highlights");
      const auto count=spans.array(fm::kWorkingHighlightCapacity);
      for(std::size_t j=0;j<count;++j) {
        const auto span=spans.element(j);
        question.workingStates.back().highlights.push_back({span.member("offset").integer(),
            span.member("length").integer(),span.member("label").string()});
      }
    }
  }
  const auto steps = root.member("steps");
  const auto stepCount = steps.array(fm::kQuestionStepCapacity);
  for(std::size_t i = 0; i < stepCount; ++i) {
    const auto source = steps.element(i);
    fm::LayeredQuestionStepContent step;
    step.id = {source.member("id").integer()};
    step.layerName = source.member("layer_name").string();
    step.prompt = source.member("prompt").string();
    step.wrongHint = source.member("wrong_hint").string();
    step.explanation = source.member("explanation").string();
    if (source.value.contains("hint")) step.hint = source.member("hint").string();
    if (source.value.contains("next_move")) step.nextMove = source.member("next_move").string();
    const auto options = source.member("options");
    const auto optionCount = options.array(fm::kQuestionChoiceCapacity);
    for(std::size_t j = 0; j < optionCount; ++j) {
      const auto option = options.element(j);
      step.options.push_back({option.member("label").string(), {option.member("id").integer()}});
    }
    step.acceptedOptions = resolveAcceptedOptions(step.options, source.member("accepted_option_ids"));
    const auto semantics = source.member("semantics");
    step.semantics = {enumValue(semantics.member("purpose"), purposes),
      enumValue(semantics.member("completion"), completions),
      {semantics.member("before").integer()}, {semantics.member("after").integer()}};
    question.steps.push_back(std::move(step));
  }
  const auto result = fm::validateQuestion(question, fm::QuestionInteraction::ArcadeCollect);
  if(!result.valid()) throw QuestionContentError(sourcePath, validationField(result), std::string(result.reason()));
  return question;
}

QuestionPack loadQuestionPack(const std::filesystem::path& path) {
  std::error_code error;
  const auto sourcePath = std::filesystem::absolute(path, error).lexically_normal();
  if(error) throw QuestionContentError(path, "", "cannot resolve pack path: " + error.message());
  const auto document = parseJson(readText(sourcePath), sourcePath);
  const Field root{document, sourcePath, ""};
  checkSchema(root);
  QuestionPack pack;
  pack.source = sourcePath;
  std::vector<std::filesystem::path> sources;
  const auto questions = root.member("questions");
  const auto count = questions.array(fm::kQuestionCatalogCapacity);
  for(std::size_t i = 0; i < count; ++i) {
    const auto field = questions.element(i);
    const std::filesystem::path relative = field.string();
    if(relative.empty() || relative.is_absolute()) field.fail("expected a nonempty pack-relative card path");
    const auto cardPath = (sourcePath.parent_path() / relative).lexically_normal();
    sources.push_back(cardPath);
    pack.catalog.push_back(parseQuestionContent(readText(cardPath), cardPath));
  }
  const auto result = fm::validateCatalog(pack.catalog, fm::QuestionInteraction::ArcadeCollect);
  if(!result.valid()) {
    if(result.questionIndex) throw QuestionContentError(sources.at(*result.questionIndex), validationField(result), std::string(result.reason()));
    questions.fail(std::string(result.reason()));
  }
  const auto decks = root.member("decks");
  decks.object();
  for(const auto& [mode, references] : decks.value.items()) {
    const auto field = decks.member(mode);
    if(mode.empty()) field.fail("deck name must not be empty");
    pack.decks.emplace(mode, resolveDeck(pack.catalog, field));
  }
  return pack;
}
}  // namespace paths
