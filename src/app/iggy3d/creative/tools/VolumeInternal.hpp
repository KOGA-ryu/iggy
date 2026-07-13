#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d::creative::volume_internal {

[[nodiscard]] bool validObjectKind(CreativeObjectKind kind) noexcept;
[[nodiscard]] bool objectInsideVolume(
    const CreativeObject& object,
    CreativeBounds volumeBounds) noexcept;

void acceptNoChange(CreativeVolumeOperationReceipt& receipt,
                    std::string_view reasonCode) noexcept;
void acceptApplied(CreativeVolumeOperationReceipt& receipt,
                   const CreativeDocument& document,
                   std::string_view reasonCode) noexcept;
void reject(CreativeVolumeOperationReceipt& receipt,
            CreativeVolumeOperationStatus status,
            std::string_view reasonCode) noexcept;
void copyVoxelMutationFacts(
    CreativeVolumeOperationReceipt& receipt,
    const CreativeVoxelMutationReceipt& voxelReceipt) noexcept;
[[nodiscard]] bool affectedCountExceeds(
    std::size_t objectCount,
    std::size_t voxelCount,
    std::uint64_t limit) noexcept;
[[nodiscard]] CreativeGridBounds3 inclusiveVolumeGridBounds(
    const CreativeVolumeSelection& selection) noexcept;

[[nodiscard]] CreativeVolumeOperationReceipt fillHandler(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt);
[[nodiscard]] CreativeVolumeOperationReceipt hollowHandler(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt);
[[nodiscard]] CreativeVolumeOperationReceipt replaceVolumeCells(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt);
[[nodiscard]] CreativeVolumeOperationReceipt eraseVolumeObjects(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt);
[[nodiscard]] CreativeVolumeOperationReceipt cloneVolumeObjects(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request,
    CreativeVolumeOperationReceipt receipt);

}  // namespace iggy3d::creative::volume_internal
