#include "content/QuestionContentIO.hpp"
#include "runtime/gallery/GallerySession.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
using namespace paths;
namespace fm = iggy3d::first_move;
namespace fs = std::filesystem;
using Json = nlohmann::json;
int failures = 0;
void expect(bool condition, std::string_view message) {
  if(!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
Json read(const fs::path& path) {
  std::ifstream input(path);
  return Json::parse(input);
}
void write(const fs::path& path, const Json& value) {
  std::ofstream output(path);
  output << value.dump(2) << '\n';
  if(!output) throw std::runtime_error("test file write failed");
}
const fs::path starterPack = PATHS_TEST_PACK;
Json card(std::string_view id) {
  return read(starterPack.parent_path().parent_path() / "cards" / (std::string(id) + ".json"));
}
template<class F>
void expectError(F action, const fs::path& source, std::string_view field, std::string_view reason) {
  try { action(); expect(false, "invalid file must throw QuestionContentError"); }
  catch(const QuestionContentError& error) {
    expect(error.source == source && error.field == field, "diagnostic preserves source and exact JSON field");
    const std::string message = error.what();
    if(error.source != source || error.field != field || message.find(reason) == std::string::npos)
      std::cerr << "Expected " << source << ':' << field << " (" << reason << "), got " << message << '\n';
    expect(message.find(source.string()) != std::string::npos && message.find(reason) != std::string::npos,
      "rendered diagnostic contains the source and useful reason");
  }
}
struct Scratch {
  fs::path root = fs::temp_directory_path() / ("paths-content-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Scratch() { if(!fs::create_directory(root)) throw std::runtime_error("test directory already exists"); }
  ~Scratch() { std::error_code error; fs::remove_all(root, error); }
  fs::path copyPack() {
    fs::create_directories(root / "cards"); fs::create_directories(root / "packs");
    const auto pack = read(starterPack);
    for(const auto& path : pack["questions"]) {
      const fs::path relative = path.get<std::string>();
      fs::copy_file(starterPack.parent_path() / relative, root / "cards" / relative.filename());
    }
    write(root / "packs/gallery_foundation.json", pack);
    return root / "packs/gallery_foundation.json";
  }
};

void testCardsAndSharedValidation() {
  const auto pack = loadQuestionPack(starterPack);
  expect(pack.catalog.size() == 5 && pack.decks.size() == 4, "starter pack contains exactly the five existing examples and four modes");
  for(const auto& mode : galleryVariations()) expect(!pack.deck(mode.id).empty(), "every existing startup mode has a file-defined deck");
  expect(fm::validateCatalog(pack.catalog, fm::QuestionInteraction::ArcadeCollect).valid(), "loaded pack passes shared validation");
  expect(pack.catalog[0].steps[0].acceptedOptions == 3 && fm::requiredAnswerCount(pack.catalog[0].steps[0]) == 2,
    "equality retains both accepted answers and AllAccepted");
  expect(pack.catalog[4].id == "foundation_substitution_integral_6x" && pack.catalog[4].version == 1 && pack.catalog[4].steps.size() == 6,
    "integral keeps its identity, content version and six decisions");

  const fs::path source = "prepared-card.json";
  struct Case { std::function<void(Json&)> edit; const char* field; const char* reason; };
  const Case cases[]{
    {[](Json& q){q["schema_version"] = 2;}, "/schema_version", "unsupported schema"},
    {[](Json& q){q.erase("id");}, "/id", "missing required"},
    {[](Json& q){q["id"] = "";}, "/id", "invalid_question_content"},
    {[](Json& q){q["content_version"] = 0;}, "/content_version", "invalid_question_content"},
    {[](Json& q){q["content_version"] = -1;}, "/content_version", "32-bit integer"},
    {[](Json& q){q["content_version"] = 1.0;}, "/content_version", "32-bit integer"},
    {[](Json& q){q["content_version"] = true;}, "/content_version", "32-bit integer"},
    {[](Json& q){q["content_version"] = 4294967296ULL;}, "/content_version", "32-bit integer"},
    {[](Json& q){q["steps"][0].erase("prompt");}, "/steps/0/prompt", "missing required"},
    {[](Json& q){q["steps"][0]["prompt"] = "";}, "/steps/0/prompt", "invalid_question_step"},
    {[](Json& q){q["steps"][0]["options"][0]["label"] = 12;}, "/steps/0/options/0/label", "string"},
    {[](Json& q){q["steps"][0]["options"][1]["id"] = 101;}, "/steps/0/options/1/id", "duplicate_option_identity"},
    {[](Json& q){q["steps"][0]["accepted_option_ids"] = Json::array();}, "/steps/0/accepted_option_ids", "invalid_question_step"},
    {[](Json& q){q["steps"][0]["accepted_option_ids"] = {999};}, "/steps/0/accepted_option_ids/0", "unknown accepted option ID"},
    {[](Json& q){q["steps"][0]["accepted_option_ids"] = {101,101};}, "/steps/0/accepted_option_ids/1", "duplicate accepted option ID"},
    {[](Json& q){q["steps"][0]["semantics"]["purpose"] = "solve";}, "/steps/0/semantics/purpose", "unknown value"},
    {[](Json& q){q["steps"][0]["semantics"]["completion"] = "any";}, "/steps/0/semantics/completion", "unknown value"},
    {[](Json& q){q["steps"][0]["semantics"]["after"] = 999;}, "/steps/0/semantics/after", "unknown_working_state"},
    {[](Json& q){q["working_states"][1]["id"] = 10;}, "/working_states/1/id", "duplicate_working_state_identity"},
    {[](Json& q){q["steps"] = Json::array();}, "/steps", "invalid_question_content"},
    {[](Json& q){q["steps"][0]["options"] = "choices";}, "/steps/0/options", "array"},
    {[](Json& q){while(q["steps"][0]["options"].size() < 9) q["steps"][0]["options"].push_back({{"id",900},{"label","extra"}});}, "/steps/0/options", "capacity of 8"},
    {[](Json& q){while(q["steps"].size() < 33) q["steps"].push_back(q["steps"][0]);}, "/steps", "capacity of 32"},
    {[](Json& q){while(q["working_states"].size() < 34) q["working_states"].push_back(q["working_states"][0]);}, "/working_states", "capacity of 33"},
  };
  for(const auto& test : cases) {
    auto value = card("foundation_relay_add"); test.edit(value);
    expectError([&]{static_cast<void>(parseQuestionContent(value.dump(), source));}, source, test.field, test.reason);
  }
  auto chain = card("foundation_substitution_integral_6x");
  chain["steps"][1]["semantics"]["before"] = 200;
  expectError([&]{static_cast<void>(parseQuestionContent(chain.dump(), source));}, source, "/steps/1/semantics/before", "broken_step_chain");
  expectError([&]{static_cast<void>(parseQuestionContent("{", source));}, source, "", "parse_error");
  expectError([&]{static_cast<void>(parseQuestionContent(std::string(1024*1024+1, ' '), source));}, source, "", "1 MiB");
}

void testWrongChoiceFeedback() {
  auto json=card("foundation_equality_24");
  const auto original=parseQuestionContent(json.dump(),"feedback.json");
  const auto& step=original.steps.front();
  const auto wrong=static_cast<std::size_t>(std::find_if(step.options.begin(),step.options.end(),[&](const auto& option){
    return !fm::acceptsOption(step,&option-step.options.data());})-step.options.begin());
  const std::string field="/steps/0/options/"+std::to_string(wrong)+"/wrong_feedback";
  for(const auto& value:std::vector<Json>{"", "   ",std::string(2001,'x'),42}) {
    json["steps"][0]["options"][wrong]["wrong_feedback"]=value;
    bool rejected=false;try{(void)parseQuestionContent(json.dump(),"feedback.json");}
    catch(const QuestionContentError& e){rejected=std::string(e.what()).find(field)!=std::string::npos;}
    expect(rejected,"Invalid option feedback names its exact JSON field");
  }
  json["steps"][0]["options"][wrong]["wrong_feedback"]="Recheck this specific choice.";
  auto q=parseQuestionContent(json.dump(),"feedback.json");
  fm::LayeredQuestionSession session({q},fm::QuestionInteraction::ArcadeCollect);
  (void)session.dispatch({fm::LayeredQuestionCommandKind::OpenQuestion});
  expect(session.review()->steps.front().attempts.empty(),"Wrong-choice feedback is not exposed before an attempt");
  (void)session.dispatch(fm::LayeredQuestionCommand::submitOption(q.steps[0].options[wrong].id));
  expect(session.review()->steps.front().attempts.back().feedback=="Recheck this specific choice.","Review owns the selected correction");
  std::reverse(json["steps"][0]["options"].begin(),json["steps"][0]["options"].end());
  const auto reordered=parseQuestionContent(json.dump(),"feedback.json");
  const auto option=std::find_if(reordered.steps[0].options.begin(),reordered.steps[0].options.end(),[&](const auto& o){return o.id==q.steps[0].options[wrong].id;});
  expect(option->wrongFeedback=="Recheck this specific choice.","Feedback follows stable option identity through authoring reorder");
  json=card("foundation_equality_24");json["steps"][0]["options"][fm::firstAcceptedOption(step)]["wrong_feedback"]="Incorrect.";
  bool rejected=false;try{(void)parseQuestionContent(json.dump(),"feedback.json");}catch(const QuestionContentError&){rejected=true;}
  expect(rejected,"A correct option cannot carry wrong-choice feedback");
  fm::LayeredQuestionSession legacy({original},fm::QuestionInteraction::ArcadeCollect);
  (void)legacy.dispatch({fm::LayeredQuestionCommandKind::OpenQuestion});
  (void)legacy.dispatch(fm::LayeredQuestionCommand::submitOption(step.options[wrong].id));
  expect(legacy.review()->steps[0].attempts.back().feedback==step.wrongHint,"Existing prepared cards retain their step correction");
}

void testOptionIdentityAndInteraction() {
  auto json = card("foundation_equality_24");
  std::reverse(json["steps"][0]["options"].begin(), json["steps"][0]["options"].end());
  auto q = parseQuestionContent(json.dump(), "reordered.json");
  expect(q.steps[0].acceptedOptions == 12, "accepted IDs resolve after the reordered options are established");
  for(std::size_t i = 0; i < q.steps[0].options.size(); ++i) {
    const auto id = q.steps[0].options[i].id.value;
    expect(fm::acceptsOption(q.steps[0], i) == (id == 101 || id == 108), "correctness follows identity, not choice position");
  }
  expect(!fm::validateQuestion(q, fm::QuestionInteraction::Guided).valid(), "Guided still rejects collecting multiple answers");
  json["steps"][0]["semantics"]["completion"] = "any_accepted";
  q = parseQuestionContent(json.dump(), "alternatives.json");
  expect(fm::validateQuestion(q, fm::QuestionInteraction::Guided).valid(), "multiple acceptable alternatives remain compatible with Guided");
  fm::LayeredQuestionSession session({q}, fm::QuestionInteraction::ArcadeCollect);
  static_cast<void>(session.dispatch({fm::LayeredQuestionCommandKind::OpenQuestion}));
  static_cast<void>(session.dispatch(fm::LayeredQuestionCommand::submitOption({122})));
  expect(!session.currentRun().steps[0].resolvedByPlayer, "reordered wrong option remains wrong");
  static_cast<void>(session.dispatch(fm::LayeredQuestionCommand::submitOption({108})));
  expect(session.currentRun().steps[0].resolvedByPlayer, "existing session judges an accepted alternative by ID");

  json = card("foundation_relay_add");
  for(std::uint32_t id = 200; id < 204; ++id)
    json["steps"][0]["options"].push_back({{"id",id},{"label","Extra choice"}});
  json["steps"][0]["accepted_option_ids"] = {203};
  q = parseQuestionContent(json.dump(), "eight-options.json");
  expect(q.steps[0].acceptedOptions == 128 && fm::firstAcceptedOption(q.steps[0]) == 7,
    "the eighth option resolves to the high bit without narrowing loss");
  json["steps"][0]["accepted_option_ids"] = {101,108,115,122,200,201,202,203};
  json["steps"][0]["semantics"]["completion"] = "all_accepted";
  q = parseQuestionContent(json.dump(), "eight-options.json");
  expect(q.steps[0].acceptedOptions == 255 && fm::requiredAnswerCount(q.steps[0]) == 8,
    "all eight accepted IDs retain the runtime mask's full capacity");
}

void testPackFailures() {
  Scratch temp;
  const auto path = temp.copyPack();
  const auto original = read(path);
  const auto fails = [&](Json value, std::string_view field, std::string_view reason) {
    write(path, value);
    expectError([&]{static_cast<void>(loadQuestionPack(path));}, path, field, reason);
  };
  auto json = original; json["schema_version"] = 0; fails(json, "/schema_version", "unsupported schema");
  json = original; json.erase("decks"); fails(json, "/decks", "missing required");
  json = original; json["questions"] = Json::array(); fails(json, "/questions", "invalid_question_catalog");
  json = original; while(json["questions"].size() < 65) json["questions"].push_back(json["questions"][0]);
  fails(json, "/questions", "capacity of 64");
  json = original; json["decks"]["question_relay"] = Json::array(); fails(json, "/decks/question_relay", "at least one");
  json = original; json["decks"]["question_relay"][1]["content_version"] = 2;
  fails(json, "/decks/question_relay/1", "unknown question reference");
  json = original; json["questions"][0] = "/absolute.json"; fails(json, "/questions/0", "pack-relative");
  json = original; json["questions"][0] = "../cards/missing.json"; write(path, json);
  expectError([&]{static_cast<void>(loadQuestionPack(path));}, temp.root / "cards/missing.json", "", "cannot open");
  expectError([&]{static_cast<void>(loadQuestionPack(temp.root / "missing-pack.json"));}, temp.root / "missing-pack.json", "", "cannot open");
  json = original; json["questions"].push_back("../cards/duplicate.json");
  write(temp.root / "cards/duplicate.json", card("foundation_relay_add")); write(path, json);
  expectError([&]{static_cast<void>(loadQuestionPack(path));}, temp.root / "cards/duplicate.json", "/id", "duplicate_question_identity");
  write(path, original);
  const auto pack = loadQuestionPack(path);
  expectError([&]{static_cast<void>(pack.deck("missing/mode"));}, path, "/decks/missing~1mode", "missing deck");
}

void testSharedRowReferences() {
  Scratch temp;fs::create_directories(temp.root/"cards");fs::create_directories(temp.root/"packs");fs::create_directories(temp.root/"references");
  const auto sourceRoot=starterPack.parent_path().parent_path();
  const auto libraryPath=temp.root/"references/rows.json",cardPath=temp.root/"cards/matrix.json",packPath=temp.root/"packs/matrix.json";
  const auto library=read(sourceRoot/"references/row_operations.json");auto question=card("sorter_matrix_rows");
  write(libraryPath,library);write(cardPath,question);
  Json pack={{"schema_version",1},{"questions",{"../cards/matrix.json","../cards/second.json"}},
      {"reference_library","../references/rows.json"},{"decks",{{"solve",Json::array({{{"question_id",question["id"]},{"content_version",question["content_version"]}}})}}}};
  auto second=question;second["id"]="another_matrix";write(temp.root/"cards/second.json",second);write(packPath,pack);
  const auto loaded=loadQuestionPack(packPath);fm::LayeredQuestionSession frozen(loaded.catalog,fm::QuestionInteraction::MathMoves);
  expect(loaded.catalog.size()==2 && loaded.catalog[0].references.size()==3 && loaded.catalog[1].references.size()==3,"two questions resolve the same three definitions from one relative library");
  const auto original=frozen.mathReference("row_swap")->definition;
  auto edited=library;edited["references"][0]["definition"]="Updated shared definition.";edited["references"][0]["content_version"]=2;write(libraryPath,edited);
  const auto updated=loadQuestionPack(packPath);
  expect(updated.catalog[0].references[0].definition=="Updated shared definition." && updated.catalog[1].references[0].version==2,"one shared edit reaches both questions on the next load");
  expect(frozen.mathReference("row_swap")->definition==original && frozen.mathReference("row_swap")->version==1,"an active run retains its original definition and revision");
  const auto invalidLibrary=[&](Json value,std::string_view field,std::string_view reason) {
    write(libraryPath,value);expectError([&]{(void)loadQuestionPack(packPath);},libraryPath,field,reason);
  };
  edited=library;edited["references"][1]=edited["references"][0];invalidLibrary(edited,"/references/1/id","duplicate concept ID");
  edited=library;edited["references"][0]["id"]="row_addition";invalidLibrary(edited,"/references/0/id","does not match");
  edited=library;edited["references"][1]["example"]["operand"]="0";invalidLibrary(edited,"/references/1","invalid_math_reference");
  edited=library;edited["references"][1]["example"]["operation"]="other";invalidLibrary(edited,"/references/1/example/operation","unknown row operation");
  edited=library;edited["references"][0]["definition"]=false;invalidLibrary(edited,"/references/0/definition","expected a string");
  edited=library;edited["references"][0]["rule"]=std::string(161,'a');invalidLibrary(edited,"/references/0","invalid_math_reference");
  edited=library;while(edited["references"].size()<17)edited["references"].push_back(edited["references"][0]);invalidLibrary(edited,"/references","capacity of 16");
  write(libraryPath,library);
  auto bad=question;bad["concept_ids"][0]="missing";write(cardPath,bad);
  expectError([&]{(void)loadQuestionPack(packPath);},cardPath,"/concept_ids/0","unknown concept ID");
  bad=question;bad["concept_ids"].push_back("row_swap");write(cardPath,bad);
  expectError([&]{(void)loadQuestionPack(packPath);},cardPath,"/concept_ids/3","duplicate concept ID");write(cardPath,question);
  auto badPack=pack;badPack["reference_library"]="/absolute.json";write(packPath,badPack);
  expectError([&]{(void)loadQuestionPack(packPath);},packPath,"/reference_library","pack-relative");
  badPack=pack;badPack["reference_library"]="../references/missing.json";write(packPath,badPack);
  expectError([&]{(void)loadQuestionPack(packPath);},temp.root/"references/missing.json","","cannot open");
  badPack=pack;badPack.erase("reference_library");write(packPath,badPack);
  expectError([&]{(void)loadQuestionPack(packPath);},cardPath,"/concept_ids/0","unknown concept ID");
  expectError([&]{(void)parseQuestionContent(question.dump(),cardPath);},cardPath,"/concept_ids/0","unknown concept ID");
  expect(parseQuestionContent(question.dump(),cardPath,loaded.catalog[0].references).references.size()==3,"in-memory import uses the same explicit library resolution");
}

void testMathNotation() {
  Scratch temp;const auto sourceRoot=starterPack.parent_path().parent_path();
  const auto libraryPath=temp.root/"notation.json",cardPath=temp.root/"question.json",packPath=temp.root/"pack.json";
  auto question=card("sorter_matrix_rows");question.erase("concept_ids");write(cardPath,question);
  const auto library=read(sourceRoot/"references/math_notation.json");write(libraryPath,library);
  Json pack={{"schema_version",1},{"questions",{"question.json"}},{"notation_library","notation.json"},
      {"notation_ids",{"row_add_syntax","augmented_matrix","equation_equality"}},
      {"decks",{{"solve",Json::array({{{"question_id",question["id"]},{"content_version",question["content_version"]}}})}}}};
  write(packPath,pack);const auto loaded=loadQuestionPack(packPath);
  const auto& lessons=loaded.catalog[0].notation;
  expect(lessons.size()==3 && lessons[0].id=="row_add_syntax","pack selects and orders notation without changing question content");
  const auto& row=lessons[0];
  expect(row.tokens[0].text==row.tokens[2].text && row.tokens[0].role!=row.tokens[2].role &&
      row.tokens[0].definition.id==row.tokens[2].definition.id,"repeated row glyphs share a definition and retain occurrence-specific roles");
  expect(fm::checkNotation(row,0)==fm::NotationVerdict::Correct && fm::checkNotation(row,2)==fm::NotationVerdict::Retry &&
      fm::checkNotation(row,99)==fm::NotationVerdict::Unavailable,"reading check distinguishes destination from source syntax and refuses missing tokens");
  auto general=row;general.check.reset();
  expect(fm::validNotationLesson(general) && fm::checkNotation(general,0)==fm::NotationVerdict::Unavailable,"a definition lesson can omit practice");
  general=row;general.context="Absolute value";general.tokens[1].text="|";
  general.tokens[1].definition={"absolute_value","Absolute value","Distance from zero.","For real x, |x| is x if x is nonnegative and -x otherwise.","|-3| = 3.",1};
  expect(fm::validNotationLesson(general) && lessons[1].tokens[1].text=="|" &&
      general.tokens[1].definition.id!=lessons[1].tokens[1].definition.id,"the same bar glyph can have independently authored mathematical meanings");
  auto edited=library;edited["terms"][8]["meaning"]="Updated row meaning.";edited["terms"][8]["content_version"]=2;write(libraryPath,edited);
  const auto changed=loadQuestionPack(packPath);
  expect(changed.catalog[0].notation[0].tokens[0].definition.meaning=="Updated row meaning." &&
      row.tokens[0].definition.meaning!="Updated row meaning.","definitions resolve from one source and remain frozen after loading");
  const auto invalid=[&](Json value,std::string_view field,std::string_view reason) {
    write(libraryPath,value);expectError([&]{(void)loadQuestionPack(packPath);},libraryPath,field,reason);
  };
  edited=library;edited["terms"][1]["id"]=edited["terms"][0]["id"];invalid(edited,"/terms/1/id","duplicate");
  edited=library;edited["lessons"][1]["id"]=edited["lessons"][0]["id"];invalid(edited,"/lessons/1/id","duplicate");
  edited=library;edited["lessons"][0]["tokens"][0]["term_id"]="missing";invalid(edited,"/lessons/0/tokens/0/term_id","unknown");
  edited=library;edited["lessons"][0]["check"]["answer_token"]=99;invalid(edited,"/lessons/0/check/answer_token","existing token");
  edited=library;edited["lessons"][0]["tokens"]=Json::array();edited["lessons"][0].erase("check");invalid(edited,"/lessons/0","invalid notation lesson");
  edited=library;edited["terms"][0]["meaning"]=std::string("hidden\0text",11);invalid(edited,"/terms/0","invalid notation definition");
  edited=library;edited["terms"][0]["definition"]=std::string(481,'a');invalid(edited,"/terms/0","text limits");
  edited=library;edited["lessons"][0]["tokens"][0]["text"]="##hidden";invalid(edited,"/lessons/0","text limits");
  edited=library;while(edited["lessons"][0]["tokens"].size()<17)edited["lessons"][0]["tokens"].push_back(edited["lessons"][0]["tokens"][0]);
  invalid(edited,"/lessons/0/tokens","capacity of 16");write(libraryPath,library);
  auto bad=pack;bad["notation_ids"].push_back("row_add_syntax");write(packPath,bad);
  expectError([&]{(void)loadQuestionPack(packPath);},packPath,"/notation_ids/3","duplicate");
  bad=pack;bad["notation_ids"][0]="unknown";write(packPath,bad);
  expectError([&]{(void)loadQuestionPack(packPath);},packPath,"/notation_ids/0","unknown");
  bad=pack;bad["notation_library"]="/absolute.json";write(packPath,bad);
  expectError([&]{(void)loadQuestionPack(packPath);},packPath,"/notation_library","pack-relative");
  bad=pack;bad["notation_library"]="missing.json";write(packPath,bad);
  expectError([&]{(void)loadQuestionPack(packPath);},temp.root/"missing.json","","cannot open");
  write(packPath,pack);question["notation_ids"]={"equation_equality"};write(cardPath,question);
  expect(loadQuestionPack(packPath).catalog[0].notation.size()==1,"explicit card notation replaces pack defaults");
  expect(parseQuestionContent(question.dump(),cardPath,{},lessons).notation[0].id=="equation_equality","parser import uses the same explicit bindings");
  auto malformed=loaded.catalog[0];malformed.notation[0].check->answer=99;
  expect(!fm::validateQuestion(malformed,fm::QuestionInteraction::MathMoves).valid(),"direct question construction validates notation too");
}

void completeCurrentQuestion(GallerySession& game) {
  const auto apply = [&](const GalleryCommand& command) { expect(game.dispatch(command).accepted, "loaded gallery action accepted"); };
  apply(GalleryViewport{{0,78,1030,822}}); apply(GalleryTick{0.15F});
  static_cast<void>(game.publishFrame());
  const auto& step = game.question().content().steps[0];
  const auto id = step.options[fm::firstAcceptedOption(step)].id;
  const auto& challenge = game.challenges().back();
  const auto bindings = std::span(challenge.bindings.data(), challenge.count);
  const auto binding = std::find_if(bindings.begin(), bindings.end(), [&](const auto& b){return b.option == id;});
  const auto bodies = game.scene().objects();
  const auto body = std::find_if(bodies.begin(), bodies.end(), [&](const auto& b){return b.id == binding->object;});
  const auto point = game.scene().project(body->position);
  apply(Shoot{game.scene().frame().id, game.view().challenge, point.x, point.y});
  expect(game.view().correctHits == 1 && game.view().transitioning, "new file's accepted target is playable through real gallery picking");
  apply(GalleryTick{0.25F}); apply(GalleryTick{0.05F});
}

void testEditablePackAndFrozenRun() {
  Scratch temp;
  const auto path = temp.copyPack();
  auto json = read(path);
  std::reverse(json["questions"].begin(), json["questions"].end()); write(path, json);
  auto pack = loadQuestionPack(path);
  expect(pack.deck("question_relay") == std::vector<std::size_t>({3,2}), "deck order resolves by ID/version after catalog reordering");
  GalleryConfig config; config.variation = GalleryVariation::QuestionRelay; config.motion = RouteKind::Stationary;
  GallerySession frozen(config, pack.catalog, pack.deck("question_relay"));

  auto added = card("foundation_relay_add"); added["id"] = "added_from_file";
  write(temp.root / "cards/added.json", added);
  json["questions"].push_back("../cards/added.json");
  auto nextVersion = added; nextVersion["content_version"] = 2; nextVersion["description"] = "Second prepared version";
  write(temp.root / "cards/added-v2.json", nextVersion);
  json["questions"].push_back("../cards/added-v2.json");
  json["decks"]["question_relay"] = Json::array({{{"question_id","added_from_file"},{"content_version",2}}});
  write(path, json);
  pack.catalog.clear(); pack.decks.clear();
  expect(frozen.question().content().id == "foundation_relay_add", "loaded catalog is frozen independently of source files and caller storage");
  completeCurrentQuestion(frozen);
  expect(frozen.question().content().id == "foundation_relay_notation", "active run retains its original deck after the pack is edited");

  pack = loadQuestionPack(path);
  GallerySession fresh(config, pack.catalog, pack.deck("question_relay"));
  expect(fresh.question().content().id == "added_from_file" && fresh.question().content().version == 2,
    "same executable loads a newly added file and selects its exact version from the edited deck");
  completeCurrentQuestion(fresh);
  expect(fresh.view().completedQuestions == 1 && fresh.question().content().id == "added_from_file", "new card completes and repeats in endless mode");
  for(const auto& deck : {std::vector<std::size_t>{}, std::vector<std::size_t>{0,999}, std::vector<std::size_t>(65,0)}) {
    try { GallerySession invalid(config, pack.catalog, deck); expect(false, "invalid injected deck must be rejected"); }
    catch(const std::invalid_argument&) {}
  }
}

struct SourceStepMapping { const char* source; std::uint32_t step,before,after; fm::StepPurpose purpose; };
void checkSourceAdaptation(const fs::path& sourceFile,const fs::path& packFile,
                           std::span<const SourceStepMapping> mappings) {
  const auto authored=read(starterPack.parent_path().parent_path()/"authoring"/sourceFile);
  const auto pack=loadQuestionPack(starterPack.parent_path()/packFile);
  expect(pack.catalog.size()==1 && pack.deck("equation_chain")==std::vector<std::size_t>{0},
    "the separate source-card pack selects its only prepared question");
  const auto& question=pack.catalog.at(0);
  expect(question.id==authored["question_id"].get<std::string>() && question.version==authored["content_version"].get<std::uint32_t>() &&
    question.description==authored["description"].get<std::string>(), "adaptation preserves the authored question identity, revision and description");
  expect(fm::validateQuestion(question,fm::QuestionInteraction::Guided).valid(),
    "the prepared single-decision steps remain compatible with Guided as well as ArcadeCollect");
  constexpr std::array optionIds{std::pair{"o1",101U},std::pair{"o2",108U},std::pair{"o3",115U},std::pair{"o4",122U}};
  expect(question.steps.size()==mappings.size() && authored["steps"].size()==mappings.size() &&
    question.workingStates.size()==mappings.size()+1,
    "every source decision and its before-workspace survives the adaptation, with one final solution state");
  for(std::size_t i=0;i<mappings.size();++i) {
    const auto mapping=mappings[i];
    const auto& step=question.steps.at(i);
    const auto source=std::find_if(authored["steps"].begin(),authored["steps"].end(),
      [&](const auto& s){return s["id"]==mapping.source;});
    if(source==authored["steps"].end())throw std::runtime_error("missing authored decision");
    expect(step.id.value==mapping.step && step.layerName==(*source)["layer"].get<std::string>() && step.prompt==(*source)["prompt"].get<std::string>() &&
      step.explanation==(*source)["explanation"].get<std::string>() && step.wrongHint==(*source)["recovery_text"].get<std::string>(),
      "authored step identities, prompts, labels and recovery explanations are preserved");
    expect(step.semantics.purpose==mapping.purpose && step.semantics.completion==fm::CompletionRule::AnyAccepted &&
      step.semantics.before.value==mapping.before && step.semantics.after.value==mapping.after,
      "the explicit identity map gives each decision one completion rule and prepared transition");
    const auto state=std::find_if(question.workingStates.begin(),question.workingStates.end(),
      [&](const auto& s){return s.id==step.semantics.before;});
    std::string working;
    for(const auto& line:(*source)["workspace"]) {if(!working.empty())working+='\n';working+=line.get<std::string>();}
    expect(state!=question.workingStates.end() && state->display==working,
      "before-working copies the reviewed authoring block without appending the answer or next calculation");
    expect(step.options.size()==4 && fm::requiredAnswerCount(step)==1, "source-card choices remain four single-answer alternatives");
    for(const auto& [oldId,newId]:optionIds) {
      const auto oldOption=std::find_if((*source)["options"].begin(),(*source)["options"].end(),
        [&](const auto& o){return o["id"]==oldId;});
      const auto option=std::find_if(step.options.begin(),step.options.end(),[&](const auto& o){return o.id.value==newId;});
      if(oldOption==(*source)["options"].end() || option==step.options.end())throw std::runtime_error("missing mapped option");
      const auto index=static_cast<std::size_t>(option-step.options.begin());
      expect(option->label==(*oldOption)["text"].get<std::string>() && fm::acceptsOption(step,index)==((*source)["correct_option_id"]==oldId),
        "stable option identities retain every authored choice and the exact accepted answer");
    }
  }
  std::string solution;
  for(const auto& line:authored["solution"]) {if(!solution.empty())solution+='\n';solution+=line["work"].get<std::string>();}
  fm::LayeredQuestionSession session(pack.catalog,fm::QuestionInteraction::ArcadeCollect);
  expect(session.dispatch({fm::LayeredQuestionCommandKind::OpenQuestion}).accepted,"prepared source question opens");
  for(const auto& step:question.steps) {
    expect(session.dispatch(fm::LayeredQuestionCommand::submitOption(step.options[fm::firstAcceptedOption(step)].id)).accepted,
      "source answer is judged by the existing question owner");
    expect(session.dispatch({fm::LayeredQuestionCommandKind::Continue}).accepted,"source decision advances normally");
  }
  expect(session.currentRun().completed && session.visibleWorking()==solution && session.visibleWorkingId().value==mappings.back().after,
    "the completed question retains the authored solution before the gallery chooses its endless restart");
}
void testSource002Adaptation() {
  using Purpose=fm::StepPurpose;
  constexpr std::array mappings{
    SourceStepMapping{"identify_unknowns",10,100,200,Purpose::AnswerChoice},
    SourceStepMapping{"identify_input",20,200,300,Purpose::AnswerChoice},
    SourceStepMapping{"linear_in_coefficients",30,300,400,Purpose::AnswerChoice},
    SourceStepMapping{"implicit_coefficient",40,400,500,Purpose::AnswerChoice},
    SourceStepMapping{"substitute_zero",50,500,600,Purpose::Calculation},
    SourceStepMapping{"substitute_minus_one",60,600,700,Purpose::Calculation},
    SourceStepMapping{"substitute_one",70,700,800,Purpose::Calculation},
    SourceStepMapping{"license_elimination",80,800,900,Purpose::Verification},
    SourceStepMapping{"solve_a",90,900,1000,Purpose::Calculation},
    SourceStepMapping{"solve_b",100,1000,1100,Purpose::Calculation},
    SourceStepMapping{"state_function",110,1100,1200,Purpose::Calculation},
    SourceStepMapping{"check_extent",120,1200,1300,Purpose::Verification},
    SourceStepMapping{"uniqueness_condition",130,1300,1400,Purpose::Verification},
  };
  checkSourceAdaptation("002_guided.json","source_002.json",mappings);
}
void testSource013Adaptation() {
  using Purpose=fm::StepPurpose;
  constexpr std::array mappings{
    SourceStepMapping{"identify_deliverable",10,100,200,Purpose::AnswerChoice},
    SourceStepMapping{"identify_parameters",20,200,300,Purpose::AnswerChoice},
    SourceStepMapping{"introduce_unknown",30,300,400,Purpose::AnswerChoice},
    SourceStepMapping{"parameterize_line",40,400,500,Purpose::OperationChoice},
    SourceStepMapping{"projection_conditions",50,500,600,Purpose::Verification},
    SourceStepMapping{"nonzero_direction",60,600,700,Purpose::Verification},
    SourceStepMapping{"choose_projection",70,700,800,Purpose::OperationChoice},
    SourceStepMapping{"compute_numerator",80,800,900,Purpose::Calculation},
    SourceStepMapping{"compute_denominator",90,900,1000,Purpose::Calculation},
    SourceStepMapping{"assemble_point",100,1000,1100,Purpose::Calculation},
    SourceStepMapping{"perpendicular_residual",110,1100,1200,Purpose::Verification},
    SourceStepMapping{"distance_decomposition",120,1200,1300,Purpose::Verification},
    SourceStepMapping{"unique_minimum",130,1300,1400,Purpose::Verification},
    SourceStepMapping{"numerical_check_extent",140,1400,1500,Purpose::Verification},
  };
  checkSourceAdaptation("013_guided.json","source_013.json",mappings);
}
}  // namespace

int main() {
  try {
    testCardsAndSharedValidation(); testOptionIdentityAndInteraction(); testWrongChoiceFeedback(); testPackFailures(); testEditablePackAndFrozenRun();
    testSource002Adaptation(); testSource013Adaptation(); testSharedRowReferences(); testMathNotation();
  } catch(const std::exception& error) { ++failures; std::cerr << "Unexpected: " << error.what() << '\n'; }
  if(!failures) std::cout << "Question content loader tests passed\n";
  return failures ? 1 : 0;
}
