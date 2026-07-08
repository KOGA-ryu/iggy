#pragma once

#include "ProductTestSupport.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "render/RenderDiagnostics.hpp"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

namespace iggy3d {
struct ProductAppWindowState;
}

namespace iggy3d::test {

struct ReceiptFieldExpectation {
  std::string_view key;
  std::string_view value;
  std::string_view message;
};

inline RenderReceipt receiptFor(const ProductAppWindowState& window) {
  ProductAppOptions options;
  ProductWorldTemplate world;
  FrontendState frontend;
  FrontendSettings settings;
  ProductSaveBridgeResult saves;
  return buildProductAppReceipt(options, world, frontend, settings, window, saves);
}

inline bool expectReceiptField(const RenderReceipt& receipt,
                               std::string_view key,
                               std::string_view value,
                               std::string_view message) {
  return expect(hasReceiptField(receipt, key, value), message);
}

inline bool expectReceiptCount(const RenderReceipt& receipt,
                               std::string_view key,
                               std::uint64_t value,
                               std::string_view message) {
  const std::string text = std::to_string(value);
  return expectReceiptField(receipt,
                            key,
                            std::string_view(text.data(), text.size()),
                            message);
}

inline bool expectReceiptFields(
    const RenderReceipt& receipt,
    std::initializer_list<ReceiptFieldExpectation> expectations,
    std::string_view group) {
  bool ok = true;
  for (const ReceiptFieldExpectation& expectation : expectations) {
    std::string message(group);
    message.append(": ");
    message.append(expectation.message);
    ok = expectReceiptField(receipt,
                            expectation.key,
                            expectation.value,
                            message) &&
         ok;
  }
  return ok;
}

template <std::size_t Size>
bool expectReceiptFields(
    const RenderReceipt& receipt,
    const std::array<ReceiptFieldExpectation, Size>& expectations,
    std::string_view group) {
  bool ok = true;
  for (const ReceiptFieldExpectation& expectation : expectations) {
    std::string message(group);
    message.append(": ");
    message.append(expectation.message);
    ok = expectReceiptField(receipt,
                            expectation.key,
                            expectation.value,
                            message) &&
         ok;
  }
  return ok;
}

}  // namespace iggy3d::test
