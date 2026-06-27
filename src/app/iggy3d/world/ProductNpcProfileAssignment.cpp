#include "app/iggy3d/world/ProductNpcProfileAssignment.hpp"

#include <unordered_set>

#include "runtime/ai/NpcBehaviorProfile.hpp"

namespace iggy3d {

std::string_view productNpcProfileAssignmentStatusName(
    ProductNpcProfileAssignmentStatus status) {
  switch (status) {
    case ProductNpcProfileAssignmentStatus::Resolved:
      return "resolved";
    case ProductNpcProfileAssignmentStatus::Defaulted:
      return "defaulted";
    case ProductNpcProfileAssignmentStatus::InvalidEntityName:
      return "invalid_entity_name";
    case ProductNpcProfileAssignmentStatus::InvalidProfileId:
      return "invalid_profile_id";
    case ProductNpcProfileAssignmentStatus::DuplicateEntityName:
      return "duplicate_entity_name";
  }
  return "invalid_profile_id";
}

bool isValidProductNpcProfileEntityName(std::string_view stableEntityName) {
  return !stableEntityName.empty();
}

ProductNpcProfileAssignmentValidationResult validateProductNpcProfileAssignments(
    const ProductNpcProfileAssignmentTable& table) {
  ProductNpcProfileAssignmentValidationResult result;
  std::unordered_set<std::string> seenNames;

  for (const ProductNpcProfileAssignment& assignment : table.assignments) {
    if (!isValidProductNpcProfileEntityName(assignment.stableEntityName)) {
      result.status = ProductNpcProfileAssignmentStatus::InvalidEntityName;
      result.reasonCode = productNpcProfileAssignmentStatusName(result.status);
      return result;
    }
    if (!isValidNpcBehaviorProfileId(assignment.behaviorProfileId)) {
      result.status = ProductNpcProfileAssignmentStatus::InvalidProfileId;
      result.reasonCode = productNpcProfileAssignmentStatusName(result.status);
      result.stableEntityName = assignment.stableEntityName;
      result.behaviorProfileId = assignment.behaviorProfileId;
      return result;
    }
    if (!seenNames.insert(assignment.stableEntityName).second) {
      result.status = ProductNpcProfileAssignmentStatus::DuplicateEntityName;
      result.reasonCode = productNpcProfileAssignmentStatusName(result.status);
      result.stableEntityName = assignment.stableEntityName;
      result.behaviorProfileId = assignment.behaviorProfileId;
      return result;
    }
  }

  result.ok = true;
  result.status = ProductNpcProfileAssignmentStatus::Resolved;
  result.reasonCode = productNpcProfileAssignmentStatusName(result.status);
  return result;
}

ProductNpcProfileResolveResult resolveProductNpcProfileAssignment(
    const ProductNpcProfileResolveRequest& request) {
  ProductNpcProfileResolveResult result;

  if (!isValidProductNpcProfileEntityName(request.stableEntityName)) {
    result.status = ProductNpcProfileAssignmentStatus::InvalidEntityName;
    result.reasonCode = productNpcProfileAssignmentStatusName(result.status);
    return result;
  }

  if (request.table == nullptr || request.table->assignments.empty()) {
    result.ok = true;
    result.status = ProductNpcProfileAssignmentStatus::Defaulted;
    result.reasonCode = productNpcProfileAssignmentStatusName(result.status);
    result.behaviorProfileId = "default";
    return result;
  }

  const ProductNpcProfileAssignmentValidationResult validation =
      validateProductNpcProfileAssignments(*request.table);
  if (!validation.ok) {
    result.status = validation.status;
    result.reasonCode = validation.reasonCode;
    return result;
  }

  for (const ProductNpcProfileAssignment& assignment : request.table->assignments) {
    if (assignment.stableEntityName != request.stableEntityName) {
      continue;
    }
    result.ok = true;
    result.status = ProductNpcProfileAssignmentStatus::Resolved;
    result.reasonCode = productNpcProfileAssignmentStatusName(result.status);
    result.behaviorProfileId = assignment.behaviorProfileId;
    return result;
  }

  result.ok = true;
  result.status = ProductNpcProfileAssignmentStatus::Defaulted;
  result.reasonCode = productNpcProfileAssignmentStatusName(result.status);
  result.behaviorProfileId = "default";
  return result;
}

}  // namespace iggy3d
