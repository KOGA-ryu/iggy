#include "creative_desktop_command_test_runners.hpp"

int main() {
  bool ok = true;
  ok = runCreativeDesktopDocumentCommandTests() && ok;
  ok = runCreativeDesktopObjectCommandTests() && ok;
  ok = runCreativeDesktopWorldLayoutStructureCommandTests() && ok;
  ok = runCreativeDesktopWorldLayoutGeneratedCommandTests() && ok;
  ok = runCreativeDesktopWorldLayoutToolCommandTests() && ok;
  return ok ? 0 : 1;
}
