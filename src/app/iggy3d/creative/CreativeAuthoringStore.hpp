#pragma once

#include "app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {

struct CreativeAuthoringStore {
  bool creativeWireframeRequested = false;
  bool creativeWireframeActive = false;
  bool creativeWireframeFacadeAvailable = false;
  bool creativeWireframeDocumentAvailable = false;
  bool creativeWireframeSourceAvailable = false;
  std::uint64_t creativeWireframeObjectCount = 0;
  std::uint64_t creativeWireframeVisibleObjectCount = 0;
  std::uint64_t creativeWireframeItemCount = 0;
  std::uint64_t creativeWireframeSegmentCount = 0;
  std::uint64_t creativeWireframeBoxItemCount = 0;
  std::uint64_t creativeWireframeLineItemCount = 0;
  std::uint64_t creativeWireframePointItemCount = 0;
  std::uint64_t creativeWireframeSkippedDegenerateCount = 0;
  std::string creativeWireframeStatus =
      "creative_wireframe_frame_not_requested";
  std::string creativeWireframeReasonCode =
      "creative_wireframe_frame_not_requested";
  std::string creativeWireframeWireframeStatus = "Unknown";
  std::string creativeWireframeWireframeReasonCode = "none";
  std::string creativeWireframeSegmentStatus = "Unknown";
  std::string creativeWireframeSegmentReasonCode = "none";
  bool creativeWireframeDebugLineRequested = false;
  bool creativeWireframeDebugLineSourceAvailable = false;
  std::uint64_t creativeWireframeDebugLineInputSegmentCount = 0;
  std::uint64_t creativeWireframeDebugLineCount = 0;
  std::uint64_t creativeWireframeDebugLineSkippedDegenerateCount = 0;
  std::string creativeWireframeDebugLineStatus = "Unknown";
  std::string creativeWireframeDebugLineReasonCode = "none";
};

}  // namespace iggy3d
