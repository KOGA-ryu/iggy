#pragma once

#include <array>
#include <optional>
#include <random>
#include <span>
#include <variant>

#include "runtime/first_move/LayeredQuestionSession.hpp"
#include "scene/GalleryScene.hpp"

namespace paths {
using iggy3d::first_move::OptionId;
struct ChallengeId {
  std::uint64_t value = 0;
  bool operator==(const ChallengeId&) const = default;
};
struct DisplayToken {
  std::uint8_t value = 0;
  bool operator==(const DisplayToken&) const = default;
};
struct DisplayColour { std::string_view marker, name; iggy3d::Vec3 rgb; };
[[nodiscard]] std::span<const DisplayColour> galleryDisplayColours();

enum class GalleryVariation : std::uint8_t { EqualitySweep, QuestionRelay, EquationChain, SubstitutionChain };
struct GalleryVariationDescriptor {
  GalleryVariation variation;
  std::string_view id, title;
};
[[nodiscard]] std::span<const GalleryVariationDescriptor> galleryVariations();
struct GalleryConfig {
  GalleryVariation variation = GalleryVariation::EqualitySweep;
  std::uint32_t seed = 1;
  bool changeCorrectColour = true;
  RouteKind motion = RouteKind::Horizontal;
  float pace = 0.7F;
  bool stopAfterQuestion = false;
  bool mathematicalMoves = false;
};
// Shared by game construction and the menu's pending setup. Route validity
// stays with TargetMotion; custom waypoint authoring stays in Workshop.
[[nodiscard]] GalleryResult validateGalleryConfig(const GalleryConfig& config);
struct TargetBinding { SceneObjectId object; OptionId option; DisplayToken token; };
struct ChallengeRecord {
  ChallengeId id;
  std::string questionId;
  std::uint32_t contentVersion = 0, runNumber = 0, assignmentRuleVersion = 1;
  iggy3d::first_move::QuestionStepId step;
  std::array<TargetBinding,8> bindings{};
  std::size_t count = 0;
  // Records the actual permutation; a seed alone is not cross-library replay.
};
struct GalleryTick { float seconds; };
struct GalleryPause { bool paused; };
struct GalleryViewport { SceneViewport viewport; };
struct Shoot { SceneFrameId frame; ChallengeId challenge; float u, v; };
struct ChooseAnswer { ChallengeId challenge; OptionId option; };
enum class GalleryHelpKind { Hint, NextMove, DoStep };
struct GalleryHelp { ChallengeId challenge; GalleryHelpKind kind; };
struct ReplayQuestion { bool archiveUnfinished=false; };
struct MathematicalMove { ChallengeId challenge; iggy3d::first_move::MathMoveCommand move; };
using GalleryCommand = std::variant<GalleryTick,GalleryPause,GalleryViewport,Shoot,ChooseAnswer,GalleryHelp,ReplayQuestion,MathematicalMove>;
struct GalleryAnswerRow { TargetBinding binding; std::string_view text; bool collected; };
enum class GalleryFeedback : std::uint8_t { None, Correct, Incorrect, Miss };
struct GallerySessionView {
  ChallengeId challenge;
  std::string_view title, equation, prompt, working;
  std::span<const iggy3d::first_move::WorkingHighlight> workingHighlights;
  GalleryFeedback feedback=GalleryFeedback::None;
  std::array<GalleryAnswerRow,8> answers{};
  std::size_t choiceCount=0, step=0, stepCount=0, required=0, collected=0;
  std::size_t correctHits=0, wrongHits=0, completedQuestions=0, aimMisses=0;
  bool paused=false, ready=false, transitioning=false, priorExposure=false;
  bool completed=false, canHint=false, canReveal=false;
  iggy3d::first_move::StepPurpose purpose=iggy3d::first_move::StepPurpose::AnswerChoice;
  std::string_view hint, nextMove, verification;
};

// Gameplay composition only. The scene owns bodies and the common clock;
// LayeredQuestionSession owns correctness, collection, progression and evidence.
class GallerySession {
public:
  GallerySession(GalleryConfig config,
      std::vector<iggy3d::first_move::LayeredQuestionContent> catalog,
      std::vector<std::size_t> resolvedDeck);
  [[nodiscard]] GalleryResult dispatch(const GalleryCommand&);
  [[nodiscard]] GallerySessionView view() const;
  [[nodiscard]] const SceneFrame& publishFrame();
  [[nodiscard]] const GalleryScene& scene() const {return scene_;}
  [[nodiscard]] const iggy3d::first_move::LayeredQuestionSession& question() const {return question_;}
  [[nodiscard]] std::span<const ChallengeRecord> challenges() const {return challenges_;}
  [[nodiscard]] const GalleryConfig& config() const {return config_;}
private:
  struct PreparedChallenge;
  [[nodiscard]] PreparedChallenge prepareChallenge(bool advance, bool replay=false, bool archiveUnfinished=false) const;
  void commitChallenge(PreparedChallenge&&);
  [[nodiscard]] GalleryResult submitHit(const SceneHit&);
  [[nodiscard]] GalleryResult tick(float seconds);
  void advanceChallenge();
  GalleryConfig config_;
  GalleryScene scene_{false};
  const std::vector<std::size_t> deck_;
  iggy3d::first_move::LayeredQuestionSession question_;
  std::mt19937 assignmentEngine_;
  std::vector<ChallengeRecord> challenges_;
  std::optional<DisplayToken> previousCorrectColour_;
  std::optional<std::uint64_t> transitionAt_;
  std::uint64_t feedbackUntilTick_=0;
  ChallengeId presentedChallenge_;
  std::size_t deckIndex_=0, aimMisses_=0;
  bool ready_=false;
  GalleryFeedback feedback_=GalleryFeedback::None;
};
}  // namespace paths
