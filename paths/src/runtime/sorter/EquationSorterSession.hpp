#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "runtime/gallery/GallerySession.hpp"

namespace paths {
inline constexpr std::size_t sorterEquationCount = 100;
using SorterEquationId = std::uint32_t;
enum class SorterBucket { A, B, C, D, E, Dump };
inline constexpr std::size_t sorterBucketCount = 6;
enum class SorterSubject { Algebra, Trig, Calculus, LinearAlgebra, DiscreteMaths };
struct SorterSubjectInfo { std::string_view key, name, caption; SorterBucket bucket; };
inline constexpr std::array<SorterSubjectInfo, 5> sorterSubjects{{
    {"algebra", "Algebra", "Algebra", SorterBucket::A},
    {"trig", "Trigonometry", "Trig", SorterBucket::B},
    {"calculus", "Calculus", "Calculus", SorterBucket::C},
    {"linear_algebra", "Linear algebra", "Linear", SorterBucket::D},
    {"discrete_maths", "Discrete maths", "Discrete", SorterBucket::E}}};
using SorterOwner = std::optional<SorterBucket>;
[[nodiscard]] std::string_view sorterBucketName(SorterBucket bucket);

struct SorterEquation {
  SorterEquationId id = 0;
  std::size_t homeIndex = 0;
  std::string text;
  std::optional<SorterSubject> subject;
  std::string hint;
  std::string solvePack;
  std::shared_ptr<const iggy3d::first_move::LayeredQuestionContent> solution;
  struct StudyTopic { std::string chapter, type, form; };
  std::optional<StudyTopic> study;
};
struct SorterContentError { std::string field, reason; };
[[nodiscard]] std::optional<SorterContentError> validateSorterContent(
    std::span<const SorterEquation> content);

enum class SorterActionKind {
  SelectBucket, BackToGrid, ActivateEquation, ClearInspection,
  RequestEmpty, ConfirmEmpty, CancelEmpty, Undo, AutoSort, ShowHint, CloseHint,
  OpenSolve, ReturnToSorter, NextSolve,
  OpenStudy, CloseStudy, ToggleStudySubject, ToggleStudyChapter, ToggleStudyType,
  SetStudyMode, SetStudyCount, ShuffleStudy, ToggleStudyQuestion, StartStudy, ResumeStudy, ReturnToStudy
};
struct SorterAction {
  SorterActionKind kind = SorterActionKind::ClearInspection;
  SorterBucket bucket = SorterBucket::A;
  SorterEquationId equation = 0;
  std::uint64_t revision = 0;
  std::uint32_t value = 0;
};
struct SorterResult { bool accepted, changed; std::string_view reason; };
enum class StudyMode { All, Random, Specific };
struct StudyProblemType {
  SorterSubject subject;
  std::size_t chapterId;
  std::string chapter, title, form;
  std::vector<std::size_t> homes;
};
struct StudySelectionView {
  std::span<const StudyProblemType> types;
  std::array<bool,sorterEquationCount> includedTypes{}, available{}, selected{};
  StudyMode mode=StudyMode::All;
  std::size_t availableCount=0, selectedCount=0, randomCount=3;
  bool canResume=false;
};
struct SorterView {
  std::array<SorterOwner, sorterEquationCount> owners{};
  std::array<std::size_t, sorterBucketCount + 1> counts{}; // Unsorted, A-E, Dump
  SorterOwner activeBucket;
  bool inventory = false;
  std::optional<SorterEquationId> inspected;
  bool pendingEmpty = false;
  std::array<SorterEquationId, sorterEquationCount> inventorySlots{};
  std::size_t slotCount = 0, undoDepth = 0;
  std::uint64_t revision = 0;
  std::size_t autoSortCount = 0, unclassifiedCount = 0;
  bool hintVisible = false;
  std::string nextStep, hint;
  bool solving = false;
  std::optional<SorterEquationId> solveCandidate;
  std::string solveLabel;
  std::optional<SorterEquationId> nextSolve;
  std::size_t solveNumber=0, solveCount=0;
  bool studying=false, studyRun=false;
  StudySelectionView study;
};

// Owns all semantic state. IDs resolve through immutable content, never as indices.
class EquationSorterSession {
public:
  explicit EquationSorterSession(std::vector<SorterEquation> content);
  [[nodiscard]] const std::vector<SorterEquation>& content() const { return content_; }
  [[nodiscard]] SorterView view() const;
  [[nodiscard]] SorterResult dispatch(const SorterAction& action);
  [[nodiscard]] GallerySession* activeSolve() { return solving_ ? solves_[solveHome_].get() : nullptr; }
  [[nodiscard]] const GallerySession* activeSolve() const { return solving_ ? savedSolve() : nullptr; }
  [[nodiscard]] const GallerySession* savedSolve() const { return solves_[solveHome_].get(); }

private:
  struct OwnerChange { SorterEquationId equation; SorterOwner from, to; };
  using Transaction = std::vector<OwnerChange>;
  std::vector<SorterEquation> content_;
  std::array<SorterOwner, sorterEquationCount> owners_{};
  SorterOwner activeBucket_;
  bool inventory_ = false, pendingEmpty_ = false, hintVisible_ = false;
  std::optional<SorterEquationId> inspected_;
  std::array<SorterEquationId, sorterEquationCount> slots_{};
  std::size_t slotCount_ = 0;
  std::vector<Transaction> history_;
  std::uint64_t revision_ = 0;
  std::array<std::unique_ptr<GallerySession>, sorterEquationCount> solves_;
  std::size_t solveHome_ = 0;
  bool solving_ = false;
  std::vector<StudyProblemType> studyTypes_;
  std::array<bool,sorterEquationCount> includedTypes_{}, studyAvailable_{}, studySelected_{};
  StudyMode studyMode_=StudyMode::All;
  std::size_t studyRandomCount_=3, studyPosition_=0;
  bool studying_=false, studyRun_=false;
  std::vector<std::size_t> studyQueue_;
  std::mt19937 studyRandom_{std::random_device{}()};

  [[nodiscard]] SorterResult accept(bool changed);
  [[nodiscard]] std::optional<std::size_t> home(SorterEquationId id) const;
  [[nodiscard]] SorterResult activateEquation(SorterEquationId id);
  [[nodiscard]] SorterResult commitTransaction(Transaction changes);
  [[nodiscard]] SorterResult undo();
  [[nodiscard]] SorterResult openSolve(std::size_t index);
  [[nodiscard]] std::unique_ptr<GallerySession> freshSolve(std::size_t index) const;
  [[nodiscard]] SorterResult dispatchStudy(const SorterAction& action);
  void refreshStudySelection();
  [[nodiscard]] std::optional<std::size_t> nextSolveHome() const;
  void openInventory();
  void reconcileInventorySlots();
  void describeNextStep(SorterView& view) const;
};
} // namespace paths
