#pragma once

#include "runtime/sorter/EquationSorterSession.hpp"

namespace paths {
inline constexpr std::size_t sorterUndoControl = sorterBucketCount;
inline constexpr std::size_t sorterAutoSortControl = sorterUndoControl + 1;
inline constexpr std::size_t sorterHintControl = sorterAutoSortControl + 1;
enum class StudyControl : std::size_t { All, Random, Specific, Count, Shuffle, Groups, Resume, Start, CountControls };
struct SorterCardBounds {
  SorterEquationId equation = 0;
  float x = 0, y = 0, width = 0, height = 0;
  bool available = false;
};
struct EquationSorterUiState {
  std::string progressMessage;
  bool progressFailed=false;
  SorterCardBounds progressStatus;
  std::optional<SorterAction> pending;
  std::optional<GalleryCommand> pendingGame;
  // The native evidence report and input tests read the same presented rectangles.
  std::array<SorterCardBounds, sorterEquationCount> cards{};
  std::array<SorterCardBounds, sorterHintControl + 1> toolbar{};
  SorterCardBounds hintClose;
  SorterCardBounds solveButton;
  SorterCardBounds studyEntry;
  SorterCardBounds studyProblemPanel;
  std::optional<std::size_t> studyChapter;
  std::array<SorterCardBounds,static_cast<std::size_t>(StudyControl::CountControls)> studyControls{};
  std::array<SorterCardBounds,sorterSubjects.size()> studySubjects{};
  std::array<SorterCardBounds,sorterEquationCount> studyChapters{}, studyChapterChecks{}, studyTypes{}, studyQuestions{};
  std::array<SorterCardBounds, 6> solveControls{}; // Back, Hint/Resume, Next move, Do step, Replay, Next problem
  std::array<SorterCardBounds, 8> solveOptions{};
  std::array<OptionId, 8> solveOptionIds{};
  std::size_t solveOptionCount=0;
  bool shootAvailable=false;
  bool solvePointerHeld=false;
  SceneViewport shootViewport;
  SceneFrameId shootFrame;
  ChallengeId shootChallenge;
  ChallengeId solveDisplayedChallenge;
  bool solveHintVisible=false, solveNextVisible=false, solveCompleteVisible=false;
  SorterCardBounds solveHelp, solveVerification;
  SorterCardBounds solveBoard, solveWorking, solveStage, solveSupport;
  SorterCardBounds solveEquation;
  bool solveInfinityVisible=false;
  SorterCardBounds graphBoard, graphPlot, graphSlider, graphReplay;
  SorterCardBounds graphTable, graphCurrentRow;
  std::array<SorterCardBounds,3> graphSampleRows{};
  std::array<SorterCardBounds,2> graphReadouts{};
  std::optional<iggy3d::first_move::GraphValueRow> graphDisplayedValues;
  std::string graphQuestion;
  std::uint32_t graphRun=0;
  iggy3d::first_move::GraphStage graphStage=iggy3d::first_move::GraphStage::Grid;
  float graphMotion=1, graphProbeX=0;
  std::array<SorterCardBounds, 8> solveAnswers{};
  std::array<SorterCardBounds,iggy3d::first_move::kMathMoveChoiceCapacity> mathOperations{};
  std::array<SorterCardBounds,iggy3d::first_move::kMathResultChoiceCount> mathResults{};
  SorterCardBounds mathUndo, mathInspection;
  SorterCardBounds mathReferenceButton, mathReferencePanel, mathReferenceBody, mathReferenceCalculation;
  std::array<SorterCardBounds,3> mathReferenceControls{}; // Close, previous example column, next example column.
  std::array<SorterCardBounds,2> mathReferenceMatrices{};
  std::string mathReferenceId;
  std::optional<iggy3d::first_move::MathReferenceExample> mathExample;
  std::size_t mathReferenceStep=0;
  bool mathReferenceFollow=false;
  std::array<SorterCardBounds,iggy3d::first_move::kMathNodeCapacity> mathNodes{};
  std::vector<iggy3d::first_move::MathMoveChoice> mathChoices;
  std::optional<std::size_t> mathSelectedMove;
  std::optional<std::size_t> mathInspected;
  std::string mathQuestion;
  std::uint32_t mathRun=0;
  std::uint64_t mathRevision=0;
  bool mathFollow=true;
  std::size_t cardCount = 0;
  float scrollY = 0;
};
void beginEquationSorterFrame(EquationSorterUiState& ui, EquationSorterSession& session, float seconds=-1);
void drawEquationSorter(EquationSorterUiState& ui, const SorterView& view,
                        std::span<const SorterEquation> content, const GallerySession* game=nullptr);
} // namespace paths
