#pragma once

#include <cstdint>

namespace iggy3d {

struct FrontendSettings;
struct FrontendState;
struct ProductAppOptions;
struct ProductAppWindowState;
struct ProductCreativeBakedActiveRoomRefreshResult;
struct ProductCreativeUiProjectionReceipt;
struct ProductCreativeUiInputFrameReceipt;
struct ProductCreativeUiDownstreamClickReceipt;
struct ProductCreativeUiCommandFrameReceipt;
struct ProductCreativeViewportPickFrameReceipt;
struct ProductCreativeWireframeFrameReceipt;
struct ProductSaveBridgeResult;
struct ProductWorldTemplate;
struct RenderReceipt;

namespace creative {
struct CreativeActiveIdentity;
}  // namespace creative

const creative::CreativeActiveIdentity& defaultProductReceiptCreativeIdentity();

RenderReceipt buildProductAppReceipt(const ProductAppOptions& options,
                                     const ProductWorldTemplate& world,
                                     const FrontendState& frontend,
                                     const FrontendSettings& settings,
                                     const ProductAppWindowState& window,
                                     const ProductSaveBridgeResult& saves,
                                     const creative::CreativeActiveIdentity&
                                         creativeIdentity =
                                             defaultProductReceiptCreativeIdentity(),
                                     std::uint64_t runtimeStateHash = 0);

void recordProductPhysicsMovementPlannerTickProof(
    ProductAppWindowState& window,
    bool requested,
    bool collisionSurfacesAvailable,
    bool movementPhysicsStatsAvailable);

void recordProductCreativeUiProjection(
    ProductAppWindowState& window,
    const ProductCreativeUiProjectionReceipt& receipt);
void recordProductCreativeUiInputFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiInputFrameReceipt& receipt);
void recordProductCreativeUiDownstreamClick(
    ProductAppWindowState& window,
    const ProductCreativeUiDownstreamClickReceipt& receipt);
void recordProductCreativeUiCommandFrame(
    ProductAppWindowState& window,
    const ProductCreativeUiCommandFrameReceipt& receipt);
void recordProductCreativeUiBakedRoomRefresh(
    ProductAppWindowState& window,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh);
void recordProductCreativeBakedRoomAutoRefresh(
    ProductAppWindowState& window,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh);
void recordProductCreativeViewportPickFrame(
    ProductAppWindowState& window,
    const ProductCreativeViewportPickFrameReceipt& receipt);
void recordProductCreativeWireframeFrame(
    ProductAppWindowState& window,
    const ProductCreativeWireframeFrameReceipt& receipt);
void recordProductCreativeDocumentRevisionFrame(
    ProductAppWindowState& window,
    bool observed,
    std::uint64_t documentIdBefore,
    std::uint64_t revisionBefore,
    std::uint64_t documentIdAfter,
    std::uint64_t revisionAfter);
void recordProductCreativeBakedRoomFresh(ProductAppWindowState& window,
                                         std::uint64_t documentId,
                                         std::uint64_t revision);

}  // namespace iggy3d
