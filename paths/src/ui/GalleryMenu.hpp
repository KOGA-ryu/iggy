#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "runtime/gallery/GallerySession.hpp"

namespace paths {
enum class GalleryScreen { ChooseQuestions, Playing, Workshop, Review };
enum class GalleryMenuActionKind { SelectPack, SelectMode, SelectMotion, SetPace, Play, ChooseQuestions, Workshop,
                                   NewGame, CancelNewGame, StartNewGame, OpenReview, CloseReview, SelectReviewRun, ToggleReviewStep };
struct GalleryMenuAction { GalleryMenuActionKind kind; std::size_t index=0; float amount=0; };
struct GalleryPackChoice {
  std::string id,title,description;
  std::filesystem::path path;
  GalleryVariation mode=GalleryVariation::EqualitySweep;
};
struct StartedGallery {
  std::size_t pack;
  GalleryVariation variation;
  std::unique_ptr<GallerySession> session;
};

// App navigation, pending setup and session lifetime. Loading, judgments, working and
// attempts remain with the existing content and game owners.
class GalleryMenu {
public:
  GalleryMenu(const std::filesystem::path& packDirectory,GalleryConfig config,
              const std::filesystem::path& customPack={});
  [[nodiscard]] bool dispatch(GalleryMenuAction action);
  // Direct CLI startup and the Play action share this preparation boundary.
  [[nodiscard]] bool launch(GalleryVariation variation);
  [[nodiscard]] GalleryScreen screen() const {return screen_;}
  [[nodiscard]] std::span<const GalleryPackChoice> packs() const {return packs_;}
  [[nodiscard]] std::span<const GalleryVariationDescriptor> modes() const {return modes_;}
  [[nodiscard]] std::size_t selectedPack() const {return selectedPack_;}
  [[nodiscard]] GalleryVariation selectedMode() const {return packs_[selectedPack_].mode;}
  [[nodiscard]] bool canResume() const;
  [[nodiscard]] bool preparingNewGame() const {return replacement_.has_value();}
  // An explicit replacement draft takes precedence while the old game survives.
  [[nodiscard]] GalleryConfig selectedConfig() const;
  [[nodiscard]] GallerySession* activeGame() const;
  [[nodiscard]] std::span<const StartedGallery> games() const {return games_;}
  [[nodiscard]] const std::string& error() const {return error_;}
  [[nodiscard]] std::size_t reviewRun() const {return reviewRun_;}
  [[nodiscard]] std::optional<std::size_t> expandedReviewStep() const {return expandedReviewStep_;}
private:
  std::size_t reviewRun_=0;
  std::optional<std::size_t> expandedReviewStep_;
  [[nodiscard]] bool start(GalleryVariation variation,bool replace);
  [[nodiscard]] GallerySession* selectedGame() const;
  void refreshSelection();
  void pauseActive();
  std::vector<GalleryPackChoice> packs_;
  std::vector<GalleryVariationDescriptor> modes_;
  std::vector<StartedGallery> games_;
  GalleryConfig config_;
  std::optional<GalleryConfig> replacement_;
  GalleryScreen screen_=GalleryScreen::ChooseQuestions;
  std::size_t selectedPack_=0;
  std::optional<std::size_t> active_;
  std::string error_;
};
} // namespace paths
