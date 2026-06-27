#include "app/iggy3d/world/ProductNpcProfileAssignment.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nullTableDefaults() {
  const iggy3d::ProductNpcProfileResolveResult result =
      iggy3d::resolveProductNpcProfileAssignment({nullptr, "training_npc"});
  return expect(result.ok, "null table ok") &&
         expect(result.status == iggy3d::ProductNpcProfileAssignmentStatus::Defaulted,
                "null table defaulted") &&
         expect(result.reasonCode == "defaulted", "null table reason") &&
         expect(result.behaviorProfileId == "default", "null table default profile");
}

bool missingRowDefaults() {
  iggy3d::ProductNpcProfileAssignmentTable table;
  table.assignments.push_back({"training_npc", "passive"});

  const iggy3d::ProductNpcProfileResolveResult result =
      iggy3d::resolveProductNpcProfileAssignment({&table, "merchant"});
  return expect(result.ok, "missing row ok") &&
         expect(result.status == iggy3d::ProductNpcProfileAssignmentStatus::Defaulted,
                "missing row defaulted") &&
         expect(result.behaviorProfileId == "default", "missing row default profile");
}

bool exactRowResolvesAssignedProfile() {
  iggy3d::ProductNpcProfileAssignmentTable table;
  table.assignments.push_back({"training_npc", "passive"});

  const iggy3d::ProductNpcProfileResolveResult result =
      iggy3d::resolveProductNpcProfileAssignment({&table, "training_npc"});
  return expect(result.ok, "exact row ok") &&
         expect(result.status == iggy3d::ProductNpcProfileAssignmentStatus::Resolved,
                "exact row resolved") &&
         expect(result.reasonCode == "resolved", "exact row reason") &&
         expect(result.behaviorProfileId == "passive", "exact row profile");
}

bool unknownSyntacticallyValidProfileIsPreserved() {
  iggy3d::ProductNpcProfileAssignmentTable table;
  table.assignments.push_back({"training_npc", "ghost_profile"});

  const iggy3d::ProductNpcProfileResolveResult result =
      iggy3d::resolveProductNpcProfileAssignment({&table, "training_npc"});
  return expect(result.ok, "unknown valid profile ok") &&
         expect(result.status == iggy3d::ProductNpcProfileAssignmentStatus::Resolved,
                "unknown valid profile resolved") &&
         expect(result.behaviorProfileId == "ghost_profile",
                "unknown valid profile preserved");
}

bool invalidProfileIdRejects() {
  iggy3d::ProductNpcProfileAssignmentTable table;
  table.assignments.push_back({"training_npc", "Bad-Id"});

  const iggy3d::ProductNpcProfileResolveResult result =
      iggy3d::resolveProductNpcProfileAssignment({&table, "training_npc"});
  const iggy3d::ProductNpcProfileAssignmentValidationResult validation =
      iggy3d::validateProductNpcProfileAssignments(table);
  return expect(!result.ok, "invalid profile resolve rejects") &&
         expect(result.status == iggy3d::ProductNpcProfileAssignmentStatus::InvalidProfileId,
                "invalid profile resolve status") &&
         expect(result.reasonCode == "invalid_profile_id", "invalid profile reason") &&
         expect(!validation.ok, "invalid profile validation rejects") &&
         expect(validation.behaviorProfileId == "Bad-Id", "invalid profile proof id");
}

bool emptyEntityNameRejects() {
  const iggy3d::ProductNpcProfileResolveResult result =
      iggy3d::resolveProductNpcProfileAssignment({nullptr, ""});
  iggy3d::ProductNpcProfileAssignmentTable table;
  table.assignments.push_back({"", "passive"});
  const iggy3d::ProductNpcProfileAssignmentValidationResult validation =
      iggy3d::validateProductNpcProfileAssignments(table);

  return expect(!iggy3d::isValidProductNpcProfileEntityName(""),
                "empty entity invalid") &&
         expect(!result.ok, "empty entity resolve rejects") &&
         expect(result.status == iggy3d::ProductNpcProfileAssignmentStatus::InvalidEntityName,
                "empty entity resolve status") &&
         expect(result.reasonCode == "invalid_entity_name", "empty entity reason") &&
         expect(!validation.ok, "empty entity validation rejects") &&
         expect(validation.status == iggy3d::ProductNpcProfileAssignmentStatus::InvalidEntityName,
                "empty entity validation status");
}

bool duplicateEntityNamesRejectValidation() {
  iggy3d::ProductNpcProfileAssignmentTable table;
  table.assignments.push_back({"training_npc", "passive"});
  table.assignments.push_back({"training_npc", "default"});

  const iggy3d::ProductNpcProfileAssignmentValidationResult validation =
      iggy3d::validateProductNpcProfileAssignments(table);
  return expect(!validation.ok, "duplicate rejects") &&
         expect(validation.status ==
                    iggy3d::ProductNpcProfileAssignmentStatus::DuplicateEntityName,
                "duplicate status") &&
         expect(validation.reasonCode == "duplicate_entity_name", "duplicate reason") &&
         expect(validation.stableEntityName == "training_npc", "duplicate proof name");
}

bool statusNamesAreLowerSnake() {
  return expect(iggy3d::productNpcProfileAssignmentStatusName(
                    iggy3d::ProductNpcProfileAssignmentStatus::Resolved) == "resolved",
                "resolved name") &&
         expect(iggy3d::productNpcProfileAssignmentStatusName(
                    iggy3d::ProductNpcProfileAssignmentStatus::Defaulted) == "defaulted",
                "defaulted name") &&
         expect(iggy3d::productNpcProfileAssignmentStatusName(
                    iggy3d::ProductNpcProfileAssignmentStatus::InvalidEntityName) ==
                    "invalid_entity_name",
                "invalid entity name") &&
         expect(iggy3d::productNpcProfileAssignmentStatusName(
                    iggy3d::ProductNpcProfileAssignmentStatus::InvalidProfileId) ==
                    "invalid_profile_id",
                "invalid profile name") &&
         expect(iggy3d::productNpcProfileAssignmentStatusName(
                    iggy3d::ProductNpcProfileAssignmentStatus::DuplicateEntityName) ==
                    "duplicate_entity_name",
                "duplicate name");
}

bool glyphLikeAssignmentHasNoSpecialMeaning() {
  iggy3d::ProductNpcProfileAssignmentTable table;
  table.assignments.push_back({"N", "passive"});

  const iggy3d::ProductNpcProfileResolveResult result =
      iggy3d::resolveProductNpcProfileAssignment({&table, "training_npc"});
  return expect(result.ok, "glyph-like row does not poison table") &&
         expect(result.status == iggy3d::ProductNpcProfileAssignmentStatus::Defaulted,
                "glyph-like row is not matched as glyph") &&
         expect(result.behaviorProfileId == "default", "glyph-like row defaults");
}

}  // namespace

int main() {
  const bool ok = nullTableDefaults() && missingRowDefaults() &&
                  exactRowResolvesAssignedProfile() &&
                  unknownSyntacticallyValidProfileIsPreserved() &&
                  invalidProfileIdRejects() && emptyEntityNameRejects() &&
                  duplicateEntityNamesRejectValidation() && statusNamesAreLowerSnake() &&
                  glyphLikeAssignmentHasNoSpecialMeaning();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
