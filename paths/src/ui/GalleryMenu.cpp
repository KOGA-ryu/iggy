#include "ui/GalleryMenu.hpp"
#include "content/QuestionContentIO.hpp"

#include <algorithm>

namespace paths {
namespace {
std::filesystem::path absolutePath(const std::filesystem::path& path) {
  return std::filesystem::absolute(path).lexically_normal();
}
std::vector<GalleryVariationDescriptor> availableModes(const QuestionPack& pack) {
  std::vector<GalleryVariationDescriptor> result;
  for(const auto& mode:galleryVariations())if(pack.decks.contains(mode.id))result.push_back(mode);
  if(result.empty())throw QuestionContentError(pack.source,"/decks","no supported gallery practice type");
  return result;
}
}
GalleryMenu::GalleryMenu(const std::filesystem::path& directory,GalleryConfig config,
                         const std::filesystem::path& customPack):config_(config) {
  packs_={
    {"starter","Starter questions","Practise matching answers, collecting sets and solving equations.",directory/"gallery_foundation.json"},
    {"source_002","Three points, one formula","Build and solve a quadratic from three points. 13 decisions.",directory/"source_002.json"},
    {"source_013","The closest point on a line","Follow a projection and justify why it works. 14 guided decisions.",directory/"source_013.json"},
  };
  for(auto& pack:packs_){pack.path=absolutePath(pack.path);pack.mode=config.variation;}
  if(!customPack.empty()) {
    const auto path=absolutePath(customPack);
    const auto found=std::find_if(packs_.begin(),packs_.end(),[&](const auto& p){return p.path==path;});
    selectedPack_=static_cast<std::size_t>(found-packs_.begin());
    if(found==packs_.end())packs_.push_back({"custom","Custom questions","Your selected question file.",path,config.variation});
  }
}
GallerySession* GalleryMenu::activeGame() const {return active_?games_[*active_].session.get():nullptr;}
void GalleryMenu::pauseActive() {
  if(auto* game=activeGame())static_cast<void>(game->dispatch(GalleryPause{true}));
}
GallerySession* GalleryMenu::selectedGame() const {
  const auto found=std::find_if(games_.begin(),games_.end(),[&](const auto& game){
    return game.pack==selectedPack_ && game.variation==selectedMode();
  });
  return found==games_.end()?nullptr:found->session.get();
}
bool GalleryMenu::canResume() const {return selectedGame()!=nullptr;}
GalleryConfig GalleryMenu::selectedConfig() const {
  if(replacement_)return *replacement_;
  if(const auto* game=selectedGame())return game->config();
  auto config=config_;config.variation=selectedMode();return config;
}
void GalleryMenu::refreshSelection() {
  modes_=availableModes(loadQuestionPack(packs_[selectedPack_].path));
  if(std::none_of(modes_.begin(),modes_.end(),[&](const auto& mode){return mode.variation==selectedMode();}))
    packs_[selectedPack_].mode=modes_.front().variation;
}
bool GalleryMenu::launch(GalleryVariation variation) {
  return start(variation,false);
}
bool GalleryMenu::start(GalleryVariation variation,bool replace) {
  try {
    if(screen_==GalleryScreen::Review)return false;
    if(replace!=preparingNewGame())throw std::invalid_argument("Finish or cancel the new-game setup first.");
    const auto descriptors=galleryVariations();
    const auto mode=std::find_if(descriptors.begin(),descriptors.end(),[&](const auto& m){return m.variation==variation;});
    if(mode==descriptors.end())throw std::invalid_argument("Unknown gallery practice type");
    const auto found=std::find_if(games_.begin(),games_.end(),[&](const auto& g){return g.pack==selectedPack_ && g.variation==variation;});
    std::size_t next=static_cast<std::size_t>(found-games_.begin());
    if(replace && found==games_.end())throw std::logic_error("No selected game to replace.");
    if(found==games_.end() || replace) {
      auto pack=loadQuestionPack(packs_[selectedPack_].path);
      auto deck=pack.deck(mode->id);
      auto modes=availableModes(pack);
      auto config=replace?*replacement_:config_;config.variation=variation;
      auto game=std::make_unique<GallerySession>(config,std::move(pack.catalog),std::move(deck));
      // Loading and construction must succeed before the old session is released.
      if(replace)found->session=std::move(game);
      else games_.push_back({selectedPack_,variation,std::move(game)});
      modes_=std::move(modes);
      if(replace)config_=config;
    }
    pauseActive();active_=next;packs_[selectedPack_].mode=variation;
    static_cast<void>(activeGame()->dispatch(GalleryPause{false}));
    replacement_.reset();screen_=GalleryScreen::Playing;error_.clear();return true;
  } catch(const std::exception& e) {error_=e.what();return false;}
}
bool GalleryMenu::dispatch(GalleryMenuAction action) {
  try {
    if(screen_==GalleryScreen::Review && action.kind!=GalleryMenuActionKind::CloseReview &&
       action.kind!=GalleryMenuActionKind::SelectReviewRun && action.kind!=GalleryMenuActionKind::ToggleReviewStep)return false;
    switch(action.kind) {
      case GalleryMenuActionKind::OpenReview:
        if(screen_!=GalleryScreen::Playing || !activeGame())return false;
        pauseActive();reviewRun_=0;
        expandedReviewStep_=activeGame()->question().review()->steps.size()-1;
        screen_=GalleryScreen::Review;break;
      case GalleryMenuActionKind::CloseReview:
        if(screen_!=GalleryScreen::Review)return false;
        screen_=GalleryScreen::Playing;break;
      case GalleryMenuActionKind::SelectReviewRun: {
        if(screen_!=GalleryScreen::Review)return false;
        const auto view=activeGame()->question().review(action.index);
        if(!view)return false;
        reviewRun_=action.index;expandedReviewStep_=view->steps.size()-1;break;
      }
      case GalleryMenuActionKind::ToggleReviewStep: {
        if(screen_!=GalleryScreen::Review || action.index>=activeGame()->question().review(reviewRun_)->steps.size())return false;
        expandedReviewStep_=expandedReviewStep_==action.index?std::nullopt:std::optional{action.index};break;
      }
      case GalleryMenuActionKind::SelectPack:
        if(screen_!=GalleryScreen::ChooseQuestions || replacement_ || action.index>=packs_.size())return false;
        selectedPack_=action.index;modes_.clear();refreshSelection();break;
      case GalleryMenuActionKind::SelectMode:
        if(screen_!=GalleryScreen::ChooseQuestions || replacement_ || action.index>=modes_.size())return false;
        packs_[selectedPack_].mode=modes_[action.index].variation;break;
      case GalleryMenuActionKind::SelectMotion:
      case GalleryMenuActionKind::SetPace: {
        if(screen_!=GalleryScreen::ChooseQuestions)return false;
        if(canResume() && !replacement_){error_="Resume keeps this game's movement settings.";return false;}
        auto next=selectedConfig();
        if(action.kind==GalleryMenuActionKind::SelectMotion) {
          const auto routes=targetRouteDescriptors();
          if(action.index>=routes.size())return false;
          next.motion=routes[action.index].kind;
        } else next.pace=action.amount;
        const auto result=validateGalleryConfig(next);
        if(!result.accepted){error_=result.reason;return false;}
        if(replacement_)replacement_=next;else config_=next;
        break;
      }
      case GalleryMenuActionKind::Play:
        if(screen_!=GalleryScreen::ChooseQuestions)return false;
        return launch(selectedMode());
      case GalleryMenuActionKind::ChooseQuestions:
        if(replacement_)return false;
        pauseActive();screen_=GalleryScreen::ChooseQuestions;break;
      case GalleryMenuActionKind::Workshop:
        if(screen_!=GalleryScreen::ChooseQuestions || replacement_)return false;
        pauseActive();screen_=GalleryScreen::Workshop;break;
      case GalleryMenuActionKind::NewGame:
        if(screen_!=GalleryScreen::ChooseQuestions || replacement_ || !canResume())return false;
        replacement_=selectedGame()->config();break;
      case GalleryMenuActionKind::CancelNewGame:
        if(screen_!=GalleryScreen::ChooseQuestions || !replacement_)return false;
        replacement_.reset();break;
      case GalleryMenuActionKind::StartNewGame:
        if(screen_!=GalleryScreen::ChooseQuestions || !replacement_)return false;
        return start(selectedMode(),true);
    }
    error_.clear();return true;
  } catch(const std::exception& e) {error_=e.what();return false;}
}
} // namespace paths
