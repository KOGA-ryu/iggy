#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d {

struct ProductNpcProfileAssignment {
  std::string stableEntityName;
  std::string behaviorProfileId;
};

struct ProductNpcProfileAssignmentTable {
  std::vector<ProductNpcProfileAssignment> assignments;
};

enum class ProductNpcProfileAssignmentStatus : std::uint8_t {
  Resolved,
  Defaulted,
  InvalidEntityName,
  InvalidProfileId,
  DuplicateEntityName,
};

std::string_view productNpcProfileAssignmentStatusName(
    ProductNpcProfileAssignmentStatus status);

struct ProductNpcProfileAssignmentValidationResult {
  bool ok = false;
  ProductNpcProfileAssignmentStatus status =
      ProductNpcProfileAssignmentStatus::Defaulted;
  std::string_view reasonCode = "defaulted";
  std::string stableEntityName;
  std::string behaviorProfileId;
};

struct ProductNpcProfileResolveRequest {
  const ProductNpcProfileAssignmentTable* table = nullptr;
  std::string_view stableEntityName;
};

struct ProductNpcProfileResolveResult {
  bool ok = false;
  ProductNpcProfileAssignmentStatus status =
      ProductNpcProfileAssignmentStatus::Defaulted;
  std::string_view reasonCode = "defaulted";
  std::string behaviorProfileId;
};

bool isValidProductNpcProfileEntityName(std::string_view stableEntityName);
ProductNpcProfileAssignmentValidationResult validateProductNpcProfileAssignments(
    const ProductNpcProfileAssignmentTable& table);
ProductNpcProfileResolveResult resolveProductNpcProfileAssignment(
    const ProductNpcProfileResolveRequest& request);

}  // namespace iggy3d
