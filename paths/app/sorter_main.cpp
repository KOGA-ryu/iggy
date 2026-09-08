#include "content/EquationSorterContentIO.hpp"
#include "content/StudyProgressIO.hpp"
#include "platform/NativeVulkanHost.hpp"
#include "ui/EquationSorterUi.hpp"

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <imgui.h>

namespace {
using namespace paths;
std::uint32_t positive(std::string_view text) {
  std::uint32_t value = 0;
  auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || !value)
    throw std::invalid_argument("expected a positive 32-bit integer: " + std::string(text));
  return value;
}
struct Options {
  NativeLaunchConfig native;
  std::filesystem::path content, library, script, report, capture, progress;
  std::uint32_t frames = 0;
  bool check = false, noProgress=false;
};
Options options(int argc, char** argv) {
  Options o;
  o.native.enableScene=true;
  const char* base = SDL_GetBasePath();
  if (!base) throw std::runtime_error("cannot locate executable directory");
  o.content = std::filesystem::path(base) / "content/sorter/study_practice_v1.json";
  o.library = std::filesystem::path(base) / "content/corpus/toc.json";
  constexpr std::array pathOptions{
      std::pair{"--content", &Options::content}, std::pair{"--library", &Options::library}, std::pair{"--script", &Options::script},
      std::pair{"--report", &Options::report}, std::pair{"--capture", &Options::capture},
      std::pair{"--progress", &Options::progress}};
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "--offscreen") { o.native.offscreen = true; continue; }
    if (arg == "--check-content") { o.check = true; continue; }
    if (arg == "--no-progress") {o.noProgress=true;continue;}
    if (i + 1 == argc) throw std::invalid_argument("missing value for " + std::string(arg));
    const std::string_view value = argv[++i];
    auto path = std::find_if(pathOptions.begin(), pathOptions.end(), [&](const auto& item) { return item.first == arg; });
    if (path != pathOptions.end()) { o.*(path->second) = value; continue; }
    if (arg == "--frames") { o.frames = positive(value); continue; }
    if (arg == "--resolution") {
      const auto x = value.find('x');
      if (x == std::string_view::npos) throw std::invalid_argument("expected WIDTHxHEIGHT");
      o.native.width = positive(value.substr(0, x));
      o.native.height = positive(value.substr(x + 1));
      if (o.native.width < 360 || o.native.width > 3840 || o.native.height < 480 || o.native.height > 2160)
        throw std::invalid_argument("resolution must be 360x480 through 3840x2160");
      continue;
    }
    throw std::invalid_argument("unknown option: " + std::string(arg));
  }
  if(o.noProgress && !o.progress.empty())throw std::invalid_argument("choose --progress or --no-progress");
  return o;
}
using ScriptAction=std::variant<SorterAction,GalleryCommand,OptionId>;
std::vector<std::optional<ScriptAction>> script(const std::filesystem::path& path) {
  if (path.empty()) return {};
  std::ifstream input(path);
  if (!input) throw std::runtime_error("cannot read script: " + path.string());
  constexpr std::array commands{
      std::pair{"bucket", SorterActionKind::SelectBucket}, std::pair{"activate", SorterActionKind::ActivateEquation},
      std::pair{"grid", SorterActionKind::BackToGrid}, std::pair{"clear", SorterActionKind::ClearInspection},
      std::pair{"empty", SorterActionKind::RequestEmpty}, std::pair{"confirm_empty", SorterActionKind::ConfirmEmpty},
      std::pair{"cancel_empty", SorterActionKind::CancelEmpty}, std::pair{"undo", SorterActionKind::Undo},
      std::pair{"auto_sort", SorterActionKind::AutoSort}, std::pair{"hint", SorterActionKind::ShowHint},
      std::pair{"close_hint", SorterActionKind::CloseHint}, std::pair{"solve",SorterActionKind::OpenSolve},
      std::pair{"return",SorterActionKind::ReturnToSorter},std::pair{"next_problem",SorterActionKind::NextSolve}};
  enum class SolveCommand { Choose, Shoot, Hint, Next, Apply, Replay, Tick };
  constexpr std::array solveCommands{
      std::pair{"choose",SolveCommand::Choose},std::pair{"shoot_answer",SolveCommand::Shoot},
      std::pair{"step_hint",SolveCommand::Hint},std::pair{"next_move",SolveCommand::Next},
      std::pair{"do_step",SolveCommand::Apply},std::pair{"replay",SolveCommand::Replay},std::pair{"tick_ms",SolveCommand::Tick}};
  std::vector<std::optional<ScriptAction>> result;
  std::string line;
  while (std::getline(input, line)) {
    line = line.substr(0, line.find('#'));
    std::istringstream tokens(line);
    std::string verb, parameter, extra;
    if (!(tokens >> verb)) continue;
    if (verb == "frame") {
      if (tokens >> extra) throw std::invalid_argument("frame takes no arguments");
      result.push_back(std::nullopt);
      continue;
    }
    const auto solve=std::find_if(solveCommands.begin(),solveCommands.end(),[&](const auto& c) {return c.first==verb;});
    if(solve!=solveCommands.end()) {
      const bool takesValue=solve->second==SolveCommand::Choose || solve->second==SolveCommand::Shoot || solve->second==SolveCommand::Tick;
      const auto value=takesValue?(tokens>>parameter,positive(parameter)):0;
      if(tokens>>extra)throw std::invalid_argument("extra solve script argument");
      switch(solve->second) {
      case SolveCommand::Choose:result.emplace_back(GalleryCommand{ChooseAnswer{{},{value}}});break;
      case SolveCommand::Shoot:result.emplace_back(OptionId{value});break;
      case SolveCommand::Hint:result.emplace_back(GalleryCommand{GalleryHelp{{},GalleryHelpKind::Hint}});break;
      case SolveCommand::Next:result.emplace_back(GalleryCommand{GalleryHelp{{},GalleryHelpKind::NextMove}});break;
      case SolveCommand::Apply:result.emplace_back(GalleryCommand{GalleryHelp{{},GalleryHelpKind::DoStep}});break;
      case SolveCommand::Replay:result.emplace_back(GalleryCommand{ReplayQuestion{}});break;
      case SolveCommand::Tick:result.emplace_back(GalleryCommand{GalleryTick{value/1000.0F}});break;
      }
      continue;
    }
    const auto command = std::find_if(commands.begin(), commands.end(), [&](const auto& c) { return c.first == verb; });
    if (command == commands.end()) throw std::invalid_argument("unknown script command: " + verb);
    SorterAction action{command->second};
    if (action.kind == SorterActionKind::ActivateEquation || action.kind == SorterActionKind::OpenSolve) {
      tokens >> parameter;
      action.equation = positive(parameter);
    } else if (action.kind == SorterActionKind::SelectBucket) {
      tokens >> parameter;
      bool found = false;
      for (std::size_t b = 0; b < sorterBucketCount; ++b) if (sorterBucketName(static_cast<SorterBucket>(b)) == parameter) {
        action.bucket = static_cast<SorterBucket>(b);
        found = true;
      }
      if (!found) throw std::invalid_argument("unknown bucket: " + parameter);
    }
    if (tokens >> extra) throw std::invalid_argument("extra script argument: " + extra);
    result.push_back(action);
  }
  return result;
}
void applyScript(EquationSorterSession& session,ScriptAction action) {
  std::visit([&](auto command) {
    using T=std::decay_t<decltype(command)>;
    if constexpr(std::is_same_v<T,SorterAction>) {
      command.revision=session.view().revision;
      const auto result=session.dispatch(command);
      if(!result.accepted)throw std::runtime_error(std::string(result.reason));
    } else {
      auto* game=session.activeSolve();
      if(!game)throw std::runtime_error("open a solution first");
      GalleryResult result;
      if constexpr(std::is_same_v<T,OptionId>) {
        const auto v=game->view();
        const auto end=v.answers.begin()+v.choiceCount;
        const auto answer=std::find_if(v.answers.begin(),end,[&](const auto& a){return a.binding.option==command;});
        if(answer==end)throw std::runtime_error("unknown answer ID");
        const auto objects=game->scene().objects();
        const auto object=std::find_if(objects.begin(),objects.end(),[&](const auto& o){return o.id==answer->binding.object;});
        const auto point=game->scene().project(object->position);
        result=game->dispatch(Shoot{game->scene().frame().id,v.challenge,point.x,point.y});
      } else {
        std::visit([&](auto& c){if constexpr(requires{c.challenge;})c.challenge=game->view().challenge;},command);
        result=game->dispatch(command);
      }
      if(!result.accepted)throw std::runtime_error(std::string(result.reason));
    }
  },std::move(action));
}
void prepareOutputs(const Options& o) {
  const auto resolve=[](const auto& path) {
    std::error_code error;auto resolved=std::filesystem::weakly_canonical(path,error);
    return error?std::filesystem::absolute(path).lexically_normal():resolved;
  };
  std::vector<std::filesystem::path> outputs;
  if (!o.progress.empty()) outputs.push_back(o.progress);
  if (!o.report.empty()) outputs.push_back(o.report);
  if (!o.capture.empty()) {
    const auto c = capturePaths(o.capture);
    outputs.insert(outputs.end(), {c.screenshotPath, c.rawPath, c.metaPath, c.hashPath});
  }
  for (std::size_t i = 0; i < outputs.size(); ++i) {
    const auto resolved = resolve(outputs[i]);
    for (const auto& source : {o.content, o.script})
      if (!source.empty() && resolved == resolve(source))
        throw std::invalid_argument("output would overwrite an input file");
    for (std::size_t j = 0; j < i; ++j)
      if (resolved == resolve(outputs[j])) throw std::invalid_argument("output paths overlap");
    if (outputs[i]!=o.progress && !outputs[i].parent_path().empty()) std::filesystem::create_directories(outputs[i].parent_path());
  }
}
void report(const std::filesystem::path& path, const EquationSorterSession& session, const EquationSorterUiState& ui) {
  if (path.empty()) return;
  using Json = nlohmann::json;
  const auto v = session.view();
  Json data{{"equations", Json::array()}, {"counts", v.counts}, {"inventory", v.inventory},
      {"pending_empty", v.pendingEmpty}, {"undo_depth", v.undoDepth}, {"revision", v.revision},
      {"scroll_y", ui.scrollY}, {"inventory_slots", Json::array()}, {"presented_cards", Json::array()}, {"toolbar", Json::array()}};
  data["next_step"] = v.nextStep;
  data["hint"] = v.hint;
  data["hint_visible"] = v.hintVisible;
  data["auto_sort_count"] = v.autoSortCount;
  data["unclassified_count"] = v.unclassifiedCount;
  data["solving"] = v.solving;
  if(const auto* game=session.savedSolve()) {
    const auto g=game->view();const auto& run=game->question().currentRun();
    data["solve"]={{"question",run.questionId},{"step",g.step},{"working",g.working},{"prompt",g.prompt},
      {"completed",g.completed},{"hint",g.hint},{"next_move",g.nextMove},{"verification",g.verification},
      {"wrong",g.wrongHits},{"correct",g.correctHits},{"steps",Json::array()}};
    for(const auto& step:run.steps) {
      Json attempts=Json::array();
      for(const auto& a:step.attempts)attempts.push_back({{"option",a.option.value},{"correct",a.correct}});
      data["solve"]["steps"].push_back({{"id",step.id.value},{"hint_requested",step.hintRequested},
        {"next_move_requested",step.nextMoveRequested},{"answer_shown",step.answerShown},{"attempts",attempts}});
    }
    if(run.math) {
      auto& math=data["solve"]["mathematical_moves"];
      math={{"run_number",run.runNumber},{"content_version",run.contentVersion},
        {"active",run.math->active},{"revision",run.math->revision},{"nodes",Json::array()},{"events",Json::array()}};
      for(const auto& node:run.math->nodes)math["nodes"].push_back({{"id",node.working.id.value},
        {"parent",node.parent},{"working",node.working.display},{"operation",node.operation},
        {"explanation",node.explanation},{"verification",node.verification}});
      for(const auto& event:run.math->events)math["events"].push_back({{"kind",event.kind==iggy3d::first_move::MathMoveKind::Undo?"undo":"submit"},
        {"from",event.from},{"to",event.to},{"operation",static_cast<unsigned>(event.operation)},
        {"operand",event.operand},{"entry",event.entry},{"correct",event.correct},{"feedback",event.feedback}});
    }
  }
  data["active_bucket"] = v.activeBucket ? Json(sorterBucketName(*v.activeBucket)) : Json(nullptr);
  data["inspected"] = v.inspected ? Json(*v.inspected) : Json(nullptr);
  for (const auto& e : session.content()) data["equations"].push_back({{"id", e.id}, {"home_index", e.homeIndex},
      {"owner", v.owners[e.homeIndex] ? std::string(sorterBucketName(*v.owners[e.homeIndex])) : "Unsorted"}});
  for (std::size_t i = 0; i < v.slotCount; ++i) data["inventory_slots"].push_back(v.inventorySlots[i]);
  for (std::size_t i = 0; i < ui.cardCount; ++i) {
    const auto& c = ui.cards[i];
    data["presented_cards"].push_back({{"id", c.equation}, {"available", c.available},
        {"rect", {c.x, c.y, c.width, c.height}}});
  }
  for (const auto& c : ui.toolbar) data["toolbar"].push_back({c.x, c.y, c.width, c.height});
  std::ofstream output(path);
  output << data.dump(2) << '\n';
  if (!output) throw std::runtime_error("cannot write report: " + path.string());
}
}
int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
      std::cout << "sorter [--content FILE] [--check-content] [--offscreen] [--frames N]\n"
                   "       [--resolution WxH] [--script FILE] [--report FILE] [--capture PNG]\n"
                   "       [--progress FILE | --no-progress] [--library FILE]\n";
      return 0;
    }
    auto o = options(argc, argv);
    EquationSorterSession session(loadSorterContent(o.content)); // Fail before creating the native host.
    const auto corpus=loadMathCorpus(o.library);
    if (o.check) { std::cout << "Validated 100 sortable cards and " << corpus.entries.size() << " library entries: " << o.content << '\n'; return 0; }
    const auto commands = script(o.script);
    std::string progressLocationError;
    // Bounded runs and scripts never touch personal progress implicitly.
    if(o.progress.empty() && !o.noProgress && o.script.empty() && !o.frames && !o.native.offscreen) {
      char* folder=SDL_GetPrefPath("KOGA ryu","Paths");
      if(!folder)progressLocationError="Practice cannot save: "+std::string(SDL_GetError());
      else {o.progress=std::filesystem::path(folder)/("practice-"+o.content.stem().string()+".json");SDL_free(folder);}
    }
    prepareOutputs(o);
    StudyProgressFile progress(o.progress);progress.load(session);
    if(progress.failed())std::cerr << progress.message() << '\n';
    if(!progressLocationError.empty())std::cerr << progressLocationError << '\n';
    if(commands.empty() && !session.view().study.types.empty())
      (void)session.dispatch({SorterActionKind::OpenStudy,SorterBucket::A,0,session.view().revision});
    if (o.frames && o.frames < commands.size() + 2) throw std::invalid_argument("--frames must allow the complete script plus two frames");
    if (o.native.offscreen && !o.frames) o.frames = static_cast<std::uint32_t>(commands.size() + 3);
    NativeVulkanHost host(o.native);
    EquationSorterUiState ui;
    ui.corpus=&corpus;
    SceneFrame renderScene;
    std::size_t rendered = 0, commandIndex = 0;
    while (!o.frames || rendered < o.frames) {
      const auto result = host.frame([](const SDL_Event&) {}, [&] {
        if (commandIndex < commands.size()) {
          if (auto action = commands[commandIndex++]) {
            applyScript(session,*action);
          }
        }
        beginEquationSorterFrame(ui, session, commands.empty()?-1.0F:0.0F);
        progress.save(session);ui.progressMessage=progressLocationError.empty()?progress.message():progressLocationError;
        ui.progressFailed=progress.failed() || !progressLocationError.empty();
        drawEquationSorter(ui, session.view(), session.content(), session.activeSolve());
        // The host consumes this stable snapshot after the callback, even when
        // opening/closing the solver changed which session supplied the frame.
        const auto* active=session.activeSolve();
        renderScene=active && !active->question().content().lineGraph && !active->question().currentRun().math?active->scene().frame():SceneFrame{};
      }, &renderScene);
      if (result.status == FrameStatus::Closed) break;
      if (result.status == FrameStatus::Failed) throw std::runtime_error(result.error);
      if (result.status == FrameStatus::Rendered) ++rendered;
    }
    progress.save(session);
    if(progress.failed())std::cerr << progress.message() << '\n';
    if (!o.capture.empty()) {
      std::string error;
      if (!host.capture(capturePaths(o.capture), error)) throw std::runtime_error(error);
    }
    report(o.report, session, ui);
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Equation sorter: " << e.what() << '\n';
    return 1;
  }
}
