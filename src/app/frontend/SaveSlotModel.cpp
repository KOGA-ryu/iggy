#include "app/frontend/SaveSlotModel.hpp"

namespace iggy3d {

std::string_view saveSlotCompatibilityName(SaveSlotCompatibility compatibility) {
  switch (compatibility) {
    case SaveSlotCompatibility::Compatible:
      return "compatible";
    case SaveSlotCompatibility::IncompatiblePackage:
      return "incompatible_package";
    case SaveSlotCompatibility::IncompatibleScenario:
      return "incompatible_scenario";
    case SaveSlotCompatibility::DecodeFailed:
      return "decode_failed";
    case SaveSlotCompatibility::LoadFailed:
      return "load_failed";
    case SaveSlotCompatibility::Unknown:
      return "unknown";
  }
  return "unknown";
}

}  // namespace iggy3d
