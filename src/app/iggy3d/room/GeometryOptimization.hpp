#pragma once

#include <cstdint>
#include <string>

#include "content/authoring/EditableRoomDocument.hpp"

namespace iggy3d {

struct ProductRoomGeometryOptimizationReport {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::uint64_t sourceFloorCount = 0;
  std::uint64_t sourceWallCount = 0;
  std::uint64_t sourceObjectCount = 0;
  std::uint64_t naiveDrawCount = 0;
  std::uint64_t naiveTriangleCount = 0;
  std::uint64_t optimizedFloorRectCount = 0;
  std::uint64_t optimizedWallRunCount = 0;
  std::uint64_t optimizedObjectDrawCount = 0;
  std::uint64_t optimizedDrawCount = 0;
  std::uint64_t optimizedTriangleCount = 0;
  std::uint64_t drawCountAvoided = 0;
  std::uint64_t triangleCountAvoided = 0;
  std::uint64_t floorRectMergeCandidateCount = 0;
  std::uint64_t wallRunMergeCandidateCount = 0;
};

ProductRoomGeometryOptimizationReport buildProductRoomGeometryOptimizationReport(
    const EditableRoomDocument* document);

}  // namespace iggy3d
