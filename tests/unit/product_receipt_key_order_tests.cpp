// Receipt key-order oracle (truth gate for receipt decomposition).
//
// buildProductAppReceipt emits an ORDERED list of key/value fields. The receipt split (and every future
// domain re-split) is behaviour-preserving only if the emitted sequence stays byte-identical. The rest of
// the test suite reads receipts through order-independent hasReceiptField, so nothing else guards ORDER.
// This test does: it builds the receipt from a fully default-constructed fixture (deterministic, exercises
// every field including the tuning loop) and asserts the exact ordered (key, value) stream against a
// checked-in golden.
//
// Intentional receipt changes: regenerate the golden by running this binary from the repo root with
//   RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests
// and commit the updated golden alongside the change.

#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/world/WorldTemplate.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "render/RenderDiagnostics.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

constexpr const char* kGoldenPath = "tests/golden/product_receipt_key_order.golden";

iggy3d::RenderReceipt buildDefaultReceipt() {
  const iggy3d::ProductAppOptions options;
  const iggy3d::ProductWorldTemplate world;
  const iggy3d::FrontendState frontend;
  const iggy3d::FrontendSettings settings;
  const iggy3d::ProductAppWindowState window;
  const iggy3d::ProductSaveBridgeResult saves;
  return iggy3d::buildProductAppReceipt(options, world, frontend, settings, window, saves);
}

std::string serialize(const iggy3d::RenderReceipt& receipt) {
  std::ostringstream os;
  for (const iggy3d::RenderReceiptField& field : receipt.fields) {
    os << field.key << '\t' << field.value << '\n';
  }
  return os.str();
}

}  // namespace

int main() {
  const iggy3d::RenderReceipt receipt = buildDefaultReceipt();
  const std::string actual = serialize(receipt);

  if (std::getenv("RECEIPT_GOLDEN_REGEN") != nullptr) {
    std::ofstream out(kGoldenPath, std::ios::binary);
    if (!out) {
      std::cerr << "[regen] cannot write golden at " << kGoldenPath
                << " (run from the repo root)\n";
      return 1;
    }
    out << actual;
    std::cout << "[regen] wrote " << receipt.fields.size() << " fields to " << kGoldenPath << '\n';
    return 0;
  }

  std::ifstream in(kGoldenPath, std::ios::binary);
  if (!in) {
    std::cerr << "receipt oracle: MISSING golden " << kGoldenPath
              << "\n  (run from repo root: RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests)\n";
    return 1;
  }
  std::stringstream buf;
  buf << in.rdbuf();
  const std::string expected = buf.str();

  if (actual == expected) {
    std::cout << "receipt key-order oracle: " << receipt.fields.size()
              << " fields match golden (order + values)\n";
    return 0;
  }

  // Report the first diverging line so a regression points straight at the offending field.
  std::istringstream a(actual);
  std::istringstream e(expected);
  std::string la;
  std::string le;
  int lineNo = 0;
  while (true) {
    const bool gotA = static_cast<bool>(std::getline(a, la));
    const bool gotE = static_cast<bool>(std::getline(e, le));
    if (!gotA && !gotE) {
      break;
    }
    ++lineNo;
    if (gotA != gotE || la != le) {
      std::cerr << "receipt key-order oracle: MISMATCH at field " << lineNo << "\n"
                << "  expected: " << (gotE ? le : std::string("<end of receipt>")) << "\n"
                << "  actual:   " << (gotA ? la : std::string("<end of receipt>")) << "\n"
                << "The receipt output changed. If intentional, regenerate from the repo root:\n"
                << "  RECEIPT_GOLDEN_REGEN=1 ./build/product_receipt_key_order_tests\n";
      return 1;
    }
  }
  std::cerr << "receipt key-order oracle: field count changed (" << receipt.fields.size()
            << " emitted). Regenerate the golden if intentional.\n";
  return 1;
}
